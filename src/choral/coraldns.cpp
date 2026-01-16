// Copyright (c) 2025 The Choral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <choral/coraldns.h>
#include <hash.h>
#include <streams.h>
#include <consensus/merkle.h>
#include <util/strencodings.h>
#include <logging.h>

// DNSRecord implementation

uint256 DNSRecord::GetHash() const
{
    return SerializeHash(*this);
}

// DNSUpdatePayload implementation

DNSUpdatePayload DNSUpdatePayload::Deserialize(const std::vector<unsigned char>& data)
{
    DNSUpdatePayload payload;
    try {
        CDataStream ds(data, SER_NETWORK, INIT_PROTO_VERSION);
        ds >> payload;
    } catch (const std::exception& e) {
        LogPrintf("CoralDNS: Failed to deserialize payload: %s\n", e.what());
    }
    return payload;
}

std::vector<unsigned char> DNSUpdatePayload::Serialize() const
{
    CDataStream ds(SER_NETWORK, INIT_PROTO_VERSION);
    ds << *this;
    return std::vector<unsigned char>(ds.begin(), ds.end());
}

// CoralDNSState implementation

void CoralDNSState::AddRecord(const DNSRecord& record)
{
    size_t index = records.size();
    records.push_back(record);
    name_index[record.name] = index;
    height++;
}

const DNSRecord* CoralDNSState::FindRecord(const std::string& name) const
{
    auto it = name_index.find(name);
    if (it == name_index.end()) {
        return nullptr;
    }
    return &records[it->second];
}

uint256 CoralDNSState::ComputeStateRoot() const
{
    if (records.empty()) {
        return uint256();
    }

    // Compute Merkle root over all record hashes
    std::vector<uint256> record_hashes;
    for (const auto& record : records) {
        record_hashes.push_back(record.GetHash());
    }

    return ComputeMerkleRoot(record_hashes);
}

// CoralDNS implementation

CoralDNS::CoralDNS()
{
    subnet_id = GetCoralDNSSubnetId();
    subnet_type = "CoralDNS/v1";
}

SubnetId CoralDNS::GetCoralDNSSubnetId()
{
    // Subnet ID is hash of the subnet type string
    const std::string subnet_type_str = "CoralDNS/v1";
    return Hash(subnet_type_str.begin(), subnet_type_str.end());
}

bool CoralDNS::ValidateDNSUpdate(const DNSUpdatePayload& payload, const CoralDNSState& state) const
{
    const DNSRecord& record = payload.record;

    // Validate name format (basic check)
    if (record.name.empty() || record.name.size() > 255) {
        return false;
    }

    // Check for invalid characters
    for (char c : record.name) {
        if (!std::isalnum(c) && c != '.' && c != '-' && c != '_') {
            return false;
        }
    }

    // Validate based on update type
    switch (payload.update_type) {
        case DNS_UPDATE_REGISTER: {
            // Name must not already exist
            if (state.FindRecord(record.name) != nullptr) {
                return false;
            }
            // Owner pubkey must be present
            if (record.owner_pubkey.empty()) {
                return false;
            }
            break;
        }

        case DNS_UPDATE_UPDATE: {
            // Name must exist
            const DNSRecord* existing = state.FindRecord(record.name);
            if (!existing) {
                return false;
            }
            // Owner must match (signature check would happen here)
            if (existing->owner_pubkey != record.owner_pubkey) {
                return false;
            }
            break;
        }

        case DNS_UPDATE_DELETE: {
            // Name must exist
            const DNSRecord* existing = state.FindRecord(record.name);
            if (!existing) {
                return false;
            }
            // Owner must match
            if (existing->owner_pubkey != record.owner_pubkey) {
                return false;
            }
            break;
        }

        case DNS_UPDATE_TRANSFER: {
            // Name must exist
            const DNSRecord* existing = state.FindRecord(record.name);
            if (!existing) {
                return false;
            }
            // Owner must match
            if (existing->owner_pubkey != record.owner_pubkey) {
                return false;
            }
            // New owner must be specified
            if (payload.new_owner_pubkey.empty()) {
                return false;
            }
            break;
        }

        default:
            return false;
    }

    return true;
}

SubnetResult<uint256> CoralDNS::ValidateUpdate(
    const SubnetUpdateEnvelope& envelope,
    const uint256& prior_state_root) const
{
    // Check subnet ID matches
    if (envelope.subnet_id != subnet_id) {
        return SubnetResult<uint256>::Err("Subnet ID mismatch");
    }

    // Deserialize payload
    DNSUpdatePayload payload = DNSUpdatePayload::Deserialize(envelope.payload_bytes);

    // Validate payload hash
    uint256 computed_hash = SerializeHash(payload);
    if (computed_hash != envelope.payload_hash) {
        return SubnetResult<uint256>::Err("Payload hash mismatch");
    }

    // For full validation, we'd need to load the prior state and validate the update
    // For now, basic validation only
    // In production, this would load state from storage

    return SubnetResult<uint256>::Ok(uint256()); // Placeholder
}

