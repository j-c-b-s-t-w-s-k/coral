// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CORAL_POKER_POKERTYPES_H
#define CORAL_POKER_POKERTYPES_H

#include <uint256.h>
#include <serialize.h>

#include <cstdint>
#include <string>
#include <vector>

namespace poker {

// Card suits
enum class Suit : uint8_t {
    CLUBS = 0,
    DIAMONDS = 1,
    HEARTS = 2,
    SPADES = 3
};

// Card ranks (2-14, where 14 = Ace)
enum class Rank : uint8_t {
    TWO = 2,
    THREE = 3,
    FOUR = 4,
    FIVE = 5,
    SIX = 6,
    SEVEN = 7,
    EIGHT = 8,
    NINE = 9,
    TEN = 10,
    JACK = 11,
    QUEEN = 12,
    KING = 13,
    ACE = 14
};

// Hand rankings (higher value = better hand)
enum class HandRank : uint8_t {
    HIGH_CARD = 0,
    ONE_PAIR = 1,
    TWO_PAIR = 2,
    THREE_OF_A_KIND = 3,
    STRAIGHT = 4,
    FLUSH = 5,
    FULL_HOUSE = 6,
    FOUR_OF_A_KIND = 7,
    STRAIGHT_FLUSH = 8,
    ROYAL_FLUSH = 9
};

// Game variants
enum class GameVariant : uint8_t {
    TEXAS_HOLDEM = 0,
    FIVE_CARD_DRAW = 1
};

// Betting actions
enum class Action : uint8_t {
    FOLD = 0,
    CHECK = 1,
    CALL = 2,
    BET = 3,
    RAISE = 4,
    ALL_IN = 5
};

// Game phases
enum class GamePhase : uint8_t {
    // Common phases
    WAITING = 0,        // Waiting for players
    ESCROW = 1,         // Players locking funds
    SHUFFLE = 2,        // Mental poker shuffle

    // Texas Hold'em phases
    PREFLOP = 10,
    FLOP = 11,
    TURN = 12,
    RIVER = 13,

    // 5-Card Draw phases
    INITIAL_DEAL = 20,
    FIRST_BET = 21,
    DRAW = 22,
    SECOND_BET = 23,

    // Ending phases
    SHOWDOWN = 90,
    SETTLEMENT = 91,
    COMPLETE = 92
};

// Player states within a hand
enum class PlayerState : uint8_t {
    WAITING = 0,
    ACTIVE = 1,
    FOLDED = 2,
    ALL_IN = 3,
    SITTING_OUT = 4
};

// Game configuration
struct GameConfig {
    GameVariant variant{GameVariant::TEXAS_HOLDEM};
    int64_t minBuyIn{0};         // Minimum buy-in in satoshis
    int64_t maxBuyIn{0};         // Maximum buy-in in satoshis
    int64_t smallBlind{0};       // Small blind in satoshis
    int64_t bigBlind{0};         // Big blind in satoshis
    uint8_t maxPlayers{9};       // Max players at table (2-9)
    uint32_t timeoutSeconds{60}; // Action timeout

    bool IsValid() const {
        return minBuyIn > 0 &&
               maxBuyIn >= minBuyIn &&
               smallBlind > 0 &&
               bigBlind >= smallBlind * 2 &&
               maxPlayers >= 2 && maxPlayers <= 9;
    }

    template<typename Stream>
    void Serialize(Stream& s) const {
        uint8_t v = static_cast<uint8_t>(variant);
        ::Serialize(s, v);
        ::Serialize(s, minBuyIn);
        ::Serialize(s, maxBuyIn);
        ::Serialize(s, smallBlind);
        ::Serialize(s, bigBlind);
        ::Serialize(s, maxPlayers);
        ::Serialize(s, timeoutSeconds);
    }

