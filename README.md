# Coral Cryptocurrency

## Abstract

Coral is a peer-to-peer electronic cash system derived from the Bitcoin Core reference implementation (version 24.0.1). The primary modification consists of the replacement of the SHA-256d proof-of-work algorithm with RandomX, a memory-hard, CPU-optimized hashing algorithm designed to resist implementation on application-specific integrated circuits (ASICs) and field-programmable gate arrays (FPGAs).

This document provides comprehensive technical specifications, build instructions, operational parameters, and security considerations for the Coral network.

## Table of Contents

1. [Technical Specifications](#technical-specifications)
2. [Consensus Parameters](#consensus-parameters)
3. [Network Configuration](#network-configuration)
4. [Build Instructions](#build-instructions)
5. [Runtime Configuration](#runtime-configuration)
6. [Remote Procedure Call Interface](#remote-procedure-call-interface)
7. [Security Model](#security-model)
8. [Cryptographic Verification](#cryptographic-verification)
9. [License](#license)

## Technical Specifications

### Proof-of-Work Algorithm

The Coral network employs RandomX version 1.1.9 as its proof-of-work function. RandomX is a proof-of-work algorithm optimized for general-purpose central processing units. The algorithm utilizes random code execution combined with memory-hard techniques to achieve ASIC resistance.

RandomX operates in two modes:

1. **Fast mode**: Requires 2080 megabytes of shared memory for the dataset. Provides optimal hashing performance.
2. **Light mode**: Requires 256 megabytes of shared memory for the cache. Suitable for verification operations on memory-constrained systems.

The expected hash rate for contemporary x86-64 processors ranges from 1000 to 10000 hashes per second per thread, depending on cache size, memory bandwidth, and instruction set extensions (AES-NI, AVX2).

### Block Structure

Each block in the Coral blockchain consists of the following components:

| Field | Size (bytes) | Description |
|-------|--------------|-------------|
| nVersion | 4 | Block version number |
| hashPrevBlock | 32 | SHA-256d hash of the previous block header |
| hashMerkleRoot | 32 | SHA-256d Merkle root of all transactions |
| nTime | 4 | Unix timestamp (seconds since 1970-01-01T00:00:00Z) |
| nBits | 4 | Compact representation of the target threshold |
| nNonce | 4 | 32-bit nonce for proof-of-work iteration |

Total header size: 80 bytes.

### Genesis Block

The genesis block was mined on 2026-01-18T00:00:00Z with the following parameters:

```
Timestamp message: "18/Jan/2026 Trump tariffs take effect as thousands rally for Greenland"
Unix timestamp: 1768694400
nBits: 0x1e0fffff
nNonce: 12230
Block hash: 00000be404e6b09de57636dcd4cadf978720286a5c9f474d27d1e546ec629e94
Merkle root: 31f6694421dd96f7c3fc212a6652cb8c5e0f774a925255c99e0b787ba7dac071
```

The genesis block coinbase transaction contains an OP_RETURN output rendering the output unspendable. All circulating supply originates from block subsidies beginning at block height 1.

## Consensus Parameters

### Mainnet Parameters

| Parameter | Value | Description |
|-----------|-------|-------------|
| Block interval target | 600 seconds | Average time between blocks |
| Difficulty adjustment interval | 2016 blocks | Blocks between difficulty recalculations |
| Difficulty adjustment timespan | 1209600 seconds | Target time for 2016 blocks (14 days) |
| Initial block subsidy | 50 CRL | Coinbase reward for blocks 1-209999 |
| Subsidy halving interval | 210000 blocks | Blocks between subsidy reductions |
| Maximum supply | 21000000 CRL | Theoretical maximum (asymptotically approached) |
| Minimum difficulty | 0x1e0fffff | Compact target for minimum difficulty |
| powLimit | 00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff | Maximum target value |

### Address Encoding

| Type | Version Byte | Prefix | Example |
|------|--------------|--------|---------|
| P2PKH | 0x00 | 1 | 1BvBMSEYstWetqTFn5Au4m4GFg7xJaNVN2 |
| P2SH | 0x05 | 3 | 3J98t1WpEZ73CNmQviecrnyiWrnqRhWNLy |
| Bech32 | coral | coral1 | coral1qw508d6qejxtdg4y5r3zarvary0c5xw7k... |
| Bech32m | coral | coral1 | coral1p0xlxvlhemja6c4dqv22uapctqupfhlxm9h8z... |

### Network Ports

| Network | P2P Port | RPC Port |
|---------|----------|----------|
| Mainnet | 8334 | 8335 |
| Testnet | 18334 | 18335 |
| Regtest | 18444 | 18445 |

## Network Configuration

### Peer-to-Peer Protocol

The Coral network utilizes the Bitcoin peer-to-peer protocol with the following version parameters:

- Protocol version: 70016
- User agent: /Coral:24.0.1/
- Services: NODE_NETWORK, NODE_WITNESS, NODE_NETWORK_LIMITED

### Message Types

Standard Bitcoin protocol messages are supported. Additional message types for future functionality are reserved in the protocol namespace.

## Build Instructions

### Prerequisites

The following dependencies are required for compilation:

| Dependency | Minimum Version | Purpose |
|------------|-----------------|---------|
| GCC or Clang | 7.0 / 5.0 | C++17 compiler |
| GNU Autotools | 2.69 | Build system |
| pkg-config | 0.29 | Dependency resolution |
| Boost | 1.64.0 | C++ libraries |
| libevent | 2.1.8 | Asynchronous I/O |
| OpenSSL | 1.1.0 | Cryptographic primitives |
| Berkeley DB | 4.8.30 | Wallet database (optional) |
| Qt | 5.11.3 | Graphical interface (optional) |
| RandomX | 1.1.9 | Proof-of-work algorithm |

### Linux (Debian/Ubuntu)

```bash
# Install build dependencies
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    libtool \
    autotools-dev \
    automake \
    pkg-config \
    bsdmainutils \
    python3 \
    libssl-dev \
    libevent-dev \
    libboost-system-dev \
    libboost-filesystem-dev \
    libboost-test-dev \
    libboost-thread-dev \
    libdb-dev \
    libdb++-dev \
    libsqlite3-dev \
    libminiupnpc-dev \
    libnatpmp-dev \
    libzmq3-dev \
    qtbase5-dev \
    qttools5-dev \
    qttools5-dev-tools \
    libqrencode-dev

# Clone repository
git clone https://github.com/j-c-b-s-t-w-s-k/coral.git
cd coral

# Build RandomX from source
git clone https://github.com/tevador/RandomX.git
cd RandomX
mkdir build && cd build
cmake -DARCH=native ..
make -j$(nproc)
sudo make install
sudo ldconfig
cd ../..

# Configure and build Coral
./autogen.sh
./configure \
    --with-gui=qt5 \
    --enable-hardening \
    --with-incompatible-bdb
make -j$(nproc)

# Optional: Install system-wide
sudo make install
```

### macOS (Intel x86_64)

```bash
# Install Homebrew package manager
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install \
    automake \
    libtool \
    boost \
    pkg-config \
    libevent \
    qt@5 \
    qrencode \
    miniupnpc \
    zeromq \
    berkeley-db@4

# Clone and build RandomX
git clone https://github.com/tevador/RandomX.git
cd RandomX
mkdir build && cd build
cmake -DARCH=native ..
make -j$(sysctl -n hw.ncpu)
sudo make install
cd ../..

# Clone and build Coral
git clone https://github.com/j-c-b-s-t-w-s-k/coral.git
cd coral
./autogen.sh
./configure --with-gui=qt5
make -j$(sysctl -n hw.ncpu)
```

### macOS (Apple Silicon ARM64)

```bash
# Install Homebrew package manager (ARM64 version)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Configure shell environment for Homebrew
echo 'eval "$(/opt/homebrew/bin/brew shellenv)"' >> ~/.zprofile
eval "$(/opt/homebrew/bin/brew shellenv)"

# Install dependencies
brew install \
    automake \
    libtool \
    boost \
    pkg-config \
    libevent \
    qt@5 \
    qrencode \
    miniupnpc \
    zeromq \
    berkeley-db@4

# Clone and build RandomX with ARM64 optimizations
git clone https://github.com/tevador/RandomX.git
cd RandomX
mkdir build && cd build
cmake -DARCH=native ..
make -j$(sysctl -n hw.ncpu)
sudo make install
cd ../..

# Clone and build Coral
git clone https://github.com/j-c-b-s-t-w-s-k/coral.git
cd coral
./autogen.sh
./configure \
    --with-gui=qt5 \
    CPPFLAGS="-I/opt/homebrew/include" \
    LDFLAGS="-L/opt/homebrew/lib -L/usr/local/lib" \
    PKG_CONFIG_PATH="/opt/homebrew/opt/qt@5/lib/pkgconfig:/opt/homebrew/lib/pkgconfig"
make -j$(sysctl -n hw.ncpu)
```

Expected hash rates for Apple Silicon processors:

| Processor | Approximate Hash Rate |
|-----------|----------------------|
| M1 | 8000-12000 H/s |
| M1 Pro/Max | 12000-20000 H/s |
| M2 | 10000-15000 H/s |
| M2 Pro/Max | 15000-25000 H/s |
| M3 | 12000-18000 H/s |
| M3 Pro/Max | 18000-30000 H/s |
| M4 | 15000-25000 H/s |

### Windows (Cross-compilation from Linux)

```bash
# Install cross-compilation toolchain
sudo apt-get install -y \
    g++-mingw-w64-x86-64 \
    mingw-w64-x86-64-dev

# Build dependencies using depends system
cd coral/depends
make HOST=x86_64-w64-mingw32 -j$(nproc)
cd ..

# Configure and build
./autogen.sh
CONFIG_SITE=$PWD/depends/x86_64-w64-mingw32/share/config.site ./configure --prefix=/
make -j$(nproc)
```

### Build Verification

After successful compilation, verify the build:

```bash
# Check binary existence and architecture
file src/corald
file src/coral-cli
file src/qt/bitcoin-qt

# Verify version
./src/corald --version
./src/coral-cli --version

# Run unit tests
make check

# Run functional tests (requires Python 3)
test/functional/test_runner.py --extended
```

## Runtime Configuration

### Configuration File

The configuration file is located at:

- Linux: `~/.coral/coral.conf`
- macOS: `~/Library/Application Support/Coral/coral.conf`
- Windows: `%APPDATA%\Coral\coral.conf`

Example configuration:

```ini
# Network
listen=1
port=8334
maxconnections=125

# RPC Server
server=1
rpcuser=<username>
rpcpassword=<password>
rpcport=8335
rpcallowip=127.0.0.1
rpcbind=127.0.0.1

# Mining
gen=0

# Wallet
disablewallet=0
keypool=1000

# Logging
debug=0
printtoconsole=0
logips=0
logtimestamps=1

# Performance
dbcache=450
maxmempool=300
maxorphantx=100
```

### Command-line Options

| Option | Description |
|--------|-------------|
| `-conf=<file>` | Specify configuration file path |
| `-datadir=<dir>` | Specify data directory path |
| `-daemon` | Run in background as daemon |
| `-server` | Accept RPC commands |
| `-testnet` | Use testnet chain |
| `-regtest` | Use regression test chain |
| `-printtoconsole` | Send trace/debug output to console |
| `-debug=<category>` | Enable debug logging for category |

## Remote Procedure Call Interface

### Authentication

RPC authentication uses HTTP Basic authentication. Credentials are specified in the configuration file or via command-line arguments.

### Mining Commands

| Command | Parameters | Description |
|---------|------------|-------------|
| `getmininginfo` | none | Returns mining-related information |
| `getblocktemplate` | template_request | Returns block template for mining |
| `submitblock` | hexdata | Submits a mined block |
| `generatetoaddress` | nblocks, address, [maxtries] | Mine blocks to specified address |

Example usage:

```bash
# Generate 10 blocks to address
coral-cli generatetoaddress 10 "coral1qw508d6qejxtdg4y5r3zarvary0c5xw7k..." 10000000

# Get current mining information
coral-cli getmininginfo

# Get block template for external miner
coral-cli getblocktemplate '{"rules": ["segwit"]}'
```

### Wallet Commands

| Command | Parameters | Description |
|---------|------------|-------------|
| `getnewaddress` | [label], [address_type] | Generate new receiving address |
| `getbalance` | [dummy], [minconf], [include_watchonly] | Returns total balance |
| `sendtoaddress` | address, amount, [comment], [comment_to], [subtractfeefromamount] | Send to address |
| `listunspent` | [minconf], [maxconf], [addresses] | List unspent transaction outputs |

### Blockchain Commands

| Command | Parameters | Description |
|---------|------------|-------------|
| `getblockchaininfo` | none | Returns blockchain state information |
| `getblock` | blockhash, [verbosity] | Returns block data |
| `getblockhash` | height | Returns hash of block at height |
| `getblockcount` | none | Returns current block height |
| `getdifficulty` | none | Returns current difficulty |

## Security Model

### Threat Model

The Coral network security model is predicated on the assumption that a majority of computational power is controlled by honest participants. The RandomX proof-of-work algorithm provides additional resistance against centralization of mining power by requiring general-purpose hardware.

### Known Limitations

1. **51% Attack**: An adversary controlling majority hash power can reorganize the blockchain, enabling double-spend attacks.

2. **Eclipse Attack**: An adversary controlling all peer connections to a node can present a false view of the blockchain.

3. **Selfish Mining**: Strategic block withholding can provide disproportionate rewards under certain conditions.

### Security Recommendations

1. Wait for sufficient confirmations before considering transactions final (6 confirmations recommended for high-value transactions).

2. Connect to diverse, geographically distributed peers.

3. Run a full validating node rather than relying on third-party services.

4. Encrypt wallet files and maintain secure backups.

5. Use hardware security modules for high-value key storage.

## Cryptographic Verification

### Maintainer PGP Public Key

The following PGP public key may be used to verify signed releases and communications:

```
Key ID: 9B9636E35894376AA4717AE8357E56FF61F8330A
User ID: jcb stwsk <j_stwsk@proton.me>
Key Type: RSA 3072-bit
Created: 2026-01-18
Expires: 2028-01-18
```

```
-----BEGIN PGP PUBLIC KEY BLOCK-----

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
vsHgWQotXCru4YKUhkFmNKGwzg2d
=WxWj
-----END PGP PUBLIC KEY BLOCK-----
```

### Verifying Signatures

To verify a signed file:

```bash
# Import the public key
gpg --import maintainer-key.asc

# Verify signature
gpg --verify file.sig file

# Verify with specific key
gpg --verify --keyid-format long file.sig file
```

## License

Coral is released under the MIT License.

```
Copyright (c) 2009-2024 The Bitcoin Core developers
Copyright (c) 2024-2026 The Coral Core developers

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
```

## References

1. Nakamoto, S. (2008). Bitcoin: A Peer-to-Peer Electronic Cash System.
2. Tevador et al. (2019). RandomX: ASIC-resistant proof-of-work algorithm.
3. Bitcoin Core Documentation. https://bitcoincore.org/en/doc/
4. BIP Repository. https://github.com/bitcoin/bips

## Contact

Maintainer: jcb stwsk <j_stwsk@proton.me>
Repository: https://github.com/j-c-b-s-t-w-s-k/coral
