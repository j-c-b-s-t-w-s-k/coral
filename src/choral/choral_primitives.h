// Copyright (c) 2025 The Choral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CHORAL_PRIMITIVES_H
#define CHORAL_PRIMITIVES_H

#include <serialize.h>
#include <uint256.h>
#include <vector>
#include <string>

/**
 * Choral L1 Clock Block Primitives
 *
 * This file defines the core data structures for Choral's multi-lane blockchain:
 * - Work receipts with PoW validation
 * - Cross-subnet messages
 * - Subnet update envelopes
 * - Merkle forest commitments
 * - Registry deltas
 */

// Forward declarations
class CBlockHeader;
struct ForestCommitments;

/** Subnet identifier - 32-byte hash */
typedef uint256 SubnetId;

/** Work type identifier - stable numeric ID with domain separation */
typedef uint32_t WorkTypeId;

/**
 * Work Receipt - Proof of work for subnet actions
 *
 * Each receipt proves computational work performed for a specific subnet action.
 * Receipts have their own difficulty target separate from base PoW.
 */
class WorkReceipt
{
public:
    SubnetId subnet_id;
    WorkTypeId work_type_id;
    uint256 commitment;         // Hash of the action payload or pointer
    uint64_t nonce;
    uint256 pow_hash;           // Computed hash
    uint64_t expires_at_height; // Anti-hoarding mechanism
    std::vector<unsigned char> sig; // Optional signature

    WorkReceipt() { SetNull(); }

    SERIALIZE_METHODS(WorkReceipt, obj)
    {
        READWRITE(obj.subnet_id, obj.work_type_id, obj.commitment,
                  obj.nonce, obj.pow_hash, obj.expires_at_height, obj.sig);
    }

    void SetNull()
    {
        subnet_id.SetNull();
        work_type_id = 0;
        commitment.SetNull();
        nonce = 0;
        pow_hash.SetNull();
        expires_at_height = 0;
        sig.clear();
    }

    bool IsNull() const
    {
        return subnet_id.IsNull();
    }

    /**
     * Compute the PoW hash for this receipt
     * pow_hash = H("CHORAL/RECEIPT" || subnet_id || work_type_id || commitment || nonce || expires_at_height)
     */
    uint256 ComputeHash() const;

    /**
     * Validate receipt PoW against target
     */
    bool CheckProofOfWork(unsigned int nBits) const;

    uint256 GetHash() const;
};

/**
 * Cross-Subnet Message
 *
 * Messages facilitate communication between different subnets.
 */
class CrossSubnetMessage
{
public:
    SubnetId source_subnet;
    SubnetId dest_subnet;
    uint64_t nonce;
    std::vector<unsigned char> payload;
    uint256 payload_hash;
    std::vector<unsigned char> sig;

    CrossSubnetMessage() { SetNull(); }

    SERIALIZE_METHODS(CrossSubnetMessage, obj)
    {
        READWRITE(obj.source_subnet, obj.dest_subnet, obj.nonce,
                  obj.payload, obj.payload_hash, obj.sig);
    }

    void SetNull()
    {
        source_subnet.SetNull();
        dest_subnet.SetNull();
        nonce = 0;
        payload.clear();
        payload_hash.SetNull();
        sig.clear();
    }

    bool IsNull() const
    {
        return source_subnet.IsNull() && dest_subnet.IsNull();
    }

    uint256 GetHash() const;
};

/**
 * Subnet Update Envelope
 *
 * Contains state transition data for a subnet, along with proof/signature.
 */
class SubnetUpdateEnvelope
{
public:
    SubnetId subnet_id;
    uint64_t subnet_height;
    uint32_t payload_type;
    std::vector<unsigned char> payload_bytes;
    uint256 payload_hash;
    std::vector<unsigned char> author_sig;
    std::vector<uint256> receipt_refs; // Optional binding to receipts

    SubnetUpdateEnvelope() { SetNull(); }

    SERIALIZE_METHODS(SubnetUpdateEnvelope, obj)
    {
        READWRITE(obj.subnet_id, obj.subnet_height, obj.payload_type,
                  obj.payload_bytes, obj.payload_hash, obj.author_sig, obj.receipt_refs);
    }

    void SetNull()
    {
        subnet_id.SetNull();
        subnet_height = 0;
        payload_type = 0;
        payload_bytes.clear();
        payload_hash.SetNull();
        author_sig.clear();
        receipt_refs.clear();
    }

    bool IsNull() const
    {
        return subnet_id.IsNull();
    }

    uint256 GetHash() const;
};

/**
 * Registry Delta
 *
 * Changes to the global registry (future governance/parameter updates).
 */
class RegistryDelta
{
public:
    std::string key;
    std::vector<unsigned char> value;
    uint32_t operation; // 0=set, 1=delete, etc.

    RegistryDelta() { SetNull(); }

    SERIALIZE_METHODS(RegistryDelta, obj)
    {
        READWRITE(obj.key, obj.value, obj.operation);
    }

    void SetNull()
    {
        key.clear();
        value.clear();
        operation = 0;
    }

    bool IsNull() const
    {
        return key.empty();
    }

    uint256 GetHash() const;
};

/**
 * Merkle Forest Branch Types
 */
enum ForestBranchType : uint32_t {
    BRANCH_SUBNETS = 0,
    BRANCH_MESSAGES = 1,
    BRANCH_RECEIPTS = 2,
    BRANCH_REGISTRY = 3,
    BRANCH_MAX = 4
};

/**
 * Forest Commitments
 *
 * The Merkle forest contains multiple branch roots, each committing to
 * different aspects of the L1 state.
 */
class ForestCommitments
{
public:
    uint256 subnets_root;
    uint256 messages_root;
    uint256 receipts_root;
    uint256 registry_root;

    uint64_t subnets_count;
    uint64_t messages_count;
    uint64_t receipts_count;
    uint64_t registry_count;

    ForestCommitments() { SetNull(); }

    SERIALIZE_METHODS(ForestCommitments, obj)
    {
        READWRITE(obj.subnets_root, obj.messages_root, obj.receipts_root, obj.registry_root,
                  obj.subnets_count, obj.messages_count, obj.receipts_count, obj.registry_count);
    }

    void SetNull()
    {
        subnets_root.SetNull();
        messages_root.SetNull();
        receipts_root.SetNull();
        registry_root.SetNull();
        subnets_count = 0;
        messages_count = 0;
        receipts_count = 0;
        registry_count = 0;
    }

    bool IsNull() const
    {
        return subnets_root.IsNull() && messages_root.IsNull() &&
               receipts_root.IsNull() && registry_root.IsNull();
    }

    /**
     * Compute the overall forest root from individual branch roots
     * Forest root = MerkleRoot over sorted (domain_tag, branch_root) pairs
     */
    uint256 ComputeForestRoot() const;

    uint256 GetHash() const { return ComputeForestRoot(); }
};

#endif // CHORAL_PRIMITIVES_H
