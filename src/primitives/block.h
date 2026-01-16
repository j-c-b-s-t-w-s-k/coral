// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2020 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CORAL_PRIMITIVES_BLOCK_H
#define CORAL_PRIMITIVES_BLOCK_H

#include <primitives/transaction.h>
#include <serialize.h>
#include <uint256.h>
#include <util/time.h>
#include <choral/choral_primitives.h>

/** Nodes collect new transactions into a block, hash them into a hash tree,
 * and scan through nonce values to make the block's hash satisfy proof-of-work
 * requirements.  When they solve the proof-of-work, they broadcast the block
 * to everyone and the block is added to the block chain.  The first transaction
 * in the block is a special one that creates a new coin owned by the creator
 * of the block.
 */
class CBlockHeader
{
public:
    // header - original Bitcoin/Coral fields
    int32_t nVersion;
    uint256 hashPrevBlock;
    uint256 hashMerkleRoot; // Traditional merkle root for transactions

    // Choral L1 extensions
    uint64_t nHeight;          // Block height (explicit for Choral)
    uint32_t nTime;
    uint256 forest_root;       // Merkle forest commitment root

    // Multi-lane difficulty targets
    uint32_t nBits;            // Base PoW difficulty (nBits_base)
    uint32_t nBits_receipt;    // Receipt lane difficulty
    uint32_t nBits_service;    // Service lane difficulty

    // Nonces for mining
    uint64_t nNonce;
    uint64_t nExtraNonce;      // Additional nonce for miners

    CBlockHeader()
    {
        SetNull();
    }

    SERIALIZE_METHODS(CBlockHeader, obj) {
        READWRITE(obj.nVersion, obj.hashPrevBlock, obj.hashMerkleRoot,
                  obj.nHeight, obj.nTime, obj.forest_root,
                  obj.nBits, obj.nBits_receipt, obj.nBits_service,
                  obj.nNonce, obj.nExtraNonce);
    }

    void SetNull()
    {
        nVersion = 0;
        hashPrevBlock.SetNull();
        hashMerkleRoot.SetNull();
        nHeight = 0;
        nTime = 0;
        forest_root.SetNull();
        nBits = 0;
        nBits_receipt = 0;
        nBits_service = 0;
        nNonce = 0;
        nExtraNonce = 0;
    }

    bool IsNull() const
    {
        return (nBits == 0);
    }

    uint256 GetHash() const;

    NodeSeconds Time() const
    {
        return NodeSeconds{std::chrono::seconds{nTime}};
    }

    int64_t GetBlockTime() const
    {
        return (int64_t)nTime;
    }
};


class CBlock : public CBlockHeader
{
public:
    // network and disk - traditional Bitcoin transactions
    std::vector<CTransactionRef> vtx;

    // Choral L1 body components
    ForestCommitments forest;
    std::vector<SubnetUpdateEnvelope> subnet_updates;
    std::vector<CrossSubnetMessage> messages;
    std::vector<WorkReceipt> receipts;
    std::vector<RegistryDelta> registry_deltas;

    // memory only
    mutable bool fChecked;

    CBlock()
    {
        SetNull();
    }

    CBlock(const CBlockHeader &header)
    {
        SetNull();
        *(static_cast<CBlockHeader*>(this)) = header;
    }

    SERIALIZE_METHODS(CBlock, obj)
    {
        READWRITEAS(CBlockHeader, obj);
        READWRITE(obj.vtx);
        READWRITE(obj.forest);
        READWRITE(obj.subnet_updates);
        READWRITE(obj.messages);
        READWRITE(obj.receipts);
        READWRITE(obj.registry_deltas);
    }

    void SetNull()
    {
        CBlockHeader::SetNull();
        vtx.clear();
        forest.SetNull();
        subnet_updates.clear();
        messages.clear();
        receipts.clear();
        registry_deltas.clear();
        fChecked = false;
    }

    CBlockHeader GetBlockHeader() const
    {
        CBlockHeader block;
        block.nVersion       = nVersion;
        block.hashPrevBlock  = hashPrevBlock;
        block.hashMerkleRoot = hashMerkleRoot;
        block.nHeight        = nHeight;
        block.nTime          = nTime;
        block.forest_root    = forest_root;
        block.nBits          = nBits;
        block.nBits_receipt  = nBits_receipt;
        block.nBits_service  = nBits_service;
        block.nNonce         = nNonce;
        block.nExtraNonce    = nExtraNonce;
        return block;
    }

    std::string ToString() const;

    /**
     * Recompute the forest root from the block body
     * Used for validation
     */
    uint256 ComputeForestRoot() const;

    /**
     * Validate Choral L1 multi-lane constraints
     */
    bool CheckChoralConstraints(unsigned int nHeight, const class Consensus::Params& consensusParams) const;
};

/** Describes a place in the block chain to another node such that if the
 * other node doesn't have the same branch, it can find a recent common trunk.
 * The further back it is, the further before the fork it may be.
 */
struct CBlockLocator
{
    std::vector<uint256> vHave;

    CBlockLocator() {}

    explicit CBlockLocator(std::vector<uint256>&& have) : vHave(std::move(have)) {}

    SERIALIZE_METHODS(CBlockLocator, obj)
    {
        int nVersion = s.GetVersion();
        if (!(s.GetType() & SER_GETHASH))
            READWRITE(nVersion);
        READWRITE(obj.vHave);
    }

    void SetNull()
    {
        vHave.clear();
    }

    bool IsNull() const
    {
        return vHave.empty();
    }
};

#endif // CORAL_PRIMITIVES_BLOCK_H
