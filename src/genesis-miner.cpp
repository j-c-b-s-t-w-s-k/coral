// Genesis Block Miner for Coral - SHA256d version
// Finds a valid nonce for the genesis block using SHA256d (same as Bitcoin)

#include <chainparams.h>
#include <consensus/params.h>
#include <consensus/merkle.h>
#include <pow.h>
#include <primitives/block.h>
#include <primitives/transaction.h>
#include <arith_uint256.h>
#include <hash.h>
#include <script/script.h>
#include <util/strencodings.h>

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <thread>
#include <atomic>
#include <chrono>
#include <vector>
#include <mutex>

// Genesis block parameters - must match chainparams.cpp
static const uint32_t GENESIS_TIME = 1768694400;  // January 18, 2026 00:00:00 UTC
static const int32_t GENESIS_VERSION = 1;
static const char* GENESIS_MESSAGE = "18/Jan/2026 Trump tariffs take effect as thousands rally for Greenland";

// Mining state
static std::atomic<bool> found(false);
static std::atomic<uint64_t> totalHashes(0);
static std::mutex resultMutex;
static uint32_t winningNonce = 0;
static uint256 winningHash;
static uint256 winningMerkle;

void PrintProgress() {
    auto startTime = std::chrono::steady_clock::now();

    while (!found) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();
        uint64_t hashes = totalHashes.load();
        double hashrate = elapsed > 0 ? (double)hashes / elapsed : 0;

        printf("\r[%llds] Hashes: %llu | Rate: %.2f KH/s          ",
               (long long)elapsed, (unsigned long long)hashes, hashrate / 1000.0);
        fflush(stdout);
    }
}

void MineGenesis(uint32_t nBits, uint32_t startNonce, uint32_t step, int threadId) {
    // Create coinbase transaction with OP_RETURN (unspendable - ceremonial genesis)
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);

    // Coinbase scriptSig: nBits + extra nonce (4) + timestamp message
    const char* pszTimestamp = GENESIS_MESSAGE;
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4)
        << std::vector<unsigned char>((const unsigned char*)pszTimestamp,
                                       (const unsigned char*)pszTimestamp + strlen(pszTimestamp));

    // Unspendable OP_RETURN output - genesis is ceremonial, no coins created
    txNew.vout[0].nValue = 0;
    txNew.vout[0].scriptPubKey = CScript() << OP_RETURN
        << std::vector<unsigned char>((const unsigned char*)pszTimestamp,
                                       (const unsigned char*)pszTimestamp + strlen(pszTimestamp));

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

    printf("Thread %d: Started (nonce %u, step %u)\n", threadId, startNonce, step);

    uint32_t nonce = startNonce;
    uint64_t localHashes = 0;
    uint64_t reportInterval = 10000;

    while (!found) {
        genesis.nNonce = nonce;

        // Calculate SHA256d hash (same as block.GetHash())
        uint256 hash = genesis.GetHash();

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
                printf("║ Nonce:         %-50u ║\n", nonce);
                printf("║ SHA256d Hash:  %-50s ║\n", hash.ToString().substr(0, 50).c_str());
                printf("║                %-50s ║\n", hash.ToString().substr(50).c_str());
                printf("║ Merkle Root:   %-50s ║\n", genesis.hashMerkleRoot.ToString().substr(0, 50).c_str());
                printf("║                %-50s ║\n", genesis.hashMerkleRoot.ToString().substr(50).c_str());
                printf("║ Time:          %-50u ║\n", genesis.nTime);
                printf("║ Bits:          0x%-48x ║\n", genesis.nBits);
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
}

int main(int argc, char* argv[]) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════════════════╗\n");
    printf("║       CORAL GENESIS BLOCK MINER - SHA256d (like Bitcoin)         ║\n");
    printf("╚══════════════════════════════════════════════════════════════════╝\n\n");

    // Default parameters
    uint32_t nBits = 0x1e0fffff;  // Coral initial difficulty (very easy for genesis)
    int numThreads = std::thread::hardware_concurrency();

    if (argc > 1) {
        nBits = strtoul(argv[1], nullptr, 16);
    }
    if (argc > 2) {
        numThreads = atoi(argv[2]);
    }

    printf("Genesis Message: \"%s\"\n", GENESIS_MESSAGE);
    printf("Timestamp:       %u (Jan 17, 2025 00:00:00 UTC)\n", GENESIS_TIME);
    printf("Coinbase Output: OP_RETURN (unspendable)\n");
    printf("Target nBits:    0x%08x\n", nBits);
    printf("CPU Threads:     %d\n", numThreads);
    printf("Algorithm:       SHA256d (same as Bitcoin genesis)\n\n");

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

    if (winningNonce != 0 || found) {
        printf("\nMining complete! Update chainparams.cpp with the values above.\n");
        return 0;
    } else {
        printf("\nMining failed - no valid nonce found\n");
        return 1;
    }
}
