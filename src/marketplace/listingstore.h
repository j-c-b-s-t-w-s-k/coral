// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CORAL_MARKETPLACE_LISTINGSTORE_H
#define CORAL_MARKETPLACE_LISTINGSTORE_H

#include <marketplace/listing.h>
#include <dbwrapper.h>

#include <vector>

namespace marketplace {

// Database key prefixes
static constexpr char DB_LISTING = 'L';        // L + listingId -> MarketListing
static constexpr char DB_CATEGORY = 'C';       // C + category + listingId -> listingId
static constexpr char DB_SELLER = 'S';         // S + sellerAddr + listingId -> listingId
static constexpr char DB_ACTIVE = 'A';         // A + timestamp + listingId -> listingId

/**
 * LevelDB-backed storage for marketplace listings.
 * Provides indexed access by category, seller, and active status.
 */
class ListingStore : public CDBWrapper
{
public:
    ListingStore(const fs::path& path, size_t cacheSize,
                 bool memory = false, bool wipe = false);

    // CRUD Operations
    bool WriteListing(const MarketListing& listing);
    bool ReadListing(const uint256& listingId, MarketListing& listing) const;
    bool DeleteListing(const uint256& listingId);
    bool ListingExists(const uint256& listingId) const;

    // Query Operations
    std::vector<MarketListing> GetByCategory(const std::string& category, int limit = 100);
    std::vector<MarketListing> GetBySeller(const CTxDestination& seller, int limit = 100);
    std::vector<MarketListing> GetActive(int limit = 100);
    std::vector<MarketListing> Search(const std::string& query, int limit = 100);

    // Get all listings (for iteration)
    std::vector<MarketListing> GetAllListings(int limit = 1000);

    // Maintenance
    bool PruneExpired(int64_t currentTime);
    size_t CountActive();

private:
    void UpdateIndexes(const MarketListing& listing, bool remove = false);
    void RemoveIndexes(const MarketListing& listing);
};

} // namespace marketplace

#endif // CORAL_MARKETPLACE_LISTINGSTORE_H
