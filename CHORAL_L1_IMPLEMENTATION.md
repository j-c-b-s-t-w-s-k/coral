# Choral L1 Implementation Guide

## Overview

This document describes the Choral L1 implementation, a multi-lane blockchain built on top of the Coral codebase. Choral introduces a novel consensus mechanism with multiple proof-of-work lanes, subnet support, and a Merkle forest commitment structure.

## Key Features

### 1. Multi-Lane Proof-of-Work

Choral implements three distinct proof-of-work lanes:

- **Base PoW Lane**: Traditional block mining using RandomX
- **Receipt Lane**: Proof-of-work for subnet actions and operations
- **Service Lane**: Service obligations for subnet updates and message delivery

### 2. Merkle Forest Commitments

Each block contains a forest root that commits to four separate Merkle trees:

- `SUBNETS_ROOT`: Commits to subnet state updates
- `MESSAGES_ROOT`: Commits to cross-subnet messages
- `RECEIPTS_ROOT`: Commits to work receipts
- `REGISTRY_ROOT`: Commits to registry changes (governance)

### 3. Subnet Support

Choral provides a pluggable subnet architecture with a well-defined interface (`ISubnet`) that allows developers to create custom subnets. The reference implementation includes **CoralDNS**, a decentralized DNS system.

### 4. Economic Parameters

