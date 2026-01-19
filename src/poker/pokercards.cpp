// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <poker/pokercards.h>
#include <crypto/sha256.h>
#include <random.h>

#include <algorithm>
#include <numeric>

namespace poker {

// Card implementation
Card::Card(Rank rank, Suit suit) {
    m_encoded = (static_cast<uint8_t>(rank) - 2) * 4 + static_cast<uint8_t>(suit);
}

Rank Card::GetRank() const {
    return static_cast<Rank>((m_encoded / 4) + 2);
}

Suit Card::GetSuit() const {
    return static_cast<Suit>(m_encoded % 4);
}

std::string Card::ToString() const {
    return RankToString(GetRank()) + SuitToString(GetSuit());
}

// Hand implementation
void Hand::InvalidateCache() {
    m_cachedRank.reset();
    m_cachedValue.reset();
}

void Hand::AddCard(const Card& card) {
    m_cards.push_back(card);
    InvalidateCache();
}

void Hand::RemoveCard(size_t index) {
    if (index < m_cards.size()) {
        m_cards.erase(m_cards.begin() + index);
        InvalidateCache();
    }
}

void Hand::Clear() {
    m_cards.clear();
    InvalidateCache();
}

void Hand::EvaluateHand() const {
    if (m_cards.size() != 5) {
        m_cachedRank = HandRank::HIGH_CARD;
        m_cachedValue = 0;
        return;
    }

    m_cachedValue = HandEvaluator::GetValue(m_cards);
    m_cachedRank = static_cast<HandRank>(*m_cachedValue >> 20);
}

HandRank Hand::GetHandRank() const {
    if (!m_cachedRank) {
        EvaluateHand();
    }
    return *m_cachedRank;
}

uint32_t Hand::GetHandValue() const {
    if (!m_cachedValue) {
        EvaluateHand();
    }
    return *m_cachedValue;
}

Hand Hand::FindBestHand(const std::vector<Card>& holeCards,
                        const std::vector<Card>& communityCards) {
    std::vector<Card> allCards;
    allCards.reserve(holeCards.size() + communityCards.size());
    allCards.insert(allCards.end(), holeCards.begin(), holeCards.end());
    allCards.insert(allCards.end(), communityCards.begin(), communityCards.end());

    auto combinations = HandEvaluator::GetAllCombinations(allCards, 5);

    Hand bestHand;
    uint32_t bestValue = 0;

    for (const auto& combo : combinations) {
        Hand hand(combo);
        uint32_t value = hand.GetHandValue();
        if (value > bestValue) {
            bestValue = value;
            bestHand = hand;
        }
    }

    return bestHand;
}

std::string Hand::ToString() const {
    std::string result;
    for (size_t i = 0; i < m_cards.size(); ++i) {
        if (i > 0) result += " ";
        result += m_cards[i].ToString();
    }
    return result;
}

std::string Hand::GetDescription() const {
    return HandRankToString(GetHandRank());
}

int Hand::Compare(const Hand& other) const {
    uint32_t thisValue = GetHandValue();
    uint32_t otherValue = other.GetHandValue();

    if (thisValue < otherValue) return -1;
    if (thisValue > otherValue) return 1;
    return 0;
}

// Deck implementation
Deck::Deck() : m_position(0) {
    Reset();
}

void Deck::Reset() {
    m_position = 0;
    for (uint8_t i = 0; i < CARDS_IN_DECK; ++i) {
        m_cards[i] = Card(i);
    }
}

void Deck::Shuffle(const uint256& seed) {
    // Fisher-Yates shuffle using deterministic random from seed
    std::array<uint8_t, 32> seedBytes;
    memcpy(seedBytes.data(), seed.begin(), 32);

    for (size_t i = CARDS_IN_DECK - 1; i > 0; --i) {
        // Generate deterministic random index
        CSHA256 hasher;
        hasher.Write(seedBytes.data(), 32);
        uint8_t indexBytes[4] = {
            static_cast<uint8_t>(i & 0xFF),
            static_cast<uint8_t>((i >> 8) & 0xFF),
            static_cast<uint8_t>((i >> 16) & 0xFF),
            static_cast<uint8_t>((i >> 24) & 0xFF)
        };
        hasher.Write(indexBytes, 4);

        unsigned char hash[CSHA256::OUTPUT_SIZE];
        hasher.Finalize(hash);

        // Use first 4 bytes as random value
        uint32_t randomValue = (hash[0] << 24) | (hash[1] << 16) | (hash[2] << 8) | hash[3];
        size_t j = randomValue % (i + 1);

        std::swap(m_cards[i], m_cards[j]);

        // Update seed for next iteration
        memcpy(seedBytes.data(), hash, 32);
    }

    m_position = 0;
}

std::optional<Card> Deck::Deal() {
    if (m_position >= CARDS_IN_DECK) {
        return std::nullopt;
    }
    return m_cards[m_position++];
}

std::vector<Card> Deck::DealCards(size_t count) {
    std::vector<Card> cards;
    cards.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        auto card = Deal();
        if (card) {
            cards.push_back(*card);
        } else {
            break;
        }
    }

