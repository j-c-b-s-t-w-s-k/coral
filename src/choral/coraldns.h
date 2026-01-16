// Copyright (c) 2025 The Choral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CHORAL_CORALDNS_H
#define CHORAL_CORALDNS_H

#include <choral/subnet_interface.h>
#include <choral/choral_primitives.h>
#include <vector>
#include <map>
#include <string>

/**
 * CoralDNS - Reference Subnet Implementation
 *
 * A DNS-like append-only log subnet that allows users to register
 * and update name->data mappings on the Choral L1 blockchain.
 *
 * Features:
 * - Append-only log of DNS-like records
 * - Name registration and updates
 * - Merkle tree state commitment
 * - Receipt-based spam prevention (optional)
 */

/** DNS Record Types */
enum DNSRecordType : uint32_t {
    DNS_TYPE_A = 1,        // IPv4 address
    DNS_TYPE_AAAA = 28,    // IPv6 address
    DNS_TYPE_TXT = 16,     // Text record
    DNS_TYPE_CNAME = 5,    // Canonical name
    DNS_TYPE_MX = 15,      // Mail exchange
    DNS_TYPE_NS = 2,       // Name server
};

/** DNS Record Entry */
struct DNSRecord {
    std::string name;           // Domain name (e.g., "example.coral")
    DNSRecordType type;         // Record type
    std::vector<unsigned char> data; // Record data
    uint64_t ttl;              // Time-to-live in seconds
    uint64_t created_height;   // L1 height when created
    uint64_t updated_height;   // L1 height when last updated
    std::vector<unsigned char> owner_pubkey; // Owner's public key

    DNSRecord() : type(DNS_TYPE_A), ttl(3600), created_height(0), updated_height(0) {}

    uint256 GetHash() const;

    SERIALIZE_METHODS(DNSRecord, obj)
    {
        READWRITE(obj.name, obj.type, obj.data, obj.ttl,
                  obj.created_height, obj.updated_height, obj.owner_pubkey);
    }
};

/** CoralDNS Update Payload Types */
enum DNSUpdateType : uint32_t {
    DNS_UPDATE_REGISTER = 0,   // Register new name
    DNS_UPDATE_UPDATE = 1,     // Update existing record
    DNS_UPDATE_DELETE = 2,     // Delete record (mark as deleted)
    DNS_UPDATE_TRANSFER = 3,   // Transfer ownership
};

/** CoralDNS Update Payload */
struct DNSUpdatePayload {
    DNSUpdateType update_type;
    DNSRecord record;
    std::vector<unsigned char> new_owner_pubkey; // For transfers

    DNSUpdatePayload() : update_type(DNS_UPDATE_REGISTER) {}

    SERIALIZE_METHODS(DNSUpdatePayload, obj)
    {
        READWRITE(obj.update_type, obj.record, obj.new_owner_pubkey);
    }

    static DNSUpdatePayload Deserialize(const std::vector<unsigned char>& data);
    std::vector<unsigned char> Serialize() const;
};

/** CoralDNS State - Append-only log with Merkle tree */
struct CoralDNSState {
    std::vector<DNSRecord> records;           // All DNS records (append-only)
    std::map<std::string, size_t> name_index; // name -> latest record index
    uint64_t height;                          // State height

    CoralDNSState() : height(0) {}

    /** Add a record to the log */
    void AddRecord(const DNSRecord& record);

    /** Find record by name */
    const DNSRecord* FindRecord(const std::string& name) const;

    /** Compute Merkle root of all records */
    uint256 ComputeStateRoot() const;

    SERIALIZE_METHODS(CoralDNSState, obj)
    {
        READWRITE(obj.records, obj.name_index, obj.height);
    }
};

/**
 * CoralDNS Subnet Implementation
 */
class CoralDNS : public ISubnet {
private:
    SubnetId subnet_id;
    std::string subnet_type;

    /** Validate a DNS update payload */
    bool ValidateDNSUpdate(const DNSUpdatePayload& payload, const CoralDNSState& state) const;

public:
    CoralDNS();

    // ISubnet interface implementation
    SubnetId GetSubnetId() const override { return subnet_id; }
    std::string GetSubnetType() const override { return subnet_type; }

    SubnetResult<uint256> ValidateUpdate(
        const SubnetUpdateEnvelope& envelope,
        const uint256& prior_state_root) const override;

    SubnetResult<SubnetStateHandle> ApplyUpdate(
        const SubnetUpdateEnvelope& envelope,
        const SubnetStateHandle& state) const override;

    uint256 ComputeStateRoot(const SubnetStateHandle& state) const override;

    uint256 EncodeAnchor(
        const SubnetId& subnet_id,
        uint64_t subnet_height,
        const uint256& state_root,
        const uint256& tx_root,
        const uint256& prev_anchor) const override;

    SubnetStateHandle GetGenesisState() const override;

    std::vector<unsigned char> SerializeState(const SubnetStateHandle& state) const override;

    SubnetResult<SubnetStateHandle> DeserializeState(
        const std::vector<unsigned char>& data) const override;

    /** Helper: Get the CoralDNS subnet ID (hash of "CoralDNS/v1") */
    static SubnetId GetCoralDNSSubnetId();

    /** Helper: Create a DNS registration payload */
    static SubnetUpdateEnvelope CreateRegistration(
        const std::string& name,
        DNSRecordType type,
        const std::vector<unsigned char>& data,
        uint64_t ttl,
        const std::vector<unsigned char>& owner_pubkey,
        uint64_t subnet_height);

    /** Helper: Verify ownership signature */
    static bool VerifyOwnerSignature(
        const DNSRecord& record,
        const std::vector<unsigned char>& signature);
};

#endif // CHORAL_CORALDNS_H
