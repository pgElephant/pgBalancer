#!/usr/bin/env python3
"""
pgBalancer Cluster Manager
A modular tool to create and manage pgBalancer clusters using Docker

Usage:
    ./pgbalancer_cluster.py --init [--config pgbalancer_cluster.json]
    ./pgbalancer_cluster.py --destroy [--config pgbalancer_cluster.json]
    ./pgbalancer_cluster.py --status [--config pgbalancer_cluster.json]
    ./pgbalancer_cluster.py --add-replica --balancer <name> --count <n>
    ./pgbalancer_cluster.py --remove-replica --balancer <name> --node <id>
"""

import argparse
import json
import subprocess
import sys
import time
import os
from pathlib import Path
from typing import Dict, List, Optional
from dataclasses import dataclass, field, asdict


@dataclass
class PostgreSQLNode:
    """Represents a PostgreSQL node (primary or replica)"""
    node_id: int
    role: str  # 'primary' or 'replica'
    host: str
    port: int
    container_name: str
    ip_address: str
    data_directory: str = "/var/lib/postgresql/data/pgdata"
    status: str = "stopped"
    
    def to_dict(self):
        return asdict(self)


@dataclass
class PgBalancer:
    """Represents a pgBalancer instance"""
    name: str
    port: int
    pcp_port: int
    rest_api_port: int
    container_name: str
    ip_address: str
    primary: Optional[PostgreSQLNode] = None
    replicas: List[PostgreSQLNode] = field(default_factory=list)
    config: Dict = field(default_factory=dict)
    status: str = "stopped"
    
    def get_all_nodes(self) -> List[PostgreSQLNode]:
        """Get all PostgreSQL nodes (primary + replicas)"""
        nodes = []
        if self.primary:
            nodes.append(self.primary)
        nodes.extend(self.replicas)
        return nodes
    
    def to_dict(self):
        data = asdict(self)
        data['primary'] = self.primary.to_dict() if self.primary else None
        data['replicas'] = [r.to_dict() for r in self.replicas]
        return data


@dataclass
class ClusterConfig:
    """Complete cluster configuration"""
    cluster_name: str
    network_name: str
    network_subnet: str
    balancers: List[PgBalancer] = field(default_factory=list)
    
    def to_dict(self):
        data = asdict(self)
        data['balancers'] = [b.to_dict() for b in self.balancers]
        return data


