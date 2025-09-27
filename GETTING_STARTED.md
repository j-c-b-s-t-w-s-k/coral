# Coral Cryptocurrency - Complete Noob's Guide

Welcome to Coral! This guide will walk you through everything you need to know to get started with Coral cryptocurrency, from installation to mining.

## Table of Contents
- [What is Coral?](#what-is-coral)
- [System Requirements](#system-requirements)
- [Installation](#installation)
  - [Linux/Ubuntu](#linuxubuntu)
  - [Windows](#windows)
  - [macOS](#macos)
- [Setting Up Your Node](#setting-up-your-node)
- [Joining the Coral Network](#joining-the-coral-network)
- [Creating a Wallet](#creating-a-wallet)
- [GUI Wallet Setup](#gui-wallet-setup)
- [Mining with RandomX](#mining-with-randomx)
- [Pool Mining Setup](#pool-mining-setup)
- [Network Statistics & Monitoring](#network-statistics--monitoring)
- [Advanced Mining Optimization](#advanced-mining-optimization)
- [Security Best Practices](#security-best-practices)
- [Troubleshooting](#troubleshooting)
- [Frequently Asked Questions (FAQ)](#frequently-asked-questions-faq)
- [Community & Support](#community--support)

## What is Coral?

Coral is a cryptocurrency that uses the RandomX proof-of-work algorithm, which is designed to be ASIC-resistant and CPU-friendly. This means you can mine Coral effectively using your regular computer's CPU without needing specialized mining equipment.

## System Requirements

### Minimum Requirements:
- **CPU**: Any modern 64-bit processor (Intel/AMD)
- **RAM**: 4GB (8GB recommended for mining)
- **Storage**: 50GB free space (blockchain grows over time)
- **Network**: Stable internet connection
- **OS**: Linux (Ubuntu 18.04+), Windows 10+, or macOS 10.14+

### Recommended for Mining:
- **CPU**: Multi-core processor (more cores = better mining performance)
- **RAM**: 8GB+ (RandomX requires significant memory)
- **Storage**: SSD for better performance

## Installation

### Linux/Ubuntu

#### Step 1: Update Your System
```bash
sudo apt update && sudo apt upgrade -y
```

#### Step 2: Install Dependencies
```bash
sudo apt install -y build-essential libtool autotools-dev automake pkg-config bsdmainutils python3 libssl-dev libevent-dev libboost-system-dev libboost-filesystem-dev libboost-test-dev libboost-thread-dev libdb-dev libdb++-dev git
```

#### Step 3: Clone the Repository
```bash
git clone https://github.com/nozmo-king/coral.git
cd coral
```

#### Step 4: Build Coral
```bash
./autogen.sh
./configure
make -j$(nproc)
```

#### Step 5: Install (Optional)
```bash
sudo make install
```

### Windows

#### Step 1: Install Prerequisites
1. Download and install [Git for Windows](https://git-scm.com/download/win)
2. Download and install [Microsoft Visual Studio Community](https://visualstudio.microsoft.com/downloads/)
3. Install the "Desktop development with C++" workload

#### Step 2: Clone and Build
1. Open Command Prompt or PowerShell as Administrator
2. Run:
```cmd
git clone https://github.com/nozmo-king/coral.git
cd coral
```

3. Follow the build instructions in `doc/build-windows.md`

### macOS

#### Step 1: Install Xcode Command Line Tools
```bash
xcode-select --install
```

#### Step 2: Install Homebrew (if not already installed)
```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

#### Step 3: Install Dependencies
```bash
brew install automake libtool boost miniupnpc openssl pkg-config python qt libevent qrencode
```

#### Step 4: Clone and Build
```bash
git clone https://github.com/nozmo-king/coral.git
cd coral
./autogen.sh
./configure
make -j$(sysctl -n hw.physicalcpu)
```

## Setting Up Your Node

### Step 1: Create Data Directory
Linux/macOS:
```bash
mkdir -p ~/.coral
```

Windows:
```cmd
mkdir %APPDATA%\Coral
```

### Step 2: Create Configuration File
Create a file called `coral.conf` in your data directory:

Linux/macOS: `~/.coral/coral.conf`
Windows: `%APPDATA%\Coral\coral.conf`

```ini
# Basic configuration
server=1
daemon=1
rpcuser=your_rpc_username
rpcpassword=your_secure_rpc_password
rpcallowip=127.0.0.1

# Network settings
listen=1
maxconnections=50

# Mining settings (if you plan to mine)
gen=1
genproclimit=4
```

Replace `your_rpc_username` and `your_secure_rpc_password` with your own secure credentials.

## Joining the Coral Network

### Step 1: Start Your Node
```bash
./src/corald
```

Or if you installed system-wide:
```bash
corald
```

### Step 2: Check Network Status
In a new terminal window:
```bash
./src/coral-cli getnetworkinfo
```

You should see information about your connection to the Coral network.

### Step 3: Wait for Synchronization
Your node needs to download the entire blockchain. This may take several hours depending on your internet connection.

Check sync status:
```bash
./src/coral-cli getblockchaininfo
```

When `"blocks"` equals `"headers"`, you're fully synchronized.

## Creating a Wallet

### Step 1: Generate a New Wallet
```bash
./src/coral-cli createwallet "mywallet"
```

### Step 2: Get Your Receiving Address
```bash
./src/coral-cli getnewaddress
```

Save this address! This is where you'll receive Coral coins.

### Step 3: Backup Your Wallet
```bash
./src/coral-cli dumpwallet "wallet_backup.txt"
```

Store this backup file in a safe place!

## GUI Wallet Setup

### Using Coral-Qt (Graphical Interface)

If you prefer a graphical interface over command line, Coral includes a GUI wallet called Coral-Qt.

#### Step 1: Build with GUI Support
When building Coral, make sure Qt is installed:

Linux/Ubuntu:
```bash
sudo apt install -y qtbase5-dev qttools5-dev-tools libqt5svg5-dev
./configure --with-gui=qt5
make -j$(nproc)
```

macOS (already installed with previous brew command):
```bash
./configure --with-gui=qt5
make -j$(sysctl -n hw.physicalcpu)
```

#### Step 2: Start the GUI Wallet
```bash
./src/qt/coral-qt
```

#### Step 3: First Time Setup
1. **Create New Wallet**: Click "Create a new wallet"
2. **Choose Wallet Name**: Enter a name (e.g., "MyCoralWallet")
3. **Set Password**: Choose a strong password (highly recommended)
4. **Backup Seed**: Write down your recovery seed phrase - **NEVER LOSE THIS!**

#### Step 4: GUI Features
- **Overview Tab**: Shows balance and recent transactions
- **Send Tab**: Send Coral to other addresses
- **Receive Tab**: Generate receiving addresses
- **Transactions Tab**: View transaction history
- **Mining Tab**: Configure and monitor mining (if available)

#### Step 5: Enable Mining in GUI
1. Go to **Settings → Options → Mining**
2. Check "Enable Mining"
3. Set number of CPU cores to use
4. Enter your receiving address
5. Click "Start Mining"

### Mobile Wallet Options
Currently, Coral doesn't have official mobile wallets. Always verify any third-party wallets before using them with real funds.

## Mining with RandomX

### Understanding RandomX
RandomX is a CPU-optimized proof-of-work algorithm that:
- Works best on CPUs (not GPUs or ASICs)
- Uses about 2GB of RAM per mining thread
- Provides fair mining opportunities for everyone

### Step 1: Configure Mining
Edit your `coral.conf` file:
```ini
# Enable mining
gen=1
# Set number of CPU cores to use (recommended: total cores - 1)
genproclimit=3
# Set mining address (replace with your address)
coinbaseaddress=your_coral_address_here
```

### Step 2: Start Mining
Restart your node:
```bash
./src/corald
```

### Step 3: Check Mining Status
```bash
./src/coral-cli getmininginfo
```

You should see:
- `"generate": true` (mining enabled)
- `"genproclimit": 3` (number of threads)
- `"hashespersec"` (your hash rate)

### Step 4: Monitor Your Mining
Check your balance:
```bash
./src/coral-cli getbalance
```

View recent transactions:
```bash
./src/coral-cli listtransactions
```

## Mining Performance Tips

### Optimize Your Setup:
1. **Close unnecessary programs** to free up CPU and RAM
2. **Use fewer cores than you have** (leave 1-2 cores for system operations)
3. **Ensure good cooling** - mining generates heat
4. **Use an SSD** for better blockchain sync performance
5. **Stable internet** connection for consistent mining

### Expected Performance:
- **4-core CPU**: ~500-2000 H/s
- **8-core CPU**: ~1500-4000 H/s
- **16-core CPU**: ~3000-8000 H/s

*Performance varies significantly based on CPU model and generation*

## Troubleshooting

### Common Issues:

#### "Connection refused" error
- Make sure `corald` is running
- Check that RPC credentials in `coral.conf` are correct

#### Low hash rate
- Ensure you have enough RAM (2GB+ per mining thread)
- Close other CPU-intensive applications
- Check CPU temperature (throttling reduces performance)

#### Won't sync
- Check internet connection
- Try adding nodes manually in `coral.conf`:
```ini
addnode=node1.coral.network
addnode=node2.coral.network
```

#### Out of memory
- Reduce `genproclimit` in your config
- Add more RAM or enable swap space

### Checking Logs:
Linux/macOS: `~/.coral/debug.log`
Windows: `%APPDATA%\Coral\debug.log`

## Advanced Configuration

### Solo vs Pool Mining
This guide covers solo mining (mining by yourself). For pool mining:
1. Join a Coral mining pool
2. Use pool-specific mining software
3. Configure with pool's address and credentials

### Security Tips
1. **Backup your wallet** regularly
2. **Use strong RPC passwords**
3. **Keep your node updated**
4. **Don't share private keys**
5. **Use firewall protection**

## Community & Support

### Getting Help:
- **GitHub Issues**: [Report bugs and issues](https://github.com/nozmo-king/coral/issues)
- **Documentation**: Check the `doc/` folder in this repository
- **Build Issues**: See platform-specific build guides in `doc/`

### Contributing:
- **Bug Reports**: Use GitHub Issues
- **Code Contributions**: Submit Pull Requests
- **Documentation**: Help improve this guide

## Quick Reference Commands

### Node Management:
```bash
# Start node
./src/corald

# Stop node
./src/coral-cli stop

# Get node info
./src/coral-cli getinfo

# Check connections
./src/coral-cli getpeerinfo
```

### Wallet Commands:
```bash
# Check balance
./src/coral-cli getbalance

# Get new address
./src/coral-cli getnewaddress

# Send coins
./src/coral-cli sendtoaddress "address" amount

# List transactions
./src/coral-cli listtransactions
```

### Mining Commands:
```bash
# Get mining info
./src/coral-cli getmininginfo

# Generate blocks (testing only)
./src/coral-cli generatetoaddress 1 "your_address"

# Check hash rate
./src/coral-cli getnetworkhashps
```

---

**Welcome to the Coral network! Happy mining! 🪸**

*This guide is for educational purposes. Cryptocurrency mining involves risks and may not always be profitable. Always do your own research.*