- **Total Supply**: 2.1 million CHORAL (exactly 10% of Bitcoin's supply)
- **Block Reward**: 7.5 CHORAL per block (initial)
- **Emission Schedule**: Reward quarters every 210,000 blocks
- **Target Block Time**: 10 minutes (same as Bitcoin)

## Architecture

### Block Header Structure

The Choral L1 block header extends the traditional Bitcoin block header:

```cpp
class CBlockHeader {
    int32_t nVersion;          // Protocol version
    uint256 hashPrevBlock;     // Previous block hash
    uint256 hashMerkleRoot;    // Traditional transaction merkle root
    uint64_t nHeight;          // Explicit block height
    uint32_t nTime;            // Block timestamp
    uint256 forest_root;       // Merkle forest commitment root

    // Multi-lane difficulty targets
    uint32_t nBits;            // Base PoW difficulty
    uint32_t nBits_receipt;    // Receipt lane difficulty
    uint32_t nBits_service;    // Service lane difficulty

    // Mining nonces
    uint64_t nNonce;           // Primary nonce
    uint64_t nExtraNonce;      // Additional nonce for miners
};
```

### Block Body Structure

```cpp
class CBlock : public CBlockHeader {
    // Traditional Bitcoin transactions
    std::vector<CTransactionRef> vtx;

    // Choral L1 components
    ForestCommitments forest;
    std::vector<SubnetUpdateEnvelope> subnet_updates;
    std::vector<CrossSubnetMessage> messages;
    std::vector<WorkReceipt> receipts;
    std::vector<RegistryDelta> registry_deltas;
};
```

## Core Components

### 1. Work Receipts (`WorkReceipt`)

Work receipts prove computational work for subnet actions:

```cpp
class WorkReceipt {
    SubnetId subnet_id;         // Target subnet
    WorkTypeId work_type_id;    // Type of work
    uint256 commitment;         // Hash of action payload
    uint64_t nonce;            // PoW nonce
    uint256 pow_hash;          // Computed hash
    uint64_t expires_at_height; // Anti-hoarding
    std::vector<unsigned char> sig; // Optional signature
};
```

Receipt PoW is computed as:
```
pow_hash = H("CHORAL/RECEIPT" || subnet_id || work_type_id || commitment || nonce || expires_at_height)
```

### 2. Cross-Subnet Messages

Messages facilitate communication between subnets:

```cpp
class CrossSubnetMessage {
    SubnetId source_subnet;
    SubnetId dest_subnet;
    uint64_t nonce;
    std::vector<unsigned char> payload;
    uint256 payload_hash;
    std::vector<unsigned char> sig;
};
```

### 3. Subnet Update Envelopes

State transitions for subnets:

```cpp
class SubnetUpdateEnvelope {
    SubnetId subnet_id;
    uint64_t subnet_height;
    uint32_t payload_type;
    std::vector<unsigned char> payload_bytes;
    uint256 payload_hash;
    std::vector<unsigned char> author_sig;
    std::vector<uint256> receipt_refs; // Optional receipt binding
};
```

### 4. Forest Commitments

```cpp
class ForestCommitments {
    uint256 subnets_root;
    uint256 messages_root;
    uint256 receipts_root;
    uint256 registry_root;

    uint64_t subnets_count;
    uint64_t messages_count;
    uint64_t receipts_count;
    uint64_t registry_count;
};
```

## Subnet Interface SDK

### ISubnet Interface

All subnets must implement the `ISubnet` interface:

```cpp
class ISubnet {
    virtual SubnetId GetSubnetId() const = 0;
    virtual std::string GetSubnetType() const = 0;

    virtual SubnetResult<uint256> ValidateUpdate(
        const SubnetUpdateEnvelope& envelope,
        const uint256& prior_state_root) const = 0;

    virtual SubnetResult<SubnetStateHandle> ApplyUpdate(
        const SubnetUpdateEnvelope& envelope,
        const SubnetStateHandle& state) const = 0;

    virtual uint256 ComputeStateRoot(const SubnetStateHandle& state) const = 0;

    virtual uint256 EncodeAnchor(
        const SubnetId& subnet_id,
        uint64_t subnet_height,
        const uint256& state_root,
        const uint256& tx_root,
        const uint256& prev_anchor) const = 0;

    virtual SubnetStateHandle GetGenesisState() const = 0;
    virtual std::vector<unsigned char> SerializeState(const SubnetStateHandle& state) const = 0;
    virtual SubnetResult<SubnetStateHandle> DeserializeState(
        const std::vector<unsigned char>& data) const = 0;
};
```

### Subnet Registration

```cpp
SubnetRegistry& registry = SubnetRegistry::GetGlobalRegistry();
auto subnet = std::make_shared<CoralDNS>();
registry.RegisterSubnet(subnet);
```

## CoralDNS Reference Subnet

CoralDNS is a decentralized DNS system built on Choral L1.

### Features

- Decentralized name registration
- Ownership transfer support
- Multiple record types (A, AAAA, TXT, CNAME, MX, NS)
- Append-only log with Merkle tree state commitment
- Anti-squatting via optional receipt requirements

### DNS Record Structure

```cpp
struct DNSRecord {
    std::string name;           // Domain name (e.g., "example.coral")
    DNSRecordType type;         // Record type
    std::vector<unsigned char> data; // Record data
    uint64_t ttl;              // Time-to-live
    uint64_t created_height;   // Creation height
    uint64_t updated_height;   // Last update height
    std::vector<unsigned char> owner_pubkey; // Owner's public key
};
```

### Update Types

1. **DNS_UPDATE_REGISTER**: Register a new domain name
2. **DNS_UPDATE_UPDATE**: Update existing record
3. **DNS_UPDATE_DELETE**: Mark record as deleted
4. **DNS_UPDATE_TRANSFER**: Transfer ownership

### Example: Registering a Domain

```cpp
auto envelope = CoralDNS::CreateRegistration(
    "mysite.coral",           // Domain name
    DNS_TYPE_A,               // Record type
    ipv4_address_bytes,       // IP address
    3600,                     // TTL (1 hour)
    owner_pubkey,             // Owner public key
    current_subnet_height     // Subnet height
);

// Submit to subnet
SubnetRegistry& registry = SubnetRegistry::GetGlobalRegistry();
auto dns = registry.GetSubnet(CoralDNS::GetCoralDNSSubnetId());
auto result = dns->ApplyUpdate(envelope, current_state);
```

## Consensus Rules

### Block Validation

A Choral L1 block is valid if:

1. **Header Chain**:
   - Parent exists and matches best chain
   - Height = parent.height + 1
   - Timestamp meets median-time-past

2. **Forest Consistency**:
   - Recomputed forest_root matches header commitment

3. **Base PoW**:
   - `H(header) < T_base(height)`

4. **Receipt Lane**:
   - At least `nMinReceiptsPerBlock` valid receipts
   - All receipts have valid PoW: `pow_hash < T_receipt`
   - No expired receipts

5. **Service Lane**:
   - Minimum message/subnet update quotas met

6. **Subnet Rules**:
   - All subnet updates validate correctly
   - State roots match committed values

### Difficulty Retargeting

- **T_base**: Classic Bitcoin-style interval retarget (every 2016 blocks)
- **T_receipt**: Retargets to maintain median receipts-per-block near k
- **T_service**: Fixed in v0 (very easy threshold)

## File Structure

```
src/
├── choral/
│   ├── choral_primitives.h      # Core Choral data structures
│   ├── choral_primitives.cpp    # Implementation
│   ├── subnet_interface.h       # Subnet SDK interface
│   ├── subnet_interface.cpp     # Registry implementation
│   ├── coraldns.h              # CoralDNS subnet
│   └── coraldns.cpp            # CoralDNS implementation
├── primitives/
│   ├── block.h                 # Extended block header/body
│   └── block.cpp               # Block methods
├── consensus/
│   └── params.h                # Updated consensus parameters
├── chainparams.cpp             # Chain parameters (2.1M supply)
└── validation.cpp              # Updated block subsidy
```

## Building Choral

### Prerequisites

```bash
sudo apt-get install build-essential libtool autotools-dev automake pkg-config \
    bsdmainutils python3 libssl-dev libevent-dev libboost-all-dev libsqlite3-dev \
    librandomx-dev
```

### Build Steps

```bash
cd /path/to/coral
./autogen.sh
./configure
make -j$(nproc)
```

### Run Node

```bash
./src/chorald -daemon
```

## Development Roadmap

### Implemented (v0.1)

- [x] Multi-lane block header structure
- [x] Merkle forest commitments
- [x] Work receipt PoW validation
- [x] Cross-subnet messaging primitives
- [x] Subnet interface SDK
- [x] CoralDNS reference subnet
- [x] 2.1M supply economics
- [x] Basic multi-lane validation

### Pending (v0.2)

- [ ] Three separate mempools (receipts, messages, subnet_updates)
- [ ] Block assembly with multi-lane template generation
- [ ] Multi-target difficulty retargeting
- [ ] P2P messages for Choral primitives
- [ ] Subnet state storage layer
- [ ] RPC methods for subnet operations
- [ ] `chorald` subcommands (node, mine, subnet, rpc, inspect)

### Future (v1.0)

- [ ] Full CoralDNS DNSSEC support
- [ ] Additional reference subnets
- [ ] Light client subnet proof verification
- [ ] Subnet cross-chain bridges
- [ ] Governance registry activation

## Security Considerations

### Domain Separation

All hash operations use strict domain separation tags:

- `"CHORAL/HEADER"` - Block header hashing
- `"CHORAL/FOREST/LEAF"` - Forest leaf construction
- `"CHORAL/RECEIPT"` - Receipt PoW computation
- `"CHORAL/SUBNET/<id>/..."` - Subnet-specific operations

### Receipt Expiry

Receipts include an `expires_at_height` field to prevent hoarding and ensure freshness.

### Subnet Isolation

Each subnet maintains independent state and cannot directly access other subnet states, ensuring security isolation.

## API Examples

### Query CoralDNS Record

```cpp
#include <choral/coraldns.h>

auto registry = SubnetRegistry::GetGlobalRegistry();
auto dns = std::static_pointer_cast<CoralDNS>(
    registry.GetSubnet(CoralDNS::GetCoralDNSSubnetId())
);

// Get current state
SubnetStateHandle state = dns->GetGenesisState();
// ... load state from storage ...

auto dns_state = std::static_pointer_cast<CoralDNSState>(state.data);
const DNSRecord* record = dns_state->FindRecord("example.coral");

if (record) {
    // Use record data
    std::cout << "IP: " << FormatIP(record->data) << std::endl;
}
```

### Create Work Receipt

```cpp
WorkReceipt receipt;
receipt.subnet_id = my_subnet_id;
receipt.work_type_id = MY_WORK_TYPE;
receipt.commitment = Hash(action_payload);
receipt.expires_at_height = current_height + 1000;

// Mine receipt PoW
while (true) {
    receipt.nonce = GetRand(UINT64_MAX);
    receipt.pow_hash = receipt.ComputeHash();
    if (receipt.CheckProofOfWork(nBits_receipt)) {
        break;
    }
}

// Submit to mempool
mempool.AddReceipt(receipt);
```

## Testing

### Unit Tests

```bash
src/test/test_bitcoin --run_test=choral_tests
```

### Test Vectors

Test vectors are provided in `/home/user/coral/src/test/data/choral/`:

- `forest_commitment_vectors.json` - Forest root calculations
- `receipt_pow_vectors.json` - Receipt PoW validation
- `subnet_state_vectors.json` - Subnet state transitions

## Contributing

When implementing new subnets:

1. Inherit from `ISubnet`
2. Implement all required methods
3. Register with `SubnetRegistry`
4. Add comprehensive tests
5. Document subnet-specific protocol

## License

Choral L1 is released under the MIT License, consistent with the Coral Core codebase.

## References

- Choral L1 Specification: (see included specification in user request)
- Bitcoin Core: https://github.com/bitcoin/bitcoin
- RandomX: https://github.com/tevador/RandomX
- Coral Implementation Summary: `/home/user/coral/CORAL_IMPLEMENTATION_SUMMARY.md`
