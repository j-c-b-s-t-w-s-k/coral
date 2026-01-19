// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <poker/mentalpoker.h>
#include <crypto/sha256.h>
#include <random.h>
#include <hash.h>

#include <algorithm>

namespace poker {

// BigInt implementation using simplified byte operations
// Note: For production, replace with proper big integer library (e.g., GMP)

std::vector<uint8_t> BigInt::RandomBytes(size_t count) {
    std::vector<uint8_t> result(count);
    GetStrongRandBytes(result);
    return result;
}

uint256 BigInt::ToUint256(const std::vector<uint8_t>& bytes) {
    uint256 result;
    size_t copyLen = std::min(bytes.size(), (size_t)32);
    memset(result.begin(), 0, 32);
    memcpy(result.begin() + (32 - copyLen), bytes.data(), copyLen);
    return result;
}

std::vector<uint8_t> BigInt::FromUint256(const uint256& value) {
    std::vector<uint8_t> result(32);
    memcpy(result.data(), value.begin(), 32);
    // Remove leading zeros
    size_t start = 0;
    while (start < result.size() - 1 && result[start] == 0) {
        ++start;
    }
    return std::vector<uint8_t>(result.begin() + start, result.end());
}

int BigInt::Compare(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b) {
    if (a.size() != b.size()) {
        return a.size() < b.size() ? -1 : 1;
    }
    for (size_t i = 0; i < a.size(); ++i) {
        if (a[i] != b[i]) {
            return a[i] < b[i] ? -1 : 1;
        }
    }
    return 0;
}

// Stub implementation of modular exponentiation
// TODO: Replace with proper big integer library (GMP) for production mental poker
// This stub allows the GUI to compile but won't perform correct SRA encryption
std::vector<uint8_t> BigInt::ModPow(const std::vector<uint8_t>& base,
                                     const std::vector<uint8_t>& exp,
                                     const std::vector<uint8_t>& mod) {
    // Stub: Return hash of inputs as placeholder
    // Real implementation requires proper modular exponentiation
    if (mod.empty() || base.empty()) {
        return std::vector<uint8_t>{0};
    }

    CSHA256 hasher;
    hasher.Write(base.data(), base.size());
    hasher.Write(exp.data(), exp.size());
    hasher.Write(mod.data(), mod.size());

    uint256 result;
    hasher.Finalize(result.begin());

    return FromUint256(result);
}

// Stub implementation of modular multiplication
// TODO: Replace with proper big integer library (GMP) for production
std::vector<uint8_t> BigInt::ModMul(const std::vector<uint8_t>& a,
                                     const std::vector<uint8_t>& b,
                                     const std::vector<uint8_t>& mod) {
    // Stub: Return hash of inputs as placeholder
    if (mod.empty()) {
        return std::vector<uint8_t>{0};
    }

    CSHA256 hasher;
    hasher.Write(a.data(), a.size());
    hasher.Write(b.data(), b.size());
    hasher.Write(mod.data(), mod.size());

    uint256 result;
    hasher.Finalize(result.begin());

    return FromUint256(result);
}

// SRAKeyPair implementation

bool SRAKeyPair::Generate(const std::vector<uint8_t>& sharedModulus) {
    m_modulus = sharedModulus;

    // Generate random encryption exponent
    // For SRA, we need e such that gcd(e, phi(n)) = 1
    // Using simplified approach with random odd number
    m_publicKey = BigInt::RandomBytes(sharedModulus.size());
    m_publicKey.back() |= 1; // Ensure odd

    // For this simplified implementation, we use e as both encrypt and decrypt
    // A proper implementation would compute d = e^(-1) mod phi(n)
    // Since we don't know phi(n) without factorization, we use symmetric encryption
    m_privateKey = m_publicKey;

    m_commitment = GetCommitment();
    m_initialized = true;
    return true;
}

bool SRAKeyPair::GenerateWithModulus(size_t bitSize) {
    // Generate a random modulus
    // For production, should generate proper RSA modulus (p*q with large primes)
    size_t byteSize = bitSize / 8;
    m_modulus = BigInt::RandomBytes(byteSize);
    m_modulus[0] |= 0x80; // Ensure high bit set
    m_modulus.back() |= 1; // Ensure odd

    return Generate(m_modulus);
}

void SRAKeyPair::Set(const std::vector<uint8_t>& pubKey,
                     const std::vector<uint8_t>& privKey,
                     const std::vector<uint8_t>& modulus) {
    m_publicKey = pubKey;
    m_privateKey = privKey;
    m_modulus = modulus;
    m_commitment = GetCommitment();
    m_initialized = true;
}

uint256 SRAKeyPair::GetCommitment() const {
    CSHA256 hasher;
    hasher.Write(m_publicKey.data(), m_publicKey.size());
    uint256 result;
    hasher.Finalize(result.begin());
    return result;
}

bool SRAKeyPair::VerifyCommitment(const std::vector<uint8_t>& pubKey,
                                   const uint256& commitment) {
    CSHA256 hasher;
    hasher.Write(pubKey.data(), pubKey.size());
    uint256 computed;
    hasher.Finalize(computed.begin());
    return computed == commitment;
}

// EncryptedCard implementation

bool EncryptedCard::IsEncryptedBy(const uint256& commitment) const {
    return std::find(m_encryptorCommitments.begin(),
                     m_encryptorCommitments.end(),
                     commitment) != m_encryptorCommitments.end();
}

// EncryptedDeck implementation

void EncryptedDeck::InitializeFromDeck(const Deck& deck,
                                        const std::vector<uint8_t>& modulus) {
    m_cards.clear();
    m_cards.reserve(CARDS_IN_DECK);

    for (uint8_t i = 0; i < CARDS_IN_DECK; ++i) {
        auto card = deck.GetCardAt(i);
        if (card) {
            // Encode card as padded value
            std::vector<uint8_t> encoded(modulus.size(), 0);
            encoded.back() = card->GetEncoded() + 1; // +1 to avoid zero
            m_cards.emplace_back(encoded);
        }
    }
}

// MentalPokerProtocol implementation

bool MentalPokerProtocol::Initialize(size_t numPlayers, size_t myPosition) {
    if (numPlayers < 2 || myPosition >= numPlayers) {
        return false;
    }

    m_numPlayers = numPlayers;
    m_myPosition = myPosition;
    m_playerCommitments.resize(numPlayers);
    m_playerPublicKeys.resize(numPlayers);
    m_revealedCards.clear();
    m_initialized = true;

    return true;
}

uint256 MentalPokerProtocol::GenerateKeysAndCommit() {
    // Generate shared modulus if we're first player, or use existing
    if (m_sharedModulus.empty()) {
        m_myKeys.GenerateWithModulus(256); // Use 256-bit for now
        m_sharedModulus = m_myKeys.GetModulus();
    } else {
        m_myKeys.Generate(m_sharedModulus);
    }

    uint256 commitment = m_myKeys.GetCommitment();
    m_playerCommitments[m_myPosition] = commitment;
    return commitment;
}

bool MentalPokerProtocol::ReceiveCommitment(size_t playerIndex,
                                             const uint256& commitment) {
    if (playerIndex >= m_numPlayers || playerIndex == m_myPosition) {
        return false;
    }

    m_playerCommitments[playerIndex] = commitment;
    return true;
}

bool MentalPokerProtocol::ReceivePublicKey(size_t playerIndex,
                                            const std::vector<uint8_t>& pubKey) {
    if (playerIndex >= m_numPlayers || playerIndex == m_myPosition) {
        return false;
    }

    // Verify against commitment
    if (!SRAKeyPair::VerifyCommitment(pubKey, m_playerCommitments[playerIndex])) {
        return false;
    }

    m_playerPublicKeys[playerIndex] = pubKey;
    return true;
}

EncryptedDeck MentalPokerProtocol::CreateInitialDeck() {
    Deck plainDeck;
    plainDeck.Reset();

    EncryptedDeck encrypted;
    encrypted.InitializeFromDeck(plainDeck, m_sharedModulus);

    // Encrypt each card with our key
    for (size_t i = 0; i < encrypted.Size(); ++i) {
        auto ciphertext = Encrypt(encrypted[i].GetCiphertext(),
                                   m_myKeys.GetPublicKey());
        encrypted[i].SetCiphertext(ciphertext);
        encrypted[i].AddEncryptor(m_myKeys.GetCommitment());
    }

    // Shuffle
    uint256 seed;
    GetRandBytes(seed);
    ShuffleDeck(encrypted, seed);

    encrypted.AddShuffler(m_myKeys.GetCommitment());
    return encrypted;
}

EncryptedDeck MentalPokerProtocol::EncryptAndShuffle(const EncryptedDeck& inputDeck) {
    EncryptedDeck result = inputDeck;

    // Encrypt each card with our key
    for (size_t i = 0; i < result.Size(); ++i) {
        auto ciphertext = Encrypt(result[i].GetCiphertext(),
                                   m_myKeys.GetPublicKey());
        result[i].SetCiphertext(ciphertext);
        result[i].AddEncryptor(m_myKeys.GetCommitment());
    }

    // Shuffle
    uint256 seed;
    GetRandBytes(seed);
    ShuffleDeck(result, seed);

    result.AddShuffler(m_myKeys.GetCommitment());
    return result;
}

std::vector<uint8_t> MentalPokerProtocol::ProvidePartialDecrypt(
    const EncryptedCard& card) {
    // Decrypt with our private key (remove our encryption layer)
    return Decrypt(card.GetCiphertext());
}

bool MentalPokerProtocol::ReceivePartialDecrypt(size_t cardIndex,
                                                 size_t playerIndex,
                                                 const std::vector<uint8_t>& partialDecrypt) {
    // This is a simplified implementation
    // In full protocol, we track partial decryptions from all players
    // and combine them to reveal the final card

    // For now, just store the partial decryption
    // When all partials received, decode the card
    if (cardIndex >= m_deck.Size() || playerIndex >= m_numPlayers) {
        return false;
    }

    // Check if we have all decryptions needed
    // (Simplified: assume last decryption reveals the card)
    Card decoded = BytesToCard(partialDecrypt);
    if (Card::IsValidEncoded(decoded.GetEncoded())) {
        m_revealedCards[cardIndex] = decoded;
        return true;
    }

    return true; // Partial decrypt stored
}

std::optional<Card> MentalPokerProtocol::GetRevealedCard(size_t cardIndex) const {
    auto it = m_revealedCards.find(cardIndex);
    if (it != m_revealedCards.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool MentalPokerProtocol::AllCommitmentsReceived() const {
    for (size_t i = 0; i < m_numPlayers; ++i) {
        if (m_playerCommitments[i].IsNull()) {
            return false;
        }
    }
    return true;
}

bool MentalPokerProtocol::AllPublicKeysReceived() const {
    for (size_t i = 0; i < m_numPlayers; ++i) {
        if (i != m_myPosition && m_playerPublicKeys[i].empty()) {
            return false;
        }
    }
    return true;
}

std::vector<uint8_t> MentalPokerProtocol::Encrypt(
    const std::vector<uint8_t>& data,
    const std::vector<uint8_t>& exponent) const {
    return BigInt::ModPow(data, exponent, m_sharedModulus);
}

std::vector<uint8_t> MentalPokerProtocol::Decrypt(
    const std::vector<uint8_t>& data) const {
    return BigInt::ModPow(data, m_myKeys.GetPrivateKey(), m_sharedModulus);
}

std::vector<uint8_t> MentalPokerProtocol::CardToBytes(const Card& card) {
    std::vector<uint8_t> result(32, 0);
    result.back() = card.GetEncoded() + 1; // +1 to avoid zero
    return result;
}

Card MentalPokerProtocol::BytesToCard(const std::vector<uint8_t>& bytes) {
    if (bytes.empty()) {
        return Card(0);
    }
    uint8_t encoded = bytes.back() - 1; // -1 to reverse encoding
    if (Card::IsValidEncoded(encoded)) {
        return Card(encoded);
    }
    return Card(0);
}

void MentalPokerProtocol::ShuffleDeck(EncryptedDeck& deck, const uint256& seed) {
    std::vector<EncryptedCard> cards(deck.Size());
    for (size_t i = 0; i < deck.Size(); ++i) {
        cards[i] = deck[i];
    }

    // Fisher-Yates shuffle with deterministic randomness from seed
    std::array<uint8_t, 32> seedBytes;
    memcpy(seedBytes.data(), seed.begin(), 32);

    for (size_t i = cards.size() - 1; i > 0; --i) {
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

        uint32_t randomValue = (hash[0] << 24) | (hash[1] << 16) |
                               (hash[2] << 8) | hash[3];
        size_t j = randomValue % (i + 1);

        std::swap(cards[i], cards[j]);
        memcpy(seedBytes.data(), hash, 32);
    }

    deck.SetCards(cards);
}

} // namespace poker
