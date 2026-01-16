// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2019 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <primitives/block.h>

#include <hash.h>
#include <tinyformat.h>
#include <consensus/merkle.h>

uint256 CBlockHeader::GetHash() const
{
    return SerializeHash(*this);
}

std::string CBlock::ToString() const
{
    std::stringstream s;
    s << strprintf("CBlock(hash=%s, ver=0x%08x, height=%u, hashPrevBlock=%s, hashMerkleRoot=%s, forest_root=%s, nTime=%u, nBits=%08x, nBits_receipt=%08x, nBits_service=%08x, nNonce=%u, nExtraNonce=%u, vtx=%u, receipts=%u, messages=%u, subnet_updates=%u, registry_deltas=%u)\n",
        GetHash().ToString(),
        nVersion,
        nHeight,
        hashPrevBlock.ToString(),
        hashMerkleRoot.ToString(),
        forest_root.ToString(),
        nTime, nBits, nBits_receipt, nBits_service, nNonce, nExtraNonce,
        vtx.size(), receipts.size(), messages.size(), subnet_updates.size(), registry_deltas.size());
    for (const auto& tx : vtx) {
        s << "  " << tx->ToString() << "\n";
    }
    s << strprintf("  Forest: subnets=%s, messages=%s, receipts=%s, registry=%s\n",
        forest.subnets_root.ToString(),
        forest.messages_root.ToString(),
        forest.receipts_root.ToString(),
        forest.registry_root.ToString());
    return s.str();
}

uint256 CBlock::ComputeForestRoot() const
{
    // Recompute each branch root from the block body

    ForestCommitments computed_forest;

    // Compute subnets root from subnet_updates
    if (!subnet_updates.empty()) {
        std::vector<uint256> subnet_hashes;
        for (const auto& update : subnet_updates) {
            subnet_hashes.push_back(update.GetHash());
        }
        computed_forest.subnets_root = ComputeMerkleRoot(subnet_hashes);
        computed_forest.subnets_count = subnet_updates.size();
    }

    // Compute messages root
    if (!messages.empty()) {
        std::vector<uint256> message_hashes;
        for (const auto& msg : messages) {
            message_hashes.push_back(msg.GetHash());
        }
        computed_forest.messages_root = ComputeMerkleRoot(message_hashes);
        computed_forest.messages_count = messages.size();
    }

    // Compute receipts root
    if (!receipts.empty()) {
        std::vector<uint256> receipt_hashes;
        for (const auto& receipt : receipts) {
            receipt_hashes.push_back(receipt.GetHash());
        }
        computed_forest.receipts_root = ComputeMerkleRoot(receipt_hashes);
        computed_forest.receipts_count = receipts.size();
    }

    // Compute registry root
    if (!registry_deltas.empty()) {
        std::vector<uint256> registry_hashes;
        for (const auto& delta : registry_deltas) {
            registry_hashes.push_back(delta.GetHash());
        }
        computed_forest.registry_root = ComputeMerkleRoot(registry_hashes);
        computed_forest.registry_count = registry_deltas.size();
    }

    return computed_forest.ComputeForestRoot();
}

bool CBlock::CheckChoralConstraints(unsigned int height, const Consensus::Params& consensusParams) const
{
    // TODO: Implement multi-lane validation
    // 1. Validate receipt lane: minimum k receipts, all valid PoW
    // 2. Validate service lane: service obligations met
    // 3. Validate subnet rules: all subnet updates valid

    // For now, basic validation
    // Check that forest_root matches computed forest root
    uint256 computed = ComputeForestRoot();
    if (forest_root != computed) {
        return false;
    }

    // Check all receipts have valid PoW
    for (const auto& receipt : receipts) {
        if (!receipt.CheckProofOfWork(nBits_receipt)) {
            return false;
        }
        // Check expiry
        if (receipt.expires_at_height < height) {
            return false;
        }
    }

    return true;
}
