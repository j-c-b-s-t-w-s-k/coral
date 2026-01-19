// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <identity/identitymanager.h>

#include <key_io.h>
#include <logging.h>
#include <util/strencodings.h>

namespace identity {

IdentityManager::IdentityManager(const fs::path& dataDir)
{
    fs::path dbPath = dataDir / "identity";
    // CDBWrapper(path, cacheSize, fMemory, fWipe, obfuscate)
    m_db = std::make_unique<CDBWrapper>(dbPath, 1 << 20, false, false, true);
}

IdentityManager::~IdentityManager() = default;

UserIdentity IdentityManager::GetIdentity(const CTxDestination& address)
{
    LOCK(m_mutex);

    std::string addrStr = EncodeDestination(address);
    std::string key = std::string(1, DB_IDENTITY) + addrStr;

    UserIdentity identity;
    if (m_db->Read(key, identity)) {
        return identity;
    }

    // Create new identity
    identity.SetAddress(address);
    return identity;
}

bool IdentityManager::SaveIdentity(const UserIdentity& identity)
{
    LOCK(m_mutex);

    std::string addrStr = identity.GetAddressString();
    std::string key = std::string(1, DB_IDENTITY) + addrStr;

    // Remove old service index entries
    UserIdentity oldIdentity;
    if (m_db->Read(key, oldIdentity)) {
        RemoveServiceIndex(oldIdentity);
    }

    // Write identity
    if (!m_db->Write(key, identity)) {
        return false;
    }

    // Update service index
    UpdateServiceIndex(identity);

    return true;
}

bool IdentityManager::DeleteIdentity(const CTxDestination& address)
{
    LOCK(m_mutex);

    std::string addrStr = EncodeDestination(address);
    std::string key = std::string(1, DB_IDENTITY) + addrStr;

    // Get identity for service index cleanup
    UserIdentity identity;
    if (m_db->Read(key, identity)) {
        RemoveServiceIndex(identity);
    }

    return m_db->Erase(key);
}

bool IdentityManager::HasIdentity(const CTxDestination& address)
{
    LOCK(m_mutex);

    std::string addrStr = EncodeDestination(address);
    std::string key = std::string(1, DB_IDENTITY) + addrStr;

    return m_db->Exists(key);
}

std::vector<UserIdentity> IdentityManager::GetAllIdentities()
{
    LOCK(m_mutex);

    std::vector<UserIdentity> identities;
    std::string prefix(1, DB_IDENTITY);

    std::unique_ptr<CDBIterator> it(m_db->NewIterator());
    for (it->Seek(prefix); it->Valid(); it->Next()) {
        std::string key;
        if (!it->GetKey(key) || key.empty() || key[0] != DB_IDENTITY) {
            break;
        }

        UserIdentity identity;
        if (it->GetValue(identity)) {
            identities.push_back(identity);
        }
    }

    return identities;
}

std::string IdentityManager::SignProofMessage(
    const std::string& message,
    const CTxDestination& address,
    std::function<bool(const std::string&, const CTxDestination&, std::string&)> signFunc)
{
    std::string signature;
    if (signFunc && signFunc(message, address, signature)) {
        return signature;
    }
    return "";
}

bool IdentityManager::VerifyProofSignature(
    const std::string& message,
    const std::string& signature,
    const CTxDestination& address)
{
    // Basic validation
    if (message.empty() || signature.empty()) {
        return false;
    }

    // TODO: Implement actual signature verification
    // This would use MessageVerify from src/util/message.h
    // For now, accept if signature is non-empty (placeholder)
    return !signature.empty();
}

IdentityManager::ProofRequest IdentityManager::CreateProofRequest(
    const std::string& service,
    const std::string& identifier,
    const CTxDestination& address,
    std::function<bool(const std::string&, const CTxDestination&, std::string&)> signFunc)
{
    ProofRequest request;
    std::string addrStr = EncodeDestination(address);

    // Generate proof message
    request.message = UserIdentity::GenerateProofMessage(service, identifier, addrStr);

    // Sign the message
    request.signature = SignProofMessage(request.message, address, signFunc);

    // Get service-specific instructions
    auto svc = IdentityService::GetService(service);
    if (svc) {
        request.instructions = svc->proofFormat;
    }

    // Generate post template
    if (service == "x") {
        request.postTemplate = "Verifying my @CoralNetwork identity:\n\n"
                               "Address: " + addrStr + "\n\n"
                               "Signature: " + request.signature;
    } else if (service == "reddit") {
        request.postTemplate = "# Coral Identity Verification\n\n"
                               "I am verifying my Coral identity.\n\n"
                               "**Address:** `" + addrStr + "`\n\n"
                               "**Signature:** `" + request.signature + "`";
    } else if (service == "github") {
        request.postTemplate = "# coral-identity.md\n\n"
                               "Verifying my Coral Network identity.\n\n"
                               "```\n"
                               "Address: " + addrStr + "\n"
                               "Signature: " + request.signature + "\n"
                               "```";
    } else if (service == "pgp") {
        request.postTemplate = "-----BEGIN PGP SIGNED MESSAGE-----\n"
                               "Hash: SHA256\n\n" +
                               request.message + "\n"
                               "-----BEGIN PGP SIGNATURE-----\n"
                               "[Your PGP signature here]\n"
                               "-----END PGP SIGNATURE-----";
    } else {
        request.postTemplate = request.message + "\n\nSignature: " + request.signature;
    }

    return request;
}

bool IdentityManager::AddVerifiedProof(
    const CTxDestination& address,
    const std::string& service,
    const std::string& identifier,
    const std::string& proofUrl,
    const std::string& signature)
{
    UserIdentity identity = GetIdentity(address);

    IdentityProof proof;
    proof.service = service;
    proof.identifier = identifier;
    proof.proofUrl = proofUrl;
    proof.signature = signature;
    proof.status = ProofStatus::VERIFIED;
    proof.verifiedAt = GetTime();
    proof.expiresAt = 0; // Never expires

    identity.AddProof(proof);

    return SaveIdentity(identity);
}

bool IdentityManager::AddPGPKey(
    const CTxDestination& address,
    const std::string& publicKey,
    const std::string& fingerprint)
{
    UserIdentity identity = GetIdentity(address);

    PGPKey pgp;
    pgp.publicKey = publicKey;
    pgp.fingerprint = fingerprint;
    pgp.keyId = fingerprint.length() >= 8 ? fingerprint.substr(fingerprint.length() - 8) : fingerprint;
    pgp.verified = true; // TODO: Actually verify PGP signature
    pgp.importedAt = GetTime();

    identity.SetPGPKey(pgp);

    return SaveIdentity(identity);
}

std::optional<UserIdentity> IdentityManager::LookupByService(
    const std::string& service,
    const std::string& identifier)
{
    LOCK(m_mutex);

    std::string indexKey = std::string(1, DB_SERVICE_INDEX) + MakeServiceKey(service, identifier);

    std::string addrStr;
    if (!m_db->Read(indexKey, addrStr)) {
        return std::nullopt;
    }

    CTxDestination address = DecodeDestination(addrStr);
    if (!IsValidDestination(address)) {
        return std::nullopt;
    }

    std::string identityKey = std::string(1, DB_IDENTITY) + addrStr;
    UserIdentity identity;
    if (m_db->Read(identityKey, identity)) {
        return identity;
    }

    return std::nullopt;
}

std::string IdentityManager::MakeServiceKey(const std::string& service, const std::string& identifier) const
{
    return service + ":" + identifier;
}

void IdentityManager::UpdateServiceIndex(const UserIdentity& identity)
{
    std::string addrStr = identity.GetAddressString();

    for (const auto& proof : identity.GetAllProofs()) {
        if (proof.IsVerified()) {
            std::string indexKey = std::string(1, DB_SERVICE_INDEX) +
                                   MakeServiceKey(proof.service, proof.identifier);
            m_db->Write(indexKey, addrStr);
        }
    }

    // Index PGP key by fingerprint
    if (identity.HasPGPKey()) {
        auto pgp = identity.GetPGPKey();
        std::string indexKey = std::string(1, DB_SERVICE_INDEX) +
                               MakeServiceKey("pgp", pgp->fingerprint);
        m_db->Write(indexKey, addrStr);
    }
}

void IdentityManager::RemoveServiceIndex(const UserIdentity& identity)
{
    for (const auto& proof : identity.GetAllProofs()) {
        std::string indexKey = std::string(1, DB_SERVICE_INDEX) +
                               MakeServiceKey(proof.service, proof.identifier);
        m_db->Erase(indexKey);
    }

    // Remove PGP index
    if (identity.HasPGPKey()) {
        auto pgp = identity.GetPGPKey();
        std::string indexKey = std::string(1, DB_SERVICE_INDEX) +
                               MakeServiceKey("pgp", pgp->fingerprint);
        m_db->Erase(indexKey);
    }
}

} // namespace identity
