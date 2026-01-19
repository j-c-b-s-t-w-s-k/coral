// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <poker/poker.h>
#include <hash.h>
#include <random.h>
#include <util/time.h>

#include <algorithm>
#include <sstream>

namespace poker {

// Forward declarations
static std::string CardsToString(const std::vector<Card>& cards);

// Player implementation

Player::Player(const PlayerId& id, const CPubKey& pubKey, const std::string& name)
    : m_id(id), m_pubKey(pubKey), m_name(name) {}

bool Player::Bet(CAmount amount) {
    if (amount > m_stack) {
        return false;
    }
    m_stack -= amount;
    m_currentBet += amount;
    return true;
}

void Player::Fold() {
    m_state = PlayerState::FOLDED;
    m_holeCards.clear();
}

void Player::Reset() {
    m_currentBet = 0;
    m_holeCards.clear();
    if (m_state != PlayerState::SITTING_OUT && m_stack > 0) {
        m_state = PlayerState::ACTIVE;
    }
}

// BettingRound implementation

void BettingRound::AddAction(const BettingAction& action) {
    m_actions.push_back(action);

    if (action.action == Action::BET || action.action == Action::RAISE) {
        m_currentBet = action.amount;
    }
}

// PokerGame implementation

PokerGame::PokerGame(const GameId& gameId, const GameConfig& config)
    : m_gameId(gameId), m_config(config) {}

bool PokerGame::AddPlayer(const Player& player) {
    if (m_players.size() >= m_config.maxPlayers) {
        return false;
    }

    // Check if player already exists
    for (const auto& p : m_players) {
        if (p.GetId() == player.GetId()) {
            return false;
        }
    }

    m_players.push_back(player);
    m_players.back().SetPosition(m_players.size() - 1);

    Log("Player " + player.GetName() + " joined the game");
    return true;
}

bool PokerGame::RemovePlayer(const PlayerId& playerId) {
    for (auto it = m_players.begin(); it != m_players.end(); ++it) {
        if (it->GetId() == playerId) {
            Log("Player " + it->GetName() + " left the game");
            m_players.erase(it);
            return true;
        }
    }
    return false;
}

bool PokerGame::StartGame() {
    if (m_players.size() < 2) {
        Log("Not enough players to start");
        return false;
    }

    // Initialize mental poker protocol
    m_mentalPoker.Initialize(m_players.size(), 0); // Position 0 for now

    m_phase = GamePhase::ESCROW;
    Log("Game started with " + std::to_string(m_players.size()) + " players");

    return true;
}

bool PokerGame::StartNewHand() {
    if (m_phase == GamePhase::WAITING) {
        return false;
    }

    // Reset for new hand
    m_deck.Reset();
    m_communityCards.clear();
    m_bettingRounds.clear();
    m_pots.clear();
    m_totalPot = 0;
    m_handHistory.clear();

    // Reset players
    for (auto& player : m_players) {
        player.Reset();
        player.RecordHandPlayed();
    }

    // Rotate dealer
    RotateDealer();

    // Post blinds
    PostBlinds();

    m_phase = GamePhase::PREFLOP;
    Log("New hand started, dealer is " + m_players[m_dealerIndex].GetName());

    return true;
}

bool PokerGame::DealCards() {
    if (m_config.variant == GameVariant::TEXAS_HOLDEM) {
        // Deal 2 hole cards to each player
        for (auto& player : m_players) {
            if (player.GetState() == PlayerState::ACTIVE) {
                std::vector<Card> holeCards;
                for (int i = 0; i < HOLDEM_HOLE_CARDS; ++i) {
                    auto card = m_deck.Deal();
                    if (card) {
                        holeCards.push_back(*card);
                    }
                }
                player.SetHoleCards(holeCards);
            }
        }
    } else if (m_config.variant == GameVariant::FIVE_CARD_DRAW) {
        // Deal 5 cards to each player
        for (auto& player : m_players) {
            if (player.GetState() == PlayerState::ACTIVE) {
                std::vector<Card> hand;
                for (int i = 0; i < DRAW_HAND_SIZE; ++i) {
                    auto card = m_deck.Deal();
                    if (card) {
                        hand.push_back(*card);
                    }
                }
                player.SetHoleCards(hand);
            }
        }
    }

    Log("Cards dealt");
    return true;
}

bool PokerGame::EndHand() {
    HandResult result = DetermineWinners();
    AwardPots(result);

    Log("Hand complete: " + result.description);

    m_phase = GamePhase::COMPLETE;
    return true;
}

bool PokerGame::ProcessAction(const PlayerId& playerId, Action action, CAmount amount) {
    if (!IsValidAction(playerId, action, amount)) {
        return false;
    }

    Player* player = GetPlayer(playerId);
    if (!player) {
        return false;
    }

    BettingAction betAction;
    betAction.playerId = playerId;
    betAction.action = action;
    betAction.amount = amount;
    betAction.timestamp = GetTime();

    switch (action) {
        case Action::FOLD:
            player->Fold();
            Log(player->GetName() + " folds");
            break;

        case Action::CHECK:
            Log(player->GetName() + " checks");
            break;

        case Action::CALL: {
            CAmount toCall = GetCurrentBet() - player->GetCurrentBet();
            if (toCall > player->GetStack()) {
                toCall = player->GetStack(); // All-in
            }
            player->Bet(toCall);
            betAction.amount = toCall;
            Log(player->GetName() + " calls " + std::to_string(toCall));
            break;
        }

        case Action::BET:
            player->Bet(amount);
            Log(player->GetName() + " bets " + std::to_string(amount));
            break;

        case Action::RAISE: {
            CAmount toCall = GetCurrentBet() - player->GetCurrentBet();
            CAmount totalBet = toCall + amount;
            player->Bet(totalBet);
            betAction.amount = totalBet;
            Log(player->GetName() + " raises to " + std::to_string(player->GetCurrentBet()));
            break;
        }

        case Action::ALL_IN:
            betAction.amount = player->GetStack();
            player->Bet(player->GetStack());
            player->SetState(PlayerState::ALL_IN);
            Log(player->GetName() + " goes all-in for " + std::to_string(betAction.amount));
            break;
    }

    m_handHistory.push_back(betAction);

    if (!m_bettingRounds.empty()) {
        m_bettingRounds.back().AddAction(betAction);

        if (action == Action::BET || action == Action::RAISE) {
            m_bettingRounds.back().SetCurrentBet(player->GetCurrentBet());
        }
    }

    // Move to next player
    m_currentPlayerIndex = GetNextActivePlayer(m_currentPlayerIndex);

    // Check if round is complete
    if (IsRoundComplete()) {
        CollectBets();
        AdvancePhase();
    }

    return true;
}

bool PokerGame::IsValidAction(const PlayerId& playerId, Action action, CAmount amount) const {
    const Player* player = nullptr;
    for (const auto& p : m_players) {
        if (p.GetId() == playerId) {
            player = &p;
            break;
        }
    }

    if (!player || player->GetState() != PlayerState::ACTIVE) {
        return false;
    }

    // Check if it's this player's turn
    if (m_players[m_currentPlayerIndex].GetId() != playerId) {
        return false;
    }

    CAmount currentBet = GetCurrentBet();
    CAmount toCall = currentBet - player->GetCurrentBet();

    switch (action) {
        case Action::FOLD:
            return true;

        case Action::CHECK:
            return toCall == 0;

        case Action::CALL:
            return toCall > 0 && player->GetStack() >= toCall;

        case Action::BET:
            return currentBet == 0 && amount >= m_config.bigBlind &&
                   player->GetStack() >= amount;

        case Action::RAISE:
            return currentBet > 0 && amount >= GetMinRaise() &&
                   player->GetStack() >= (toCall + amount);

        case Action::ALL_IN:
            return player->GetStack() > 0;
    }

    return false;
}

std::vector<Action> PokerGame::GetValidActions(const PlayerId& playerId) const {
    std::vector<Action> validActions;

    for (int i = 0; i <= static_cast<int>(Action::ALL_IN); ++i) {
        Action action = static_cast<Action>(i);
        CAmount testAmount = (action == Action::BET) ? m_config.bigBlind :
                             (action == Action::RAISE) ? GetMinRaise() : 0;
        if (IsValidAction(playerId, action, testAmount)) {
            validActions.push_back(action);
        }
    }

    return validActions;
}

bool PokerGame::AdvancePhase() {
    if (CountPlayersInHand() <= 1) {
        m_phase = GamePhase::SHOWDOWN;
        return EndHand();
    }

    switch (m_phase) {
        case GamePhase::PREFLOP:
            m_phase = GamePhase::FLOP;
            // Deal flop
            m_deck.Burn();
            for (int i = 0; i < 3; ++i) {
                if (auto card = m_deck.Deal()) {
                    m_communityCards.push_back(*card);
                }
            }
            Log("Flop: " + CardsToString(m_communityCards));
            break;

        case GamePhase::FLOP:
            m_phase = GamePhase::TURN;
            // Deal turn
            m_deck.Burn();
            if (auto card = m_deck.Deal()) {
                m_communityCards.push_back(*card);
            }
            Log("Turn: " + m_communityCards.back().ToString());
            break;

        case GamePhase::TURN:
            m_phase = GamePhase::RIVER;
            // Deal river
            m_deck.Burn();
            if (auto card = m_deck.Deal()) {
                m_communityCards.push_back(*card);
            }
            Log("River: " + m_communityCards.back().ToString());
            break;

        case GamePhase::RIVER:
            m_phase = GamePhase::SHOWDOWN;
            return EndHand();

        default:
            break;
    }

    // Start new betting round
    m_bettingRounds.emplace_back();
    m_currentPlayerIndex = GetNextActivePlayer(m_dealerIndex);

    return true;
}

bool PokerGame::IsHandComplete() const {
    return m_phase == GamePhase::COMPLETE || m_phase == GamePhase::SHOWDOWN;
}

bool PokerGame::IsRoundComplete() const {
    if (CountPlayersInHand() <= 1) {
        return true;
    }

    CAmount currentBet = GetCurrentBet();

    // Check if all active players have acted and matched the bet
    for (const auto& player : m_players) {
        if (player.GetState() == PlayerState::ACTIVE) {
            if (player.GetCurrentBet() < currentBet) {
                return false;
            }
        }
    }

    // Check if at least one round of actions has occurred
    return !m_bettingRounds.empty() && !m_bettingRounds.back().GetActions().empty();
}

Player* PokerGame::GetPlayer(const PlayerId& playerId) {
    for (auto& player : m_players) {
        if (player.GetId() == playerId) {
            return &player;
        }
    }
    return nullptr;
}

const Player* PokerGame::GetCurrentPlayer() const {
    if (m_currentPlayerIndex < m_players.size()) {
        return &m_players[m_currentPlayerIndex];
    }
    return nullptr;
}

CAmount PokerGame::GetCurrentBet() const {
    if (m_bettingRounds.empty()) {
        return 0;
    }
    return m_bettingRounds.back().GetCurrentBet();
}

CAmount PokerGame::GetMinRaise() const {
    CAmount lastRaise = m_config.bigBlind;

    if (!m_bettingRounds.empty()) {
        const auto& actions = m_bettingRounds.back().GetActions();
        for (auto it = actions.rbegin(); it != actions.rend(); ++it) {
            if (it->action == Action::BET || it->action == Action::RAISE) {
                lastRaise = it->amount;
                break;
            }
        }
    }

    return lastRaise;
}

void PokerGame::Log(const std::string& message) {
    if (m_logCallback) {
        m_logCallback(message);
    }
}

void PokerGame::RotateDealer() {
    m_dealerIndex = (m_dealerIndex + 1) % m_players.size();
}

void PokerGame::PostBlinds() {
    size_t sbIndex = (m_dealerIndex + 1) % m_players.size();
    size_t bbIndex = (m_dealerIndex + 2) % m_players.size();

    // In heads-up, dealer is SB
    if (m_players.size() == 2) {
        sbIndex = m_dealerIndex;
        bbIndex = (m_dealerIndex + 1) % m_players.size();
    }

    m_players[sbIndex].Bet(std::min(m_config.smallBlind, m_players[sbIndex].GetStack()));
    m_players[bbIndex].Bet(std::min(m_config.bigBlind, m_players[bbIndex].GetStack()));

    Log(m_players[sbIndex].GetName() + " posts SB " + std::to_string(m_config.smallBlind));
    Log(m_players[bbIndex].GetName() + " posts BB " + std::to_string(m_config.bigBlind));

    // Start betting round
    m_bettingRounds.emplace_back();
    m_bettingRounds.back().SetCurrentBet(m_config.bigBlind);

    // Action starts after BB
    m_currentPlayerIndex = GetNextActivePlayer(bbIndex);
}

void PokerGame::CollectBets() {
    for (auto& player : m_players) {
        m_totalPot += player.GetCurrentBet();
        player.SetCurrentBet(0);
    }
    CalculatePots();
}

void PokerGame::CalculatePots() {
    m_pots.clear();

    // Simplified: single main pot
    Pot mainPot;
    mainPot.amount = m_totalPot;
    for (const auto& player : m_players) {
        if (player.GetState() != PlayerState::FOLDED) {
            mainPot.eligiblePlayers.push_back(player.GetId());
        }
    }
    m_pots.push_back(mainPot);
}

HandResult PokerGame::DetermineWinners() {
    HandResult result;

    // Find best hands
    std::vector<std::pair<const Player*, Hand>> playerHands;

    for (const auto& player : m_players) {
        if (player.GetState() != PlayerState::FOLDED) {
            Hand bestHand = Hand::FindBestHand(player.GetHoleCards(), m_communityCards);
            playerHands.emplace_back(&player, bestHand);
            result.showdownHands.emplace_back(player.GetId(), bestHand);
        }
    }

    if (playerHands.empty()) {
        return result;
    }

    // Sort by hand value (descending)
    std::sort(playerHands.begin(), playerHands.end(),
              [](const auto& a, const auto& b) {
                  return a.second > b.second;
              });

    // Find all winners (may be tied)
    uint32_t bestValue = playerHands[0].second.GetHandValue();
    CAmount potPerWinner = m_totalPot;
    size_t numWinners = 1;

    for (size_t i = 1; i < playerHands.size(); ++i) {
        if (playerHands[i].second.GetHandValue() == bestValue) {
            numWinners++;
        } else {
            break;
        }
    }

    potPerWinner = m_totalPot / numWinners;

    for (size_t i = 0; i < numWinners; ++i) {
        result.winnings.emplace_back(playerHands[i].first->GetId(), potPerWinner);
    }

    // Build description
    std::stringstream ss;
    ss << playerHands[0].first->GetName() << " wins with "
       << playerHands[0].second.GetDescription();
    if (numWinners > 1) {
        ss << " (split pot)";
    }
    result.description = ss.str();

    return result;
}

void PokerGame::AwardPots(const HandResult& result) {
    for (const auto& [playerId, amount] : result.winnings) {
        Player* player = GetPlayer(playerId);
        if (player) {
            player->SetStack(player->GetStack() + amount);
            player->RecordWin();
            Log(player->GetName() + " wins " + std::to_string(amount));
        }
    }
}

size_t PokerGame::GetNextActivePlayer(size_t from) const {
    size_t next = (from + 1) % m_players.size();
    size_t checked = 0;

    while (checked < m_players.size()) {
        if (m_players[next].GetState() == PlayerState::ACTIVE) {
            return next;
        }
        next = (next + 1) % m_players.size();
        checked++;
    }

    return from; // No active players found
}

size_t PokerGame::CountActivePlayers() const {
    size_t count = 0;
    for (const auto& player : m_players) {
        if (player.GetState() == PlayerState::ACTIVE ||
            player.GetState() == PlayerState::ALL_IN) {
            count++;
        }
    }
    return count;
}

size_t PokerGame::CountPlayersInHand() const {
    size_t count = 0;
    for (const auto& player : m_players) {
        if (player.GetState() != PlayerState::FOLDED &&
            player.GetState() != PlayerState::SITTING_OUT) {
            count++;
        }
    }
    return count;
}

// Helper function
static std::string CardsToString(const std::vector<Card>& cards) {
    std::string result;
    for (size_t i = 0; i < cards.size(); ++i) {
        if (i > 0) result += " ";
        result += cards[i].ToString();
    }
    return result;
}

// TexasHoldemGame implementation

bool TexasHoldemGame::DealHoleCards() {
    return DealCards();
}

bool TexasHoldemGame::DealFlop() {
    return AdvancePhase(); // Will deal flop in phase transition
}

bool TexasHoldemGame::DealTurn() {
    return AdvancePhase();
}

bool TexasHoldemGame::DealRiver() {
    return AdvancePhase();
}

Hand TexasHoldemGame::EvaluatePlayerHand(const PlayerId& playerId) const {
    const Player* player = nullptr;
    for (const auto& p : GetPlayers()) {
        if (p.GetId() == playerId) {
            player = &p;
            break;
        }
    }

    if (!player) {
        return Hand();
    }

    return Hand::FindBestHand(player->GetHoleCards(), GetCommunityCards());
}

// FiveCardDrawGame implementation

bool FiveCardDrawGame::DealInitialHand() {
    return DealCards();
}

bool FiveCardDrawGame::ProcessDiscard(const PlayerId& playerId,
                                       const std::vector<size_t>& discardIndices) {
    Player* player = GetPlayer(playerId);
    if (!player) {
        return false;
    }

    // Validate discard count
    if (discardIndices.size() > DRAW_MAX_DISCARD) {
        return false;
    }

    std::vector<Card> holeCards = player->GetHoleCards();

    // Remove cards in reverse order to maintain indices
    std::vector<size_t> sortedIndices = discardIndices;
    std::sort(sortedIndices.rbegin(), sortedIndices.rend());

    for (size_t index : sortedIndices) {
        if (index < holeCards.size()) {
            holeCards.erase(holeCards.begin() + index);
        }
    }

    player->SetHoleCards(holeCards);
    return true;
}

bool FiveCardDrawGame::DealReplacements(const PlayerId& playerId, size_t count) {
    Player* player = GetPlayer(playerId);
    if (!player) {
        return false;
    }

    std::vector<Card> holeCards = player->GetHoleCards();

    for (size_t i = 0; i < count; ++i) {
        auto card = m_deck.Deal();
        if (card) {
            holeCards.push_back(*card);
        }
    }

    player->SetHoleCards(holeCards);
    return true;
}

// PokerGameFactory implementation

std::unique_ptr<PokerGame> PokerGameFactory::CreateGame(const GameId& gameId,
                                                         const GameConfig& config) {
    switch (config.variant) {
        case GameVariant::TEXAS_HOLDEM:
            return std::make_unique<TexasHoldemGame>(gameId, config);
        case GameVariant::FIVE_CARD_DRAW:
            return std::make_unique<FiveCardDrawGame>(gameId, config);
    }
    return nullptr;
}

} // namespace poker
