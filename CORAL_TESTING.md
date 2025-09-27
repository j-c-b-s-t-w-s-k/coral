# Coral Cryptocurrency Testing Guide

## Overview
This document provides comprehensive testing procedures for the Coral cryptocurrency implementation based on Bitcoin Core v24.0.1.

## Coral Features Implemented

1. **Custom Genesis Block**: Message "Supreme Court keeps in place Trump funding freeze that threatens billions of dollars in foreign aid - Coral Genesis Sept 27, 2025"
2. **RandomX Proof-of-Work**: ASIC-resistant mining algorithm
3. **100 Coral Block Rewards**: Double Bitcoin's reward (100 vs 50 BTC)
4. **42M Total Supply**: Double Bitcoin's supply (42M vs 21M)
5. **Block #1 High Difficulty**: 1000x harder than genesis block
6. **Volume-based Transaction Fees**: 0.1% of transaction amount
7. **Custom Network Parameters**: Unique ports, addresses, and magic bytes

## Testing Procedures

### 1. Basic Functionality Tests

#### Start Coral Daemon (Regtest Mode)
```bash
./src/bitcoind -regtest -daemon -debug=1 -printtoconsole
```

#### Basic RPC Commands
```bash
# Get basic info
./src/bitcoin-cli -regtest getblockchaininfo

# Generate initial blocks (bypass block #1 difficulty for testing)
./src/bitcoin-cli -regtest generatetoaddress 2 $(./src/bitcoin-cli -regtest getnewaddress)

# Check balance (should show 200 CORAL for 2 blocks at 100 CORAL each)
./src/bitcoin-cli -regtest getbalance
```

#### Test RandomX Mining
```bash
# Generate one block to test RandomX proof-of-work
time ./src/bitcoin-cli -regtest generatetoaddress 1 $(./src/bitcoin-cli -regtest getnewaddress)

# Verify RandomX is working (should see RandomX logs if debug is enabled)
tail -n 50 ~/.bitcoin/regtest/debug.log | grep -i randomx
```

### 2. Network Parameter Tests

#### Test Custom Ports
```bash
# Start mainnet daemon (should use port 8334)
./src/bitcoind -printtoconsole -port=8334

# Test connection on custom port
netstat -an | grep 8334
```

#### Test Bech32 Addresses
```bash
# Generate Coral bech32 address (should start with "coral")
./src/bitcoin-cli -regtest getnewaddress "" bech32

# Generate legacy address (should start with 'C')
./src/bitcoin-cli -regtest getnewaddress "" legacy
```

### 3. Transaction Fee Tests

#### Test Volume-based Fees (0.1%)
```bash
# Create a test transaction and verify fee calculation
ADDR=$(./src/bitcoin-cli -regtest getnewaddress)
TXID=$(./src/bitcoin-cli -regtest sendtoaddress $ADDR 10.0)
./src/bitcoin-cli -regtest gettransaction $TXID

# Fee should be at least 0.01 CORAL (0.1% of 10 CORAL)
# Verify transaction is rejected with insufficient fee:
# Create raw transaction with minimal fee and broadcast
```

#### Low Fee Transaction Test
```bash
# This should fail due to insufficient fee (0.1% requirement)
RAW_TX_LOW_FEE="..."  # Create transaction with 0.001 CORAL fee for 10 CORAL transfer
./src/bitcoin-cli -regtest sendrawtransaction $RAW_TX_LOW_FEE
# Expected: "insufficient-fee-coral" error
```

### 4. Block Reward Tests

#### Verify 100 CORAL Block Rewards
```bash
# Generate a block and check coinbase output
ADDR=$(./src/bitcoin-cli -regtest getnewaddress)
BLOCKHASH=$(./src/bitcoin-cli -regtest generatetoaddress 1 $ADDR | jq -r '.[0]')
BLOCK=$(./src/bitcoin-cli -regtest getblock $BLOCKHASH)
COINBASE_TXID=$(echo $BLOCK | jq -r '.tx[0]')
./src/bitcoin-cli -regtest getrawtransaction $COINBASE_TXID true

# Verify vout[0].value is 100.00000000 CORAL
```

#### Test Halving Schedule
```bash
# Test subsidy calculation at different heights
./src/bitcoin-cli -regtest getblocksubsidy 0      # Should return 100 CORAL
./src/bitcoin-cli -regtest getblocksubsidy 210000 # Should return 50 CORAL
./src/bitcoin-cli -regtest getblocksubsidy 420000 # Should return 25 CORAL
```

### 5. Genesis Block Tests

#### Verify Custom Genesis Block
```bash
# Get genesis block
GENESIS=$(./src/bitcoin-cli -regtest getblockhash 0)
./src/bitcoin-cli -regtest getblock $GENESIS 2

# Verify coinbase message contains Coral genesis text
# Verify block reward is 100 CORAL
# Verify timestamp is 1727432400 (Sept 27, 2025)
```

