// Genesis Block Miner for Coral - Optimized for Apple Silicon (M-series)
// Finds a valid nonce for the genesis block using RandomX

#include <chainparams.h>
#include <consensus/params.h>
#include <consensus/merkle.h>
#include <pow.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <arith_uint256.h>
#include <hash.h>
#include <script/script.h>
#include <util/time.h>
#include <util/system.h>
#include <util/strencodings.h>
#include <randomx.h>

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>
#include <mutex>

// Genesis block parameters - must match chainparams.cpp
static const uint32_t GENESIS_TIME = 1768629600;  // January 17, 2026 00:00:00 UTC
static const int32_t GENESIS_VERSION = 1;
static const char* GENESIS_MESSAGE = "17/Jan/2026 Copper prices hit historic highs as electric vehicle demand surges";
static const CAmount GENESIS_REWARD = 50 * COIN;
// Genesis public key (compressed) - recipient of genesis block reward
static const char* GENESIS_PUBKEY_HEX = "020600a997bdceb13273e9de1db3c252f91731fba6cfb4a33411dccb17f113f8af";

// Mining state
static std::atomic<bool> found(false);
static std::atomic<uint64_t> totalHashes(0);
static std::mutex resultMutex;
static uint32_t winningNonce = 0;
static uint256 winningHash;
static uint256 winningMerkle;

// Shared RandomX cache and dataset (read-only after init, can be shared across threads)
static randomx_cache* sharedCache = nullptr;
static randomx_dataset* sharedDataset = nullptr;
static randomx_flags sharedFlags;
static bool useFullMem = true;  // Use 2GB dataset for faster mining

void PrintProgress() {
    auto startTime = std::chrono::steady_clock::now();

    while (!found) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();
        uint64_t hashes = totalHashes.load();
        double hashrate = elapsed > 0 ? (double)hashes / elapsed : 0;

        printf("\r[%llds] Hashes: %llu | Rate: %.2f H/s | %.2f KH/s          ",
               (long long)elapsed, (unsigned long long)hashes, hashrate, hashrate / 1000.0);
        fflush(stdout);
    }
}