class DockerManager:
    """Handles Docker operations"""
    
    verbose = False  # Class variable for verbose mode
    
    @staticmethod
    def run_command(cmd: List[str], capture_output=True, silent_errors=False) -> subprocess.CompletedProcess:
        """Execute a shell command"""
        if DockerManager.verbose:
            print(f"🔧 Executing: {' '.join(cmd)}")
        
        try:
            result = subprocess.run(
                cmd,
                capture_output=capture_output,
                text=True,
                check=True
            )
            if DockerManager.verbose and result.stdout:
                print(f"   Output: {result.stdout.strip()[:200]}")
            return result
        except subprocess.CalledProcessError as e:
            if not silent_errors:
                print(f"❌ Command failed: {' '.join(cmd)}")
                print(f"Error: {e.stderr}")
            raise
    
    @staticmethod
    def create_network(name: str, subnet: str):
        """Create Docker network"""
        print(f"📡 Creating network: {name}")
        if DockerManager.verbose:
            print(f"   Subnet: {subnet}")
        try:
            DockerManager.run_command([
                "docker", "network", "create",
                "--driver", "bridge",
                "--subnet", subnet,
                name
            ])
            print(f"✅ Network {name} created")
        except subprocess.CalledProcessError:
            print(f"ℹ️  Network {name} already exists")
    
    @staticmethod
    def remove_network(name: str):
        """Remove Docker network"""
        print(f"📡 Removing network: {name}")
        try:
            DockerManager.run_command(["docker", "network", "rm", name])
            print(f"✅ Network {name} removed")
        except subprocess.CalledProcessError:
            print(f"ℹ️  Network {name} doesn't exist")
    
    @staticmethod
    def create_postgres_container(node: PostgreSQLNode, network: str):
        """Create PostgreSQL container"""
        print(f"🐘 Creating PostgreSQL {node.role}: {node.container_name}")
        if DockerManager.verbose:
            print(f"   Host: {node.host} | IP: {node.ip_address} | Port: {node.port}")
        
        cmd = [
            "docker", "run", "-d",
            "--name", node.container_name,
            "--hostname", node.host,
            "--network", network,
            "--ip", node.ip_address,
            "-p", f"{node.port}:5432",
            "-e", "POSTGRES_PASSWORD=postgres",
            "-e", "POSTGRES_USER=postgres",
            "-e", "POSTGRES_DB=testdb",
            "-e", "PGDATA=/var/lib/postgresql/data/pgdata",
            "--health-cmd", "pg_isready -U postgres",
            "--health-interval", "10s",
            "--health-timeout", "5s",
            "--health-retries", "5",
            "--health-start-period", "30s"
        ]
        
        # Add volume mounts BEFORE the image name
        if node.role == "primary":
            # Mount primary initialization script
            script_path = Path(__file__).parent / "postgres" / "primary" / "init.sql"
            if script_path.exists():
                if DockerManager.verbose:
                    print(f"   Mounting init script: {script_path}")
                cmd.extend(["-v", f"{script_path}:/docker-entrypoint-initdb.d/01-init.sql:ro"])
        
        # Add the image name at the end
        cmd.append("postgres:17")
        
        DockerManager.run_command(cmd)
        print(f"✅ {node.container_name} created")
    
    @staticmethod
    def create_pgbalancer_container(balancer: PgBalancer, network: str):
        """Create pgBalancer container"""
        print(f"⚖️  Creating pgBalancer: {balancer.container_name}")
        
        # Check if pgBalancer image exists
        try:
            DockerManager.run_command(
                ["docker", "image", "inspect", "pgbalancer:latest"],
                silent_errors=True
            )
        except subprocess.CalledProcessError:
            print(f"⚠️  pgBalancer image not found. Skipping pgBalancer creation.")
            print(f"   Build the image with: cd /Users/ibrarahmed/pgelephant/pge/pgbalancer/cluster && make build")
            print(f"   PostgreSQL containers will continue running independently.")
            return
        
        # Build environment variables for backends
        env_vars = []
        all_nodes = balancer.get_all_nodes()
        
        for i, node in enumerate(all_nodes):
            env_vars.extend([
                "-e", f"BACKEND{i}_HOST={node.host}",
                "-e", f"BACKEND{i}_PORT=5432",
                "-e", f"BACKEND{i}_WEIGHT=1",
                "-e", f"BACKEND{i}_DATA_DIRECTORY={node.data_directory}"
            ])
        
        # Add pgBalancer configuration
        env_vars.extend([
            "-e", f"NUM_INIT_CHILDREN={balancer.config.get('num_init_children', 32)}",
            "-e", f"MAX_POOL={balancer.config.get('max_pool', 4)}",
            "-e", "LOAD_BALANCE_MODE=on",
            "-e", "ENABLE_REST_API=on",
            "-e", f"REST_API_PORT={balancer.rest_api_port}",
            "-e", "HEALTH_CHECK_USER=postgres",
            "-e", "HEALTH_CHECK_PASSWORD=postgres",
            "-e", "SR_CHECK_USER=postgres",
            "-e", "SR_CHECK_PASSWORD=postgres",
            "-e", "WAIT_FOR_BACKENDS=yes"
        ])
        
        cmd = [
            "docker", "run", "-d",
            "--name", balancer.container_name,
            "--hostname", balancer.name,
            "--network", network,
            "--ip", balancer.ip_address,
            "-p", f"{balancer.port}:9999",
            "-p", f"{balancer.pcp_port}:9898",
            "-p", f"{balancer.rest_api_port}:8080",
            "--health-cmd", "pgrep pgbalancer",
            "--health-interval", "30s",
            "--health-timeout", "10s",
            "--health-retries", "3",
            "--health-start-period", "60s",
        ]
        
        cmd.extend(env_vars)
        cmd.append("pgbalancer:latest")
        
        DockerManager.run_command(cmd)
        print(f"✅ {balancer.container_name} created")
    
    @staticmethod
    def stop_container(name: str):
        """Stop a container"""
        try:
            DockerManager.run_command(["docker", "stop", name])
            print(f"🛑 Stopped: {name}")
        except subprocess.CalledProcessError:
            pass
    
    @staticmethod
    def remove_container(name: str):
        """Remove a container"""
        try:
            DockerManager.run_command(["docker", "rm", "-f", name])
            print(f"🗑️  Removed: {name}")
        except subprocess.CalledProcessError:
            pass
    
    @staticmethod
    def get_container_status(name: str) -> str:
        """Get container status"""
        try:
            result = DockerManager.run_command([
                "docker", "inspect",
                "--format", "{{.State.Status}}",
                name
            ], silent_errors=True)
            return result.stdout.strip()
        except subprocess.CalledProcessError:
            return "not_found"
    
    @staticmethod
    def get_container_health(name: str) -> str:
        """Get container health"""
        try:
            result = DockerManager.run_command([
                "docker", "inspect",
                "--format", "{{.State.Health.Status}}",
                name
            ], silent_errors=True)
            health = result.stdout.strip()
            return health if health else "no_healthcheck"
        except subprocess.CalledProcessError:
            return "unknown"