    template<typename Stream>
    void Unserialize(Stream& s) {
        uint8_t v;
        ::Unserialize(s, v);
        variant = static_cast<GameVariant>(v);
        ::Unserialize(s, minBuyIn);
        ::Unserialize(s, maxBuyIn);
        ::Unserialize(s, smallBlind);
        ::Unserialize(s, bigBlind);
        ::Unserialize(s, maxPlayers);
        ::Unserialize(s, timeoutSeconds);
    }
};

// Game identifier (hash of creation params)
using GameId = uint256;

// Player identifier (their public key hash)
using PlayerId = uint160;

// Constants
constexpr uint8_t CARDS_IN_DECK = 52;
constexpr uint8_t SUITS_COUNT = 4;
constexpr uint8_t RANKS_COUNT = 13;

// Texas Hold'em constants
constexpr uint8_t HOLDEM_HOLE_CARDS = 2;
constexpr uint8_t HOLDEM_COMMUNITY_CARDS = 5;
constexpr uint8_t HOLDEM_BEST_HAND = 5;

// 5-Card Draw constants
constexpr uint8_t DRAW_HAND_SIZE = 5;
constexpr uint8_t DRAW_MAX_DISCARD = 3; // Max cards to discard (5 if showing ace)

// Escrow constants
constexpr uint32_t ESCROW_TIMEOUT_BLOCKS = 144; // ~1 day timeout for disputes

// Helper functions
inline std::string SuitToString(Suit suit) {
    switch (suit) {
        case Suit::CLUBS: return "c";
        case Suit::DIAMONDS: return "d";
        case Suit::HEARTS: return "h";
        case Suit::SPADES: return "s";
    }
    return "?";
}

inline std::string RankToString(Rank rank) {
    switch (rank) {
        case Rank::TWO: return "2";
        case Rank::THREE: return "3";
        case Rank::FOUR: return "4";
        case Rank::FIVE: return "5";
        case Rank::SIX: return "6";
        case Rank::SEVEN: return "7";
        case Rank::EIGHT: return "8";
        case Rank::NINE: return "9";
        case Rank::TEN: return "T";
        case Rank::JACK: return "J";
        case Rank::QUEEN: return "Q";
        case Rank::KING: return "K";
        case Rank::ACE: return "A";
    }
    return "?";
}

inline std::string HandRankToString(HandRank rank) {
    switch (rank) {
        case HandRank::HIGH_CARD: return "High Card";
        case HandRank::ONE_PAIR: return "One Pair";
        case HandRank::TWO_PAIR: return "Two Pair";
        case HandRank::THREE_OF_A_KIND: return "Three of a Kind";
        case HandRank::STRAIGHT: return "Straight";
        case HandRank::FLUSH: return "Flush";
        case HandRank::FULL_HOUSE: return "Full House";
        case HandRank::FOUR_OF_A_KIND: return "Four of a Kind";
        case HandRank::STRAIGHT_FLUSH: return "Straight Flush";
        case HandRank::ROYAL_FLUSH: return "Royal Flush";
    }
    return "Unknown";
}

inline std::string ActionToString(Action action) {
    switch (action) {
        case Action::FOLD: return "Fold";
        case Action::CHECK: return "Check";
        case Action::CALL: return "Call";
        case Action::BET: return "Bet";
        case Action::RAISE: return "Raise";
        case Action::ALL_IN: return "All-In";
    }
    return "Unknown";
}

inline std::string GameVariantToString(GameVariant variant) {
    switch (variant) {
        case GameVariant::TEXAS_HOLDEM: return "Texas Hold'em";
        case GameVariant::FIVE_CARD_DRAW: return "5-Card Draw";
    }
    return "Unknown";
}

} // namespace poker

// Serialization support for poker enum classes (global namespace for ADL)
template<typename Stream>
void Serialize(Stream& s, poker::Action a) {
    Serialize(s, static_cast<uint8_t>(a));
}

template<typename Stream>
void Unserialize(Stream& s, poker::Action& a) {
    uint8_t v;
    Unserialize(s, v);
    a = static_cast<poker::Action>(v);
}

template<typename Stream>
void Serialize(Stream& s, poker::GamePhase p) {
    Serialize(s, static_cast<uint8_t>(p));
}

template<typename Stream>
void Unserialize(Stream& s, poker::GamePhase& p) {
    uint8_t v;
    Unserialize(s, v);
    p = static_cast<poker::GamePhase>(v);
}

template<typename Stream>
void Serialize(Stream& s, poker::GameVariant v) {
    Serialize(s, static_cast<uint8_t>(v));
}

template<typename Stream>
void Unserialize(Stream& s, poker::GameVariant& v) {
    uint8_t val;
    Unserialize(s, val);
    v = static_cast<poker::GameVariant>(val);
}

#endif // CORAL_POKER_POKERTYPES_H