void MineGenesis(uint32_t nBits, uint32_t startNonce, uint32_t step, int threadId) {
    // Create coinbase transaction
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);

    // Coinbase scriptSig: nBits + extra nonce (4) + timestamp message
    const char* pszTimestamp = GENESIS_MESSAGE;
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4)
        << std::vector<unsigned char>((const unsigned char*)pszTimestamp,
                                       (const unsigned char*)pszTimestamp + strlen(pszTimestamp));

    // Output: pay to genesis public key
    std::vector<unsigned char> genesisPubKey = ParseHex(GENESIS_PUBKEY_HEX);
    txNew.vout[0].nValue = GENESIS_REWARD;
    txNew.vout[0].scriptPubKey = CScript() << genesisPubKey << OP_CHECKSIG;

    // Create genesis block
    CBlock genesis;
    genesis.nVersion = GENESIS_VERSION;
    genesis.nTime = GENESIS_TIME;
    genesis.nBits = nBits;
    genesis.hashPrevBlock.SetNull();
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);

    // Calculate target
    arith_uint256 target;
    target.SetCompact(nBits);

    if (threadId == 0) {
        printf("Target: %s\n", ArithToUint256(target).ToString().c_str());
        printf("Merkle Root: %s\n", genesis.hashMerkleRoot.ToString().c_str());
        printf("Coinbase Tx: %s\n\n", genesis.vtx[0]->GetHash().ToString().c_str());
    }

    // Create thread-local VM using shared cache/dataset
    randomx_vm* vm = randomx_create_vm(sharedFlags, sharedCache, sharedDataset);
    if (!vm) {
        printf("Thread %d: Failed to create RandomX VM\n", threadId);
        return;
    }

    printf("Thread %d: Started (nonce %u, step %u)\n", threadId, startNonce, step);

    uint32_t nonce = startNonce;
    uint64_t localHashes = 0;
    uint64_t reportInterval = 100;

    // Pre-serialize the block header template (only nonce changes)
    CBlockHeader header = genesis.GetBlockHeader();

    while (!found) {
        header.nNonce = nonce;

        // Serialize block header
        CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
        ss << header;

        // Calculate RandomX hash
        uint256 hash;
        randomx_calculate_hash(vm, ss.data(), ss.size(), hash.begin());

        localHashes++;

        // Check if hash meets target
        if (UintToArith256(hash) <= target) {
            std::lock_guard<std::mutex> lock(resultMutex);
            if (!found) {
                found = true;
                winningNonce = nonce;
                winningHash = hash;
                winningMerkle = genesis.hashMerkleRoot;

                printf("\n\n");
                printf("╔══════════════════════════════════════════════════════════════════╗\n");
                printf("║              GENESIS BLOCK FOUND!                                ║\n");
                printf("╠══════════════════════════════════════════════════════════════════╣\n");
                printf("║ Nonce:       %-52u ║\n", nonce);
                printf("║ Hash:        %-52s ║\n", hash.ToString().substr(0, 52).c_str());
                printf("║              %-52s ║\n", hash.ToString().substr(52).c_str());
                printf("║ Merkle Root: %-52s ║\n", genesis.hashMerkleRoot.ToString().substr(0, 52).c_str());
                printf("║              %-52s ║\n", genesis.hashMerkleRoot.ToString().substr(52).c_str());
                printf("║ Time:        %-52u ║\n", genesis.nTime);
                printf("║ Bits:        0x%-50x ║\n", genesis.nBits);
                printf("╠══════════════════════════════════════════════════════════════════╣\n");
                printf("║ UPDATE chainparams.cpp:                                          ║\n");
                printf("╠══════════════════════════════════════════════════════════════════╣\n");
                printf("║ nNonce = %u;%-53s ║\n", nonce, "");
                printf("║ assert(consensus.hashGenesisBlock ==                             ║\n");
                printf("║   uint256S(\"0x%s\")); ║\n", hash.ToString().c_str());
                printf("║ assert(genesis.hashMerkleRoot ==                                 ║\n");
                printf("║   uint256S(\"0x%s\")); ║\n", genesis.hashMerkleRoot.ToString().c_str());
                printf("╚══════════════════════════════════════════════════════════════════╝\n");
            }
            break;
        }

        // Update global counter periodically
        if (localHashes % reportInterval == 0) {
            totalHashes += reportInterval;
        }

        nonce += step;

        // Handle overflow
        if (nonce < startNonce) {
            printf("Thread %d: Nonce space exhausted\n", threadId);
            break;
        }
    }

    totalHashes += (localHashes % reportInterval);
    randomx_destroy_vm(vm);
}

