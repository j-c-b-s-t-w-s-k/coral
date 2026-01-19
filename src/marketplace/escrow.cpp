// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <marketplace/escrow.h>

#include <hash.h>
#include <script/interpreter.h>
#include <script/sign.h>
#include <streams.h>
#include <util/strencodings.h>

#include <algorithm>

namespace marketplace {

bool MarketplaceEscrow::Initialize(const CPubKey& buyerPubKey,
                                    const CPubKey& sellerPubKey,
                                    const CPubKey& arbiterPubKey,
                                    CAmount amount,
                                    int currentHeight)
{
    if (!buyerPubKey.IsValid() || !sellerPubKey.IsValid() || !arbiterPubKey.IsValid()) {
        return false;
    }

    if (amount <= 0) {
        return false;
    }

    m_buyerPubKey = buyerPubKey;
    m_sellerPubKey = sellerPubKey;
    m_arbiterPubKey = arbiterPubKey;
    m_amount = amount;
    m_creationHeight = currentHeight;

    // Create the 2-of-3 multisig script
    m_escrowScript = Create2of3MultisigScript();

    // Create P2WSH scriptPubKey
    uint256 scriptHash;
    CSHA256().Write(m_escrowScript.data(), m_escrowScript.size()).Finalize(scriptHash.begin());
    m_escrowScriptPubKey = CScript() << OP_0 << ToByteVector(scriptHash);

    m_state = EscrowState::CREATED;
    return true;
}

CTxDestination MarketplaceEscrow::GetEscrowAddress() const
{
    // Return the P2WSH address
    WitnessV0ScriptHash hash;
    CSHA256().Write(m_escrowScript.data(), m_escrowScript.size()).Finalize(hash.begin());
    return hash;
}

CScript MarketplaceEscrow::Create2of3MultisigScript() const
{
    // Sort public keys for deterministic script
    std::vector<CPubKey> pubkeys = {m_buyerPubKey, m_sellerPubKey, m_arbiterPubKey};
    std::sort(pubkeys.begin(), pubkeys.end(), [](const CPubKey& a, const CPubKey& b) {
        return a < b;
    });

    // Create 2-of-3 multisig: OP_2 <pubkey1> <pubkey2> <pubkey3> OP_3 OP_CHECKMULTISIG
    CScript script;
    script << OP_2;
    for (const auto& pk : pubkeys) {
        script << ToByteVector(pk);
    }
    script << OP_3 << OP_CHECKMULTISIG;

    return script;
}

bool MarketplaceEscrow::CreateReleaseTransaction(const CTxDestination& sellerDest, CAmount fee)
{
    if (m_fundingOutpoint.IsNull()) {
        return false;
    }

    m_settlementTx = CMutableTransaction();
    m_settlementTx.nVersion = 2;

    // Input: the escrow output
    CTxIn input(m_fundingOutpoint);
    m_settlementTx.vin.push_back(input);

    // Output: send to seller (minus fee)
    CAmount outputAmount = m_amount - fee;
    if (outputAmount <= 0) {
        return false;
    }

    CTxOut output(outputAmount, GetScriptForDestination(sellerDest));
    m_settlementTx.vout.push_back(output);

    m_signatures.clear();
    m_isRelease = true;
    m_state = EscrowState::RELEASING;

    return true;
}

bool MarketplaceEscrow::CreateRefundTransaction(const CTxDestination& buyerDest, CAmount fee)
{
    if (m_fundingOutpoint.IsNull()) {
        return false;
    }

    m_settlementTx = CMutableTransaction();
    m_settlementTx.nVersion = 2;

    // Input: the escrow output
    CTxIn input(m_fundingOutpoint);
    m_settlementTx.vin.push_back(input);

    // Output: refund to buyer (minus fee)
    CAmount outputAmount = m_amount - fee;
    if (outputAmount <= 0) {
        return false;
    }

    CTxOut output(outputAmount, GetScriptForDestination(buyerDest));
    m_settlementTx.vout.push_back(output);

    m_signatures.clear();
    m_isRelease = false;
    m_state = EscrowState::REFUNDING;

    return true;
}

uint256 MarketplaceEscrow::GetSignatureHash() const
{
    // Get the sighash for the first (only) input
    // Using SIGHASH_ALL for the complete transaction
    return SignatureHash(m_escrowScript, m_settlementTx, 0, SIGHASH_ALL,
                         m_amount, SigVersion::WITNESS_V0);
}

bool MarketplaceEscrow::SignTransaction(const CKey& privateKey, std::vector<unsigned char>& sigOut)
{
    CPubKey pubKey = privateKey.GetPubKey();

    // Verify this key is one of our participants
    if (pubKey != m_buyerPubKey && pubKey != m_sellerPubKey && pubKey != m_arbiterPubKey) {
        return false;
    }

    uint256 sighash = GetSignatureHash();

    // Create signature
    std::vector<unsigned char> vchSig;
    if (!privateKey.Sign(sighash, vchSig)) {
        return false;
    }

    // Append SIGHASH_ALL
    vchSig.push_back(static_cast<unsigned char>(SIGHASH_ALL));

    sigOut = vchSig;
    return true;
}

bool MarketplaceEscrow::AddSignature(const CPubKey& pubKey, const std::vector<unsigned char>& signature)
{
    // Verify this is one of our participants
    if (pubKey != m_buyerPubKey && pubKey != m_sellerPubKey && pubKey != m_arbiterPubKey) {
        return false;
    }

    // Store signature
    m_signatures[pubKey] = signature;

    return true;
}

CTransaction MarketplaceEscrow::GetSignedTransaction() const
{
    if (!IsFullySigned()) {
        return CTransaction(CMutableTransaction());
    }

    CMutableTransaction signedTx = m_settlementTx;

    // Build the witness for the multisig
    // Format: OP_0 <sig1> <sig2> <redeemScript>
    // OP_0 is required because of the CHECKMULTISIG off-by-one bug

    std::vector<std::vector<unsigned char>> witnessStack;
    witnessStack.push_back(std::vector<unsigned char>()); // OP_0 placeholder

    // Add signatures in the same order as pubkeys appear in the script
    std::vector<CPubKey> sortedPubkeys = {m_buyerPubKey, m_sellerPubKey, m_arbiterPubKey};
    std::sort(sortedPubkeys.begin(), sortedPubkeys.end(), [](const CPubKey& a, const CPubKey& b) {
        return a < b;
    });

    for (const auto& pk : sortedPubkeys) {
        auto it = m_signatures.find(pk);
        if (it != m_signatures.end()) {
            witnessStack.push_back(it->second);
        }
    }

    // Add the redeem script
    witnessStack.push_back(std::vector<unsigned char>(m_escrowScript.begin(), m_escrowScript.end()));

    signedTx.vin[0].scriptWitness.stack = witnessStack;

    return CTransaction(signedTx);
}

bool MarketplaceEscrow::CreateTimeoutTransaction(int currentHeight,
                                                   const CTxDestination& refundDest,
                                                   CAmount fee)
{
    if (!CanTriggerTimeout(currentHeight)) {
        return false;
    }

    // Timeout transaction is essentially a refund
    // In a more advanced implementation, we could use OP_CSV for trustless timeout
    return CreateRefundTransaction(refundDest, fee);
}

} // namespace marketplace
