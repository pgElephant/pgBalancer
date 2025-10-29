#!/usr/bin/env bash

#==============================================================================
# pgbalancer Build Script
# 
# A modular, OS-aware build script that automatically detects dependencies,
# configures the build environment, and compiles pgbalancer for your platform.
#
# Supported platforms:
#   - macOS (Apple Silicon & Intel)
#   - Ubuntu/Debian
#   - Rocky Linux/RHEL/CentOS/Fedora
#
# Copyright (c) 2024-2025, pgElephant, Inc.
#==============================================================================

set -e  # Exit on error
set -o pipefail  # Catch errors in pipes

#==============================================================================
# Configuration & Global Variables
#==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_LOG="${SCRIPT_DIR}/build.log"
VERBOSE=${VERBOSE:-0}

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Build configuration
CONFIGURE_OPTS=""
MAKE_JOBS=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

#==============================================================================
# Logging Functions
#==============================================================================

log_info() {
    echo -e "${BLUE}ℹ${NC} $*" | tee -a "$BUILD_LOG"
}

log_success() {
    echo -e "${GREEN}✓${NC} $*" | tee -a "$BUILD_LOG"
}

log_warn() {
    echo -e "${YELLOW}⚠${NC} $*" | tee -a "$BUILD_LOG"
}

log_error() {
    echo -e "${RED}✗${NC} $*" | tee -a "$BUILD_LOG"
}

fatal_error() {
    log_error "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    log_error "BUILD FAILED!"
    log_error "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    log_error ""
    log_error "$@"
    log_error ""
    log_error "Build log saved to: $BUILD_LOG"
    log_error "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    exit 1
}

log_step() {
    echo -e "\n${GREEN}▶${NC} $*" | tee -a "$BUILD_LOG"
}

#==============================================================================
# OS Detection
#==============================================================================

detect_os() {
    log_step "Detecting operating system..."
    
    if [[ "$OSTYPE" == "darwin"* ]]; then
        OS="macos"
        if [[ $(uname -m) == "arm64" ]]; then
            ARCH="arm64"
            HOMEBREW_PREFIX="/opt/homebrew"
        else
            ARCH="x86_64"
            HOMEBREW_PREFIX="/usr/local"
        fi
        log_info "Detected: macOS ($ARCH)"
    elif [[ -f /etc/os-release ]]; then
        . /etc/os-release
        case "$ID" in
            ubuntu|debian)
                OS="ubuntu"
                ARCH=$(uname -m)
                log_info "Detected: Ubuntu/Debian ($ARCH)"
                ;;
            rhel|centos|rocky|almalinux|fedora)
                OS="rhel"
                ARCH=$(uname -m)
                log_info "Detected: RHEL/Rocky/CentOS ($ARCH)"
                ;;
            *)
                log_error "Unsupported Linux distribution: $ID"
                exit 1
                ;;
        esac
    else
        log_error "Unable to detect operating system"
        exit 1
    fi
    
    export OS ARCH
}

#==============================================================================
# Dependency Detection & Installation - macOS
#==============================================================================

check_homebrew() {
    if ! command -v brew &> /dev/null; then
        fatal_error \
            "Homebrew is not installed!" \
            "" \
            "pgbalancer build on macOS requires Homebrew package manager." \
            "" \
            "Install Homebrew:" \
            "  /bin/bash -c \"\$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\"" \
            "" \
            "Or visit: https://brew.sh"
    fi
    log_success "Homebrew found: $(brew --version | head -1)"
    return 0
}

