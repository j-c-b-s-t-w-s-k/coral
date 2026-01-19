// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CORAL_POKER_POKERNET_H
#define CORAL_POKER_POKERNET_H

#include <poker/pokertypes.h>
#include <poker/poker.h>
#include <poker/mentalpoker.h>
#include <poker/pokerescrow.h>
#include <net.h>
#include <protocol.h>
#include <serialize.h>
#include <sync.h>

#include <map>
#include <memory>
#include <set>
#include <vector>

namespace poker {

/**
 * Poker game announcement message.
 * Broadcast to advertise an available game.
 */
struct MsgAnnounce {
    GameId gameId;
    GameConfig config;
    CPubKey hostPubKey;
    std::string hostName;
    uint8_t currentPlayers{0};
    int64_t timestamp{0};
    std::vector<unsigned char> signature;

    SERIALIZE_METHODS(MsgAnnounce, obj) {
        READWRITE(obj.gameId, obj.config, obj.hostPubKey, obj.hostName,
                  obj.currentPlayers, obj.timestamp, obj.signature);
    }
};

/**
 * Join request message.
 */
struct MsgJoin {
    GameId gameId;
    CPubKey playerPubKey;
    std::string playerName;
    int64_t timestamp{0};
    std::vector<unsigned char> signature;

    SERIALIZE_METHODS(MsgJoin, obj) {
        READWRITE(obj.gameId, obj.playerPubKey, obj.playerName,
                  obj.timestamp, obj.signature);
    }
};

/**
 * Join acceptance message.
 */
struct MsgAccept {
    GameId gameId;
    CPubKey playerPubKey;
    uint8_t seatNumber{0};
    int64_t timestamp{0};
    std::vector<unsigned char> signature;

    SERIALIZE_METHODS(MsgAccept, obj) {
        READWRITE(obj.gameId, obj.playerPubKey, obj.seatNumber,
                  obj.timestamp, obj.signature);
    }
};

/**
 * Player ready message (with escrow proof).
 */
struct MsgReady {
    GameId gameId;
    CPubKey playerPubKey;
    uint256 escrowTxId;  // Proof of escrow
    int64_t timestamp{0};
    std::vector<unsigned char> signature;

    SERIALIZE_METHODS(MsgReady, obj) {
        READWRITE(obj.gameId, obj.playerPubKey, obj.escrowTxId,
                  obj.timestamp, obj.signature);
    }
};

/**
 * Game start message.
 */
struct MsgStart {
    GameId gameId;
    std::vector<CPubKey> playerOrder; // Seating order
    uint256 deckSeed;  // Seed for initial deck
    int64_t timestamp{0};
    std::vector<unsigned char> signature;

    SERIALIZE_METHODS(MsgStart, obj) {
        READWRITE(obj.gameId, obj.playerOrder, obj.deckSeed,
                  obj.timestamp, obj.signature);
    }
};

/**
 * Mental poker key exchange message.
 */
struct MsgKey {
    GameId gameId;
    CPubKey playerPubKey;
    uint256 keyCommitment;  // Hash commitment first
    std::vector<uint8_t> publicKey;  // Revealed after all commitments
    bool isReveal{false};  // false = commitment, true = reveal
    int64_t timestamp{0};
    std::vector<unsigned char> signature;

    SERIALIZE_METHODS(MsgKey, obj) {
        READWRITE(obj.gameId, obj.playerPubKey, obj.keyCommitment,
                  obj.publicKey, obj.isReveal, obj.timestamp, obj.signature);
    }
};

/**
 * Encrypted deck message.
 */
struct MsgDeck {
    GameId gameId;
    CPubKey playerPubKey;
    EncryptedDeck deck;
    int64_t timestamp{0};
    std::vector<unsigned char> signature;

    SERIALIZE_METHODS(MsgDeck, obj) {
        READWRITE(obj.gameId, obj.playerPubKey, obj.deck,
                  obj.timestamp, obj.signature);
    }
};

/**
 * Card reveal (partial decryption) message.
 */
struct MsgReveal {
    GameId gameId;
    CPubKey playerPubKey;
    size_t cardIndex;
    std::vector<uint8_t> partialDecrypt;
    int64_t timestamp{0};
    std::vector<unsigned char> signature;

    SERIALIZE_METHODS(MsgReveal, obj) {
        READWRITE(obj.gameId, obj.playerPubKey, obj.cardIndex,
                  obj.partialDecrypt, obj.timestamp, obj.signature);
    }
};

/**
 * Betting action message.
 */
struct MsgAction {
    GameId gameId;
    CPubKey playerPubKey;
    Action action;
    CAmount amount{0};
    int64_t timestamp{0};
    std::vector<unsigned char> signature;

    SERIALIZE_METHODS(MsgAction, obj) {
        READWRITE(obj.gameId, obj.playerPubKey);
        uint8_t actionInt;
        SER_WRITE(obj, actionInt = static_cast<uint8_t>(obj.action));
        READWRITE(actionInt);
        SER_READ(obj, obj.action = static_cast<Action>(actionInt));
        READWRITE(obj.amount, obj.timestamp, obj.signature);
    }
};

/**
 * Game state synchronization message.
 */
struct MsgState {
    GameId gameId;
    CPubKey senderPubKey;
    GamePhase phase;
    uint256 stateHash;  // Hash of current game state
    int64_t timestamp{0};
    std::vector<unsigned char> signature;

