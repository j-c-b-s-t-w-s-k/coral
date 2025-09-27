// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CORAL_CONSENSUS_CONSENSUS_H
#define CORAL_CONSENSUS_CONSENSUS_H

#include <stdlib.h>
#include <stdint.h>

/** Coral: Base block size starts at 1MB and doubles every quartering epoch */
static const unsigned int CORAL_BASE_BLOCK_SIZE = 1000000; // 1MB
/** Maximum allowed size for a serialized block, dynamically calculated */
// Note: MAX_BLOCK_SERIALIZED_SIZE grows exponentially with halvings
static const unsigned int MAX_BLOCK_SERIALIZED_SIZE = 1073741824; // Max cap at 1GB for very distant future
/** Coral: No SegWit weight system, using pure block size */
static const unsigned int MAX_BLOCK_WEIGHT = MAX_BLOCK_SERIALIZED_SIZE;
/** The maximum allowed number of signature check operations in a block (network rule) */
static const int64_t MAX_BLOCK_SIGOPS_COST = 80000;
/** Coinbase transaction outputs can only be spent after this number of new blocks (network rule) */
static const int COINBASE_MATURITY = 100;

static const int WITNESS_SCALE_FACTOR = 4;

static const size_t MIN_TRANSACTION_WEIGHT = WITNESS_SCALE_FACTOR * 60; // 60 is the lower bound for the size of a valid serialized CTransaction
static const size_t MIN_SERIALIZABLE_TRANSACTION_WEIGHT = WITNESS_SCALE_FACTOR * 10; // 10 is the lower bound for the size of a serialized CTransaction

/** Flags for nSequence and nLockTime locks */
/** Interpret sequence numbers as relative lock-time constraints. */
static constexpr unsigned int LOCKTIME_VERIFY_SEQUENCE = (1 << 0);

/** Coral: Calculate maximum block size based on quartering epoch count */
inline unsigned int GetMaxBlockSize(int nHeight) {
    // Quartering epoch occurs every 210,000 blocks
    int nEpochs = nHeight / 210000;

    // Start with 1MB, double exponentially for each epoch: 1→2→4→8→16→32→64→128→256...
    // Block size doubles while rewards quarter - inverse relationship for scaling
    unsigned int blockSize = CORAL_BASE_BLOCK_SIZE << nEpochs;

    // Cap at 1GB for very distant future (prevents integer overflow)
    if (blockSize > MAX_BLOCK_SERIALIZED_SIZE || nEpochs > 30) {
        blockSize = MAX_BLOCK_SERIALIZED_SIZE;
    }

    return blockSize;
}

#endif // CORAL_CONSENSUS_CONSENSUS_H
