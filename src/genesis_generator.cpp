// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// Genesis block generator for Coral cryptocurrency
// This utility generates the genesis block with proper nonce mining

#include <chainparams.h>
#include <consensus/merkle.h>
#include <hash.h>
#include <arith_uint256.h>
#include <primitives/block.h>
#include <script/script.h>
#include <util/strencodings.h>

#include <iostream>
#include <string>

static CBlock CreateCoralGenesisBlock(const char* pszTimestamp, const CScript& genesisOutputScript, uint32_t nTime, uint32_t nNonce, uint32_t nBits, int32_t nVersion, const CAmount& genesisReward)
{
    CMutableTransaction txNew;
    txNew.nVersion = 1;
    txNew.vin.resize(1);
    txNew.vout.resize(1);
    txNew.vin[0].scriptSig = CScript() << 486604799 << CScriptNum(4) << std::vector<unsigned char>((const unsigned char*)pszTimestamp, (const unsigned char*)pszTimestamp + strlen(pszTimestamp));
    txNew.vout[0].nValue = genesisReward;
    txNew.vout[0].scriptPubKey = genesisOutputScript;

    CBlock genesis;
    genesis.nTime    = nTime;
    genesis.nBits    = nBits;
    genesis.nNonce   = nNonce;
    genesis.nVersion = nVersion;
    genesis.vtx.push_back(MakeTransactionRef(std::move(txNew)));
    genesis.hashPrevBlock.SetNull();
    genesis.hashMerkleRoot = BlockMerkleRoot(genesis);
    return genesis;
}

void GenerateGenesisBlock() {
    const char* pszTimestamp = "Supreme Court keeps in place Trump funding freeze that threatens billions of dollars in foreign aid - Coral Genesis Sept 27, 2025";
    const CScript genesisOutputScript = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;

    uint32_t nTime = 1727432400; // Sept 27, 2025 timestamp
    uint32_t nBits = 0x1d00ffff; // Initial difficulty
    int32_t nVersion = 1;
    CAmount genesisReward = 100 * COIN; // 100 Coral reward

    std::cout << "Mining genesis block for Coral..." << std::endl;
    std::cout << "Timestamp: " << pszTimestamp << std::endl;
    std::cout << "Time: " << nTime << std::endl;
    std::cout << "Reward: " << genesisReward / COIN << " CORAL" << std::endl;

    for (uint32_t nonce = 0; nonce < UINT32_MAX; nonce++) {
        CBlock genesis = CreateCoralGenesisBlock(pszTimestamp, genesisOutputScript, nTime, nonce, nBits, nVersion, genesisReward);

        uint256 hash = genesis.GetHash();
        arith_uint256 bnTarget;
        bool fNegative, fOverflow;
        bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

        if (UintToArith256(hash) <= bnTarget) {
            std::cout << "Found genesis block!" << std::endl;
            std::cout << "Hash: " << hash.ToString() << std::endl;
            std::cout << "Merkle root: " << genesis.hashMerkleRoot.ToString() << std::endl;
            std::cout << "Nonce: " << nonce << std::endl;
            std::cout << "Time: " << nTime << std::endl;
            std::cout << "Bits: 0x" << std::hex << nBits << std::dec << std::endl;
            break;
        }

        if (nonce % 1000000 == 0) {
            std::cout << "Nonce: " << nonce << " Hash: " << hash.ToString() << std::endl;
        }
    }
}

int main() {
    GenerateGenesisBlock();
    return 0;
}