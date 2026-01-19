// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CORAL_MARKETPLACE_LISTING_H
#define CORAL_MARKETPLACE_LISTING_H

#include <marketplace/marketplacetypes.h>
#include <consensus/amount.h>
#include <key_io.h>
#include <uint256.h>
#include <script/standard.h>
#include <serialize.h>
#include <pubkey.h>

#include <string>
#include <vector>

namespace marketplace {

/**
 * MarketListing represents a single item for sale on the marketplace.
 * Serializable for storage in LevelDB.
 */
struct MarketListing
{
    // Identifiers
    uint256 id;                         // SHA256 hash of (seller + title + timestamp + nonce)
    uint256 announceTxid;               // On-chain announcement tx (optional)

    // Content
    std::string title;
    std::string description;
    std::string category;
    CAmount price{0};
    std::string imageHash;              // IPFS or similar content hash

    // Seller info
    CTxDestination sellerAddress;
    CPubKey sellerPubKey;

    // State
    ListingStatus status{ListingStatus::DRAFT};
    int64_t createdAt{0};
    int64_t expiresAt{0};
    int64_t updatedAt{0};

    // Nonce for ID generation
    uint64_t nonce{0};

    MarketListing() = default;

    // Compute the listing ID hash
    uint256 ComputeHash() const;

    // Validate listing data
    bool IsValid() const;

    // Check if listing is expired
    bool IsExpired(int64_t now) const {
        return expiresAt > 0 && now > expiresAt;
    }

    // Check if listing can be purchased
    bool CanPurchase() const {
        return status == ListingStatus::ACTIVE;
    }

    SERIALIZE_METHODS(MarketListing, obj)
    {
        READWRITE(obj.id);
        READWRITE(obj.announceTxid);
        READWRITE(obj.title);
        READWRITE(obj.description);
        READWRITE(obj.category);
        READWRITE(obj.price);
        READWRITE(obj.imageHash);

        // Serialize destination as string
        std::string addrStr;
        SER_WRITE(obj, addrStr = EncodeDestination(obj.sellerAddress));
        READWRITE(addrStr);
        SER_READ(obj, obj.sellerAddress = DecodeDestination(addrStr));

        READWRITE(obj.sellerPubKey);

        // Serialize status as uint8_t
        uint8_t statusInt;
        SER_WRITE(obj, statusInt = static_cast<uint8_t>(obj.status));
        READWRITE(statusInt);
        SER_READ(obj, obj.status = static_cast<ListingStatus>(statusInt));

        READWRITE(obj.createdAt);
        READWRITE(obj.expiresAt);
        READWRITE(obj.updatedAt);
        READWRITE(obj.nonce);
    }
};

} // namespace marketplace

#endif // CORAL_MARKETPLACE_LISTING_H