    return cards;
}

bool Deck::Burn() {
    if (m_position >= CARDS_IN_DECK) {
        return false;
    }
    ++m_position;
    return true;
}

std::optional<Card> Deck::GetCardAt(size_t position) const {
    if (position >= CARDS_IN_DECK) {
        return std::nullopt;
    }
    return m_cards[position];
}

// HandEvaluator implementation
bool HandEvaluator::IsFlush(const std::vector<Card>& cards) {
    if (cards.size() < 5) return false;
    Suit firstSuit = cards[0].GetSuit();
    for (size_t i = 1; i < cards.size(); ++i) {
        if (cards[i].GetSuit() != firstSuit) return false;
    }
    return true;
}

bool HandEvaluator::IsStraight(const std::vector<Card>& cards) {
    if (cards.size() < 5) return false;

    std::vector<uint8_t> ranks;
    ranks.reserve(cards.size());
    for (const auto& card : cards) {
        ranks.push_back(static_cast<uint8_t>(card.GetRank()));
    }
    std::sort(ranks.begin(), ranks.end());

    // Check for A-2-3-4-5 (wheel)
    if (ranks[0] == 2 && ranks[1] == 3 && ranks[2] == 4 &&
        ranks[3] == 5 && ranks[4] == 14) {
        return true;
    }

    // Check for consecutive ranks
    for (size_t i = 1; i < ranks.size(); ++i) {
        if (ranks[i] != ranks[i - 1] + 1) return false;
    }
    return true;
}

std::vector<uint8_t> HandEvaluator::GetRankCounts(const std::vector<Card>& cards) {
    std::vector<uint8_t> counts(15, 0); // Index 2-14 for ranks
    for (const auto& card : cards) {
        counts[static_cast<uint8_t>(card.GetRank())]++;
    }
    return counts;
}

HandRank HandEvaluator::Evaluate(const std::vector<Card>& cards) {
    if (cards.size() != 5) return HandRank::HIGH_CARD;

    bool flush = IsFlush(cards);
    bool straight = IsStraight(cards);
    auto counts = GetRankCounts(cards);

    // Count pairs, trips, quads
    int pairs = 0, trips = 0, quads = 0;
    for (int i = 2; i <= 14; ++i) {
        if (counts[i] == 2) pairs++;
        else if (counts[i] == 3) trips++;
        else if (counts[i] == 4) quads++;
    }

    if (straight && flush) {
        // Check for royal flush (10-J-Q-K-A)
        std::vector<uint8_t> ranks;
        for (const auto& card : cards) {
            ranks.push_back(static_cast<uint8_t>(card.GetRank()));
        }
        std::sort(ranks.begin(), ranks.end());
        if (ranks[0] == 10 && ranks[4] == 14) {
            return HandRank::ROYAL_FLUSH;
        }
        return HandRank::STRAIGHT_FLUSH;
    }
    if (quads) return HandRank::FOUR_OF_A_KIND;
    if (trips && pairs) return HandRank::FULL_HOUSE;
    if (flush) return HandRank::FLUSH;
    if (straight) return HandRank::STRAIGHT;
    if (trips) return HandRank::THREE_OF_A_KIND;
    if (pairs == 2) return HandRank::TWO_PAIR;
    if (pairs == 1) return HandRank::ONE_PAIR;
    return HandRank::HIGH_CARD;
}

