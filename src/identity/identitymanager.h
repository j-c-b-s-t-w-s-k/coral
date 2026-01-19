// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CORAL_IDENTITY_IDENTITYMANAGER_H
#define CORAL_IDENTITY_IDENTITYMANAGER_H

#include <identity/useridentity.h>
#include <dbwrapper.h>
#include <sync.h>

#include <memory>
#include <functional>

namespace identity {

/**
 * Manages identity storage and verification.
 * Provides persistence via LevelDB and verification helpers.
 */
class IdentityManager
{
public:
    explicit IdentityManager(const fs::path& dataDir);
    ~IdentityManager();

    // Prevent copying
    IdentityManager(const IdentityManager&) = delete;
    IdentityManager& operator=(const IdentityManager&) = delete;

    /**
     * Get or create identity for an address
     */
    UserIdentity GetIdentity(const CTxDestination& address);

    /**
     * Save identity to storage
     */
    bool SaveIdentity(const UserIdentity& identity);

    /**
     * Delete identity
     */
    bool DeleteIdentity(const CTxDestination& address);

    /**
     * Check if identity exists
     */
    bool HasIdentity(const CTxDestination& address);

    /**
     * Get all known identities (for display/caching)
     */
    std::vector<UserIdentity> GetAllIdentities();

    /**
     * Sign a proof message with wallet key
     * Returns empty string on failure
     */
    std::string SignProofMessage(const std::string& message,
                                  const CTxDestination& address,
                                  std::function<bool(const std::string&, const CTxDestination&, std::string&)> signFunc);

    /**
     * Verify a signed proof message
     */
    bool VerifyProofSignature(const std::string& message,
                               const std::string& signature,
                               const CTxDestination& address);

    /**
     * Create a proof for a service (generates message and instructions)
     */
    struct ProofRequest {
        std::string message;
        std::string signature;
        std::string instructions;
        std::string postTemplate;
    };
    ProofRequest CreateProofRequest(const std::string& service,
                                     const std::string& identifier,
                                     const CTxDestination& address,
                                     std::function<bool(const std::string&, const CTxDestination&, std::string&)> signFunc);

    /**
     * Add a verified proof to identity
     */
    bool AddVerifiedProof(const CTxDestination& address,
                          const std::string& service,
                          const std::string& identifier,
                          const std::string& proofUrl,
                          const std::string& signature);

    /**
     * Add PGP key to identity
     */
    bool AddPGPKey(const CTxDestination& address,
                   const std::string& publicKey,
                   const std::string& fingerprint);

    /**
     * Lookup identity by service identifier
     * e.g., find Coral address by X username
     */
    std::optional<UserIdentity> LookupByService(const std::string& service,
                                                 const std::string& identifier);

private:
    mutable Mutex m_mutex;
    std::unique_ptr<CDBWrapper> m_db;

    // DB key prefixes
    static constexpr char DB_IDENTITY = 'I';      // Address -> UserIdentity
    static constexpr char DB_SERVICE_INDEX = 'S'; // Service:Identifier -> Address

    std::string MakeServiceKey(const std::string& service, const std::string& identifier) const;
    void UpdateServiceIndex(const UserIdentity& identity);
    void RemoveServiceIndex(const UserIdentity& identity);
};

} // namespace identity

#endif // CORAL_IDENTITY_IDENTITYMANAGER_H
