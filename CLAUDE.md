# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is Coral, a Bitcoin Core fork that integrates RandomX proof-of-work algorithm. The codebase is built on Bitcoin Core 24.0.1 and uses the GNU Autotools build system. The project is currently on the `coral-implementation` branch with customizations for RandomX integration.

## Build System & Common Commands

### Building from Source
```bash
# Initial setup (after cloning)
./autogen.sh
./configure
make -j$(nproc)  # parallel build using all CPU cores
```

### Development Build Commands
```bash
# Clean build
make clean
make -j$(nproc)

# Unit tests
make check

# Functional tests (Python-based)
test/functional/test_runner.py

# Individual unit test
src/test/test_bitcoin

# Specific functional test
test/functional/test_runner.py feature_block.py
```

### Useful Development Targets
```bash
# Build specific components
make -C src bitcoind        # Daemon only
make -C src bitcoin-cli     # CLI tool only
make -C src bitcoin-tx      # Transaction utility
make -C src qt/bitcoin-qt   # GUI (if Qt dependencies met)

# Documentation
make docs  # Requires doxygen
```

## Architecture & Key Components

### Core Directory Structure
- `src/` - All C++ source code
  - `consensus/` - Consensus rules and validation logic
  - `primitives/` - Basic blockchain data structures (blocks, transactions)
  - `crypto/` - Cryptographic implementations
  - `net*` files - P2P networking layer
  - `validation.*` - Main blockchain validation logic
  - `chainparams.*` - Network parameters (mainnet, testnet, regtest)
  - `pow.*` - Proof-of-work algorithms
  - `init.*` - Application initialization
  - `rpc/` - JSON-RPC server implementation
  - `wallet/` - Wallet functionality
  - `qt/` - GUI implementation using Qt

### Coral-Specific Modifications
This fork integrates RandomX proof-of-work with modifications in:
- `src/chainparams.cpp` - Custom genesis block and network parameters
- `src/pow.cpp` - RandomX proof-of-work implementation
- `src/validation.cpp` - Block validation with RandomX
- `integrate-randomx.sh` - Automated integration script

### Build System Architecture
- Uses GNU Autotools (autoconf/automake)
- `configure.ac` - Main configuration script
- `Makefile.am` - Top-level makefile template
- `src/Makefile.am` - Source build configuration
- Multiple included makefiles for different components (leveldb, secp256k1, etc.)

### Testing Framework
- **Unit Tests**: C++ tests using Boost.Test framework in `src/test/`
  - Run with `make check` or `src/test/test_bitcoin`
- **Functional Tests**: Python-based integration tests in `test/functional/`
  - Run with `test/functional/test_runner.py`
- **Fuzz Tests**: Located in `test/fuzz/`

### Key Data Flow
1. **Network Layer**: P2P protocol handling in `net.cpp` and `net_processing.cpp`
2. **Block Processing**: New blocks validated in `validation.cpp`
3. **Consensus**: Rules enforced through `consensus/` directory
4. **Storage**: LevelDB for blockchain data via `txdb.cpp`
5. **Mempool**: Transaction pool management in `txmempool.cpp`

## Development Guidelines

### Code Style
- Follows Bitcoin Core coding standards (see `doc/developer-notes.md`)
- Uses `.clang-format` for C++ formatting
- Python code follows PEP 8

### Testing Requirements
- Always run unit tests after changes: `make check`
- Run relevant functional tests for modified features
- For consensus changes, run extended test suite: `test/functional/test_runner.py --extended`

### Consensus-Critical Code
Be extremely cautious when modifying:
- `src/consensus/` directory
- `src/validation.cpp`
- `src/primitives/` (block/transaction structures)
- `src/pow.cpp` (proof-of-work validation)
- `src/chainparams.cpp` (network parameters)

### Debugging & Development
- Enable debug mode: `./configure --enable-debug`
- Debug logging via `src/logging.h` categories
- Use `src/test/test_bitcoin --list_content` to see available unit tests
- Functional tests support `--pdb` flag for Python debugging

### Dependencies
- Core dependencies managed through `depends/` directory for reproducible builds
- System dependencies documented in `doc/dependencies.md`
- RandomX library must be installed system-wide for Coral build

## Network Modes
- **Mainnet**: Default production network
- **Testnet**: Test network with different parameters
- **Regtest**: Local testing with instant block generation
- **Signet**: Signed test network
Configure with `-testnet`, `-regtest`, or `-signet` flags.

## RandomX Integration Notes
This fork uses RandomX as proof-of-work algorithm instead of SHA-256. Key integration points:
- RandomX validation in `pow.cpp`
- Custom genesis block parameters in `chainparams.cpp`
- Build system modifications to link RandomX library
- Integration script at `integrate-randomx.sh` for automated setup