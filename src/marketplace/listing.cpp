// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <marketplace/listing.h>

#include <hash.h>
#include <streams.h>

namespace marketplace {

uint256 MarketListing::ComputeHash() const
{
    CHashWriter ss(SER_GETHASH, 0);

    // Hash seller address
    std::string addrStr = EncodeDestination(sellerAddress);
    ss << addrStr;

    // Hash title
    ss << title;

    // Hash creation timestamp
    ss << createdAt;

    // Hash nonce
    ss << nonce;

    return ss.GetHash();
}

bool MarketListing::IsValid() const
{
    // Must have a title
    if (title.empty()) {
        return false;
    }

    // Title length check
    if (title.length() > 256) {
        return false;
    }

    // Description length check
    if (description.length() > 4096) {
        return false;
    }

    // Price must be positive
    if (price <= 0) {
        return false;
    }

    // Price sanity check (max 21 million CRL)
    if (price > 21000000 * COIN) {
        return false;
    }

    // Must have seller address
    if (!IsValidDestination(sellerAddress)) {
        return false;
    }

    // Timestamps must be sensible
    if (createdAt <= 0) {
        return false;
    }

    return true;
}

} // namespace marketplace
