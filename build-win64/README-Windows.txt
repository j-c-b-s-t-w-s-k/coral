===============================================================================
                   CORAL CRYPTOCURRENCY - WINDOWS PACKAGE
===============================================================================

🪸 Welcome to Coral - The RandomX CPU Mining Cryptocurrency! 🪸

WHAT IS CORAL?
-------------
Coral is a CPU-friendly cryptocurrency fork that uses the RandomX proof-of-work
algorithm. It's designed to be ASIC-resistant and democratically mineable by
anyone with a regular computer.

GENESIS BLOCK FEATURES:
----------------------
⚡ Extreme Difficulty: 21e800 (21 followed by 800 zeros)
🔒 Header-only genesis block (completely unspendable)
🇵🇱 Polish Message: "FUCK SATOSHI! Byl glupim snobem"
⛏️  RandomX Algorithm: CPU-optimized mining
🚫 ASIC-Resistant: Fair mining for everyone

PACKAGE CONTENTS:
----------------
📁 bin/
   ├── coral-test.exe        - Network test utility
   ├── coral-wallet.bat      - Wallet helper script
   └── coral-mine.bat        - Mining helper script

📄 coral-installer.nsi       - NSIS installer configuration
📄 README-Windows.txt        - This documentation

INSTALLATION:
------------
1. Run Coral-1.0.0-Windows-Setup.exe as Administrator
2. Choose installation directory (default: C:\Program Files\Coral)
3. Installation creates Start Menu shortcuts
4. Launch "Coral Wallet" or "Coral Mining" from Start Menu

MINING REQUIREMENTS:
-------------------
💻 CPU: Any modern 64-bit processor (more cores = better performance)
🧠 RAM: 8GB+ recommended (RandomX uses ~2GB per mining thread)
💾 Storage: 50GB+ free space for blockchain
🌐 Network: Stable internet connection
🪟 OS: Windows 10/11 (64-bit)

EXPECTED MINING PERFORMANCE:
---------------------------
🔥 4-core CPU:  ~500-2000 H/s
🔥 8-core CPU:  ~1500-4000 H/s
🔥 16-core CPU: ~3000-8000 H/s

*Performance varies significantly by CPU model and generation*

GETTING STARTED:
---------------
1. Install the Coral software package
2. Wait for full node binaries to be released
3. Run corald.exe to join the network
4. Use coral-cli.exe for wallet operations
5. Start mining with: coral-cli generate 1 [your_address]

NETWORK INFORMATION:
-------------------
🌐 Network: Coral Mainnet
🔌 Port: 8334
⏰ Block Time: 10 minutes
🎁 Block Reward: 100 CORAL (quarters every 210k blocks)
🔗 Address Format: Starts with '1' (P2PKH) or 'c' (P2SH)

IMPORTANT SECURITY NOTES:
------------------------
🔐 Backup your wallet.dat file regularly
🗝️  Never share your private keys with anyone
🛡️  Use strong passwords for wallet encryption
⚠️  This is experimental software - use at your own risk

FULL NODE REQUIREMENTS:
----------------------
To run a full Coral node and mine, you'll need:
1. The complete Coral node software (corald.exe, coral-cli.exe)
2. RandomX libraries properly linked
3. Network connectivity to peer nodes
4. Sufficient disk space for blockchain data

CURRENT STATUS:
--------------
✅ Genesis block configured with extreme difficulty
✅ RandomX mining algorithm integrated
✅ Complete Bitcoin → Coral rebrand
✅ Windows cross-compilation tools ready
🔄 Full Windows node binaries in development
⏳ GUI wallet coming soon

COMMUNITY & SUPPORT:
-------------------
📧 Issues: https://github.com/nozmo-king/coral/issues
📋 Documentation: Check repository /doc folder
🛠️  Build Instructions: See GETTING_STARTED.md

MINING TIPS:
-----------
🎯 Use all CPU cores minus 1-2 for system stability
🌡️  Monitor CPU temperature (mining generates heat)
❄️  Ensure adequate cooling for sustained mining
🔋 Consider electricity costs vs mining rewards
📊 Join mining pools for steady income

LEGAL DISCLAIMER:
----------------
Coral cryptocurrency is experimental software. Mining and trading
cryptocurrencies involves financial risk. Users mine and trade at their
own risk. The developers are not responsible for any financial losses.

===============================================================================
                      🪸 WELCOME TO THE CORAL NETWORK! 🪸
===============================================================================

Version: 1.0.0
Build Date: September 2025
Build Type: Windows Cross-Compiled (MinGW)
Architecture: x86_64 (64-bit)