find_macos_dependencies() {
    log_step "Checking macOS dependencies..."
    
    local missing_deps=()
    local required_deps=(
        "postgresql@17:PostgreSQL 17"
        "libyaml:libyaml"
        "openssl@3:OpenSSL 3"
        "json-c:JSON-C"
        "curl:cURL"
        "bison:GNU Bison"
        "gawk:GNU Awk"
    )
    
    for dep_spec in "${required_deps[@]}"; do
        IFS=':' read -r dep_name dep_desc <<< "$dep_spec"
        if brew list "$dep_name" &>/dev/null; then
            log_success "$dep_desc is installed"
        else
            log_warn "$dep_desc is missing"
            missing_deps+=("$dep_name")
        fi
    done
    
    if [[ ${#missing_deps[@]} -gt 0 ]]; then
        log_warn "Missing dependencies: ${missing_deps[*]}"
        read -p "Install missing dependencies? (y/n) " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            for dep in "${missing_deps[@]}"; do
                log_info "Installing $dep..."
                brew install "$dep"
            done
        else
            log_error "Cannot proceed without required dependencies"
        exit 1
        fi
    fi
    
    # Find PostgreSQL installation - NO DEFAULTS, must be explicitly found
    log_info "Searching for PostgreSQL installation..."
    
    # Priority 1: Check if user specified PG_PREFIX environment variable
    if [[ -n "$PG_PREFIX" ]]; then
        if [[ -x "$PG_PREFIX/bin/pg_config" ]]; then
            PG_CONFIG="$PG_PREFIX/bin/pg_config"
            log_success "Using user-specified PostgreSQL: $PG_PREFIX"
        else
            log_error "PG_PREFIX is set to '$PG_PREFIX' but pg_config not found at $PG_PREFIX/bin/pg_config"
            exit 1
        fi
    fi
    
    # Priority 2: Try Homebrew installations (only if PG_PREFIX not set)
    if [[ -z "$PG_PREFIX" ]]; then
        for pg_version in 17 16 15 14 13; do
            local pg_brew_path=$(brew --prefix postgresql@${pg_version} 2>/dev/null)
            if [[ -n "$pg_brew_path" && -x "$pg_brew_path/bin/pg_config" ]]; then
                PG_PREFIX="$pg_brew_path"
                PG_CONFIG="$pg_brew_path/bin/pg_config"
                log_success "Found PostgreSQL ${pg_version} via Homebrew: $PG_PREFIX"
                break
            fi
        done
    fi
    
    # Priority 3: Try generic postgresql package
    if [[ -z "$PG_PREFIX" ]]; then
        local pg_brew_path=$(brew --prefix postgresql 2>/dev/null)
        if [[ -n "$pg_brew_path" && -x "$pg_brew_path/bin/pg_config" ]]; then
            PG_PREFIX="$pg_brew_path"
            PG_CONFIG="$pg_brew_path/bin/pg_config"
            log_success "Found PostgreSQL via Homebrew: $PG_PREFIX"
        fi
    fi
    
    # Priority 4: If not found via Homebrew, search system installations
    if [[ -z "$PG_PREFIX" ]]; then
        log_info "Not found in Homebrew, searching system installations..."
        
        # Build list of potential PostgreSQL installation directories dynamically
        local search_paths=()
        
        # Common installation prefixes (no hard-coded versions)
        for prefix in /usr/local /opt /usr; do
            # Look for any versioned installations dynamically
            if [[ -d "$prefix" ]]; then
                # Find directories matching PostgreSQL patterns
                for pg_dir in "$prefix"/pgsql* "$prefix"/postgresql*; do
                    [[ -d "$pg_dir" ]] && search_paths+=("$pg_dir")
                done
            fi
        done
        
        # macOS specific paths (only if on macOS)
        if [[ "$OS" == "macos" ]]; then
            # Search /Library/PostgreSQL for any version
            if [[ -d "/Library/PostgreSQL" ]]; then
                for pg_dir in /Library/PostgreSQL/*; do
                    [[ -d "$pg_dir" ]] && search_paths+=("$pg_dir")
                done
            fi
            # Search Postgres.app installations
            if [[ -d "/Applications/Postgres.app/Contents/Versions" ]]; then
                for pg_dir in /Applications/Postgres.app/Contents/Versions/*; do
                    [[ -d "$pg_dir" ]] && search_paths+=("$pg_dir")
                done
            fi
        fi
        
        # Search all discovered paths for pg_config
        for pg_path in "${search_paths[@]}"; do
            if [[ -x "$pg_path/bin/pg_config" ]]; then
                PG_PREFIX="$pg_path"
                PG_CONFIG="$pg_path/bin/pg_config"
                log_success "Found PostgreSQL at: $PG_PREFIX"
                break
            fi
        done
    fi
    
    # Final check
    if [[ -z "$PG_PREFIX" || ! -x "$PG_CONFIG" ]]; then
        fatal_error \
            "PostgreSQL NOT FOUND!" \
            "" \
            "pgbalancer requires PostgreSQL 13 or later." \
            "" \
            "Installation instructions:" \
            "  brew install postgresql@17" \
            "" \
            "Or specify custom PostgreSQL installation:" \
            "  export PG_PREFIX=/path/to/postgresql" \
            "  ./build.sh" \
            "" \
            "To use PostgreSQL from /usr/local/pgsql.17:" \
            "  export PG_PREFIX=/usr/local/pgsql.17" \
            "  ./build.sh"
    fi
    
    # Verify pg_config works
    if ! "$PG_CONFIG" --version &>/dev/null; then
        fatal_error \
            "Found pg_config at $PG_CONFIG but it doesn't work!" \
            "" \
            "The pg_config binary exists but failed to execute." \
            "This usually means:" \
            "  • PostgreSQL is not properly installed" \
            "  • Missing shared libraries" \
            "  • Architecture mismatch (x86_64 vs arm64)" \
            "" \
            "Try reinstalling PostgreSQL:" \
            "  brew reinstall postgresql@17"
    fi
    
    # Get actual paths from pg_config
    PG_VERSION=$($PG_CONFIG --version | awk '{print $2}')
    PG_INCLUDE=$($PG_CONFIG --includedir)
    PG_LIB=$($PG_CONFIG --libdir)
    
    log_info "PostgreSQL version: $PG_VERSION"
    log_info "PostgreSQL include: $PG_INCLUDE"
    log_info "PostgreSQL lib: $PG_LIB"
    
    # Verify libpq exists
    if [[ ! -f "$PG_LIB/libpq.dylib" && ! -f "$PG_LIB/libpq.a" ]]; then
        fatal_error \
            "libpq NOT FOUND in $PG_LIB" \
            "" \
            "PostgreSQL client libraries are missing." \
            "" \
            "Solutions:" \
            "  • Reinstall PostgreSQL: brew reinstall postgresql@17" \
            "  • Check library path: ls -la $PG_LIB/libpq*" \
            "  • Verify PostgreSQL installation: pg_config --libdir"
    fi
    
    log_success "Verified libpq at: $PG_LIB/libpq.dylib"
    
    export PG_PREFIX PG_CONFIG PG_INCLUDE PG_LIB PG_VERSION
}

setup_macos_build_env() {
    log_step "Setting up macOS build environment..."
    
    # Add PostgreSQL and Homebrew to PATH (using detected paths)
    export PATH="${PG_PREFIX}/bin:${HOMEBREW_PREFIX}/bin:${HOMEBREW_PREFIX}/opt/bison/bin:$PATH"
    
    # Dynamically find library prefixes
    local libyaml_prefix=$(brew --prefix libyaml 2>/dev/null || echo "${HOMEBREW_PREFIX}")
    local openssl_prefix=$(brew --prefix openssl@3 2>/dev/null || echo "${HOMEBREW_PREFIX}")
    local jsonc_prefix=$(brew --prefix json-c 2>/dev/null || echo "${HOMEBREW_PREFIX}")
    
    # Build include and library paths using detected PostgreSQL paths
    export CPPFLAGS="-I${PG_INCLUDE} -I${PG_INCLUDE}/server -I${libyaml_prefix}/include -I${openssl_prefix}/include -I${jsonc_prefix}/include"
    export LDFLAGS="-L${PG_LIB} -L${libyaml_prefix}/lib -L${openssl_prefix}/lib -L${jsonc_prefix}/lib"
    
    # Use pg_config directly for configure
    CONFIGURE_OPTS="--with-pgsql=${PG_PREFIX} --with-openssl"
    
    log_info "Build configuration:"
    log_info "  PATH: $PATH"
    log_info "  CPPFLAGS: $CPPFLAGS"
    log_info "  LDFLAGS: $LDFLAGS"
    log_info "  PG_CONFIG: $PG_CONFIG"
    log_info "  Configure: $CONFIGURE_OPTS"
}

#==============================================================================
# Dependency Detection & Installation - Ubuntu/Debian
#==============================================================================

check_apt() {
    if ! command -v apt-get &> /dev/null; then
        fatal_error \
            "apt-get not found!" \
            "" \
            "This script requires apt-get package manager for Ubuntu/Debian." \
            "Your system appears to be Ubuntu/Debian but apt-get is not available."
    fi
    return 0
}

find_ubuntu_dependencies() {
    log_step "Checking Ubuntu/Debian dependencies..."
    
    local missing_deps=()
    local required_packages=(
        "build-essential:Build tools"
        "libpq-dev:PostgreSQL development headers"
        "libyaml-dev:libyaml development headers"
        "libssl-dev:OpenSSL development headers"
        "libjson-c-dev:JSON-C development headers"
        "libcurl4-openssl-dev:cURL development headers"
        "bison:GNU Bison"
        "flex:Flex lexical analyzer"
        "gawk:GNU Awk"
    )
    
    for pkg_spec in "${required_packages[@]}"; do
        IFS=':' read -r pkg_name pkg_desc <<< "$pkg_spec"
        if dpkg -l "$pkg_name" 2>/dev/null | grep -q "^ii"; then
            log_success "$pkg_desc is installed"
        else
            log_warn "$pkg_desc is missing"
            missing_deps+=("$pkg_name")
        fi
    done
    
    if [[ ${#missing_deps[@]} -gt 0 ]]; then
        log_warn "Missing dependencies: ${missing_deps[*]}"
        read -p "Install missing dependencies? (requires sudo) (y/n) " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            sudo apt-get update
            sudo apt-get install -y "${missing_deps[@]}"
        else
            log_error "Cannot proceed without required dependencies"
            exit 1
        fi
    fi
    
    # Find PostgreSQL - require explicit detection, no defaults
    log_info "Searching for PostgreSQL installation..."
    
    # Check if user specified PG_PREFIX
    if [[ -n "$PG_PREFIX" && -x "$PG_PREFIX/bin/pg_config" ]]; then
        PG_CONFIG="$PG_PREFIX/bin/pg_config"
        log_success "Using user-specified PostgreSQL: $PG_PREFIX"
    else
        # Try to find pg_config in PATH
        PG_CONFIG=$(command -v pg_config 2>/dev/null || true)
        if [[ -x "$PG_CONFIG" ]]; then
            PG_PREFIX=$(dirname $(dirname "$PG_CONFIG"))
            log_success "Found PostgreSQL in PATH: $PG_PREFIX"
        else
            # Search common installation paths
            local found=0
            for pg_path in /usr/lib/postgresql/*/bin/pg_config /usr/pgsql-*/bin/pg_config; do
                if [[ -x "$pg_path" ]]; then
                    PG_CONFIG="$pg_path"
                    PG_PREFIX=$(dirname $(dirname "$pg_path"))
                    log_success "Found PostgreSQL: $PG_PREFIX"
                    found=1
                    break
                fi
            done
            
            if [[ $found -eq 0 ]]; then
                fatal_error \
                    "PostgreSQL NOT FOUND on Ubuntu/Debian!" \
                    "" \
                    "pg_config not found in PATH or common locations." \
                    "" \
                    "Install PostgreSQL development package:" \
                    "  sudo apt-get update" \
                    "  sudo apt-get install postgresql-server-dev-all" \
                    "" \
                    "Or for a specific version:" \
                    "  sudo apt-get install postgresql-server-dev-17" \
                    "" \
                    "Or specify custom installation:" \
                    "  export PG_PREFIX=/path/to/postgresql" \
                    "  ./build.sh"
            fi
        fi
    fi
    
    # Get actual paths from pg_config
    PG_VERSION=$($PG_CONFIG --version | awk '{print $2}')
    PG_INCLUDE=$($PG_CONFIG --includedir)
    PG_LIB=$($PG_CONFIG --libdir)
    
    log_info "PostgreSQL version: $PG_VERSION"
    log_info "PostgreSQL include: $PG_INCLUDE"
    log_info "PostgreSQL lib: $PG_LIB"
    
    export PG_PREFIX PG_CONFIG PG_INCLUDE PG_LIB PG_VERSION
}

setup_ubuntu_build_env() {
    log_step "Setting up Ubuntu/Debian build environment..."
    
    CONFIGURE_OPTS="--with-pgsql=${PG_PREFIX} --with-openssl"
    
    log_info "Configure options: $CONFIGURE_OPTS"
}

#==============================================================================
# Dependency Detection & Installation - RHEL/Rocky/CentOS
#==============================================================================

check_yum() {
    if command -v dnf &> /dev/null; then
        PKG_MANAGER="dnf"
    elif command -v yum &> /dev/null; then
        PKG_MANAGER="yum"
    else
        fatal_error \
            "Package manager not found!" \
            "" \
            "This script requires dnf or yum package manager for RHEL/Rocky." \
            "Your system appears to be RHEL-based but no package manager is available."
    fi
    log_info "Package manager: $PKG_MANAGER"
    return 0
}

find_rhel_dependencies() {
    log_step "Checking RHEL/Rocky dependencies..."
    
    local missing_deps=()
    local required_packages=(
        "gcc:GCC compiler"
        "make:GNU Make"
        "postgresql-devel:PostgreSQL development headers"
        "libyaml-devel:libyaml development headers"
        "openssl-devel:OpenSSL development headers"
        "json-c-devel:JSON-C development headers"
        "libcurl-devel:cURL development headers"
        "bison:GNU Bison"
        "flex:Flex lexical analyzer"
        "gawk:GNU Awk"
    )
    
    for pkg_spec in "${required_packages[@]}"; do
        IFS=':' read -r pkg_name pkg_desc <<< "$pkg_spec"
        if rpm -q "$pkg_name" &>/dev/null; then
            log_success "$pkg_desc is installed"
        else
            log_warn "$pkg_desc is missing"
            missing_deps+=("$pkg_name")
        fi
    done
    
    if [[ ${#missing_deps[@]} -gt 0 ]]; then
        log_warn "Missing dependencies: ${missing_deps[*]}"
        read -p "Install missing dependencies? (requires sudo) (y/n) " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            sudo $PKG_MANAGER install -y "${missing_deps[@]}"
        else
            log_error "Cannot proceed without required dependencies"
        exit 1
        fi
    fi
    
    # Find PostgreSQL - require explicit detection, no defaults
    log_info "Searching for PostgreSQL installation..."
    
    # Check if user specified PG_PREFIX
    if [[ -n "$PG_PREFIX" && -x "$PG_PREFIX/bin/pg_config" ]]; then
        PG_CONFIG="$PG_PREFIX/bin/pg_config"
        log_success "Using user-specified PostgreSQL: $PG_PREFIX"
    else
        # Try to find pg_config in PATH
        PG_CONFIG=$(command -v pg_config 2>/dev/null || true)
        if [[ -x "$PG_CONFIG" ]]; then
            PG_PREFIX=$(dirname $(dirname "$PG_CONFIG"))
            log_success "Found PostgreSQL in PATH: $PG_PREFIX"
        else
            # Search common RHEL/Rocky paths
            local found=0
            for pg_path in /usr/pgsql-*/bin/pg_config /usr/lib64/pgsql/bin/pg_config; do
                if [[ -x "$pg_path" ]]; then
                    PG_CONFIG="$pg_path"
                    PG_PREFIX=$(dirname $(dirname "$pg_path"))
                    log_success "Found PostgreSQL: $PG_PREFIX"
                    found=1
                    break
                fi
            done
            
            if [[ $found -eq 0 ]]; then
                fatal_error \
                    "PostgreSQL NOT FOUND on RHEL/Rocky!" \
                    "" \
                    "pg_config not found in PATH or common locations." \
                    "" \
                    "Install PostgreSQL development package:" \
                    "  sudo $PKG_MANAGER install postgresql-devel" \
                    "" \
                    "For PostgreSQL official repository:" \
                    "  sudo $PKG_MANAGER install postgresql17-devel" \
                    "" \
                    "Or specify custom installation:" \
                    "  export PG_PREFIX=/path/to/postgresql" \
                    "  ./build.sh"
            fi
        fi
    fi
    
    # Get actual paths from pg_config
    PG_VERSION=$($PG_CONFIG --version | awk '{print $2}')
    PG_INCLUDE=$($PG_CONFIG --includedir)
    PG_LIB=$($PG_CONFIG --libdir)
    
    log_info "PostgreSQL version: $PG_VERSION"
    log_info "PostgreSQL include: $PG_INCLUDE"
    log_info "PostgreSQL lib: $PG_LIB"
    
    export PG_PREFIX PG_CONFIG PG_INCLUDE PG_LIB PG_VERSION
}

setup_rhel_build_env() {
    log_step "Setting up RHEL/Rocky build environment..."
    
    CONFIGURE_OPTS="--with-pgsql=${PG_PREFIX} --with-openssl"
    
    log_info "Configure options: $CONFIGURE_OPTS"
}

#==============================================================================
# Build Functions
#==============================================================================

run_configure() {
    log_step "Running configure..."
    
    cd "$SCRIPT_DIR"
    
    log_info "Command: ./configure $CONFIGURE_OPTS"
    
    if [[ $VERBOSE -eq 1 ]]; then
        if ! ./configure $CONFIGURE_OPTS 2>&1 | tee -a "$BUILD_LOG"; then
            configure_failed
        fi
    else
        if ! ./configure $CONFIGURE_OPTS >> "$BUILD_LOG" 2>&1; then
            configure_failed
        fi
    fi
    
    log_success "Configure completed successfully"
}

configure_failed() {
    local error_hints=""
    
    # Check for common configure errors
    if grep -q "libpq is not installed" "$BUILD_LOG" 2>/dev/null; then
        error_hints="libpq (PostgreSQL client library) not found"
    elif grep -q "openssl/ssl.h" "$BUILD_LOG" 2>/dev/null; then
        error_hints="OpenSSL headers not found"
    elif grep -q "yaml.h" "$BUILD_LOG" 2>/dev/null; then
        error_hints="libyaml headers not found"
    elif grep -q "json-c" "$BUILD_LOG" 2>/dev/null; then
        error_hints="json-c library not found"
    fi
    
    case "$OS" in
        macos)
            fatal_error \
                "Configure failed!" \
                "" \
                "${error_hints:+Issue: $error_hints}" \
                "" \
                "Install missing dependencies:" \
                "  brew install postgresql@17 libyaml openssl@3 json-c curl" \
                "" \
                "Check the build log for details:" \
                "  tail -50 $BUILD_LOG"
            ;;
        ubuntu)
            fatal_error \
                "Configure failed!" \
                "" \
                "${error_hints:+Issue: $error_hints}" \
                "" \
                "Install missing dependencies:" \
                "  sudo apt-get install libpq-dev libyaml-dev libssl-dev libjson-c-dev libcurl4-openssl-dev" \
                "" \
                "Check the build log for details:" \
                "  tail -50 $BUILD_LOG"
            ;;
        rhel)
            fatal_error \
                "Configure failed!" \
                "" \
                "${error_hints:+Issue: $error_hints}" \
                "" \
                "Install missing dependencies:" \
                "  sudo $PKG_MANAGER install postgresql-devel libyaml-devel openssl-devel json-c-devel libcurl-devel" \
                "" \
                "Check the build log for details:" \
                "  tail -50 $BUILD_LOG"
            ;;
    esac
}

run_make_clean() {
    log_step "Cleaning previous build..."
    
    cd "$SCRIPT_DIR"
    
    if [[ -f Makefile ]]; then
        make clean >> "$BUILD_LOG" 2>&1 || true
    fi
    
    log_success "Clean completed"
}

run_make() {
    log_step "Building pgbalancer (using $MAKE_JOBS parallel jobs)..."
    
    cd "$SCRIPT_DIR"
    
    if [[ $VERBOSE -eq 1 ]]; then
        if ! make -j"$MAKE_JOBS" 2>&1 | tee -a "$BUILD_LOG"; then
            build_failed
        fi
    else
        if ! make -j"$MAKE_JOBS" >> "$BUILD_LOG" 2>&1; then
            build_failed
        fi
    fi
    
    log_success "Build completed successfully"
}

build_failed() {
    fatal_error \
        "Compilation failed!" \
        "" \
        "The build process encountered errors." \
        "" \
        "Common issues:" \
        "  • Missing compiler (gcc/clang)" \
        "  • Missing headers or libraries" \
        "  • Incompatible compiler version" \
        "" \
        "Check the last 50 lines of the build log:" \
        "  tail -50 $BUILD_LOG" \
        "" \
        "For verbose output, try:" \
        "  ./build.sh -v --clean"
}

build_bctl() {
    log_step "Building bctl CLI tool..."
    
    cd "$SCRIPT_DIR/bctl"
    
    # Fix macOS-specific issues in bctl Makefile
    if [[ "$OS" == "macos" ]]; then
        # Remove -Wno-stringop-truncation (not supported on macOS clang)
        if grep -q "Wno-stringop-truncation" Makefile 2>/dev/null; then
            sed -i '' 's/-Wno-stringop-truncation//g' Makefile
        fi
        # Remove -lcrypt (not needed on macOS)
        if grep -q "\-lcrypt" Makefile 2>/dev/null; then
            sed -i '' 's/-lcrypt//g' Makefile
        fi
    fi
    
    make clean >> "$BUILD_LOG" 2>&1 || true
    
    if [[ $VERBOSE -eq 1 ]]; then
        make 2>&1 | tee -a "$BUILD_LOG"
    else
        make >> "$BUILD_LOG" 2>&1
    fi
    
    cd "$SCRIPT_DIR"
    log_success "bctl build completed"
}

build_pgbalancer_config() {
    log_step "Building pgbalancer_config utility..."
    
    local config_dir="$SCRIPT_DIR/src/tools/pgbalancer_config"
    
    if [[ ! -d "$config_dir" ]]; then
        log_warn "pgbalancer_config directory not found, skipping"
        return 0
    fi
    
    cd "$config_dir"
    
    # Build with detected paths
    if [[ $VERBOSE -eq 1 ]]; then
        make clean >> "$BUILD_LOG" 2>&1 || true
        make PGSQL_BIN_DIR="${PG_PREFIX}/bin" PGSQL_LIB_DIR="${PG_LIB}" 2>&1 | tee -a "$BUILD_LOG"
    else
        make clean >> "$BUILD_LOG" 2>&1 || true
        make PGSQL_BIN_DIR="${PG_PREFIX}/bin" PGSQL_LIB_DIR="${PG_LIB}" >> "$BUILD_LOG" 2>&1
    fi
    
    cd "$SCRIPT_DIR"
    log_success "pgbalancer_config build completed"
}

verify_build() {
    log_step "Verifying build artifacts..."
    
    local errors=0
    
    # Check main binary
    if [[ -x "$SCRIPT_DIR/src/pgbalancer" ]]; then
        local version=$("$SCRIPT_DIR/src/pgbalancer" --version 2>&1 | head -1)
        log_success "pgbalancer binary: $version"
    else
        log_error "pgbalancer binary not found or not executable"
        errors=$((errors + 1))
    fi
    
    # Check bctl
    if [[ -x "$SCRIPT_DIR/bctl/bctl" ]]; then
        log_success "bctl binary found"
    else
        log_warn "bctl binary not found (optional)"
    fi
    
    # Check tools
    for tool in src/tools/pgmd5/pg_md5 src/tools/pgenc/pg_enc; do
        if [[ -x "$SCRIPT_DIR/$tool" ]]; then
            log_success "Tool found: $tool"
        else
            log_warn "Tool not found: $tool (optional)"
        fi
    done
    
    return $errors
}

create_build_structure() {
    log_step "Creating organized build directory structure..."
    
    local build_dir="$SCRIPT_DIR/build"
    
    # Create directory structure
    mkdir -p "$build_dir"/{bin,lib,etc,share/doc}
    
    log_success "Created build directories:"
    log_info "  $build_dir/bin  - Binaries"
    log_info "  $build_dir/lib  - Libraries"
    log_info "  $build_dir/etc  - Configuration files"
    log_info "  $build_dir/share/doc - Documentation"
    
    # Copy binaries
    log_step "Copying binaries to build/bin..."
    if [[ -x "$SCRIPT_DIR/src/pgbalancer" ]]; then
        cp -v "$SCRIPT_DIR/src/pgbalancer" "$build_dir/bin/" | tee -a "$BUILD_LOG"
        log_success "Copied pgbalancer"
    fi
    
    if [[ -x "$SCRIPT_DIR/bctl/bctl" ]]; then
        cp -v "$SCRIPT_DIR/bctl/bctl" "$build_dir/bin/" | tee -a "$BUILD_LOG"
        log_success "Copied bctl"
    fi
    
    if [[ -x "$SCRIPT_DIR/src/tools/pgbalancer_config/pgbalancer_config" ]]; then
        cp -v "$SCRIPT_DIR/src/tools/pgbalancer_config/pgbalancer_config" "$build_dir/bin/" | tee -a "$BUILD_LOG"
        log_success "Copied pgbalancer_config"
    fi
    
    # Copy tools
    for tool in pg_md5 pg_enc; do
        for tool_path in "$SCRIPT_DIR"/src/tools/*/"$tool"; do
            if [[ -x "$tool_path" ]]; then
                cp -v "$tool_path" "$build_dir/bin/" | tee -a "$BUILD_LOG"
                log_success "Copied $tool"
            fi
        done
    done
    
    # Copy watchdog CLI if exists
    if [[ -x "$SCRIPT_DIR/src/tools/watchdog/wd_cli" ]]; then
        cp -v "$SCRIPT_DIR/src/tools/watchdog/wd_cli" "$build_dir/bin/" | tee -a "$BUILD_LOG"
        log_success "Copied wd_cli"
    fi
    
    # Copy libraries
    log_step "Copying libraries to build/lib..."
    find "$SCRIPT_DIR/src" -name "*.a" -exec cp -v {} "$build_dir/lib/" \; 2>/dev/null | tee -a "$BUILD_LOG" || true
    
    # Copy configuration examples
    log_step "Copying configuration files to build/etc..."
    
    # Determine default paths based on detected PostgreSQL installation
    local default_data_dir="${PG_PREFIX%/*}/data"
    local default_log_dir="${SCRIPT_DIR}/logs"
    local default_socket_dir="\${TMPDIR:-/tmp}"
    
    # Create sample configuration (using dynamic paths)
    cat > "$build_dir/etc/pgbalancer.conf.sample" << EOF
# pgbalancer Sample Configuration
# Generated on $(date)
# Copy this file to pgbalancer.conf and customize for your environment

# Server Settings
listen_addresses = '*'
port = 5432
socket_dir = '${default_socket_dir}'
pcp_listen_addresses = '*'
pcp_port = 9898

# Connection Pool Settings
num_init_children = 32
max_pool = 4
child_life_time = 300
child_max_connections = 0

# Backend Database Servers
# Customize these paths for your PostgreSQL instances
backend_hostname0 = 'localhost'
backend_port0 = 5433
backend_weight0 = 1
backend_data_directory0 = '${default_data_dir}1'

backend_hostname1 = 'localhost'
backend_port1 = 5434
backend_weight1 = 1
backend_data_directory1 = '${default_data_dir}2'

# Health Check
health_check_period = 30
health_check_timeout = 20
health_check_user = 'postgres'
health_check_password = ''
health_check_database = 'postgres'

# Logging
log_destination = 'stderr'
logging_collector = on
log_directory = '${default_log_dir}'
log_filename = 'pgbalancer-%Y-%m-%d_%H%M%S.log'
log_line_prefix = '%t: pid %p: '
log_connections = on
log_hostname = on
log_statement = off

# Load Balancing
load_balance_mode = on
ignore_leading_white_space = on

# Replication
master_slave_mode = on
master_slave_sub_mode = 'stream'

# Online Recovery
recovery_user = 'postgres'
recovery_password = ''

# Paths detected during build:
# PostgreSQL: ${PG_PREFIX}
# Build Date: $(date)
# OS: ${OS} (${ARCH})
EOF
    
    # Copy the official YAML sample if it exists
    if [[ -f "$SCRIPT_DIR/src/sample/pgbalancer.yaml.sample" ]]; then
        # Simply copy the sample file as-is (no modifications)
        cp "$SCRIPT_DIR/src/sample/pgbalancer.yaml.sample" "$build_dir/etc/pgbalancer.yaml"
        
        log_success "Copied pgbalancer.yaml from sample (unmodified)"
    else
        # Generate YAML with proper nested structure
        cat > "$build_dir/etc/pgbalancer.yaml.sample" << EOF
---
# pgbalancer YAML Configuration Sample
# Generated on $(date)
# Build Info: PostgreSQL ${PG_VERSION} at ${PG_PREFIX}
# OS: ${OS} (${ARCH})

# Clustering Configuration
clustering:
  mode: streaming_replication

# Network Configuration
network:
  listen_addresses: "*"
  port: 5432
  socket_dir: ${default_socket_dir}
  unix_socket_permissions: 0777

# Connection Pool Settings
connection_pool:
  num_init_children: 32
  max_pool: 4
  child_life_time: 300
  child_max_connections: 0
  connection_cache: on
  reset_query_list: 'ABORT; DISCARD ALL'

# Authentication
authentication:
  enable_pool_hba: off
  authentication_timeout: 60
  allow_clear_text_frontend_auth: off
  ssl: off

# Logging Configuration
logging:
  destination: stderr
  line_prefix: '%t: pid %p: '
  connections: on
  hostname: on
  statement: off
  per_node_statement: off
  pid_file_name: ${default_socket_dir}/pgbalancer.pid
  logdir: ${default_log_dir}

# Load Balancing
load_balancing:
  mode: on
  ignore_leading_white_space: on
  disable_load_balance_on_write: transaction
  statement_level_load_balance: off
  black_function_list: 'currval,lastval,nextval,setval'

# Replication Settings
replication:
  mode: on
  sub_mode: stream
  check_period: 10
  check_user: postgres
  check_password: ''
  check_database: postgres
  delay_threshold: 0
  prefer_lower_delay_standby: off

# Health Check
health_check:
  period: 30
  timeout: 20
  user: postgres
  password: ''
  database: postgres
  max_retries: 3
  retry_delay: 1
  connect_timeout: 10000

# Failover and Failback
failover:
  command: ''
  on_backend_error: on
  detach_false_primary: off
  search_primary_node_timeout: 300
  
  failback:
    command: ''
    auto_failback: off
    interval: 60

# Online Recovery
recovery:
  user: postgres
  password: ''
  first_stage_command: ''
  second_stage_command: ''
  timeout: 90
  client_idle_limit_in_recovery: 0

# Backend PostgreSQL Nodes
backends:
  # Backend 0 (Primary/Master)
  - hostname: localhost
    port: 5433
    weight: 1
    data_directory: ${default_data_dir}1
    flag: ALLOW_TO_FAILOVER
    application_name: server0
    
    health_check:
      period: 30
      timeout: 20
      user: postgres
      password: ''
      database: postgres
      max_retries: 3
      retry_delay: 1
      connect_timeout: 10000
  
  # Backend 1 (Standby/Replica)
  - hostname: localhost
    port: 5434
    weight: 1
    data_directory: ${default_data_dir}2
    flag: ALLOW_TO_FAILOVER
    application_name: server1
    
    health_check:
      period: 30
      timeout: 20
      user: postgres
      password: ''
      database: postgres
      max_retries: 3
      retry_delay: 1
      connect_timeout: 10000

# Watchdog (High Availability)
watchdog:
  enabled: off
  trusted_servers: ''
  ping_path: /bin
  priority: 1
  authkey: ''
  ipc_socket_dir: ${default_socket_dir}
  delegate_ip: ''
  if_cmd_path: /sbin
  monitoring_interfaces_list: ''
  lifecheck_method: heartbeat
  interval: 10
  heartbeat_port: 9694
  heartbeat_keepalive: 2
  heartbeat_deadtime: 30

# Query Cache (In-Memory Result Cache)
query_cache:
  enabled: off
  method: shmem
  memcached_host: localhost
  memcached_port: 11211
  total_size: 67108864
  max_num_cache: 1000000
  expire: 0
  auto_cache_invalidation: on
  maxcache: 409600
  cache_block_size: 1048576
  oiddir: ${default_log_dir}/oiddir
EOF
        log_success "Generated YAML configuration"
    fi
    
    log_success "Created configuration files"
    log_info "  • pgbalancer.conf.sample - Traditional format"
    log_info "  • pgbalancer.yaml - YAML format (ready to use!)"
    
    # Copy additional sample files from src/sample directory
    log_step "Copying sample scripts and auxiliary files..."
    if [[ -d "$SCRIPT_DIR/src/sample/scripts" ]]; then
        mkdir -p "$build_dir/etc/scripts"
        for script in "$SCRIPT_DIR/src/sample/scripts"/*; do
            if [[ -f "$script" ]]; then
                cp -v "$script" "$build_dir/etc/scripts/" >> "$BUILD_LOG" 2>&1
            fi
        done
        log_success "Copied sample scripts to build/etc/scripts/"
    fi
    
    # Copy pool_hba.conf.sample and other auxiliary files
    for aux_file in pool_hba.conf.sample pgpool.pam pgpool_recovery; do
        if [[ -f "$SCRIPT_DIR/src/sample/$aux_file" ]]; then
            cp -v "$SCRIPT_DIR/src/sample/$aux_file" "$build_dir/etc/" >> "$BUILD_LOG" 2>&1
            log_success "Copied $aux_file"
        fi
    done
    
    # Copy documentation
    log_step "Copying documentation to build/share/doc..."
    for doc in README.md LICENSE CONTRIBUTING.md; do
        if [[ -f "$SCRIPT_DIR/$doc" ]]; then
            cp -v "$SCRIPT_DIR/$doc" "$build_dir/share/doc/" | tee -a "$BUILD_LOG"
        fi
    done
    
    # Create build info
    cat > "$build_dir/BUILD_INFO.txt" << EOF
╔═══════════════════════════════════════════════════════════════════════╗
║                      pgbalancer Build Information                     ║
╚═══════════════════════════════════════════════════════════════════════╝

Build Date: $(date)
Build Host: $(hostname)
Operating System: $OS ($ARCH)
PostgreSQL Version: $PG_VERSION
PostgreSQL Path: $PG_PREFIX

Build Configuration:
  CPPFLAGS: $CPPFLAGS
  LDFLAGS: $LDFLAGS
  Configure Options: $CONFIGURE_OPTS

Build Directory Structure:
  bin/          - Executable binaries
  lib/          - Static libraries
  etc/          - Configuration file samples
  share/doc/    - Documentation

Binaries:
EOF
    
    ls -lh "$build_dir/bin/" >> "$build_dir/BUILD_INFO.txt" 2>/dev/null || true
    
    log_success "Build structure completed!"
    log_info "Build output directory: $build_dir"
    
    # Set permissions
    chmod +x "$build_dir"/bin/* 2>/dev/null || true
    
    return 0
}

install_binaries() {
    log_step "Installing binaries..."
    
    read -p "Install to /usr/local? (requires sudo) (y/n) " -n 1 -r
    echo
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        cd "$SCRIPT_DIR"
        sudo make install
        log_success "Installation completed"
    else
        log_info "Skipping installation. Binaries available in:"
        log_info "  - pgbalancer: $SCRIPT_DIR/src/pgbalancer"
        log_info "  - bctl: $SCRIPT_DIR/bctl/bctl"
    fi
}

#==============================================================================
# Main Script
#===========================================================================
show_banner() {
    cat << 'EOF'
╔═══════════════════════════════════════════════════════════════════════╗
║                                                                       ║
║   ▄▄▄▄▄  ▄▄▄▄  ▄▄▄▄   ▄▄▄  ▄     ▄▄▄  ▄▄   ▄  ▄▄▄▄ ▄▄▄▄▄ ▄▄▄▄         ║
║   █   █ █    █ █   █ █   █ █    █   █ █ █  █ █    █   █ █   █         ║
║   █▄▄▄█ █  ▄▄█ █▄▄▄█ █▄▄▄█ █    █▄▄▄█ █  █ █ █      █   █▄▄▄█         ║
║   █     █   █  █   █ █   █ █    █   █ █  ██  █      █   █   █         ║
║   █     █▄▄▄▄█ █▄▄▄█ █   █ █▄▄▄ █   █ █   █  █▄▄▄▄ █   █   █          ║
║                                                                       ║
║              Modern PostgreSQL Connection Pooler & Load Balancer      ║
║                        Build Script v1.0.0                            ║
║                                                                       ║
╚═══════════════════════════════════════════════════════════════════════╝
EOF
}

show_usage() {
    cat << EOF

Usage: $0 [OPTIONS]

Options:
    -h, --help          Show this help message
    -v, --verbose       Verbose output
    -c, --clean         Clean build (remove previous build artifacts)
    -i, --install       Install binaries after building
    --skip-bctl         Skip building bctl CLI tool
    --jobs N            Number of parallel make jobs (default: auto-detect)

Environment Variables:
    PG_PREFIX           PostgreSQL installation prefix
    VERBOSE             Enable verbose output (0 or 1)

Examples:
    $0                  # Auto-detect and build
    $0 -v -i            # Verbose build with installation
    $0 --clean --jobs 8 # Clean build with 8 parallel jobs

EOF
}

main() {
    local clean_build=0
    local install_after=0
    local skip_bctl=0
    
    # Parse command line arguments
    while [[ $# -gt 0 ]]; do
        case $1 in
            -h|--help)
                show_usage
                exit 0
                ;;
            -v|--verbose)
                VERBOSE=1
                shift
                ;;
            -c|--clean)
                clean_build=1
                shift
                ;;
            -i|--install)
                install_after=1
                shift
                ;;
            --skip-bctl)
                skip_bctl=1
                shift
                ;;
            --jobs)
                MAKE_JOBS="$2"
                shift 2
                ;;
            *)
                log_error "Unknown option: $1"
                show_usage
                exit 1
                ;;
        esac
    done
    
    # Initialize build log
    echo "pgbalancer Build Log - $(date)" > "$BUILD_LOG"
    
    show_banner
    
    log_info "Build log: $BUILD_LOG"
    log_info "Parallel jobs: $MAKE_JOBS"
    
    # Detect OS and setup environment
    detect_os
    
    case "$OS" in
        macos)
            check_homebrew
            find_macos_dependencies
            setup_macos_build_env
            ;;
        ubuntu)
            check_apt
            find_ubuntu_dependencies
            setup_ubuntu_build_env
            ;;
        rhel)
            check_yum
            find_rhel_dependencies
            setup_rhel_build_env
            ;;
        *)
            log_error "Unsupported OS: $OS"
            exit 1
            ;;
    esac
    
    # Build steps
    if [[ $clean_build -eq 1 ]]; then
        run_make_clean
    fi
    
    run_configure
    run_make
    
    if [[ $skip_bctl -eq 0 ]]; then
        build_bctl || log_warn "bctl build failed, continuing..."
    fi
    
    # Build pgbalancer_config utility
    build_pgbalancer_config || log_warn "pgbalancer_config build failed, continuing..."
    
    # Verify build
    if verify_build; then
        log_success "Build verification passed!"
        
        # Create organized build directory structure
        create_build_structure
        
        echo ""
        log_success "╔═══════════════════════════════════════════════════════════╗"
        log_success "║  ✓ Build completed successfully!                         ║"
        log_success "╚═══════════════════════════════════════════════════════════╝"
    echo ""
    
        log_info "Build artifacts:"
        log_info "  • Source binary:  $SCRIPT_DIR/src/pgbalancer"
        [[ -x "$SCRIPT_DIR/bctl/bctl" ]] && log_info "  • Source bctl:    $SCRIPT_DIR/bctl/bctl"
        echo ""
        log_info "Organized build directory:"
        log_info "  • Build root:     $SCRIPT_DIR/build/"
        log_info "  • Binaries:       $SCRIPT_DIR/build/bin/"
        log_info "  • Libraries:      $SCRIPT_DIR/build/lib/"
        log_info "  • Config samples: $SCRIPT_DIR/build/etc/"
        log_info "  • Documentation:  $SCRIPT_DIR/build/share/doc/"
    echo ""
    
        log_info "Test the build:"
        log_info "  $SCRIPT_DIR/build/bin/pgbalancer --version"
        [[ -x "$SCRIPT_DIR/build/bin/bctl" ]] && log_info "  $SCRIPT_DIR/build/bin/bctl --help"
    echo ""
    
        if [[ $install_after -eq 1 ]]; then
            install_binaries
        else
            log_info "To install system-wide:"
            log_info "  sudo make install"
            log_info "  Or: $0 -i"
        echo ""
            log_info "To use from build directory:"
            log_info "  export PATH=\"$SCRIPT_DIR/build/bin:\$PATH\""
        fi
        
        echo ""
        log_success "Build information saved to: $SCRIPT_DIR/build/BUILD_INFO.txt"
        log_info "Full build log: $BUILD_LOG"
        
    else
        fatal_error \
            "Build verification failed!" \
            "" \
            "One or more binaries were not created successfully." \
            "" \
            "Check the build log:" \
            "  tail -100 $BUILD_LOG" \
            "" \
            "Try a clean rebuild:" \
            "  ./build.sh --clean -v"
    fi
}

# Run main function
main "$@"