uint32_t HandEvaluator::GetValue(const std::vector<Card>& cards) {
    if (cards.size() != 5) return 0;

    bool flush = IsFlush(cards);
    bool straight = IsStraight(cards);
    auto counts = GetRankCounts(cards);

    // Count pairs, trips, quads
    int pairs = 0, trips = 0, quads = 0;
    for (int i = 2; i <= 14; ++i) {
        if (counts[i] == 2) pairs++;
        else if (counts[i] == 3) trips++;
        else if (counts[i] == 4) quads++;
    }

    if (straight && flush) {
        return EvaluateStraightFlush(cards, flush);
    }
    if (quads) return EvaluateFourOfAKind(counts, cards);
    if (trips && pairs) return EvaluateFullHouse(counts, cards);
    if (flush) return EvaluateFlush(cards);
    if (straight) return EvaluateStraight(cards);
    if (trips) return EvaluateThreeOfAKind(counts, cards);
    if (pairs == 2) return EvaluateTwoPair(counts, cards);
    if (pairs == 1) return EvaluateOnePair(counts, cards);
    return EvaluateHighCard(cards);
}

uint32_t HandEvaluator::EvaluateStraightFlush(const std::vector<Card>& cards, bool /*isFlush*/) {
    std::vector<uint8_t> ranks;
    for (const auto& card : cards) {
        ranks.push_back(static_cast<uint8_t>(card.GetRank()));
    }
    std::sort(ranks.begin(), ranks.end());

    // Royal flush
    if (ranks[0] == 10 && ranks[4] == 14) {
        return (static_cast<uint32_t>(HandRank::ROYAL_FLUSH) << 20) | (14 << 16);
    }

    // Wheel straight flush (A-2-3-4-5)
    uint8_t highCard = ranks[4];
    if (ranks[0] == 2 && ranks[4] == 14) {
        highCard = 5; // 5-high straight
    }

    return (static_cast<uint32_t>(HandRank::STRAIGHT_FLUSH) << 20) | (highCard << 16);
}

uint32_t HandEvaluator::EvaluateFourOfAKind(const std::vector<uint8_t>& counts, const std::vector<Card>& cards) {
    uint8_t quadRank = 0, kickerRank = 0;

    for (int i = 14; i >= 2; --i) {
        if (counts[i] == 4) quadRank = i;
        else if (counts[i] == 1) kickerRank = i;
    }

    return (static_cast<uint32_t>(HandRank::FOUR_OF_A_KIND) << 20) |
           (quadRank << 16) | (kickerRank << 12);
}

uint32_t HandEvaluator::EvaluateFullHouse(const std::vector<uint8_t>& counts, const std::vector<Card>& /*cards*/) {
    uint8_t tripRank = 0, pairRank = 0;

    for (int i = 14; i >= 2; --i) {
        if (counts[i] == 3) tripRank = i;
        else if (counts[i] == 2) pairRank = i;
    }

    return (static_cast<uint32_t>(HandRank::FULL_HOUSE) << 20) |
           (tripRank << 16) | (pairRank << 12);
}

uint32_t HandEvaluator::EvaluateFlush(const std::vector<Card>& cards) {
    std::vector<uint8_t> ranks;
    for (const auto& card : cards) {
        ranks.push_back(static_cast<uint8_t>(card.GetRank()));
    }
    std::sort(ranks.rbegin(), ranks.rend()); // Descending

    return (static_cast<uint32_t>(HandRank::FLUSH) << 20) |
           (ranks[0] << 16) | (ranks[1] << 12) | (ranks[2] << 8) |
           (ranks[3] << 4) | ranks[4];
}

