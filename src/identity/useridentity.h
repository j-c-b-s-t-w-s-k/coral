// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CORAL_IDENTITY_USERIDENTITY_H
#define CORAL_IDENTITY_USERIDENTITY_H

#include <key_io.h>
#include <pubkey.h>
#include <serialize.h>
#include <uint256.h>

#include <string>
#include <vector>
#include <optional>

namespace identity {

/**
 * Proof status for identity claims
 */
enum class ProofStatus : uint8_t {
    UNVERIFIED = 0,   // Claim made but not verified
    VERIFIED = 1,     // Proof verified successfully
    EXPIRED = 2,      // Proof expired or removed
    INVALID = 3       // Verification failed
};

/**
 * Individual identity proof (X, Reddit, PGP, etc)
 */
struct IdentityProof {
    std::string service;      // "x", "reddit", "pgp", "github", "keybase"
    std::string identifier;   // username or key fingerprint
    std::string proofUrl;     // URL to proof (tweet, reddit post, etc)
    std::string signature;    // Coral wallet signature of claim
    ProofStatus status{ProofStatus::UNVERIFIED};
    int64_t verifiedAt{0};    // Timestamp of last verification
    int64_t expiresAt{0};     // When proof expires (0 = never)

    SERIALIZE_METHODS(IdentityProof, obj)
    {
        READWRITE(obj.service, obj.identifier, obj.proofUrl, obj.signature);
        uint8_t statusInt;
        SER_WRITE(obj, statusInt = static_cast<uint8_t>(obj.status));
        READWRITE(statusInt);
        SER_READ(obj, obj.status = static_cast<ProofStatus>(statusInt));
        READWRITE(obj.verifiedAt, obj.expiresAt);
    }

    bool IsVerified() const { return status == ProofStatus::VERIFIED; }
    bool IsExpired() const { return expiresAt > 0 && GetTime() > expiresAt; }
};

/**
 * PGP key information
 */
struct PGPKey {
    std::string fingerprint;  // 40-char hex fingerprint
    std::string keyId;        // Short key ID (last 8 chars)
    std::string publicKey;    // ASCII-armored public key
    std::string keyServer;    // Key server URL (optional)
    bool verified{false};     // Whether signature verified
    int64_t importedAt{0};

    SERIALIZE_METHODS(PGPKey, obj)
    {
        READWRITE(obj.fingerprint, obj.keyId, obj.publicKey, obj.keyServer);
        READWRITE(obj.verified, obj.importedAt);
    }

    bool IsValid() const { return fingerprint.length() == 40; }
    std::string ShortId() const { return fingerprint.substr(fingerprint.length() - 16); }
};

/**
 * Complete user identity profile
 * Anchored to a Coral address
 */
class UserIdentity {
public:
    UserIdentity() = default;
    explicit UserIdentity(const CTxDestination& address);

    // Core identity
    void SetAddress(const CTxDestination& address);
    CTxDestination GetAddress() const { return m_address; }
    std::string GetAddressString() const { return EncodeDestination(m_address); }

    // Display name (optional pseudonym)
    void SetDisplayName(const std::string& name) { m_displayName = name; }
    std::string GetDisplayName() const { return m_displayName; }

    // Bio/description
    void SetBio(const std::string& bio) { m_bio = bio; }
    std::string GetBio() const { return m_bio; }

    // Avatar (IPFS hash or data URL)
    void SetAvatar(const std::string& avatar) { m_avatar = avatar; }
    std::string GetAvatar() const { return m_avatar; }

    // PGP Key
    void SetPGPKey(const PGPKey& key) { m_pgpKey = key; }
    std::optional<PGPKey> GetPGPKey() const { return m_pgpKey; }
    bool HasPGPKey() const { return m_pgpKey.has_value(); }
    void ClearPGPKey() { m_pgpKey.reset(); }

    // Identity proofs
    void AddProof(const IdentityProof& proof);
    void RemoveProof(const std::string& service);
    std::optional<IdentityProof> GetProof(const std::string& service) const;
    std::vector<IdentityProof> GetAllProofs() const { return m_proofs; }
    bool HasProof(const std::string& service) const;
    int GetVerifiedProofCount() const;

    // Trust score (based on verified proofs)
    int GetTrustScore() const;

    // Timestamps
    int64_t GetCreatedAt() const { return m_createdAt; }
    int64_t GetUpdatedAt() const { return m_updatedAt; }
    void Touch() { m_updatedAt = GetTime(); }

    // Serialization
    SERIALIZE_METHODS(UserIdentity, obj)
    {
        // Serialize address as string
        std::string addrStr;
        SER_WRITE(obj, addrStr = EncodeDestination(obj.m_address));
        READWRITE(addrStr);
        SER_READ(obj, obj.m_address = DecodeDestination(addrStr));

        READWRITE(obj.m_displayName, obj.m_bio, obj.m_avatar);

        // Serialize optional PGP key
        bool hasPgp;
        SER_WRITE(obj, hasPgp = obj.m_pgpKey.has_value());
        READWRITE(hasPgp);
        if (hasPgp) {
            PGPKey pgp;
            SER_WRITE(obj, pgp = obj.m_pgpKey.value());
            READWRITE(pgp);
            SER_READ(obj, obj.m_pgpKey = pgp);
        }

        READWRITE(obj.m_proofs, obj.m_createdAt, obj.m_updatedAt);
    }

    // Generate proof message for signing
    static std::string GenerateProofMessage(const std::string& service,
                                            const std::string& identifier,
                                            const std::string& coralAddress);

private:
    CTxDestination m_address;
    std::string m_displayName;
    std::string m_bio;
    std::string m_avatar;
    std::optional<PGPKey> m_pgpKey;
    std::vector<IdentityProof> m_proofs;
    int64_t m_createdAt{0};
    int64_t m_updatedAt{0};
};

/**
 * Supported identity services
 */
struct IdentityService {
    std::string id;           // "x", "reddit", "github", etc
    std::string displayName;  // "X (Twitter)", "Reddit", etc
    std::string proofFormat;  // Instructions for posting proof
    std::string verifyUrl;    // URL pattern for verification
    int trustPoints;          // Points toward trust score

    static std::vector<IdentityService> GetSupportedServices();
    static std::optional<IdentityService> GetService(const std::string& id);
};

} // namespace identity

#endif // CORAL_IDENTITY_USERIDENTITY_H