int main(int argc, char* argv[]) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════════╗\n");
    printf("║     CORAL GENESIS BLOCK MINER - Optimized for Apple Silicon      ║\n");
    printf("╚══════════════════════════════════════════════════════════════════╝\n\n");

    // Default parameters
    uint32_t nBits = 0x1d00ffff;  // Standard Bitcoin-like initial difficulty
    int numThreads = std::thread::hardware_concurrency();

    if (argc > 1) {
        nBits = strtoul(argv[1], nullptr, 16);
    }
    if (argc > 2) {
        numThreads = atoi(argv[2]);
    }

    printf("Genesis Message: \"%s\"\n", GENESIS_MESSAGE);
    printf("Timestamp:       %u (Jan 17, 2026 00:00:00 UTC)\n", GENESIS_TIME);
    printf("Target nBits:    0x%08x\n", nBits);
    printf("CPU Threads:     %d\n\n", numThreads);

    // Configure RandomX flags for Apple Silicon
    printf("Initializing RandomX...\n");

    sharedFlags = randomx_get_flags();

    // Enable JIT for better performance on M-series
    sharedFlags = static_cast<randomx_flags>(sharedFlags | RANDOMX_FLAG_JIT);

    // Enable hardware AES if available
    sharedFlags = static_cast<randomx_flags>(sharedFlags | RANDOMX_FLAG_HARD_AES);

    // Enable full memory mode for much faster hashing (uses 2GB RAM)
    if (useFullMem) {
        sharedFlags = static_cast<randomx_flags>(sharedFlags | RANDOMX_FLAG_FULL_MEM);
    }

    printf("  RandomX flags: 0x%x\n", sharedFlags);
    printf("  JIT:           %s\n", (sharedFlags & RANDOMX_FLAG_JIT) ? "Enabled" : "Disabled");
    printf("  Hard AES:      %s\n", (sharedFlags & RANDOMX_FLAG_HARD_AES) ? "Enabled" : "Disabled");
    printf("  Full Memory:   %s (2GB dataset = FAST)\n", (sharedFlags & RANDOMX_FLAG_FULL_MEM) ? "Enabled" : "Disabled");

    // Allocate shared cache
    printf("\nAllocating RandomX cache (~256 MB)...\n");
    sharedCache = randomx_alloc_cache(sharedFlags);
    if (!sharedCache) {
        printf("ERROR: Failed to allocate RandomX cache!\n");
        return 1;
    }

    // Initialize cache with key (genesis prev block hash = all zeros)
    printf("Initializing cache...\n");
    char key[65];
    memset(key, '0', 64);
    key[64] = '\0';

    randomx_init_cache(sharedCache, key, 64);

    // Allocate and initialize dataset for full memory mode
    if (useFullMem) {
        printf("Allocating RandomX dataset (~2 GB)...\n");
        sharedDataset = randomx_alloc_dataset(sharedFlags);
        if (!sharedDataset) {
            printf("ERROR: Failed to allocate RandomX dataset! Try with less threads or disable full memory mode.\n");
            randomx_release_cache(sharedCache);
            return 1;
        }

        printf("Initializing dataset (this takes 1-2 minutes, using %d threads)...\n", numThreads);
        auto initStart = std::chrono::steady_clock::now();

        // Initialize dataset in parallel
        uint32_t datasetItemCount = randomx_dataset_item_count();
        uint32_t itemsPerThread = datasetItemCount / numThreads;

        std::vector<std::thread> initThreads;
        for (int i = 0; i < numThreads; i++) {
            uint32_t startItem = i * itemsPerThread;
            uint32_t count = (i == numThreads - 1) ? (datasetItemCount - startItem) : itemsPerThread;
            initThreads.emplace_back([startItem, count]() {
                randomx_init_dataset(sharedDataset, sharedCache, startItem, count);
            });
        }
        for (auto& t : initThreads) {
            t.join();
        }

        auto initEnd = std::chrono::steady_clock::now();
        auto initTime = std::chrono::duration_cast<std::chrono::seconds>(initEnd - initStart).count();
        printf("Dataset initialized in %lld seconds\n\n", (long long)initTime);
    } else {
        printf("Cache initialized (light mode - slower but less RAM)\n\n");
    }

    printf("Starting mining with %d threads...\n\n", numThreads);

    // Start progress thread
    std::thread progressThread(PrintProgress);

    // Start mining threads
    std::vector<std::thread> threads;
    for (int i = 0; i < numThreads; i++) {
        threads.emplace_back(MineGenesis, nBits, i, numThreads, i);
    }

    // Wait for all threads
    for (auto& t : threads) {
        t.join();
    }

    found = true;  // Signal progress thread to stop
    progressThread.join();

    // Cleanup
    if (sharedDataset) {
        randomx_release_dataset(sharedDataset);
    }
    randomx_release_cache(sharedCache);

    if (winningNonce != 0 || found) {
        printf("\nMining complete! Update chainparams.cpp with the values above.\n");
        return 0;
    } else {
        printf("\nMining failed - no valid nonce found\n");
        return 1;
    }
}