class ClusterManager:
    """Main cluster management class"""
    
    def __init__(self, config_file: str = "pgbalancer_cluster.json"):
        self.config_file = config_file
        self.cluster: Optional[ClusterConfig] = None
    
    def load_config(self):
        """Load configuration from JSON file"""
        print(f"📖 Loading configuration from: {self.config_file}")
        
        if not Path(self.config_file).exists():
            print(f"❌ Configuration file not found: {self.config_file}")
            sys.exit(1)
        
        with open(self.config_file, 'r') as f:
            data = json.load(f)
        
        # Parse configuration
        cluster = ClusterConfig(
            cluster_name=data['cluster_name'],
            network_name=data['network']['name'],
            network_subnet=data['network']['subnet']
        )
        
        # Parse balancers
        for bal_data in data['balancers']:
            # Create primary node
            primary = PostgreSQLNode(
                node_id=0,
                role='primary',
                host=bal_data['primary']['host'],
                port=bal_data['primary']['port'],
                container_name=bal_data['primary']['container_name'],
                ip_address=bal_data['primary']['ip_address']
            )
            
            # Create replica nodes
            replicas = []
            for rep_data in bal_data.get('replicas', []):
                replica = PostgreSQLNode(
                    node_id=rep_data['node_id'],
                    role='replica',
                    host=rep_data['host'],
                    port=rep_data['port'],
                    container_name=rep_data['container_name'],
                    ip_address=rep_data['ip_address']
                )
                replicas.append(replica)
            
            # Create balancer
            balancer = PgBalancer(
                name=bal_data['name'],
                port=bal_data['port'],
                pcp_port=bal_data['pcp_port'],
                rest_api_port=bal_data['rest_api_port'],
                container_name=bal_data['container_name'],
                ip_address=bal_data['ip_address'],
                primary=primary,
                replicas=replicas,
                config=bal_data.get('config', {})
            )
            
            cluster.balancers.append(balancer)
        
        self.cluster = cluster
        print(f"✅ Configuration loaded: {cluster.cluster_name}")
        print(f"   Balancers: {len(cluster.balancers)}")
        for bal in cluster.balancers:
            print(f"     - {bal.name}: 1 primary + {len(bal.replicas)} replicas")
    
    def init_cluster(self):
        """Initialize the entire cluster"""
        if not self.cluster:
            self.load_config()
        
        print("\n" + "="*70)
        print(f"🚀 Initializing Cluster: {self.cluster.cluster_name}")
        print("="*70 + "\n")
        
        # Create network
        DockerManager.create_network(
            self.cluster.network_name,
            self.cluster.network_subnet
        )
        
        # Create all components for each balancer
        for balancer in self.cluster.balancers:
            print(f"\n{'─'*70}")
            print(f"⚖️  Setting up pgBalancer: {balancer.name}")
            print(f"{'─'*70}\n")
            
            # Create primary
            DockerManager.create_postgres_container(
                balancer.primary,
                self.cluster.network_name
            )
            
            # Wait for primary to be healthy
            print("⏳ Waiting for primary to be healthy (this may take 60-90 seconds)...")
            self._wait_for_health(balancer.primary.container_name, timeout=90)
            
            # Create replicas
            for replica in balancer.replicas:
                DockerManager.create_postgres_container(
                    replica,
                    self.cluster.network_name
                )
                time.sleep(2)
            
            # Wait for replicas
            if balancer.replicas:
                print("⏳ Waiting for replicas to be healthy (30-60 seconds each)...")
                for replica in balancer.replicas:
                    self._wait_for_health(replica.container_name, timeout=90)
            
            # Create pgBalancer
            time.sleep(5)  # Give backends time to fully start
            DockerManager.create_pgbalancer_container(
                balancer,
                self.cluster.network_name
            )
        
        print("\n" + "="*70)
        print("✅ Cluster initialization complete!")
        print("="*70 + "\n")
        
        # Show status
        time.sleep(10)
        self.show_status()
    
    def destroy_cluster(self):
        """Destroy the entire cluster"""
        if not self.cluster:
            self.load_config()
        
        print("\n" + "="*70)
        print(f"🗑️  Destroying Cluster: {self.cluster.cluster_name}")
        print("="*70 + "\n")
        
        # Stop and remove all containers
        for balancer in self.cluster.balancers:
            print(f"\n⚖️  Removing pgBalancer: {balancer.name}")
            
            # Remove balancer
            DockerManager.remove_container(balancer.container_name)
            
            # Remove primary
            DockerManager.remove_container(balancer.primary.container_name)
            
            # Remove replicas
            for replica in balancer.replicas:
                DockerManager.remove_container(replica.container_name)
        
        # Remove network
        DockerManager.remove_network(self.cluster.network_name)
        
        print("\n" + "="*70)
        print("✅ Cluster destroyed!")
        print("="*70 + "\n")
    
    def show_status(self):
        """Show cluster status"""
        if not self.cluster:
            self.load_config()
        
        print("\n" + "="*70)
        print(f"📊 Cluster Status: {self.cluster.cluster_name}")
        print("="*70 + "\n")
        
        for balancer in self.cluster.balancers:
            print(f"⚖️  pgBalancer: {balancer.name}")
            print(f"   Container: {balancer.container_name}")
            status = DockerManager.get_container_status(balancer.container_name)
            health = DockerManager.get_container_health(balancer.container_name)
            print(f"   Status: {status} | Health: {health}")
            print(f"   Ports: {balancer.port} (main), {balancer.pcp_port} (PCP), {balancer.rest_api_port} (REST)")
            print(f"   IP: {balancer.ip_address}")
            
            print(f"\n   📦 Backend Nodes:")
            
            # Primary
            node = balancer.primary
            status = DockerManager.get_container_status(node.container_name)
            health = DockerManager.get_container_health(node.container_name)
            print(f"      [PRIMARY] {node.container_name}")
            print(f"                Host: {node.host}:{node.port} | IP: {node.ip_address}")
            print(f"                Status: {status} | Health: {health}")
            
            # Replicas
            for replica in balancer.replicas:
                status = DockerManager.get_container_status(replica.container_name)
                health = DockerManager.get_container_health(replica.container_name)
                print(f"      [REPLICA {replica.node_id}] {replica.container_name}")
                print(f"                   Host: {replica.host}:{replica.port} | IP: {replica.ip_address}")
                print(f"                   Status: {status} | Health: {health}")
            
            print()
        
        print("="*70 + "\n")
    
    def _wait_for_health(self, container_name: str, timeout: int = 60):
        """Wait for a container to become healthy"""
        start_time = time.time()
        checks = 0
        while time.time() - start_time < timeout:
            health = DockerManager.get_container_health(container_name)
            status = DockerManager.get_container_status(container_name)
            checks += 1
            
            if health == "healthy":
                print(f"   ✅ {container_name} is healthy (after {checks} checks)")
                return
            
            time.sleep(2)
            if DockerManager.verbose:
                elapsed = int(time.time() - start_time)
                print(f"   ⏳ Check {checks}: status={status}, health={health}, elapsed={elapsed}s")
            else:
                print(f"   ⏳ Waiting... ({health})")
        
        elapsed = int(time.time() - start_time)
        print(f"   ⚠️  {container_name} did not become healthy in {timeout}s ({checks} checks)")
        if DockerManager.verbose:
            # Show logs for debugging
            try:
                result = DockerManager.run_command(
                    ["docker", "logs", "--tail", "20", container_name],
                    silent_errors=True
                )
                print(f"   Last 20 log lines:")
                for line in result.stdout.splitlines()[-5:]:
                    print(f"      {line}")
            except:
                pass
    
    def add_replica(self, balancer_name: str, count: int = 1):
        """Add replica(s) to a balancer"""
        if not self.cluster:
            self.load_config()
        
        # Find balancer
        balancer = next((b for b in self.cluster.balancers if b.name == balancer_name), None)
        if not balancer:
            print(f"❌ Balancer '{balancer_name}' not found")
            return
        
        print(f"➕ Adding {count} replica(s) to {balancer_name}")
        
        # Determine next node ID and IP
        existing_ids = [r.node_id for r in balancer.replicas]
        next_id = max(existing_ids) + 1 if existing_ids else 1
        
        # Parse IP for incrementing
        base_ip = balancer.ip_address.rsplit('.', 1)[0]
        last_octet = int(balancer.replicas[-1].ip_address.rsplit('.', 1)[1]) if balancer.replicas else int(balancer.primary.ip_address.rsplit('.', 1)[1])
        
        for i in range(count):
            node_id = next_id + i
            ip = f"{base_ip}.{last_octet + i + 1}"
            port = 15430 + node_id
            
            replica = PostgreSQLNode(
                node_id=node_id,
                role='replica',
                host=f"{balancer_name}-replica{node_id}",
                port=port,
                container_name=f"{balancer_name}_replica{node_id}",
                ip_address=ip
            )
            
            DockerManager.create_postgres_container(replica, self.cluster.network_name)
            balancer.replicas.append(replica)
        
        print(f"✅ Added {count} replica(s)")
        
        # TODO: Reload pgBalancer configuration
        print("⚠️  Note: Restart pgBalancer to use new replicas")


