# Security Policy

## Supported Versions

We provide security updates for the following versions of pgbalancer:

| Version | PostgreSQL Versions | Supported          |
|---------|---------------------|:------------------:|
| 1.0.x   | 13, 14, 15, 16, 17, 18 | :white_check_mark: |

## Reporting a Vulnerability

We take the security of pgbalancer seriously. If you discover a security vulnerability, please follow these steps:

### 1. **Do Not** Open a Public Issue

Please do not create a public GitHub issue for security vulnerabilities.

### 2. Report Privately

Send a detailed report to: **security@pgelephant.org**

Include:
- Description of the vulnerability
- Steps to reproduce
- Potential impact (authentication bypass, information disclosure, DoS, etc.)
- Affected versions
- Suggested fix (if any)
- Your contact information

### 3. Response Timeline

- **Initial Response**: Within 48 hours
- **Status Update**: Within 7 days
- **Fix Timeline**: Depends on severity
  - Critical: 1-3 days
  - High: 1-2 weeks
  - Medium: Next release
  - Low: Future release

### 4. Disclosure Process

1. We will confirm receipt of your report
2. We will investigate and validate the issue
3. We will develop and test a fix
4. We will coordinate disclosure timing with you
5. We will release a security update
6. We will publicly acknowledge your contribution (unless you prefer to remain anonymous)

## Security Best Practices

### Production Deployments

1. **Authentication & Authorization**
   - Enable JWT authentication for REST API
   - Use strong JWT secrets (256-bit minimum)
   - Restrict API access to trusted networks
   - Use SSL/TLS for all connections
   - Configure pg_hba.conf properly

2. **Network Security**
   - Bind REST API to localhost if not needed externally
   - Use firewall rules to restrict access
   - Enable SSL/TLS for PostgreSQL connections
   - Use VPN for cross-datacenter communication

3. **Configuration Security**
   - Protect configuration files (chmod 600)
   - Don't commit passwords to version control
   - Use .pgpass or connection service files
   - Rotate credentials regularly

4. **Monitoring & Logging**
   - Enable audit logging
   - Monitor failed authentication attempts
   - Track API access patterns
   - Set up alerts for suspicious activity

### Secure Configuration Example

```yaml
# pgbalancer.yml
listen_addresses: '127.0.0.1'  # Localhost only
port: 9999

# JWT Authentication (recommended)
rest_api:
  enabled: true
  port: 8080
  bind_address: '127.0.0.1'
  jwt_secret: 'your-256-bit-secret-key-here-change-this'
  jwt_enabled: true

# SSL/TLS
ssl: true
ssl_cert: '/path/to/cert.pem'
ssl_key: '/path/to/key.pem'
ssl_ca_cert: '/path/to/ca.pem'

# Connection encryption
backend_encryption: true

# Logging
log_connections: true
log_disconnections: true
log_statement: 'all'
```

### Example: Restrict API Access

```bash
# Firewall rules (iptables)
sudo iptables -A INPUT -p tcp --dport 8080 -s 10.0.0.0/8 -j ACCEPT
sudo iptables -A INPUT -p tcp --dport 8080 -j DROP

# Or use nginx as reverse proxy
upstream pgbalancer_api {
    server 127.0.0.1:8080;
}

server {
    listen 443 ssl;
    ssl_certificate /path/to/cert.pem;
    ssl_certificate_key /path/to/key.pem;
    
    location /api/ {
        proxy_pass http://pgbalancer_api;
        proxy_set_header Authorization $http_authorization;
    }
}
```

## Known Security Considerations

### REST API Exposure

- **Issue**: REST API exposes cluster configuration and status
- **Mitigation**: Enable JWT authentication, bind to localhost, use firewall rules
- **Severity**: Medium (if not properly configured)

### Connection Pool Exhaustion

- **Issue**: Malicious clients could exhaust connection pool
- **Mitigation**: Set max_pool, use connection limits, enable rate limiting
- **Severity**: Medium (DoS potential)

### Password Storage

- **Issue**: Backend passwords stored in configuration
- **Mitigation**: Use .pgpass file, connection service file, or encrypted config
- **Severity**: High (if config file permissions not restricted)

### Information Disclosure

- **Issue**: Error messages may reveal internal structure
- **Mitigation**: Customize error messages, use generic responses in production
- **Severity**: Low

## Security Features

### JWT Authentication

pgbalancer supports JWT tokens for REST API authentication:

```bash
# Generate token
bctl --jwt-secret "your-secret" --generate-token

# Use token
curl -H "Authorization: Bearer YOUR_TOKEN" http://localhost:8080/api/v1/status
```

### SSL/TLS Support

Full SSL/TLS support for:
- Client connections
- Backend connections
- REST API (via reverse proxy)

### Access Control

- Role-based access in PostgreSQL
- pg_hba.conf integration
- IP-based restrictions
- JWT claims validation

## Audit Trail

Enable comprehensive logging:

```yaml
log_destination: 'stderr,syslog'
log_line_prefix: '%t [%p]: user=%u,db=%d,app=%a,client=%h '
log_connections: true
log_disconnections: true
log_duration: true
log_statement: 'all'
```

## Compliance

pgbalancer can help meet various compliance requirements:

- **SOC 2**: Logging, access control, encryption
- **HIPAA**: Encryption, audit trails, access restrictions
- **PCI DSS**: Encryption, logging, access control
- **GDPR**: Data protection, audit logs

## Security Updates

Security updates will be released as:
- Patch versions (e.g., 1.0.1, 1.0.2) for security fixes
- Announced via GitHub Security Advisories
- Notified to all users who watch the repository

## Responsible Disclosure

We follow responsible disclosure practices:
- 90-day disclosure timeline for vulnerabilities
- Coordinated disclosure with security researchers
- Public acknowledgment of researchers (if desired)
- CVE assignment for significant vulnerabilities

## Security Checklist for Production

- [ ] Enable JWT authentication for REST API
- [ ] Bind REST API to localhost or trusted network only
- [ ] Use SSL/TLS for all connections
- [ ] Restrict config file permissions (chmod 600)
- [ ] Don't store passwords in config files
- [ ] Enable comprehensive logging
- [ ] Set up monitoring and alerts
- [ ] Regular security updates
- [ ] Firewall rules configured
- [ ] Regular credential rotation
- [ ] Backup and disaster recovery plan

## Contact

**Security Issues**: security@pgelephant.org  
**General Questions**: team@pgelephant.org  
**GitHub Issues**: https://github.com/pgelephant/pgbalancer/issues

---

**Last Updated**: October 27, 2024

