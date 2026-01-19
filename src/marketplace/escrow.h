// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CORAL_MARKETPLACE_ESCROW_H
#define CORAL_MARKETPLACE_ESCROW_H

#include <marketplace/marketplacetypes.h>
#include <consensus/amount.h>
#include <uint256.h>
#include <script/standard.h>
#include <script/script.h>
#include <pubkey.h>
#include <key.h>
#include <primitives/transaction.h>

#include <map>
#include <vector>

namespace marketplace {

/**
 * MarketplaceEscrow implements a 2-of-3 multisig escrow for secure trades.
 *
 * Participants:
 * - Buyer: Funds the escrow
 * - Seller: Receives funds on successful delivery
 * - Arbiter: Resolves disputes (can release to seller OR refund to buyer)
 *
 * Any 2 of the 3 participants can sign to move funds:
 * - Buyer + Seller: Normal completion (buyer confirms, seller gets funds)
 * - Buyer + Arbiter: Dispute resolved for buyer (refund)
 * - Seller + Arbiter: Dispute resolved for seller (release)
 */
class MarketplaceEscrow
{
public:
    MarketplaceEscrow() = default;

    /**
     * Initialize escrow with participant public keys
     * @param buyerPubKey Buyer's public key
     * @param sellerPubKey Seller's public key
     * @param arbiterPubKey Arbiter's public key (can be platform default)
     * @param amount Amount to escrow (in satoshis)
     * @param currentHeight Current block height for timeout calculation
     * @return true if initialization succeeded
     */
    bool Initialize(const CPubKey& buyerPubKey,
                    const CPubKey& sellerPubKey,
                    const CPubKey& arbiterPubKey,
                    CAmount amount,
                    int currentHeight);

    /**
     * Get the P2WSH escrow address for funding
     */
    CTxDestination GetEscrowAddress() const;

    /**
     * Get the escrow script (2-of-3 multisig)
     */
    CScript GetEscrowScript() const { return m_escrowScript; }

    /**
     * Get the scriptPubKey for the escrow output
     */
    CScript GetEscrowScriptPubKey() const { return m_escrowScriptPubKey; }

    /**
     * Set the funding transaction output point
     * Call this after the funding tx is confirmed
     */
    void SetFundingOutpoint(const COutPoint& outpoint) { m_fundingOutpoint = outpoint; }

    /**
     * Create a release transaction (sends funds to seller)
     * Requires 2-of-3 signatures from (buyer+seller) or (arbiter+seller)
     */
    bool CreateReleaseTransaction(const CTxDestination& sellerDest, CAmount fee);

    /**
     * Create a refund transaction (sends funds back to buyer)
     * Requires 2-of-3 signatures from (buyer+seller) or (arbiter+buyer)
     */
    bool CreateRefundTransaction(const CTxDestination& buyerDest, CAmount fee);

    /**
     * Sign the settlement transaction with a private key
     * @param privateKey The signing key
     * @param sigOut Output signature
     * @return true if signing succeeded
     */
    bool SignTransaction(const CKey& privateKey, std::vector<unsigned char>& sigOut);

    /**
     * Add a signature from a participant
     */
    bool AddSignature(const CPubKey& pubKey, const std::vector<unsigned char>& signature);

    /**
     * Check if we have enough signatures (2 of 3)
     */
    bool IsFullySigned() const { return m_signatures.size() >= 2; }

    /**
     * Get the signed settlement transaction
     * Only valid after IsFullySigned() returns true
     */
    CTransaction GetSignedTransaction() const;

    /**
     * Check if timeout can be triggered
     */
    bool CanTriggerTimeout(int currentHeight) const {
        return currentHeight >= (m_creationHeight + m_timeoutBlocks);
    }

    /**
     * Create a timeout transaction (returns funds to buyer after timeout)
     * Does not require multiple signatures if timeout has elapsed
     */
    bool CreateTimeoutTransaction(int currentHeight,
                                   const CTxDestination& refundDest,
                                   CAmount fee);

    // State accessors
    EscrowState GetState() const { return m_state; }
    CAmount GetAmount() const { return m_amount; }
    int GetCreationHeight() const { return m_creationHeight; }
    int GetTimeoutBlocks() const { return m_timeoutBlocks; }

    void SetTimeoutBlocks(int blocks) { m_timeoutBlocks = blocks; }

private:
    /**
     * Create the 2-of-3 multisig redeem script
     * Scripts are ordered: buyer, seller, arbiter (sorted by pubkey)
     */
    CScript Create2of3MultisigScript() const;

    /**
     * Get the signature hash for signing
     */
    uint256 GetSignatureHash() const;

    // Participants
    CPubKey m_buyerPubKey;
    CPubKey m_sellerPubKey;
    CPubKey m_arbiterPubKey;

    // Financial
    CAmount m_amount{0};

    // Scripts
    CScript m_escrowScript;         // 2-of-3 multisig redeem script
    CScript m_escrowScriptPubKey;   // P2WSH scriptPubKey

    // Transactions
    COutPoint m_fundingOutpoint;
    CMutableTransaction m_settlementTx;

    // Signatures (pubkey -> signature)
    std::map<CPubKey, std::vector<unsigned char>> m_signatures;

    // Timing
    int m_creationHeight{0};
    int m_timeoutBlocks{DEFAULT_ESCROW_TIMEOUT_BLOCKS};

    // State
    EscrowState m_state{EscrowState::NONE};
    bool m_isRelease{true};  // true = release to seller, false = refund to buyer
};

} // namespace marketplace

#endif // CORAL_MARKETPLACE_ESCROW_H