    SERIALIZE_METHODS(MsgState, obj) {
        READWRITE(obj.gameId, obj.senderPubKey);
        uint8_t phaseInt;
        SER_WRITE(obj, phaseInt = static_cast<uint8_t>(obj.phase));
        READWRITE(phaseInt);
        SER_READ(obj, obj.phase = static_cast<GamePhase>(phaseInt));
        READWRITE(obj.stateHash, obj.timestamp, obj.signature);
    }
};

/**
 * Settlement message.
 */
struct MsgSettle {
    GameId gameId;
    CPubKey playerPubKey;
    SettlementOutcome outcome;
    std::vector<unsigned char> settlementSig;  // Signature on settlement tx
    int64_t timestamp{0};
    std::vector<unsigned char> signature;

    SERIALIZE_METHODS(MsgSettle, obj) {
        READWRITE(obj.gameId, obj.playerPubKey, obj.outcome,
                  obj.settlementSig, obj.timestamp, obj.signature);
    }
};

/**
 * Leave game message.
 */
struct MsgLeave {
    GameId gameId;
    CPubKey playerPubKey;
    std::string reason;
    int64_t timestamp{0};
    std::vector<unsigned char> signature;

    SERIALIZE_METHODS(MsgLeave, obj) {
        READWRITE(obj.gameId, obj.playerPubKey, obj.reason,
                  obj.timestamp, obj.signature);
    }
};

/**
 * Poker network manager.
 * Handles all P2P communication for poker games.
 */
class PokerNetManager {
private:
    mutable RecursiveMutex cs_poker;

    // Available games (discovered through announcements)
    std::map<GameId, MsgAnnounce> m_availableGames GUARDED_BY(cs_poker);

    // Active games we're participating in
    std::map<GameId, std::shared_ptr<PokerGame>> m_activeGames GUARDED_BY(cs_poker);

    // Player connections per game
    std::map<GameId, std::set<NodeId>> m_gameNodes GUARDED_BY(cs_poker);

    // Our keys for signing messages
    CKey m_signingKey;
    CPubKey m_pubKey;

    // Callbacks for UI notification
    std::function<void(const MsgAnnounce&)> m_onGameAnnounce;
    std::function<void(const GameId&, const MsgJoin&)> m_onPlayerJoin;
    std::function<void(const GameId&, const MsgAction&)> m_onAction;
    std::function<void(const GameId&, GamePhase)> m_onPhaseChange;

public:
    PokerNetManager();

    // Initialize with signing key
    bool Initialize(const CKey& signingKey);

    // Game discovery
    void BroadcastGameAnnounce(const MsgAnnounce& announce);
    const std::map<GameId, MsgAnnounce>& GetAvailableGames() const;

    // Game management
    bool CreateGame(const GameConfig& config, GameId& gameIdOut);
    bool JoinGame(const GameId& gameId);
    bool LeaveGame(const GameId& gameId, const std::string& reason = "");

    // Get active game
    std::shared_ptr<PokerGame> GetGame(const GameId& gameId);

    // Message processing (called from net_processing.cpp)
    bool ProcessAnnounce(CNode* pfrom, const MsgAnnounce& msg);
    bool ProcessJoin(CNode* pfrom, const MsgJoin& msg);
    bool ProcessAccept(CNode* pfrom, const MsgAccept& msg);
    bool ProcessReady(CNode* pfrom, const MsgReady& msg);
    bool ProcessStart(CNode* pfrom, const MsgStart& msg);
    bool ProcessKey(CNode* pfrom, const MsgKey& msg);
    bool ProcessDeck(CNode* pfrom, const MsgDeck& msg);
    bool ProcessReveal(CNode* pfrom, const MsgReveal& msg);
    bool ProcessAction(CNode* pfrom, const MsgAction& msg);
    bool ProcessState(CNode* pfrom, const MsgState& msg);
    bool ProcessSettle(CNode* pfrom, const MsgSettle& msg);
    bool ProcessLeave(CNode* pfrom, const MsgLeave& msg);

    // Send messages
    void SendToGame(const GameId& gameId, const std::string& msgType, const CDataStream& data);
    void SendToPlayer(NodeId nodeId, const std::string& msgType, const CDataStream& data);

    // Action sending
    bool SendAction(const GameId& gameId, Action action, CAmount amount = 0);
    bool SendReady(const GameId& gameId, const uint256& escrowTxId);

    // Callbacks
    void SetOnGameAnnounce(std::function<void(const MsgAnnounce&)> callback) {
        m_onGameAnnounce = callback;
    }
    void SetOnPlayerJoin(std::function<void(const GameId&, const MsgJoin&)> callback) {
        m_onPlayerJoin = callback;
    }
    void SetOnAction(std::function<void(const GameId&, const MsgAction&)> callback) {
        m_onAction = callback;
    }
    void SetOnPhaseChange(std::function<void(const GameId&, GamePhase)> callback) {
        m_onPhaseChange = callback;
    }

    // Utility
    const CPubKey& GetPubKey() const { return m_pubKey; }

private:
    // Message signing/verification
    bool SignMessage(std::vector<unsigned char>& sig, const uint256& hash) const;
    bool VerifyMessage(const CPubKey& pubKey, const std::vector<unsigned char>& sig,
                       const uint256& hash) const;

    // Hash message for signing
    template<typename T>
    uint256 HashMessage(const T& msg) const;

    // Cleanup expired announcements
    void CleanupExpiredAnnouncements();
};

/**
 * Global poker network manager instance.
 */
extern std::unique_ptr<PokerNetManager> g_poker_net;

/**
 * Initialize the poker network subsystem.
 */
bool InitPokerNet();

/**
 * Shutdown the poker network subsystem.
 */
void ShutdownPokerNet();

} // namespace poker

#endif // CORAL_POKER_POKERNET_H