uint32_t HandEvaluator::EvaluateStraight(const std::vector<Card>& cards) {
    std::vector<uint8_t> ranks;
    for (const auto& card : cards) {
        ranks.push_back(static_cast<uint8_t>(card.GetRank()));
    }
    std::sort(ranks.begin(), ranks.end());

    uint8_t highCard = ranks[4];
    // Wheel straight (A-2-3-4-5)
    if (ranks[0] == 2 && ranks[4] == 14) {
        highCard = 5;
    }

    return (static_cast<uint32_t>(HandRank::STRAIGHT) << 20) | (highCard << 16);
}

uint32_t HandEvaluator::EvaluateThreeOfAKind(const std::vector<uint8_t>& counts, const std::vector<Card>& cards) {
    uint8_t tripRank = 0;
    std::vector<uint8_t> kickers;

    for (int i = 14; i >= 2; --i) {
        if (counts[i] == 3) tripRank = i;
        else if (counts[i] == 1) kickers.push_back(i);
    }

    std::sort(kickers.rbegin(), kickers.rend());

    return (static_cast<uint32_t>(HandRank::THREE_OF_A_KIND) << 20) |
           (tripRank << 16) | (kickers[0] << 12) | (kickers[1] << 8);
}

uint32_t HandEvaluator::EvaluateTwoPair(const std::vector<uint8_t>& counts, const std::vector<Card>& cards) {
    std::vector<uint8_t> pairRanks;
    uint8_t kickerRank = 0;

    for (int i = 14; i >= 2; --i) {
        if (counts[i] == 2) pairRanks.push_back(i);
        else if (counts[i] == 1) kickerRank = i;
    }

    std::sort(pairRanks.rbegin(), pairRanks.rend());

    return (static_cast<uint32_t>(HandRank::TWO_PAIR) << 20) |
           (pairRanks[0] << 16) | (pairRanks[1] << 12) | (kickerRank << 8);
}

uint32_t HandEvaluator::EvaluateOnePair(const std::vector<uint8_t>& counts, const std::vector<Card>& cards) {
    uint8_t pairRank = 0;
    std::vector<uint8_t> kickers;

    for (int i = 14; i >= 2; --i) {
        if (counts[i] == 2) pairRank = i;
        else if (counts[i] == 1) kickers.push_back(i);
    }

    std::sort(kickers.rbegin(), kickers.rend());

    return (static_cast<uint32_t>(HandRank::ONE_PAIR) << 20) |
           (pairRank << 16) | (kickers[0] << 12) | (kickers[1] << 8) | (kickers[2] << 4);
}

uint32_t HandEvaluator::EvaluateHighCard(const std::vector<Card>& cards) {
    std::vector<uint8_t> ranks;
    for (const auto& card : cards) {
        ranks.push_back(static_cast<uint8_t>(card.GetRank()));
    }
    std::sort(ranks.rbegin(), ranks.rend());

    return (static_cast<uint32_t>(HandRank::HIGH_CARD) << 20) |
           (ranks[0] << 16) | (ranks[1] << 12) | (ranks[2] << 8) |
           (ranks[3] << 4) | ranks[4];
}

std::vector<std::vector<Card>> HandEvaluator::GetAllCombinations(
    const std::vector<Card>& cards, size_t choose) {
    std::vector<std::vector<Card>> result;

    if (choose > cards.size()) return result;

    std::vector<bool> selector(cards.size(), false);
    std::fill(selector.end() - choose, selector.end(), true);

    do {
        std::vector<Card> combination;
        for (size_t i = 0; i < cards.size(); ++i) {
            if (selector[i]) {
                combination.push_back(cards[i]);
            }
        }
        result.push_back(combination);
    } while (std::next_permutation(selector.begin(), selector.end()));

    return result;
}

} // namespace poker
