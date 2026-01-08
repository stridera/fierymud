# FieryMUD

The original vision of FieryMUD was to create a challenging MUD for advanced players. This Fiery is reborn in the hope of bringing back the goals of the past by inflicting certain death on unsuspecting players...
FieryMUD will continue to grow and change through the coming years and those players who seek challenge and possess imagination will come in search of what the 3D world fails to offer them.

**Play now:** telnet://fierymud.org:4003 | **Secure:** telnets://fierymud.org:4443

## Prerequisites

### System Dependencies

**Ubuntu/Debian (including WSL2):**

```bash
sudo apt update
sudo apt install -y \
    cmake \
    ninja-build \
    g++-13 \
    git \
    pkg-config \
    libssl-dev \
    libpq-dev \
    postgresql-server-dev-all \
    libhiredis-dev \
    libcrypt-dev
```

**Fedora/RHEL:**

```bash
sudo dnf install -y \
    cmake \
    ninja-build \
    gcc-c++ \
    git \
    pkg-config \
    openssl-devel \
    libpq-devel \
    postgresql-server-devel \
    hiredis-devel \
    libxcrypt-devel
```

**Arch Linux:**

```bash
sudo pacman -S \
    cmake \
    ninja \
    gcc \
    git \
    pkg-config \
    openssl \
    postgresql-libs \
    hiredis
```

### C++23 Compiler Requirement

This project requires **GCC 13+** or **Clang 16+** for C++23 support.

```bash
# Verify your compiler version
g++ --version   # Should show 13.x or higher

# On Ubuntu, set GCC 13 as default if needed
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-13 100
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-13 100
```

### Automatically Downloaded Dependencies

The following are fetched automatically via CPM during build:
- fmt (string formatting)
- spdlog (logging)
- nlohmann/json (JSON processing)
- magic_enum (enum utilities)
- cxxopts (CLI parsing)
- Catch2 (testing framework)
- Lua 5.4 + sol2 (scripting)
- libpqxx (PostgreSQL client)
- asio (networking)

## Building

```bash
# Configure with Ninja (recommended)
cmake -B build -G Ninja .

# Build the server
cmake --build build
```

The compiled binary will be at `./build/fierymud`.

## Configuration

### Environment Variables

Copy the example environment file and adjust for your setup:

```bash
cp .env.example .env
```

Key environment variables:

| Variable | Description | Default |
|----------|-------------|---------|
| `POSTGRES_HOST` | PostgreSQL server host | `localhost` |
| `POSTGRES_PORT` | PostgreSQL server port | `5432` |
| `POSTGRES_DB` | Database name | `fierydev` |
| `POSTGRES_USER` | Database username | `muditor` |
| `POSTGRES_PASSWORD` | Database password | `password` |
| `TLS_CERTIFICATE_FILE` | Path to TLS certificate | `certs/server.crt` |
| `TLS_PRIVATE_KEY_FILE` | Path to TLS private key | `certs/server.key` |
| `TLS_DH_PARAMS_FILE` | Path to DH parameters | `certs/dhparams.pem` |
| `LOG_LEVEL` | Logging verbosity | `info` |
| `LOG_DIRECTORY` | Directory for log files | `logs` |

### TLS/SSL Configuration

The server supports TLS connections on port 4443 (configurable via database). TLS is enabled by default but requires valid certificates.

**Generate self-signed certificates for development:**

```bash
# Create certs directory
mkdir -p certs

# Generate certificate and private key
openssl req -x509 -nodes -days 365 -newkey rsa:2048 \
  -keyout certs/server.key -out certs/server.crt \
  -subj "/CN=localhost/O=FieryMUD/C=US"

# Generate Diffie-Hellman parameters (takes a minute)
openssl dhparam -out certs/dhparams.pem 2048
```

**Use Let's Encrypt for production:**

```bash
# Install certbot and obtain certificate
sudo certbot certonly --standalone -d fierymud.org

# Set environment variables to point to Let's Encrypt certs
export TLS_CERTIFICATE_FILE="/etc/letsencrypt/live/fierymud.org/fullchain.pem"
export TLS_PRIVATE_KEY_FILE="/etc/letsencrypt/live/fierymud.org/privkey.pem"
export TLS_DH_PARAMS_FILE="/path/to/certs/dhparams.pem"
```

**Test TLS connection:**

```bash
openssl s_client -connect localhost:4443
```

### Library Files

Copy the default library files before running for the first time:

```bash
cp -r lib.default lib
```

The `lib/` directory structure:
- `etc/` - Board, clan, and mail data
- `misc/` - Bugs, ideas, quests, socials, typos, and banned names
- `text/` - Help files, MOTDs, and other player-facing text
- `world/` - Areas, items, mobs, and zone data

## Running

```bash
./build/fierymud
```

Connect to the server:
- **Plain text:** `telnet localhost:4003` (default port)
- **TLS/SSL:** `openssl s_client -connect localhost:4443` or use a TLS-capable MUD client

## Testing

```bash
# Run recommended tests (unit + stable integration)
./run_tests.sh

# Run specific test suites
./run_tests.sh unit      # Unit tests only
./run_tests.sh stable    # Stable integration tests
./run_tests.sh verbose   # With detailed output

# Or use CTest directly
ctest --test-dir build -L unit
ctest --test-dir build -L stable
```

## Development

### VSCode Debugging

Add to `.vscode/launch.json`:

```json
{
    "type": "lldb",
    "request": "launch",
    "name": "Debug FieryMUD",
    "program": "${workspaceFolder}/build/fierymud",
    "args": [],
    "cwd": "${workspaceFolder}"
}
```

### Project Structure

- `src/` - Modern C++23 source code
- `legacy/` - Archived legacy CircleMUD code
- `tests/` - Catch2 test suites
- `data/` - Runtime game data
- `scripts/` - Python utilities
- `docs/` - Documentation

See `CLAUDE.md` for detailed development guidelines and coding standards.

## License

See LICENSE file for details.
