// Copyright (c) 2025 The Choral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <choral/choral_primitives.h>
#include <hash.h>
#include <streams.h>
#include <arith_uint256.h>
#include <util/strencodings.h>
#include <algorithm>

// Domain separation tags
static const std::string CHORAL_RECEIPT_TAG = "CHORAL/RECEIPT";
static const std::string CHORAL_MESSAGE_TAG = "CHORAL/MESSAGE";
static const std::string CHORAL_SUBNET_TAG = "CHORAL/SUBNET";
static const std::string CHORAL_REGISTRY_TAG = "CHORAL/REGISTRY";
static const std::string CHORAL_FOREST_LEAF_TAG = "CHORAL/FOREST/LEAF";

// WorkReceipt implementation

uint256 WorkReceipt::ComputeHash() const
{
    CHashWriter ss(SER_GETHASH, 0);

    // Domain separation
    ss << CHORAL_RECEIPT_TAG;

    // Receipt fields
    ss << subnet_id;
    ss << work_type_id;
    ss << commitment;
    ss << nonce;
    ss << expires_at_height;

    return ss.GetHash();
}

bool WorkReceipt::CheckProofOfWork(unsigned int nBits) const
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow)
        return false;

    // Check proof of work matches claimed amount
    if (UintToArith256(pow_hash) > bnTarget)
        return false;

    // Verify the pow_hash was computed correctly
    uint256 computed_hash = ComputeHash();
    if (pow_hash != computed_hash)
        return false;

    return true;
}

uint256 WorkReceipt::GetHash() const
{
    return SerializeHash(*this);
}

// CrossSubnetMessage implementation

uint256 CrossSubnetMessage::GetHash() const
{
    return SerializeHash(*this);
}

// SubnetUpdateEnvelope implementation

uint256 SubnetUpdateEnvelope::GetHash() const
{
    return SerializeHash(*this);
}

// RegistryDelta implementation

uint256 RegistryDelta::GetHash() const
{
    return SerializeHash(*this);
}

// ForestCommitments implementation

uint256 ForestCommitments::ComputeForestRoot() const
{
    // Create forest leaves with domain tags
    struct ForestLeaf {
        uint32_t branch_type;
        uint256 root;

        uint256 GetLeafHash() const {
            CHashWriter ss(SER_GETHASH, 0);
            ss << CHORAL_FOREST_LEAF_TAG;
            ss << branch_type;
            ss << root;
            return ss.GetHash();
        }

        bool operator<(const ForestLeaf& other) const {
            return branch_type < other.branch_type;
        }
    };

    std::vector<ForestLeaf> leaves;

    // Add all branch roots as leaves
    leaves.push_back({BRANCH_SUBNETS, subnets_root});
    leaves.push_back({BRANCH_MESSAGES, messages_root});
    leaves.push_back({BRANCH_RECEIPTS, receipts_root});
    leaves.push_back({BRANCH_REGISTRY, registry_root});

    // Sort leaves by branch type (should already be sorted, but enforce it)
    std::sort(leaves.begin(), leaves.end());

    // Compute Merkle root over leaf hashes
    std::vector<uint256> leaf_hashes;
    for (const auto& leaf : leaves) {
        leaf_hashes.push_back(leaf.GetLeafHash());
    }

    // Simple Merkle tree construction
    while (leaf_hashes.size() > 1) {
        std::vector<uint256> next_level;

        for (size_t i = 0; i < leaf_hashes.size(); i += 2) {
            if (i + 1 < leaf_hashes.size()) {
                // Hash pair
                CHashWriter ss(SER_GETHASH, 0);
                ss << leaf_hashes[i];
                ss << leaf_hashes[i + 1];
                next_level.push_back(ss.GetHash());
            } else {
                // Odd one out - hash with itself (Bitcoin-style)
                CHashWriter ss(SER_GETHASH, 0);
                ss << leaf_hashes[i];
                ss << leaf_hashes[i];
                next_level.push_back(ss.GetHash());
            }
        }

        leaf_hashes = next_level;
    }

    return leaf_hashes.empty() ? uint256() : leaf_hashes[0];
}
