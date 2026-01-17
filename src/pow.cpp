// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Coral Core developers
// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow.h>

#include <arith_uint256.h>
#include <chain.h>
#include <primitives/block.h>
#include <uint256.h>
#include <logging.h>
#include <util/strencodings.h>
#include <streams.h>
#include <version.h>

#include <randomx.h>

// RandomX instance management
static randomx_cache* rx_cache = nullptr;
static randomx_dataset* rx_dataset = nullptr;
static randomx_vm* rx_vm = nullptr;
static uint256 rx_key_hash;
static bool rx_initialized = false;

unsigned int GetNextWorkRequired(const CBlockIndex* pindexLast, const CBlockHeader *pblock, const Consensus::Params& params)
{
    assert(pindexLast != nullptr);
    
    // Coral: Special case for block #1 - 1000x higher difficulty
    if ((pindexLast->nHeight + 1) == 1) {
        // Make block #1 1000x harder than genesis
        arith_uint256 bnLimit = UintToArith256(params.powLimit);
        arith_uint256 bnTarget = bnLimit / 1000;
        return bnTarget.GetCompact();
    }
    unsigned int nProofOfWorkLimit = UintToArith256(params.powLimit).GetCompact();

    // Only change once per difficulty adjustment interval
    if ((pindexLast->nHeight+1) % params.DifficultyAdjustmentInterval() != 0)
    {
        if (params.fPowAllowMinDifficultyBlocks)
        {
            // Special difficulty rule for testnet:
            // If the new block's timestamp is more than 2* 10 minutes
            // then allow mining of a min-difficulty block.
            if (pblock->GetBlockTime() > pindexLast->GetBlockTime() + params.nPowTargetSpacing*2)
                return nProofOfWorkLimit;
            else
            {
                // Return the last non-special-min-difficulty-rules-block
                const CBlockIndex* pindex = pindexLast;
                while (pindex->pprev && pindex->nHeight % params.DifficultyAdjustmentInterval() != 0 && pindex->nBits == nProofOfWorkLimit)
                    pindex = pindex->pprev;
                return pindex->nBits;
            }
        }
        return pindexLast->nBits;
    }

    // Go back by what we want to be 14 days worth of blocks
    int nHeightFirst = pindexLast->nHeight - (params.DifficultyAdjustmentInterval()-1);
    assert(nHeightFirst >= 0);
    const CBlockIndex* pindexFirst = pindexLast->GetAncestor(nHeightFirst);
    assert(pindexFirst);

    return CalculateNextWorkRequired(pindexLast, pindexFirst->GetBlockTime(), params);
}

unsigned int CalculateNextWorkRequired(const CBlockIndex* pindexLast, int64_t nFirstBlockTime, const Consensus::Params& params)
{
    if (params.fPowNoRetargeting)
        return pindexLast->nBits;

    // Limit adjustment step
    int64_t nActualTimespan = pindexLast->GetBlockTime() - nFirstBlockTime;
    if (nActualTimespan < params.nPowTargetTimespan/4)
        nActualTimespan = params.nPowTargetTimespan/4;
    if (nActualTimespan > params.nPowTargetTimespan*4)
        nActualTimespan = params.nPowTargetTimespan*4;

    // Retarget
    const arith_uint256 bnPowLimit = UintToArith256(params.powLimit);
    arith_uint256 bnNew;
    bnNew.SetCompact(pindexLast->nBits);
    bnNew *= nActualTimespan;
    bnNew /= params.nPowTargetTimespan;

    if (bnNew > bnPowLimit)
        bnNew = bnPowLimit;

    return bnNew.GetCompact();
}

// Check that on difficulty adjustments, the new difficulty does not increase
// or decrease beyond the permitted limits.
bool PermittedDifficultyTransition(const Consensus::Params& params, int64_t height, uint32_t old_nbits, uint32_t new_nbits)
{
    if (params.fPowAllowMinDifficultyBlocks) return true;

    if (height % params.DifficultyAdjustmentInterval() == 0) {
        int64_t smallest_timespan = params.nPowTargetTimespan/4;
        int64_t largest_timespan = params.nPowTargetTimespan*4;

        const arith_uint256 pow_limit = UintToArith256(params.powLimit);
        arith_uint256 observed_new_target;
        observed_new_target.SetCompact(new_nbits);

        // Calculate the largest difficulty value possible:
        arith_uint256 largest_difficulty_target;
        largest_difficulty_target.SetCompact(old_nbits);
        largest_difficulty_target *= largest_timespan;
        largest_difficulty_target /= params.nPowTargetTimespan;

        if (largest_difficulty_target > pow_limit) {
            largest_difficulty_target = pow_limit;
        }

        // Round and then compare this new calculated value to what is
        // observed.
        arith_uint256 maximum_new_target;
        maximum_new_target.SetCompact(largest_difficulty_target.GetCompact());
        if (maximum_new_target < observed_new_target) return false;

        // Calculate the smallest difficulty value possible:
        arith_uint256 smallest_difficulty_target;
        smallest_difficulty_target.SetCompact(old_nbits);
        smallest_difficulty_target *= smallest_timespan;
        smallest_difficulty_target /= params.nPowTargetTimespan;

        if (smallest_difficulty_target > pow_limit) {
            smallest_difficulty_target = pow_limit;
        }

        // Round and then compare this new calculated value to what is
        // observed.
        arith_uint256 minimum_new_target;
        minimum_new_target.SetCompact(smallest_difficulty_target.GetCompact());
        if (minimum_new_target > observed_new_target) return false;
    } else if (old_nbits != new_nbits) {
        return false;
    }
    return true;
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit))
        return false;

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}

