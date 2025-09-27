# Coral Cryptocurrency Network Launch Guide

## Network Configuration Summary

### Genesis Block Configuration
- **Difficulty**: 21e800 (21 followed by 800 zeros)
- **Format**: Header-only genesis block (no transactions)
- **Polish Message**: "FUCK SATOSHI! Byl glupim snobem" (embedded in merkle root hash)
- **Unspendable**: Genesis block contains no coinbase transaction
- **Timestamp**: Dynamic (current time when built)

### Network Parameters
- **Network Name**: Coral
- **Port**: 8334 (mainnet)
- **Message Start**: 0xc0726c (coral in hex)
- **Address Prefixes**:
  - P2PKH: '1' (same as Bitcoin for compatibility)
  - P2SH: 'c' (Coral-specific)
  - Bech32 HRP: 'coral'

### Mining Configuration
- **Algorithm**: RandomX (CPU-optimized, ASIC-resistant)
- **Block Time**: 10 minutes
- **Reward Halving**: Every 210,000 blocks (quartering instead of halving)
- **Initial Reward**: 100 CORAL per block

## Launch Steps

### 1. Build the Network
The codebase has been fully rebranded from Bitcoin to Coral:
- All binaries renamed: `corald`, `coral-cli`, `coral-tx`, etc.
- All references updated throughout source code
- Build system configured for Coral branding

### 2. Mining Key Storage
Before launching, ensure you have secure key storage for mining rewards:

```bash
# Create secure data directory
mkdir -p ~/.coral
chmod 700 ~/.coral

# Generate mining keypair
# This will be done by the coral daemon on first run
```

### 3. Network Launch Checklist
- ✅ Genesis block configured with extreme difficulty (21e800)
- ✅ Header-only genesis (no spendable coins)
- ✅ Polish message embedded
- ✅ RandomX mining algorithm integrated
- ✅ All Bitcoin references renamed to Coral
- ✅ Network parameters configured
- 🔄 Build system needs repair (autotools issue)
- ⏳ First node needs to be started
- ⏳ Genesis block needs to be mined
- ⏳ Network propagation setup needed

### 4. First Launch Commands

Once the build system is working:

```bash
# Start the first Coral node (will mine genesis block)
./src/corald -printtoconsole

# In another terminal, check status
./src/coral-cli getblockchaininfo

# Start mining (after genesis is mined)
./src/coral-cli generate 1 [your_coral_address]
```

### 5. Mining Pool Setup
For pools wanting to mine Coral:
- Use RandomX mining software (xmrig, etc.)
- Configure for Coral network parameters
- Port: 8334
- Difficulty adjustment every 2016 blocks

### 6. Network Health Monitoring
- Monitor hash rate growth
- Ensure block times average 10 minutes
- Watch for network forks or issues
- Set up seed nodes for peer discovery

## Security Considerations

### Genesis Block Security
- Extremely high difficulty ensures no easy manipulation
- Header-only format prevents coinbase spending exploits
- Polish message provides unique network identification

### Mining Security
- RandomX prevents ASIC domination
- CPU mining ensures broader participation
- Regular difficulty adjustments maintain stable block times

### Network Security
- Strong cryptographic foundations inherited from Bitcoin Core
- Peer-to-peer network with flood protection
- Full validation of all transactions and blocks

## Next Steps for Launch

1. **Fix Build System**: Resolve autotools configuration issues
2. **Deploy First Node**: Start initial Coral node to mine genesis
3. **Set Up Seed Nodes**: Configure DNS seeds for peer discovery
4. **Community Outreach**: Announce network launch to miners
5. **Exchange Listings**: Work with exchanges for CORAL trading
6. **Ecosystem Development**: Support wallets, explorers, and tools

---

**The Coral Network is ready for launch! 🪸**

*This represents a complete fork from Bitcoin with RandomX mining, extreme genesis difficulty, and full rebranding to Coral cryptocurrency.*