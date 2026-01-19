// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CORAL_POKER_POKERESCROW_H
#define CORAL_POKER_POKERESCROW_H

#include <poker/pokertypes.h>
#include <primitives/transaction.h>
#include <script/script.h>
#include <script/standard.h>
#include <key.h>
#include <pubkey.h>
#include <uint256.h>

#include <memory>
#include <vector>

namespace poker {

/**
 * Represents a player's stake in a poker game escrow.
 */
struct PlayerStake {
    CPubKey pubKey;         // Player's public key
    CAmount amount;         // Amount staked
    COutPoint fundingUtxo;  // UTXO used for funding
    std::vector<unsigned char> signature; // Signature on escrow tx

    SERIALIZE_METHODS(PlayerStake, obj) {
        READWRITE(obj.pubKey, obj.amount, obj.fundingUtxo, obj.signature);
    }
};

/**
 * Escrow state for a poker game.
 */
enum class EscrowState : uint8_t {
    CREATED = 0,      // Escrow created, waiting for funds
    FUNDING = 1,      // Players funding the escrow
    FUNDED = 2,       // All players have funded
    ACTIVE = 3,       // Game in progress
    SETTLING = 4,     // Settlement in progress
    SETTLED = 5,      // Settlement complete
    DISPUTED = 6,     // Dispute raised
    TIMEOUT = 7,      // Timeout triggered
    CANCELLED = 8     // Game cancelled
};

/**
 * Settlement outcome for a poker game.
 */
struct SettlementOutcome {
    std::vector<std::pair<CPubKey, CAmount>> payouts; // Who gets what
    uint256 gameHash;    // Hash of final game state
    int64_t timestamp;   // Settlement timestamp

    CAmount TotalPayout() const {
        CAmount total = 0;
        for (const auto& [key, amount] : payouts) {
            total += amount;
        }
        return total;
    }

    SERIALIZE_METHODS(SettlementOutcome, obj) {
        READWRITE(obj.payouts, obj.gameHash, obj.timestamp);
    }
};

/**
 * Poker game escrow manager.
 *
 * Handles the financial aspect of poker games:
 * 1. Creating N-of-N multisig escrow address
 * 2. Collecting funding transactions from players
 * 3. Creating and signing settlement transactions
 * 4. Handling timeouts and disputes
 */
class PokerEscrow {
private:
    GameId m_gameId;
    EscrowState m_state{EscrowState::CREATED};

    std::vector<PlayerStake> m_players;
    CAmount m_totalPot{0};

    // Escrow script and address
    CScript m_escrowScript;
    CScript m_escrowScriptPubKey;

    // Transactions
    CMutableTransaction m_fundingTx;
    CMutableTransaction m_settlementTx;

    // Timeout parameters
    int m_timeoutBlocks{ESCROW_TIMEOUT_BLOCKS};
    int m_creationHeight{0};

    // Signatures collected for settlement
    std::map<CPubKey, std::vector<unsigned char>> m_settlementSigs;

public:
    PokerEscrow() = default;
    explicit PokerEscrow(const GameId& gameId) : m_gameId(gameId) {}

    // Initialize escrow with player public keys
    bool Initialize(const std::vector<CPubKey>& playerPubKeys,
                    CAmount buyInAmount,
                    int currentHeight);

    // Get escrow address for funding
    CTxDestination GetEscrowAddress() const;

    // Add player funding
    bool AddPlayerFunding(const CPubKey& player,
                          const COutPoint& utxo,
                          CAmount amount);

    // Check if fully funded
    bool IsFullyFunded() const;

    // Create the funding transaction
    bool CreateFundingTransaction();

    // Sign funding transaction (called by each player)
    bool SignFundingTransaction(const CKey& privateKey,
                                std::vector<unsigned char>& signatureOut);

    // Add signature to funding transaction
    bool AddFundingSignature(const CPubKey& player,
                             const std::vector<unsigned char>& signature);

    // Create settlement transaction based on outcome
    bool CreateSettlementTransaction(const SettlementOutcome& outcome);

