#!/bin/bash
# FieryMUD TLS Certificate Setup Script
# Creates self-signed certificates for development testing

set -e

echo "Setting up TLS certificates for FieryMUD development..."

# Create certificates directory
mkdir -p certs
cd certs

# Generate private key
echo "Generating private key..."
openssl genpkey -algorithm RSA -out server.key -pkcs8 -aes256 -pass pass:fierymud

# Remove password from key for development ease
echo "Removing password from private key for development..."
openssl rsa -in server.key -out server.key -passin pass:fierymud

# Generate certificate signing request
echo "Generating certificate signing request..."
openssl req -new -key server.key -out server.csr -config <(
cat <<EOF
[req]
default_bits = 2048
prompt = no
distinguished_name = dn
req_extensions = v3_req

[dn]
CN=localhost
C=US
ST=Development
L=FieryMUD
O=FieryMUD Development
OU=MUD Server

[v3_req]
basicConstraints = CA:FALSE
keyUsage = nonRepudiation, digitalSignature, keyEncipherment
subjectAltName = @alt_names

[alt_names]
DNS.1 = localhost
DNS.2 = 127.0.0.1
DNS.3 = ::1
IP.1 = 127.0.0.1
IP.2 = ::1
EOF
)

# Generate self-signed certificate
echo "Generating self-signed certificate..."
openssl x509 -req -in server.csr -signkey server.key -out server.crt -days 365 -extensions v3_req -extfile <(
cat <<EOF
[v3_req]
basicConstraints = CA:FALSE
keyUsage = nonRepudiation, digitalSignature, keyEncipherment
subjectAltName = @alt_names

[alt_names]
DNS.1 = localhost
DNS.2 = 127.0.0.1
DNS.3 = ::1
IP.1 = 127.0.0.1
IP.2 = ::1
EOF
)

# Generate DH parameters for enhanced security
echo "Generating DH parameters (this may take a while)..."
openssl dhparam -out dhparams.pem 2048

# Set proper permissions
chmod 600 server.key
chmod 644 server.crt dhparams.pem

echo ""
echo "TLS certificates generated successfully!"
echo "Files created:"
echo "  certs/server.crt - Server certificate"
echo "  certs/server.key - Private key"
echo "  certs/dhparams.pem - DH parameters"
echo ""
echo "Certificate details:"
openssl x509 -in server.crt -text -noout | grep -A 2 "Subject:"
openssl x509 -in server.crt -text -noout | grep -A 5 "Subject Alternative Name"
echo ""
echo "Certificate expires:"
openssl x509 -in server.crt -dates -noout | grep notAfter
echo ""
echo "To enable TLS, update your server configuration to set enable_tls = true"
echo "The server will listen on port 4000 (plain) and port 4443 (TLS)"