def main():
    parser = argparse.ArgumentParser(
        description="pgBalancer Cluster Manager",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Initialize cluster from config
  ./pgbalancer_cluster.py --init

  # Check cluster status
  ./pgbalancer_cluster.py --status

  # Destroy cluster
  ./pgbalancer_cluster.py --destroy

  # Add 2 replicas to a balancer
  ./pgbalancer_cluster.py --add-replica --balancer lb1 --count 2
  
  # Verbose mode
  ./pgbalancer_cluster.py -v --init
        """
    )
    
    parser.add_argument('--config', default='pgbalancer_cluster.json',
                       help='Configuration file (default: pgbalancer_cluster.json)')
    parser.add_argument('-v', '--verbose', action='store_true',
                       help='Verbose output (show Docker commands)')
    parser.add_argument('--init', action='store_true',
                       help='Initialize the cluster')
    parser.add_argument('--destroy', action='store_true',
                       help='Destroy the cluster')
    parser.add_argument('--status', action='store_true',
                       help='Show cluster status')
    parser.add_argument('--add-replica', action='store_true',
                       help='Add replica(s) to a balancer')
    parser.add_argument('--balancer', help='Balancer name')
    parser.add_argument('--count', type=int, default=1,
                       help='Number of replicas to add')
    
    args = parser.parse_args()
    
    if len(sys.argv) == 1:
        parser.print_help()
        sys.exit(0)
    
    # Set verbose mode
    if args.verbose:
        DockerManager.verbose = True
        print("🔍 Verbose mode enabled\n")
    
    manager = ClusterManager(args.config)
    
    try:
        if args.init:
            manager.init_cluster()
        elif args.destroy:
            manager.destroy_cluster()
        elif args.status:
            manager.show_status()
        elif args.add_replica:
            if not args.balancer:
                print("❌ --balancer required for --add-replica")
                sys.exit(1)
            manager.add_replica(args.balancer, args.count)
        else:
            parser.print_help()
    
    except KeyboardInterrupt:
        print("\n\n⚠️  Operation cancelled by user")
        sys.exit(1)
    except Exception as e:
        print(f"\n❌ Error: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)


if __name__ == "__main__":
    main()