// RandomX Key Generation - uses previous block hash as key
uint256 GetRandomXKey(const uint256& prevBlockHash) {
    // For genesis block, use a fixed key
    if (prevBlockHash.IsNull()) {
        return uint256S("0x436f72616c2047656e65736973204b657920536570742032372c2032303235"); // "Coral Genesis Key Sept 27, 2025"
    }
    return prevBlockHash;
}

// Initialize RandomX VM with given key
bool InitializeRandomX(const uint256& key) {
    try {
        // Clean up existing instances
        if (rx_vm) {
            randomx_destroy_vm(rx_vm);
            rx_vm = nullptr;
        }
        if (rx_dataset) {
            randomx_release_dataset(rx_dataset);
            rx_dataset = nullptr;
        }
        if (rx_cache) {
            randomx_release_cache(rx_cache);
            rx_cache = nullptr;
        }

        // Create new cache with the key
        rx_cache = randomx_alloc_cache(RANDOMX_FLAG_DEFAULT);
        if (!rx_cache) {
            LogPrintf("RandomX: Failed to allocate cache\n");
            return false;
        }

        randomx_init_cache(rx_cache, key.begin(), 32);

        // Create dataset (for better performance)
        rx_dataset = randomx_alloc_dataset(RANDOMX_FLAG_FULL_MEM);
        if (rx_dataset) {
            randomx_init_dataset(rx_dataset, rx_cache, 0, randomx_dataset_item_count());

            // Create VM with dataset
            rx_vm = randomx_create_vm(RANDOMX_FLAG_FULL_MEM, rx_cache, rx_dataset);
        } else {
            // Fallback to light mode if not enough memory
            LogPrintf("RandomX: Using light mode (dataset allocation failed)\n");
            rx_vm = randomx_create_vm(RANDOMX_FLAG_DEFAULT, rx_cache, nullptr);
        }

        if (!rx_vm) {
            LogPrintf("RandomX: Failed to create VM\n");
            return false;
        }

        rx_key_hash = key;
        rx_initialized = true;
        LogPrintf("RandomX: Initialized with key %s\n", key.ToString());
        return true;

    } catch (const std::exception& e) {
        LogPrintf("RandomX: Exception during initialization: %s\n", e.what());
        return false;
    }
}

// Compute RandomX hash for a block header
uint256 GetRandomXHash(const CBlockHeader& block) {
    // Get the key for this block (previous block hash)
    uint256 key = GetRandomXKey(block.hashPrevBlock);

    // Initialize RandomX if needed or key changed
    if (!rx_initialized || rx_key_hash != key) {
        if (!InitializeRandomX(key)) {
            LogPrintf("RandomX: Failed to initialize, falling back to SHA256\n");
            return block.GetHash(); // Fallback to SHA256
        }
    }

    try {
        // Serialize block header for hashing
        CDataStream ss(SER_NETWORK, INIT_PROTO_VERSION);
        ss << block;

        // Calculate RandomX hash
        char hash[RANDOMX_HASH_SIZE];
        randomx_calculate_hash(rx_vm, ss.data(), ss.size(), hash);

        // Convert to uint256
        uint256 result;
        memcpy(result.begin(), hash, 32);

        return result;

    } catch (const std::exception& e) {
        LogPrintf("RandomX: Exception during hash calculation: %s\n", e.what());
        return block.GetHash(); // Fallback to SHA256
    }
}

// Check RandomX proof of work
bool CheckRandomXProofOfWork(const CBlockHeader& block, unsigned int nBits, const Consensus::Params& params) {
    // Genesis block uses SHA256d (hashPrevBlock is null)
    // All subsequent blocks use RandomX
    if (block.hashPrevBlock.IsNull()) {
        return CheckProofOfWork(block.GetHash(), nBits, params);
    }

    uint256 randomx_hash = GetRandomXHash(block);
    return CheckProofOfWork(randomx_hash, nBits, params);
}

// Cleanup RandomX resources
void ShutdownRandomX() {
    if (rx_vm) {
        randomx_destroy_vm(rx_vm);
        rx_vm = nullptr;
    }
    if (rx_dataset) {
        randomx_release_dataset(rx_dataset);
        rx_dataset = nullptr;
    }
    if (rx_cache) {
        randomx_release_cache(rx_cache);
        rx_cache = nullptr;
    }
    rx_initialized = false;
    LogPrintf("RandomX: Shutdown complete\n");
}

