// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <marketplace/order.h>

#include <hash.h>
#include <streams.h>

namespace marketplace {

uint256 MarketOrder::ComputeHash() const
{
    CHashWriter ss(SER_GETHASH, 0);

    // Hash listing ID
    ss << listingId;

    // Hash buyer address
    std::string buyerAddrStr = EncodeDestination(buyerAddress);
    ss << buyerAddrStr;

    // Hash seller address
    std::string sellerAddrStr = EncodeDestination(sellerAddress);
    ss << sellerAddrStr;

    // Hash amount
    ss << amount;

    // Hash creation timestamp
    ss << createdAt;

    return ss.GetHash();
}

bool MarketOrder::IsValid() const
{
    // Must have a listing ID
    if (listingId.IsNull()) {
        return false;
    }

    // Must have buyer and seller addresses
    if (!IsValidDestination(buyerAddress)) {
        return false;
    }

    if (!IsValidDestination(sellerAddress)) {
        return false;
    }

    // Amount must be positive
    if (amount <= 0) {
        return false;
    }

    // Escrow amount must be at least the price
    if (escrowAmount < amount) {
        return false;
    }

    // Timestamps must be sensible
    if (createdAt <= 0) {
        return false;
    }

    return true;
}

} // namespace marketplace
