// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CORAL_POKER_POKER_H
#define CORAL_POKER_POKER_H

#include <poker/pokertypes.h>
#include <poker/pokercards.h>
#include <poker/mentalpoker.h>
#include <poker/pokerescrow.h>
#include <pubkey.h>
#include <serialize.h>

#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace poker {

/**
 * Represents a player at the poker table.
 */
class Player {
private:
    PlayerId m_id;
    CPubKey m_pubKey;
    std::string m_name;

    CAmount m_stack{0};       // Current chip stack
    CAmount m_currentBet{0};  // Bet in current round
    PlayerState m_state{PlayerState::WAITING};

    // Hole cards (only visible to the player)
    std::vector<Card> m_holeCards;

    // Position at table (0 = button, 1 = SB, 2 = BB, etc.)
    uint8_t m_position{0};

    // Statistics
    uint32_t m_handsPlayed{0};
    uint32_t m_handsWon{0};

public:
    Player() = default;
    Player(const PlayerId& id, const CPubKey& pubKey, const std::string& name);

    // Getters
    const PlayerId& GetId() const { return m_id; }
    const CPubKey& GetPubKey() const { return m_pubKey; }
    const std::string& GetName() const { return m_name; }
    CAmount GetStack() const { return m_stack; }
    CAmount GetCurrentBet() const { return m_currentBet; }
    PlayerState GetState() const { return m_state; }
    const std::vector<Card>& GetHoleCards() const { return m_holeCards; }
    uint8_t GetPosition() const { return m_position; }

    // Setters
    void SetStack(CAmount stack) { m_stack = stack; }
    void SetCurrentBet(CAmount bet) { m_currentBet = bet; }
    void SetState(PlayerState state) { m_state = state; }
    void SetPosition(uint8_t pos) { m_position = pos; }
    void SetHoleCards(const std::vector<Card>& cards) { m_holeCards = cards; }

    // Actions
    bool Bet(CAmount amount);
    void Fold();
    void Reset(); // Reset for new hand

    // Statistics
    void RecordHandPlayed() { m_handsPlayed++; }
    void RecordWin() { m_handsWon++; }
    uint32_t GetHandsPlayed() const { return m_handsPlayed; }
    uint32_t GetHandsWon() const { return m_handsWon; }

    SERIALIZE_METHODS(Player, obj) {
        READWRITE(obj.m_id, obj.m_pubKey, obj.m_name, obj.m_stack,
                  obj.m_currentBet, obj.m_state, obj.m_holeCards,
                  obj.m_position, obj.m_handsPlayed, obj.m_handsWon);
    }
};

/**
 * Represents a betting action with its details.
 */
struct BettingAction {
    PlayerId playerId;
    Action action;
    CAmount amount{0};
    int64_t timestamp{0};
    std::vector<unsigned char> signature; // For verification

    SERIALIZE_METHODS(BettingAction, obj) {
        READWRITE(obj.playerId, obj.action, obj.amount, obj.timestamp, obj.signature);
    }
};

/**
 * Represents a single betting round.
 */
class BettingRound {
private:
    std::vector<BettingAction> m_actions;
    CAmount m_currentBet{0};  // Current bet to match
    size_t m_actorIndex{0};   // Current player to act
    bool m_complete{false};

public:
    BettingRound() = default;

    void AddAction(const BettingAction& action);
    bool IsComplete() const { return m_complete; }
    void SetComplete(bool complete) { m_complete = complete; }

    const std::vector<BettingAction>& GetActions() const { return m_actions; }
    CAmount GetCurrentBet() const { return m_currentBet; }
    void SetCurrentBet(CAmount bet) { m_currentBet = bet; }

    size_t GetActorIndex() const { return m_actorIndex; }
    void SetActorIndex(size_t index) { m_actorIndex = index; }

    SERIALIZE_METHODS(BettingRound, obj) {
        READWRITE(obj.m_actions, obj.m_currentBet, obj.m_actorIndex, obj.m_complete);
    }
};

/**
 * Pot structure for split pot calculations.
 */
struct Pot {
    CAmount amount{0};
    std::vector<PlayerId> eligiblePlayers;

    SERIALIZE_METHODS(Pot, obj) {
        READWRITE(obj.amount, obj.eligiblePlayers);
    }
};

/**
 * Result of a completed hand.
 */
struct HandResult {
    std::vector<std::pair<PlayerId, CAmount>> winnings;
    std::vector<std::pair<PlayerId, Hand>> showdownHands;
    std::string description;

    SERIALIZE_METHODS(HandResult, obj) {
        READWRITE(obj.winnings, obj.showdownHands, obj.description);
    }
};

/**
 * Main poker game class.
 * Manages the state machine for a poker game.
 */
class PokerGame {
protected:
    // Cards - protected so subclasses can access
    Deck m_deck;
    std::vector<Card> m_communityCards;

private:
    GameId m_gameId;
    GameConfig m_config;
    GamePhase m_phase{GamePhase::WAITING};