SubnetResult<SubnetStateHandle> CoralDNS::ApplyUpdate(
    const SubnetUpdateEnvelope& envelope,
    const SubnetStateHandle& state) const
{
    // Deserialize current state
    auto state_data = std::static_pointer_cast<CoralDNSState>(state.data);
    if (!state_data) {
        state_data = std::make_shared<CoralDNSState>();
    }

    CoralDNSState new_state = *state_data;

    // Deserialize payload
    DNSUpdatePayload payload = DNSUpdatePayload::Deserialize(envelope.payload_bytes);

    // Validate update
    if (!ValidateDNSUpdate(payload, new_state)) {
        return SubnetResult<SubnetStateHandle>::Err("Invalid DNS update");
    }

    // Apply update based on type
    DNSRecord record = payload.record;
    record.updated_height = envelope.subnet_height;

    switch (payload.update_type) {
        case DNS_UPDATE_REGISTER:
            record.created_height = envelope.subnet_height;
            new_state.AddRecord(record);
            break;

        case DNS_UPDATE_UPDATE:
            new_state.AddRecord(record);
            break;

        case DNS_UPDATE_DELETE:
            // In append-only log, we add a tombstone record
            record.data.clear();
            new_state.AddRecord(record);
            break;

        case DNS_UPDATE_TRANSFER:
            record.owner_pubkey = payload.new_owner_pubkey;
            new_state.AddRecord(record);
            break;
    }

    // Create new state handle
    SubnetStateHandle new_handle;
    new_handle.data = std::make_shared<CoralDNSState>(new_state);
    new_handle.subnet_id = subnet_id;
    new_handle.height = envelope.subnet_height;
    new_handle.state_root = new_state.ComputeStateRoot();

    return SubnetResult<SubnetStateHandle>::Ok(new_handle);
}

uint256 CoralDNS::ComputeStateRoot(const SubnetStateHandle& state) const
{
    auto state_data = std::static_pointer_cast<CoralDNSState>(state.data);
    if (!state_data) {
        return uint256();
    }
    return state_data->ComputeStateRoot();
}

uint256 CoralDNS::EncodeAnchor(
    const SubnetId& subnet_id,
    uint64_t subnet_height,
    const uint256& state_root,
    const uint256& tx_root,
    const uint256& prev_anchor) const
{
    CHashWriter ss(SER_GETHASH, 0);
    ss << std::string("CHORAL/ANCHOR/CORALDNS");
    ss << subnet_id;
    ss << subnet_height;
    ss << state_root;
    ss << tx_root;
    ss << prev_anchor;
    return ss.GetHash();
}

SubnetStateHandle CoralDNS::GetGenesisState() const
{
    SubnetStateHandle handle;
    handle.data = std::make_shared<CoralDNSState>();
    handle.subnet_id = subnet_id;
    handle.height = 0;
    handle.state_root = uint256(); // Empty state root
    return handle;
}

std::vector<unsigned char> CoralDNS::SerializeState(const SubnetStateHandle& state) const
{
    auto state_data = std::static_pointer_cast<CoralDNSState>(state.data);
    if (!state_data) {
        return {};
    }

    CDataStream ds(SER_DISK, INIT_PROTO_VERSION);
    ds << *state_data;
    return std::vector<unsigned char>(ds.begin(), ds.end());
}

SubnetResult<SubnetStateHandle> CoralDNS::DeserializeState(
    const std::vector<unsigned char>& data) const
{
    try {
        CoralDNSState state;
        CDataStream ds(data, SER_DISK, INIT_PROTO_VERSION);
        ds >> state;

        SubnetStateHandle handle;
        handle.data = std::make_shared<CoralDNSState>(state);
        handle.subnet_id = subnet_id;
        handle.height = state.height;
        handle.state_root = state.ComputeStateRoot();

        return SubnetResult<SubnetStateHandle>::Ok(handle);
    } catch (const std::exception& e) {
        return SubnetResult<SubnetStateHandle>::Err(std::string("Deserialization failed: ") + e.what());
    }
}

SubnetUpdateEnvelope CoralDNS::CreateRegistration(
    const std::string& name,
    DNSRecordType type,
    const std::vector<unsigned char>& data,
    uint64_t ttl,
    const std::vector<unsigned char>& owner_pubkey,
    uint64_t subnet_height)
{
    DNSRecord record;
    record.name = name;
    record.type = type;
    record.data = data;
    record.ttl = ttl;
    record.owner_pubkey = owner_pubkey;

    DNSUpdatePayload payload;
    payload.update_type = DNS_UPDATE_REGISTER;
    payload.record = record;

    SubnetUpdateEnvelope envelope;
    envelope.subnet_id = GetCoralDNSSubnetId();
    envelope.subnet_height = subnet_height;
    envelope.payload_type = DNS_UPDATE_REGISTER;
    envelope.payload_bytes = payload.Serialize();
    envelope.payload_hash = SerializeHash(payload);

    return envelope;
}

bool CoralDNS::VerifyOwnerSignature(
    const DNSRecord& record,
    const std::vector<unsigned char>& signature)
{
    // TODO: Implement signature verification
    // For now, placeholder
    return true;
}
