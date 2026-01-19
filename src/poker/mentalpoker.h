// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CORAL_POKER_MENTALPOKER_H
#define CORAL_POKER_MENTALPOKER_H

#include <poker/pokertypes.h>
#include <poker/pokercards.h>
#include <serialize.h>
#include <uint256.h>

#include <array>
#include <memory>
#include <string>
#include <vector>

namespace poker {

/**
 * SRA (Shamir-Rivest-Adleman) commutative encryption key pair.
 *
 * The key properties ensure:
 * - E_a(E_b(M)) = E_b(E_a(M)) (commutative)
 * - D_a(E_a(M)) = M (decryption)
 * - Knowledge of (e, d, n) required for decryption
 *
 * We use simplified modular exponentiation for the card game context.
 * For production, this should use proper big integer arithmetic.
 */
class SRAKeyPair {
private:
    std::vector<uint8_t> m_publicKey;   // e (encryption exponent)
    std::vector<uint8_t> m_privateKey;  // d (decryption exponent)
    std::vector<uint8_t> m_modulus;     // n (shared modulus)
    uint256 m_commitment;               // Hash commitment of public key

    bool m_initialized{false};

public:
    SRAKeyPair() = default;

    // Generate a new key pair with the given modulus
    bool Generate(const std::vector<uint8_t>& sharedModulus);

    // Generate with random modulus (for initial setup)
    bool GenerateWithModulus(size_t bitSize = 2048);

    // Set from existing values
    void Set(const std::vector<uint8_t>& pubKey,
             const std::vector<uint8_t>& privKey,
             const std::vector<uint8_t>& modulus);

    // Get key components
    const std::vector<uint8_t>& GetPublicKey() const { return m_publicKey; }
    const std::vector<uint8_t>& GetPrivateKey() const { return m_privateKey; }
    const std::vector<uint8_t>& GetModulus() const { return m_modulus; }

    // Get commitment (hash of public key for commitment scheme)
    uint256 GetCommitment() const;

    // Verify that revealed key matches commitment
    static bool VerifyCommitment(const std::vector<uint8_t>& pubKey,
                                 const uint256& commitment);

    bool IsInitialized() const { return m_initialized; }

    SERIALIZE_METHODS(SRAKeyPair, obj) {
        READWRITE(obj.m_publicKey, obj.m_privateKey, obj.m_modulus,
                  obj.m_commitment, obj.m_initialized);
    }
};

/**
 * Encrypted card representation.
 * A card encrypted with one or more SRA keys.
 */
class EncryptedCard {
private:
    std::vector<uint8_t> m_ciphertext;
    std::vector<uint256> m_encryptorCommitments; // Who has encrypted this

public:
    EncryptedCard() = default;
    explicit EncryptedCard(const std::vector<uint8_t>& ciphertext)
        : m_ciphertext(ciphertext) {}

    const std::vector<uint8_t>& GetCiphertext() const { return m_ciphertext; }
    void SetCiphertext(const std::vector<uint8_t>& ct) { m_ciphertext = ct; }

    void AddEncryptor(const uint256& commitment) {
        m_encryptorCommitments.push_back(commitment);
    }

    const std::vector<uint256>& GetEncryptors() const {
        return m_encryptorCommitments;
    }

    bool IsEncryptedBy(const uint256& commitment) const;

    SERIALIZE_METHODS(EncryptedCard, obj) {
        READWRITE(obj.m_ciphertext, obj.m_encryptorCommitments);
    }
};

/**
 * An encrypted deck of cards for mental poker.
 */
class EncryptedDeck {
private:
    std::vector<EncryptedCard> m_cards;
    std::vector<uint256> m_shufflerOrder; // Order of players who shuffled

public:
    EncryptedDeck() = default;

    // Initialize from a plain deck (card values encoded as big integers)
    void InitializeFromDeck(const Deck& deck, const std::vector<uint8_t>& modulus);

    // Get/set cards
    const std::vector<EncryptedCard>& GetCards() const { return m_cards; }
    void SetCards(const std::vector<EncryptedCard>& cards) { m_cards = cards; }

    size_t Size() const { return m_cards.size(); }

    EncryptedCard& operator[](size_t index) { return m_cards[index]; }
    const EncryptedCard& operator[](size_t index) const { return m_cards[index]; }

    // Track shuffle order
    void AddShuffler(const uint256& commitment) {
        m_shufflerOrder.push_back(commitment);
    }

    const std::vector<uint256>& GetShufflerOrder() const {
        return m_shufflerOrder;
    }