    // Sign settlement transaction
    bool SignSettlementTransaction(const CKey& privateKey,
                                   std::vector<unsigned char>& signatureOut);

    // Add settlement signature
    bool AddSettlementSignature(const CPubKey& player,
                                const std::vector<unsigned char>& signature);

    // Check if settlement transaction is fully signed
    bool IsSettlementFullySigned() const;

    // Get the final signed settlement transaction
    CTransaction GetSignedSettlementTransaction() const;

    // Create timeout transaction (after dispute period)
    bool CreateTimeoutTransaction(int currentHeight,
                                  const CTxDestination& refundDest);

    // Check if timeout can be triggered
    bool CanTriggerTimeout(int currentHeight) const;

    // Getters
    const GameId& GetGameId() const { return m_gameId; }
    EscrowState GetState() const { return m_state; }
    CAmount GetTotalPot() const { return m_totalPot; }
    const std::vector<PlayerStake>& GetPlayers() const { return m_players; }
    size_t GetNumPlayers() const { return m_players.size(); }

    // State transitions
    void SetState(EscrowState state) { m_state = state; }

    SERIALIZE_METHODS(PokerEscrow, obj) {
        READWRITE(obj.m_gameId, obj.m_state, obj.m_players, obj.m_totalPot,
                  obj.m_escrowScript, obj.m_escrowScriptPubKey,
                  obj.m_fundingTx, obj.m_settlementTx,
                  obj.m_timeoutBlocks, obj.m_creationHeight,
                  obj.m_settlementSigs);
    }

private:
    // Create N-of-N multisig script
    CScript CreateMultisigScript(const std::vector<CPubKey>& pubKeys) const;

    // Create timeout script with timelock
    CScript CreateTimeoutScript(const std::vector<CPubKey>& pubKeys,
                                int timeoutBlocks) const;
};

/**
 * Escrow transaction builder utilities.
 */
class EscrowTxBuilder {
public:
    // Create a simple N-of-N multisig output script
    static CScript CreateNofNMultisig(const std::vector<CPubKey>& pubKeys);

    // Create a timelocked script (for timeout fallback)
    static CScript CreateTimelockScript(const CScript& mainScript,
                                        int relativeBlocks);

    // Create combined script: multisig OR (timelock + reduced-sig)
    static CScript CreateEscrowScript(const std::vector<CPubKey>& pubKeys,
                                      int timeoutBlocks);

    // Sign a transaction input
    static bool SignInput(CMutableTransaction& tx,
                          size_t inputIndex,
                          const CScript& scriptPubKey,
                          const CKey& key,
                          std::vector<unsigned char>& sigOut);

    // Combine signatures for multisig
    static CScript CombineMultisigSignatures(
        const std::vector<std::vector<unsigned char>>& signatures,
        const CScript& redeemScript);

    // Verify a signature
    static bool VerifySignature(const CTransaction& tx,
                                size_t inputIndex,
                                const CScript& scriptPubKey,
                                const std::vector<unsigned char>& sig,
                                const CPubKey& pubKey);
};

/**
 * Manages multiple poker escrows for a wallet.
 */
class EscrowManager {
private:
    std::map<GameId, std::shared_ptr<PokerEscrow>> m_escrows;

public:
    EscrowManager() = default;

    // Create new escrow for a game
    std::shared_ptr<PokerEscrow> CreateEscrow(const GameId& gameId,
                                               const std::vector<CPubKey>& players,
                                               CAmount buyIn,
                                               int currentHeight);

    // Get escrow by game ID
    std::shared_ptr<PokerEscrow> GetEscrow(const GameId& gameId);

    // Remove completed escrow
    void RemoveEscrow(const GameId& gameId);

    // Get all active escrows
    std::vector<std::shared_ptr<PokerEscrow>> GetActiveEscrows() const;

    // Check for timeouts
    std::vector<GameId> CheckTimeouts(int currentHeight) const;
};

} // namespace poker

#endif // CORAL_POKER_POKERESCROW_H
