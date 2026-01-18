# 🪸 Coral Cryptocurrency
**The RandomX CPU Mining Revolution**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Build Status](https://img.shields.io/badge/Build-Passing-brightgreen.svg)]()
[![Mining Algorithm](https://img.shields.io/badge/Mining-RandomX-blue.svg)]()
[![Network](https://img.shields.io/badge/Network-Live-success.svg)]()

---

## 🌟 What Makes Coral Unique?

Coral isn't just another cryptocurrency—it's a **revolutionary approach to fair, democratic mining** that puts power back in the hands of everyday users.

### ⚡ **Extreme Genesis Security**
- **Astronomical Difficulty**: 21e800 (21 followed by 800 zeros)
- **Header-Only Genesis**: Completely unspendable genesis block
- **Embedded Message**: "FUCK SATOSHI! Był głupim snobem" (Polish: "FUCK SATOSHI! He was a foolish snob")
- **Impossible to Manipulate**: Genesis block requires computational power beyond current technology

### 🎯 **True CPU Democracy**
- **RandomX Algorithm**: ASIC-resistant, CPU-optimized proof-of-work
- **Fair Mining**: No ASIC dominance, anyone with a computer can participate
- **Memory-Hard**: Uses ~2GB RAM per thread, prevents specialized hardware advantage
- **Global Accessibility**: Mine with your laptop, desktop, or server

### 🚀 **Enhanced Economics**
- **Block Time**: 10 minutes (stable and predictable)
- **Initial Reward**: 100 CORAL per block
- **Unique Quartering**: Rewards divide by 4 (not 2) every 210,000 blocks
- **Deflationary**: Maximum supply more limited than Bitcoin

### 🛡️ **Proven Security Foundation**
- **Bitcoin Core Base**: Built on the most battle-tested blockchain codebase
- **Complete Rebrand**: Every "Bitcoin" reference replaced with "Coral"
- **Modern Features**: Latest security improvements and optimizations
- **Peer-to-Peer**: Decentralized network with flood protection

---

## 🚀 Quick Start

### For Windows Users
See [Build Instructions](doc/build-windows.md) for cross-compiling on Linux, or build natively using MSVC.

### For Linux/macOS Users
```bash
# Clone repository
git clone https://github.com/j-c-b-s-t-w-s-k/coral.git
cd coral

# Install dependencies (Ubuntu/Debian)
sudo apt update && sudo apt install -y build-essential libtool autotools-dev \
automake pkg-config bsdmainutils python3 libssl-dev libevent-dev \
libboost-system-dev libboost-filesystem-dev libboost-test-dev \
libboost-thread-dev libdb-dev libdb++-dev git

# Build Coral
./autogen.sh
./configure
make -j$(nproc)

# Start mining
./src/corald &
./src/coral-cli generate 1 $(./src/coral-cli getnewaddress)
```

---

## 📊 Mining Performance Guide

| CPU Type | Expected Hash Rate | Power Usage | Profitability |
|----------|-------------------|-------------|---------------|
| 4-core Laptop | 500-2000 H/s | 45-65W | ⭐⭐⭐ |
| 8-core Desktop | 1500-4000 H/s | 95-150W | ⭐⭐⭐⭐ |
| 16-core Workstation | 3000-8000 H/s | 180-300W | ⭐⭐⭐⭐⭐ |
| 32+ core Server | 8000+ H/s | 400W+ | ⭐⭐⭐⭐⭐ |

### 🎯 **Optimization Tips**
- **Use N-1 cores** (leave 1-2 cores for system operations)
- **Ensure 8GB+ RAM** (RandomX is memory-intensive)
- **Monitor temperatures** (mining generates heat)
- **Stable internet connection** for consistent mining
- **SSD storage** for faster blockchain sync

---

## 🏗️ **Network Specifications**

| Parameter | Value | Description |
|-----------|--------|-------------|
| **Algorithm** | RandomX | CPU-optimized, ASIC-resistant |
| **Block Time** | 10 minutes | Consistent, predictable blocks |
| **Block Reward** | 100 CORAL | High initial rewards |
| **Halving** | Quartering every 210k blocks | More deflationary than Bitcoin |
| **Port** | 8334 | Mainnet connection port |
| **Address Prefix** | '1' (P2PKH), 'c' (P2SH) | Coral-specific addressing |
| **Bech32 HRP** | coral | Modern address format |

---

## 🛠️ **Building from Source**

### Prerequisites
- **C++ Compiler**: GCC 7+ or Clang 5+
- **Build Tools**: autotools, pkg-config
- **Libraries**: OpenSSL, libevent, Boost 1.64+
- **Database**: Berkeley DB 4.8 (for wallet)
- **RandomX**: Included and automatically built

### Build Instructions

#### Linux (Ubuntu/Debian)
```bash
sudo apt update && sudo apt install -y \
    build-essential libtool autotools-dev automake pkg-config \
    bsdmainutils python3 libssl-dev libevent-dev libboost-system-dev \
    libboost-filesystem-dev libboost-test-dev libboost-thread-dev \
    libdb-dev libdb++-dev git

git clone https://github.com/j-c-b-s-t-w-s-k/coral.git
cd coral
./autogen.sh
./configure
make -j$(nproc)
```

#### macOS
```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install automake libtool boost miniupnpc openssl pkg-config python qt libevent qrencode

# Build Coral
git clone https://github.com/j-c-b-s-t-w-s-k/coral.git
cd coral
./autogen.sh
./configure --with-gui=qt5
make -j$(sysctl -n hw.physicalcpu)
```

#### Windows (Cross-Compilation)
```bash
# On Ubuntu/Debian
sudo apt install -y g++-mingw-w64-x86-64 mingw-w64-x86-64-dev wine64

# Build
git clone https://github.com/j-c-b-s-t-w-s-k/coral.git
cd coral
./contrib/coral/windows/build.sh
```

---

## 🖥️ **Qt GUI Client (coral-qt)**

The Qt GUI provides a full graphical wallet interface with network monitoring, transaction history, and mining controls.

### Building the Qt Client

#### Linux (Ubuntu/Debian)
```bash
# Install Qt5 dependencies (in addition to base dependencies above)
sudo apt install -y qtbase5-dev qttools5-dev qttools5-dev-tools libqrencode-dev

# Clone and build with GUI
git clone https://github.com/j-c-b-s-t-w-s-k/coral.git
cd coral
./autogen.sh
./configure --with-gui=qt5
make -j$(nproc)

# Run the Qt client
./src/qt/coral-qt
```

#### macOS
```bash
# Install dependencies including Qt
brew install automake libtool boost pkg-config libevent qt@5 qrencode berkeley-db@4

# Clone and build
git clone https://github.com/j-c-b-s-t-w-s-k/coral.git
cd coral
./autogen.sh
./configure --with-gui=qt5
make -j$(sysctl -n hw.physicalcpu)

# Run the Qt client
./src/qt/coral-qt

# Optional: Create a .dmg installer
pip3 install ds_store mac_alias
make deploy
```

### Qt Client Features
- **Wallet Management**: Send, receive, and manage CORAL transactions
- **Mining Controls**: Start/stop mining directly from the GUI
- **Network Status**: Real-time peer connections and sync status
- **Address Book**: Organize and label your addresses
- **Transaction History**: Complete history with search and filtering
- **QR Code Support**: Generate QR codes for receiving addresses
- **Dark Theme**: Modern dark UI theme included

### Running the Qt Client

```bash
# Start with default settings (connects to mainnet)
./src/qt/coral-qt

# Start on testnet
./src/qt/coral-qt -testnet

# Start on regtest (for development)
./src/qt/coral-qt -regtest

# Start with specific data directory
./src/qt/coral-qt -datadir=/path/to/data
```

---

## 📖 **Documentation**

### For Beginners
- **[Complete Noob's Guide](doc/coral/GETTING_STARTED.md)** - Step-by-step setup instructions
- **[Download Instructions](doc/coral/DOWNLOAD.md)** - How to obtain Coral

### For Advanced Users
- **[Build Instructions](doc/build-unix.md)** - Detailed compilation guide
- **[Network Launch Guide](doc/coral/coral-launch.md)** - Technical network details
- **[Testing Guide](doc/coral/CORAL_TESTING.md)** - Testing procedures

### For Developers
- **[Developer Notes](doc/developer-notes.md)** - Code contribution guidelines
- **[API Documentation](doc/)** - RPC interface details
- **[Architecture](doc/)** - Technical architecture overview

---

## 🌍 **Why Coral Matters**

### **Democratic Mining**
Unlike Bitcoin, where ASIC farms dominate, Coral's RandomX algorithm ensures anyone with a CPU can compete fairly. This creates true decentralization and prevents mining monopolies.

### **Environmental Responsibility**
CPU mining is significantly more energy-efficient than ASIC mining. Coral promotes sustainable cryptocurrency that doesn't require massive power consumption or specialized hardware waste.

### **Financial Accessibility**
No need for expensive mining equipment. Your existing computer can start earning Coral immediately, making cryptocurrency accessible to everyone globally.

### **Technical Innovation**
- **Memory-hard algorithm** prevents ASIC development
- **Extreme genesis security** prevents network manipulation
- **Proven codebase** built on Bitcoin Core's foundation
- **Modern optimizations** for better performance

---

## 🚨 **Security & Risks**

### **What We've Done**
- ✅ Built on Bitcoin Core's battle-tested foundation
- ✅ Comprehensive security auditing of changes
- ✅ RandomX algorithm prevents ASIC attacks
- ✅ Extreme genesis difficulty prevents manipulation
- ✅ Complete code review and testing

### **Your Responsibilities**
- 🔐 **Backup your wallet** regularly and securely
- 🗝️ **Never share private keys** with anyone
- 🛡️ **Use strong passwords** for wallet encryption
- ⚠️ **Understand the risks** - this is experimental software
- 💰 **Only invest what you can afford to lose**

---

## 🤝 **Contributing**

We welcome contributions from developers, miners, and enthusiasts!

### **How to Contribute**
1. **Fork** this repository
2. **Create** a feature branch (`git checkout -b feature/amazing-feature`)
3. **Commit** your changes (`git commit -m 'Add amazing feature'`)
4. **Push** to the branch (`git push origin feature/amazing-feature`)
5. **Open** a Pull Request

### **Areas We Need Help With**
- 🎨 **GUI Wallet Development** (Qt interface improvements)
- 🌐 **Mining Pool Software** (Stratum implementation)
- 📱 **Mobile Wallets** (iOS/Android applications)
- 🔍 **Block Explorer** (Web-based blockchain explorer)
- 📊 **Mining Software** (Optimized RandomX miners)
- 🌍 **Translations** (Multiple language support)

### **Development Guidelines**
- Follow existing code style and conventions
- Write comprehensive tests for new features
- Update documentation for any changes
- Ensure all tests pass before submitting PRs
- Be respectful and constructive in discussions

---

## 📈 **Roadmap**

### **Phase 1: Network Launch** ✅
- [x] Genesis block with extreme difficulty
- [x] RandomX integration and testing
- [x] Complete Bitcoin → Coral rebrand
- [x] Windows executable and installer
- [x] Basic mining functionality

### **Phase 2: Ecosystem Development** 🔄
- [ ] GUI wallet improvements (Qt interface)
- [ ] Mining pool reference implementation
- [ ] Web-based block explorer
- [ ] Exchange integration support
- [ ] Mobile wallet prototypes

### **Phase 3: Advanced Features** ⏳
- [ ] Lightning Network support
- [ ] Multi-signature wallet features
- [ ] Hardware wallet integration
- [ ] Advanced privacy features
- [ ] Smart contract exploration

### **Phase 4: Mass Adoption** 🎯
- [ ] Merchant integration tools
- [ ] Payment processing solutions
- [ ] Enterprise mining solutions
- [ ] Educational resources and tutorials
- [ ] Partnership development

---

## 🌐 **Community**

### **Get Involved**
- **GitHub**: [Issues and Development](https://github.com/j-c-b-s-t-w-s-k/coral/issues)
- **Mining**: Join the network and start earning CORAL
- **Support**: Help other users in GitHub Discussions
- **Development**: Contribute code, documentation, or ideas

### **Stay Updated**
- ⭐ **Star this repository** for updates
- 👁️ **Watch** for release notifications
- 🍴 **Fork** to contribute or customize
- 📢 **Share** with other crypto enthusiasts

---

## ⚖️ **License**

Coral is released under the **MIT License** - see [COPYING](COPYING) for details.

This means you can:
- ✅ Use Coral for any purpose (commercial or personal)
- ✅ Modify the source code as needed
- ✅ Distribute copies of Coral
- ✅ Include Coral in proprietary software

With the requirement to:
- 📄 Include the original license and copyright notice

---

## 🏆 **Achievements**

- 🥇 **First cryptocurrency with 21e800 genesis difficulty**
- 🥇 **Most ASIC-resistant mining algorithm implementation**
- 🥇 **Complete successful Bitcoin → Coral rebrand**
- 🥇 **Cross-platform Windows executable generation**
- 🥇 **Comprehensive noob-friendly documentation**

---

## 📞 **Support**

Need help? We're here for you:

- 📋 **Technical Issues**: [GitHub Issues](https://github.com/j-c-b-s-t-w-s-k/coral/issues)
- 📖 **Documentation**: Check our comprehensive guides
- 💬 **Community Support**: GitHub Discussions
- 🔧 **Build Problems**: See platform-specific build guides

**Remember**: Coral is experimental software. Always do your own research and never invest more than you can afford to lose.

---

<div align="center">

**🪸 Welcome to the Coral Revolution! 🪸**

*Fair Mining • CPU Democracy • Financial Freedom*

**[📖 Read Full Guide](doc/coral/GETTING_STARTED.md) | [🔧 Build from Source](#building-from-source)**

</div>

---

*Coral Cryptocurrency - Democratizing mining, one CPU at a time.*