    SERIALIZE_METHODS(EncryptedDeck, obj) {
        READWRITE(obj.m_cards, obj.m_shufflerOrder);
    }
};

/**
 * Mental poker protocol handler.
 *
 * Implements the SRA-based mental poker protocol:
 * 1. All players generate key pairs and broadcast commitments
 * 2. After all commitments received, reveal public keys
 * 3. Dealer encrypts deck with own key, shuffles, broadcasts
 * 4. Each player encrypts with own key, shuffles, passes on
 * 5. To reveal a card: all other players provide partial decryptions
 */
class MentalPokerProtocol {
private:
    SRAKeyPair m_myKeys;
    std::vector<uint8_t> m_sharedModulus;

    // Other players' commitments and keys
    std::vector<uint256> m_playerCommitments;
    std::vector<std::vector<uint8_t>> m_playerPublicKeys;

    // Current deck state
    EncryptedDeck m_deck;

    // Revealed cards (card index -> plaintext card value)
    std::map<size_t, Card> m_revealedCards;

    // My position in the protocol
    size_t m_myPosition{0};
    size_t m_numPlayers{0};

    bool m_initialized{false};

public:
    MentalPokerProtocol() = default;

    // Initialize protocol with number of players and my position
    bool Initialize(size_t numPlayers, size_t myPosition);

    // Phase 1: Generate keys and get commitment
    uint256 GenerateKeysAndCommit();

    // Phase 2: Receive commitment from another player
    bool ReceiveCommitment(size_t playerIndex, const uint256& commitment);

    // Phase 3: All commitments received, reveal public key
    const std::vector<uint8_t>& RevealPublicKey() const { return m_myKeys.GetPublicKey(); }

    // Phase 4: Receive and verify public key
    bool ReceivePublicKey(size_t playerIndex, const std::vector<uint8_t>& pubKey);

    // Phase 5: Encrypt and shuffle deck (call when it's my turn)
    EncryptedDeck EncryptAndShuffle(const EncryptedDeck& inputDeck);

    // For initial dealer: create initial encrypted deck
    EncryptedDeck CreateInitialDeck();

    // Phase 6: Provide partial decryption for a card
    std::vector<uint8_t> ProvidePartialDecrypt(const EncryptedCard& card);

    // Phase 7: Receive partial decryption and try to reveal card
    bool ReceivePartialDecrypt(size_t cardIndex, size_t playerIndex,
                               const std::vector<uint8_t>& partialDecrypt);

    // Get revealed card (returns nullopt if not yet revealed)
    std::optional<Card> GetRevealedCard(size_t cardIndex) const;

    // Check if ready for next phase
    bool AllCommitmentsReceived() const;
    bool AllPublicKeysReceived() const;

    // Get current deck state
    const EncryptedDeck& GetDeck() const { return m_deck; }
    void SetDeck(const EncryptedDeck& deck) { m_deck = deck; }

    // Utility functions
    size_t GetNumPlayers() const { return m_numPlayers; }
    size_t GetMyPosition() const { return m_myPosition; }
    bool IsInitialized() const { return m_initialized; }

    SERIALIZE_METHODS(MentalPokerProtocol, obj) {
        READWRITE(obj.m_myKeys, obj.m_sharedModulus, obj.m_playerCommitments,
                  obj.m_playerPublicKeys, obj.m_deck, obj.m_revealedCards,
                  obj.m_myPosition, obj.m_numPlayers, obj.m_initialized);
    }

private:
    // Encryption/decryption using modular exponentiation
    std::vector<uint8_t> Encrypt(const std::vector<uint8_t>& data,
                                  const std::vector<uint8_t>& exponent) const;
    std::vector<uint8_t> Decrypt(const std::vector<uint8_t>& data) const;

    // Card encoding/decoding
    static std::vector<uint8_t> CardToBytes(const Card& card);
    static Card BytesToCard(const std::vector<uint8_t>& bytes);

    // Shuffle using Fisher-Yates with random seed
    void ShuffleDeck(EncryptedDeck& deck, const uint256& seed);
};

/**
 * Simplified big integer operations for mental poker.
 * In production, should use a proper bignum library.
 */
class BigInt {
public:
    // Modular exponentiation: base^exp mod mod
    static std::vector<uint8_t> ModPow(const std::vector<uint8_t>& base,
                                        const std::vector<uint8_t>& exp,
                                        const std::vector<uint8_t>& mod);

    // Modular multiplication: a * b mod mod
    static std::vector<uint8_t> ModMul(const std::vector<uint8_t>& a,
                                        const std::vector<uint8_t>& b,
                                        const std::vector<uint8_t>& mod);

    // Generate random bytes
    static std::vector<uint8_t> RandomBytes(size_t count);

    // Compare two big integers
    static int Compare(const std::vector<uint8_t>& a,
                       const std::vector<uint8_t>& b);

    // Convert to/from uint256
    static uint256 ToUint256(const std::vector<uint8_t>& bytes);
    static std::vector<uint8_t> FromUint256(const uint256& value);
};

} // namespace poker

#endif // CORAL_POKER_MENTALPOKER_H