    // Players
    std::vector<Player> m_players;
    size_t m_dealerIndex{0};
    size_t m_currentPlayerIndex{0};

    // Betting
    std::vector<BettingRound> m_bettingRounds;
    std::vector<Pot> m_pots;
    CAmount m_totalPot{0};

    // Mental poker protocol
    MentalPokerProtocol m_mentalPoker;

    // Escrow
    std::shared_ptr<PokerEscrow> m_escrow;

    // Hand history
    std::vector<BettingAction> m_handHistory;

    // Callbacks
    std::function<void(const std::string&)> m_logCallback;

public:
    PokerGame() = default;
    explicit PokerGame(const GameId& gameId, const GameConfig& config);

    // Game setup
    bool AddPlayer(const Player& player);
    bool RemovePlayer(const PlayerId& playerId);
    bool StartGame();

    // Hand management
    bool StartNewHand();
    bool DealCards();
    bool EndHand();

    // Betting actions
    bool ProcessAction(const PlayerId& playerId, Action action, CAmount amount = 0);
    bool IsValidAction(const PlayerId& playerId, Action action, CAmount amount) const;
    std::vector<Action> GetValidActions(const PlayerId& playerId) const;

    // Phase transitions
    bool AdvancePhase();
    bool IsHandComplete() const;
    bool IsRoundComplete() const;

    // Game state queries
    const GameId& GetGameId() const { return m_gameId; }
    const GameConfig& GetConfig() const { return m_config; }
    GamePhase GetPhase() const { return m_phase; }
    const std::vector<Player>& GetPlayers() const { return m_players; }
    Player* GetPlayer(const PlayerId& playerId);
    const Player* GetCurrentPlayer() const;

    // Pot and betting queries
    CAmount GetTotalPot() const { return m_totalPot; }
    CAmount GetCurrentBet() const;
    CAmount GetMinRaise() const;

    // Card queries
    const std::vector<Card>& GetCommunityCards() const { return m_communityCards; }

    // Mental poker access
    MentalPokerProtocol& GetMentalPoker() { return m_mentalPoker; }
    const MentalPokerProtocol& GetMentalPoker() const { return m_mentalPoker; }

    // Escrow access
    void SetEscrow(std::shared_ptr<PokerEscrow> escrow) { m_escrow = escrow; }
    std::shared_ptr<PokerEscrow> GetEscrow() const { return m_escrow; }

    // Logging
    void SetLogCallback(std::function<void(const std::string&)> callback) {
        m_logCallback = callback;
    }

    // Serialization
    SERIALIZE_METHODS(PokerGame, obj) {
        READWRITE(obj.m_gameId, obj.m_config, obj.m_phase, obj.m_players,
                  obj.m_dealerIndex, obj.m_currentPlayerIndex, obj.m_deck,
                  obj.m_communityCards, obj.m_bettingRounds, obj.m_pots,
                  obj.m_totalPot, obj.m_mentalPoker, obj.m_handHistory);
    }

private:
    // Internal helpers
    void Log(const std::string& message);
    void RotateDealer();
    void PostBlinds();
    void CollectBets();
    void CalculatePots();
    HandResult DetermineWinners();
    void AwardPots(const HandResult& result);
    size_t GetNextActivePlayer(size_t from) const;
    size_t CountActivePlayers() const;
    size_t CountPlayersInHand() const;

    // Phase-specific logic
    bool ProcessPreflopPhase();
    bool ProcessFlopPhase();
    bool ProcessTurnPhase();
    bool ProcessRiverPhase();
    bool ProcessShowdownPhase();

    // Draw poker specific
    bool ProcessDrawPhase(const PlayerId& playerId,
                          const std::vector<size_t>& discardIndices);
};

/**
 * Texas Hold'em specific game logic.
 */
class TexasHoldemGame : public PokerGame {
public:
    using PokerGame::PokerGame;

    // Hold'em specific methods
    bool DealHoleCards();
    bool DealFlop();
    bool DealTurn();
    bool DealRiver();

    // Find best 5-card hand from 7 cards
    Hand EvaluatePlayerHand(const PlayerId& playerId) const;
};

/**
 * 5-Card Draw specific game logic.
 */
class FiveCardDrawGame : public PokerGame {
public:
    using PokerGame::PokerGame;

    // Draw specific methods
    bool DealInitialHand();
    bool ProcessDiscard(const PlayerId& playerId,
                        const std::vector<size_t>& discardIndices);
    bool DealReplacements(const PlayerId& playerId, size_t count);
};

/**
 * Factory for creating poker games.
 */
class PokerGameFactory {
public:
    static std::unique_ptr<PokerGame> CreateGame(const GameId& gameId,
                                                  const GameConfig& config);
};

} // namespace poker

#endif // CORAL_POKER_POKER_H
