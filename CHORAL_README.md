# Choral L1 - Multi-Lane Blockchain with Subnet Support

## What is Choral?

Choral L1 is an advanced blockchain protocol built on the Coral codebase that introduces:

- **Multi-Lane Proof-of-Work**: Three independent PoW lanes (base, receipt, service) for different blockchain functions
- **Subnet Architecture**: Pluggable subnet system allowing custom application-specific blockchains
- **Merkle Forest**: Commitment structure for parallel data streams
- **CoralDNS**: Reference subnet implementing decentralized DNS

## Quick Start

### Installation

```bash
# Clone the repository
git clone https://github.com/nozmo-king/coral.git
cd coral
git checkout claude/choral-l1-cpp-0SDCv

# Build Choral
./autogen.sh
./configure
make -j$(nproc)
```

### Running a Node

```bash
# Start the Choral daemon
./src/chorald -daemon

# Check node status
./src/choral-cli getblockchaininfo

# Mine a block (regtest)
./src/choral-cli -regtest generate 1
```

### Register a Domain with CoralDNS

```bash
# Create a DNS registration (via RPC - to be implemented)
./src/choral-cli coraldns_register "myname.coral" A "192.168.1.1" 3600

# Query a DNS record
./src/choral-cli coraldns_query "myname.coral" A
```

## Key Specifications

| Parameter | Value |
|-----------|-------|
| Total Supply | 2,100,000 CHORAL |
| Initial Block Reward | 7.5 CHORAL |
| Emission Schedule | Quarter every 210,000 blocks |
| Block Time | 10 minutes |
| PoW Algorithm | RandomX |
| Min Receipts/Block | 10 |
| Max Receipts/Block | 1000 |

## Architecture Highlights

### Multi-Lane System

1. **Base Lane**: Traditional block mining
   - Secures the main chain
   - Uses RandomX PoW
   - Difficulty adjusts every 2016 blocks

2. **Receipt Lane**: Proof-of-work for subnet operations
   - Minimum 10 valid receipts per block
   - Independent difficulty targeting
   - Anti-spam mechanism for subnets

3. **Service Lane**: Subnet update delivery
   - Ensures subnet state synchronization
   - Message routing between subnets
   - Service quotas for miners

### Subnet System

Subnets are independent application-specific blockchains that anchor to Choral L1:

- **Isolated State**: Each subnet maintains its own state
- **Merkle Proofs**: Light client verification via forest commitments
- **Composability**: Cross-subnet messaging
- **Flexibility**: Implement custom consensus rules

### CoralDNS: The Reference Subnet

CoralDNS demonstrates the subnet system with a decentralized DNS:

- **Censorship Resistant**: No central authority
- **Ownership Proofs**: Cryptographic ownership via public keys
- **Append-Only Log**: Full audit trail
- **Standard DNS Records**: A, AAAA, TXT, CNAME, MX, NS

## Development

### Creating a Custom Subnet

```cpp
#include <choral/subnet_interface.h>

class MySubnet : public ISubnet {
public:
    SubnetId GetSubnetId() const override {
        return Hash("MySubnet/v1");
    }

    std::string GetSubnetType() const override {
        return "MySubnet/v1";
    }

    // Implement required methods...
    SubnetResult<uint256> ValidateUpdate(...) override { ... }
    SubnetResult<SubnetStateHandle> ApplyUpdate(...) override { ... }
    uint256 ComputeStateRoot(...) const override { ... }
    // ... etc
};

// Register the subnet
auto subnet = std::make_shared<MySubnet>();
SubnetRegistry::GetGlobalRegistry().RegisterSubnet(subnet);
```

### Testing

```bash
# Run unit tests
make check

# Run Choral-specific tests
src/test/test_bitcoin --run_test=choral_tests

# Run functional tests
test/functional/test_runner.py
```

## File Organization

```
src/choral/
├── choral_primitives.{h,cpp}   # Core data structures
├── subnet_interface.{h,cpp}    # Subnet SDK
└── coraldns.{h,cpp}           # DNS subnet implementation
```

## Economic Model

Choral's 2.1M supply (10% of Bitcoin) with aggressive quartering creates scarcity:

| Blocks | Reward | Total Supply |
|--------|--------|--------------|
| 0-210k | 7.5 | 1,575,000 |
| 210k-420k | 1.875 | 1,968,750 |
| 420k-630k | 0.46875 | 2,067,187.5 |
| ... | ... | → 2,100,000 |

## Use Cases

### 1. Decentralized Name Services (CoralDNS)
- Domain registration without ICANN
- Censorship-resistant websites
- Blockchain-based identity

### 2. Custom Application Chains
- Gaming state (inventory, achievements)
- Social media (posts, follows)
- Supply chain tracking
- Identity management

### 3. Cross-Chain Communication
- Asset bridges between subnets
- Shared reputation systems
- Interoperable governance

## Network Participation

### Mining

Miners earn rewards through:
1. **Block Mining**: Base PoW solving (7.5 CHORAL + fees)
2. **Receipt Inclusion**: Fees from subnet operations
3. **Service Bonuses**: Additional rewards for subnet updates

### Running a Subnet

Subnet operators:
1. Implement `ISubnet` interface
2. Register with SubnetRegistry
3. Generate subnet updates
4. Submit updates with receipts
5. Users anchor state to L1

## Roadmap

### Phase 1: Foundation (Current)
- [x] Multi-lane block structure
- [x] Merkle forest commitments
- [x] Subnet interface SDK
- [x] CoralDNS reference implementation

### Phase 2: Network (Next)
- [ ] Mempool segregation (receipts, messages, updates)
- [ ] P2P protocol extensions
- [ ] RPC API for subnet operations
- [ ] Block template generation

### Phase 3: Ecosystem
- [ ] Additional reference subnets
- [ ] Light client implementation
- [ ] Subnet development toolkit
- [ ] Cross-chain bridges

## Security

### Audit Status
- **Code Review**: Internal review completed
- **Security Audit**: Pending
- **Bug Bounty**: TBD

### Best Practices
1. Use domain-separated hashing
2. Validate all subnet updates
3. Enforce receipt expiry
4. Isolate subnet state

## Community

- **GitHub**: https://github.com/nozmo-king/coral
- **Issues**: Report bugs via GitHub Issues
- **Discussions**: GitHub Discussions for proposals

## Contributing

We welcome contributions! Please:

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests
5. Submit a pull request

See `CONTRIBUTING.md` for detailed guidelines.

## License

Choral L1 is released under the MIT License.

## Acknowledgments

- **Bitcoin Core**: Foundation for the codebase
- **RandomX**: Memory-hard PoW algorithm
- **Coral Team**: Base implementation

---

**Disclaimer**: Choral L1 is experimental software. Use at your own risk. Not financial advice.
