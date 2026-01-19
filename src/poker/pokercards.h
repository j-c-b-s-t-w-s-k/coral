// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CORAL_POKER_POKERCARDS_H
#define CORAL_POKER_POKERCARDS_H

#include <poker/pokertypes.h>
#include <serialize.h>
#include <uint256.h>

#include <array>
#include <string>
#include <vector>
#include <optional>

namespace poker {

/**
 * Represents a single playing card.
 * Cards are encoded as a single byte: (rank - 2) * 4 + suit
 * This gives values 0-51 for a standard deck.
 */
class Card {
private:
    uint8_t m_encoded;

public:
    Card() : m_encoded(0) {}
    Card(Rank rank, Suit suit);
    explicit Card(uint8_t encoded) : m_encoded(encoded) {}

    Rank GetRank() const;
    Suit GetSuit() const;
    uint8_t GetEncoded() const { return m_encoded; }

    std::string ToString() const;

    bool operator==(const Card& other) const { return m_encoded == other.m_encoded; }
    bool operator!=(const Card& other) const { return m_encoded != other.m_encoded; }
    bool operator<(const Card& other) const { return m_encoded < other.m_encoded; }

    // Compare by rank only
    static bool CompareByRank(const Card& a, const Card& b) {
        return a.GetRank() < b.GetRank();
    }

    SERIALIZE_METHODS(Card, obj) {
        READWRITE(obj.m_encoded);
    }

    static bool IsValidEncoded(uint8_t encoded) { return encoded < CARDS_IN_DECK; }
};

/**
 * Represents a hand of cards with evaluation.
 */
class Hand {
private:
    std::vector<Card> m_cards;
    mutable std::optional<HandRank> m_cachedRank;
    mutable std::optional<uint32_t> m_cachedValue;

    void InvalidateCache();
    void EvaluateHand() const;

public:
    Hand() = default;
    explicit Hand(const std::vector<Card>& cards) : m_cards(cards) {}

    void AddCard(const Card& card);
    void RemoveCard(size_t index);
    void Clear();

    const std::vector<Card>& GetCards() const { return m_cards; }
    size_t Size() const { return m_cards.size(); }
    bool Empty() const { return m_cards.empty(); }

    // Hand evaluation (for 5-card hands)
    HandRank GetHandRank() const;
    uint32_t GetHandValue() const; // Full hand value for comparison

    // Find best 5-card hand from available cards (for Hold'em)
    static Hand FindBestHand(const std::vector<Card>& holeCards,
                             const std::vector<Card>& communityCards);

    std::string ToString() const;
    std::string GetDescription() const;

    // Compare hands (returns negative if this < other, 0 if equal, positive if this > other)
    int Compare(const Hand& other) const;

    bool operator<(const Hand& other) const { return Compare(other) < 0; }
    bool operator==(const Hand& other) const { return Compare(other) == 0; }
    bool operator>(const Hand& other) const { return Compare(other) > 0; }

    SERIALIZE_METHODS(Hand, obj) {
        READWRITE(obj.m_cards);
    }
};

/**
 * Represents a standard 52-card deck.
 */
class Deck {
private:
    std::array<Card, CARDS_IN_DECK> m_cards;
    size_t m_position; // Next card to deal

public:
    Deck();

    // Reset deck to ordered state
    void Reset();

    // Shuffle using provided random bytes (deterministic shuffle)
    void Shuffle(const uint256& seed);

    // Deal next card
    std::optional<Card> Deal();

    // Deal multiple cards
    std::vector<Card> DealCards(size_t count);

    // Get remaining cards
    size_t RemainingCards() const { return CARDS_IN_DECK - m_position; }

    // Burn a card (discard without returning)
    bool Burn();

    // Get card at specific position (for verification)
    std::optional<Card> GetCardAt(size_t position) const;

    // Serialize the deck state
    SERIALIZE_METHODS(Deck, obj) {
        READWRITE(obj.m_cards, obj.m_position);
    }
};

/**
 * Hand evaluation utilities.
 */
class HandEvaluator {
public:
    // Evaluate a 5-card hand and return its rank
    static HandRank Evaluate(const std::vector<Card>& cards);

    // Get full hand value for comparison (higher = better)
    // Format: HandRank << 20 | kicker values
    static uint32_t GetValue(const std::vector<Card>& cards);

    // Check for specific hand types
    static bool IsFlush(const std::vector<Card>& cards);
    static bool IsStraight(const std::vector<Card>& cards);
    static std::vector<uint8_t> GetRankCounts(const std::vector<Card>& cards);

    // Generate all 5-card combinations from n cards
    static std::vector<std::vector<Card>> GetAllCombinations(
        const std::vector<Card>& cards, size_t choose = 5);

private:
    static uint32_t EvaluateStraightFlush(const std::vector<Card>& cards, bool isFlush);
    static uint32_t EvaluateFourOfAKind(const std::vector<uint8_t>& counts, const std::vector<Card>& cards);
    static uint32_t EvaluateFullHouse(const std::vector<uint8_t>& counts, const std::vector<Card>& cards);
    static uint32_t EvaluateFlush(const std::vector<Card>& cards);
    static uint32_t EvaluateStraight(const std::vector<Card>& cards);
    static uint32_t EvaluateThreeOfAKind(const std::vector<uint8_t>& counts, const std::vector<Card>& cards);
    static uint32_t EvaluateTwoPair(const std::vector<uint8_t>& counts, const std::vector<Card>& cards);
    static uint32_t EvaluateOnePair(const std::vector<uint8_t>& counts, const std::vector<Card>& cards);
    static uint32_t EvaluateHighCard(const std::vector<Card>& cards);
};

} // namespace poker

#endif // CORAL_POKER_POKERCARDS_H
