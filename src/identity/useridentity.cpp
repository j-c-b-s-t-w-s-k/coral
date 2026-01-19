// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <identity/useridentity.h>

#include <util/time.h>

namespace identity {

UserIdentity::UserIdentity(const CTxDestination& address)
    : m_address(address)
    , m_createdAt(GetTime())
    , m_updatedAt(GetTime())
{
}

void UserIdentity::SetAddress(const CTxDestination& address)
{
    m_address = address;
    if (m_createdAt == 0) {
        m_createdAt = GetTime();
    }
    Touch();
}

void UserIdentity::AddProof(const IdentityProof& proof)
{
    // Remove existing proof for same service
    RemoveProof(proof.service);
    m_proofs.push_back(proof);
    Touch();
}

void UserIdentity::RemoveProof(const std::string& service)
{
    m_proofs.erase(
        std::remove_if(m_proofs.begin(), m_proofs.end(),
            [&service](const IdentityProof& p) { return p.service == service; }),
        m_proofs.end()
    );
    Touch();
}

std::optional<IdentityProof> UserIdentity::GetProof(const std::string& service) const
{
    for (const auto& proof : m_proofs) {
        if (proof.service == service) {
            return proof;
        }
    }
    return std::nullopt;
}

bool UserIdentity::HasProof(const std::string& service) const
{
    return GetProof(service).has_value();
}

int UserIdentity::GetVerifiedProofCount() const
{
    int count = 0;
    for (const auto& proof : m_proofs) {
        if (proof.IsVerified() && !proof.IsExpired()) {
            count++;
        }
    }
    // Count PGP key as a proof
    if (m_pgpKey && m_pgpKey->verified) {
        count++;
    }
    return count;
}

int UserIdentity::GetTrustScore() const
{
    int score = 0;

    // Base score for having an identity
    score += 10;

    // Points for verified proofs
    for (const auto& proof : m_proofs) {
        if (proof.IsVerified() && !proof.IsExpired()) {
            auto service = IdentityService::GetService(proof.service);
            if (service) {
                score += service->trustPoints;
            } else {
                score += 10; // Default points
            }
        }
    }

    // Bonus for PGP key
    if (m_pgpKey && m_pgpKey->verified) {
        score += 25;
    }

    // Bonus for display name
    if (!m_displayName.empty()) {
        score += 5;
    }

    return std::min(score, 100); // Cap at 100
}

std::string UserIdentity::GenerateProofMessage(const std::string& service,
                                                const std::string& identifier,
                                                const std::string& coralAddress)
{
    return "Verifying my Coral identity:\n"
           "Address: " + coralAddress + "\n"
           "Service: " + service + "\n"
           "Username: " + identifier + "\n"
           "This is a cryptographic proof that I control this Coral address.";
}

std::vector<IdentityService> IdentityService::GetSupportedServices()
{
    return {
        {
            "pgp",
            "PGP Key",
            "Add your PGP public key and sign a message with it to verify ownership.",
            "",
            25
        },
        {
            "x",
            "X (Twitter)",
            "Post a tweet containing your signed proof message. "
            "Format: \"Verifying my @CoralNetwork identity: [signature]\"",
            "https://x.com/%s/status/",
            20
        },
        {
            "reddit",
            "Reddit",
            "Post in r/CoralNetwork or your profile with the signed proof. "
            "Include your Coral address and signature in the post.",
            "https://reddit.com/user/%s/",
            15
        },
        {
            "github",
            "GitHub",
            "Create a public gist named 'coral-identity.md' containing your "
            "signed proof message.",
            "https://gist.github.com/%s/",
            20
        },
        {
            "keybase",
            "Keybase",
            "Add a Coral proof to your Keybase profile by adding the signed "
            "message to your public folder.",
            "https://keybase.io/%s/",
            30
        },
        {
            "dns",
            "Domain (DNS)",
            "Add a TXT record to your domain: coral-identity=[signature]. "
            "This proves you control the domain.",
            "",
            25
        },
        {
            "nostr",
            "Nostr",
            "Publish a kind 0 profile event with coral_address field, or "
            "publish a kind 1 note with your signed proof.",
            "",
            15
        }
    };
}

std::optional<IdentityService> IdentityService::GetService(const std::string& id)
{
    for (const auto& service : GetSupportedServices()) {
        if (service.id == id) {
            return service;
        }
    }
    return std::nullopt;
}

} // namespace identity