### 6. Block #1 Difficulty Test

#### Verify High Difficulty Block #1
```bash
# Check block #1 difficulty (should be 1000x higher than genesis)
BLOCK1_HASH=$(./src/bitcoin-cli -regtest getblockhash 1)
BLOCK1=$(./src/bitcoin-cli -regtest getblock $BLOCK1_HASH)
echo $BLOCK1 | jq '.difficulty'

# Compare with genesis difficulty
GENESIS_HASH=$(./src/bitcoin-cli -regtest getblockhash 0)
GENESIS=$(./src/bitcoin-cli -regtest getblock $GENESIS_HASH)
echo $GENESIS | jq '.difficulty'

# Block #1 difficulty should be ~1000x higher
```

### 7. RandomX Integration Tests

#### Test RandomX Hash Calculation
```bash
# Monitor debug.log for RandomX initialization messages
./src/bitcoind -regtest -debug=1 -printtoconsole 2>&1 | grep -i randomx

# Expected messages:
# "RandomX: Initialized with key [KEY]"
# "RandomX: Using light mode" (if insufficient RAM for dataset)
```

#### Performance Test
```bash
# Time block generation (RandomX should be slower than SHA256)
time ./src/bitcoin-cli -regtest generatetoaddress 5 $(./src/bitcoin-cli -regtest getnewaddress)
```

### 8. Network Isolation Tests

#### Test Magic Bytes
```bash
# Coral should not connect to Bitcoin nodes due to different magic bytes
# Magic bytes: 0xc0, 0x72, 0x61, 0x6c ("coral")
./src/bitcoind -regtest -debug=net -printtoconsole 2>&1 | grep "ProcessMessage"
```

### 9. Integration Tests

#### Full Node Sync Test (Testnet)
```bash
# Start testnet node and verify it creates Coral-specific files
./src/bitcoind -testnet -datadir=/tmp/coral-testnet

# Verify correct port (18334), correct address prefixes
```

#### Mining Pool Simulation
```bash
# Test mining with getblocktemplate
./src/bitcoin-cli -regtest getblocktemplate

# Verify block template includes:
# - 100 CORAL coinbase reward
# - Volume-based fee validation
# - RandomX proof-of-work requirements
```

### 10. Error Handling Tests

#### Invalid Transactions
```bash
# Test transaction with insufficient fee
# Test double-spending
# Test invalid signatures
# All should be rejected with proper error messages
```

#### Resource Management
```bash
# Test RandomX cleanup on shutdown
./src/bitcoin-cli -regtest stop

# Check debug.log for "RandomX: Shutdown complete"
tail -n 20 ~/.bitcoin/regtest/debug.log | grep -i randomx
```

## Expected Results Summary

1. **Block Rewards**: Exactly 100.00000000 CORAL per block (first epoch)
2. **Transaction Fees**: Minimum 0.1% of transaction value
3. **Genesis Block**: Contains Coral message and 100 CORAL reward
4. **Block #1**: Successfully mines with 1000x difficulty
5. **RandomX**: Initializes correctly and produces valid hashes
6. **Network**: Uses custom ports (8334, 18334, 38334, 18445)
7. **Addresses**: "coral" for mainnet bech32, "C" for legacy addresses

## Troubleshooting

### Common Issues

1. **RandomX Library Not Found**
   ```bash
   sudo apt update
   sudo apt install librandomx-dev
   # Or compile from source at https://github.com/tevador/RandomX
   ```

2. **High Memory Usage**
   - RandomX dataset requires ~2GB RAM for full mode
   - Falls back to light mode automatically if insufficient memory

3. **Slow Block Generation**
   - RandomX is intentionally slower than SHA256
   - This is expected behavior for ASIC resistance

### Debug Commands
```bash
# Enable all debug categories
./src/bitcoind -regtest -debug=all -printtoconsole

# RandomX-specific debugging
./src/bitcoind -regtest -debug=1 -printtoconsole 2>&1 | grep -i randomx

# Fee debugging
./src/bitcoind -regtest -debug=mempool -printtoconsole 2>&1 | grep -i fee
```

## Production Deployment Notes

1. **Genesis Block**: Update chainparams.cpp with actual mined genesis hashes
2. **DNS Seeds**: Replace placeholder seeds with actual Coral network seeds
3. **Checkpoints**: Add block checkpoints for security
4. **Port Configuration**: Ensure firewall allows Coral ports (8334, etc.)
5. **RandomX**: Ensure adequate RAM for dataset mode (recommended)

## Security Considerations

1. **Volume-based Fees**: Prevents dust attacks but may affect microtransactions
2. **RandomX**: Provides ASIC resistance but requires more computational resources
3. **Custom Network**: Isolated from Bitcoin network, preventing accidental cross-chain transactions
4. **Block #1 Difficulty**: May cause initial network startup challenges in mainnet

This testing guide ensures comprehensive validation of all Coral-specific features while maintaining Bitcoin Core's robustness and security.