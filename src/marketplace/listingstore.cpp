// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <marketplace/listingstore.h>

#include <util/time.h>

namespace marketplace {

ListingStore::ListingStore(const fs::path& path, size_t cacheSize,
                           bool memory, bool wipe)
    : CDBWrapper(path, cacheSize, memory, wipe)
{
}

bool ListingStore::WriteListing(const MarketListing& listing)
{
    CDBBatch batch(*this);

    // Write main listing record
    std::pair<char, uint256> key{DB_LISTING, listing.id};
    batch.Write(key, listing);

    // Update indexes
    if (listing.status == ListingStatus::ACTIVE) {
        // Category index
        if (!listing.category.empty()) {
            std::tuple<char, std::string, uint256> catKey{DB_CATEGORY, listing.category, listing.id};
            batch.Write(catKey, listing.id);
        }

        // Seller index
        std::string sellerStr = EncodeDestination(listing.sellerAddress);
        std::tuple<char, std::string, uint256> sellerKey{DB_SELLER, sellerStr, listing.id};
        batch.Write(sellerKey, listing.id);

        // Active index (by timestamp for sorting)
        std::tuple<char, int64_t, uint256> activeKey{DB_ACTIVE, listing.createdAt, listing.id};
        batch.Write(activeKey, listing.id);
    }

    return WriteBatch(batch);
}

bool ListingStore::ReadListing(const uint256& listingId, MarketListing& listing) const
{
    std::pair<char, uint256> key{DB_LISTING, listingId};
    return Read(key, listing);
}

bool ListingStore::DeleteListing(const uint256& listingId)
{
    MarketListing listing;
    if (!ReadListing(listingId, listing)) {
        return false;
    }

    CDBBatch batch(*this);

    // Remove main record
    std::pair<char, uint256> key{DB_LISTING, listingId};
    batch.Erase(key);

    // Remove indexes
    RemoveIndexes(listing);

    return WriteBatch(batch);
}

bool ListingStore::ListingExists(const uint256& listingId) const
{
    std::pair<char, uint256> key{DB_LISTING, listingId};
    return Exists(key);
}

std::vector<MarketListing> ListingStore::GetByCategory(const std::string& category, int limit)
{
    std::vector<MarketListing> results;

    std::unique_ptr<CDBIterator> iter(NewIterator());
    std::tuple<char, std::string, uint256> startKey{DB_CATEGORY, category, uint256()};
    iter->Seek(startKey);

    while (iter->Valid() && results.size() < static_cast<size_t>(limit)) {
        std::tuple<char, std::string, uint256> key;
        if (!iter->GetKey(key)) break;

        // Check if still in category
        if (std::get<0>(key) != DB_CATEGORY || std::get<1>(key) != category) break;

        uint256 listingId;
        if (iter->GetValue(listingId)) {
            MarketListing listing;
            if (ReadListing(listingId, listing)) {
                results.push_back(listing);
            }
        }

        iter->Next();
    }

    return results;
}

std::vector<MarketListing> ListingStore::GetBySeller(const CTxDestination& seller, int limit)
{
    std::vector<MarketListing> results;
    std::string sellerStr = EncodeDestination(seller);

    std::unique_ptr<CDBIterator> iter(NewIterator());
    std::tuple<char, std::string, uint256> startKey{DB_SELLER, sellerStr, uint256()};
    iter->Seek(startKey);

    while (iter->Valid() && results.size() < static_cast<size_t>(limit)) {
        std::tuple<char, std::string, uint256> key;
        if (!iter->GetKey(key)) break;

        if (std::get<0>(key) != DB_SELLER || std::get<1>(key) != sellerStr) break;

        uint256 listingId;
        if (iter->GetValue(listingId)) {
            MarketListing listing;
            if (ReadListing(listingId, listing)) {
                results.push_back(listing);
            }
        }

        iter->Next();
    }

    return results;
}

std::vector<MarketListing> ListingStore::GetActive(int limit)
{
    std::vector<MarketListing> results;

    std::unique_ptr<CDBIterator> iter(NewIterator());
    std::tuple<char, int64_t, uint256> startKey{DB_ACTIVE, 0, uint256()};
    iter->Seek(startKey);

    while (iter->Valid() && results.size() < static_cast<size_t>(limit)) {
        std::tuple<char, int64_t, uint256> key;
        if (!iter->GetKey(key)) break;

        if (std::get<0>(key) != DB_ACTIVE) break;

        uint256 listingId;
        if (iter->GetValue(listingId)) {
            MarketListing listing;
            if (ReadListing(listingId, listing)) {
                // Verify still active
                if (listing.status == ListingStatus::ACTIVE) {
                    results.push_back(listing);
                }
            }
        }

        iter->Next();
    }

    return results;
}

std::vector<MarketListing> ListingStore::Search(const std::string& query, int limit)
{
    std::vector<MarketListing> results;
    std::string lowerQuery = query;
    std::transform(lowerQuery.begin(), lowerQuery.end(), lowerQuery.begin(), ::tolower);

    // Simple linear search through active listings
    // A real implementation would use a proper search index
    auto active = GetActive(1000);
    for (const auto& listing : active) {
        if (results.size() >= static_cast<size_t>(limit)) break;

        std::string lowerTitle = listing.title;
        std::transform(lowerTitle.begin(), lowerTitle.end(), lowerTitle.begin(), ::tolower);

        std::string lowerDesc = listing.description;
        std::transform(lowerDesc.begin(), lowerDesc.end(), lowerDesc.begin(), ::tolower);

        if (lowerTitle.find(lowerQuery) != std::string::npos ||
            lowerDesc.find(lowerQuery) != std::string::npos) {
            results.push_back(listing);
        }
    }

    return results;
}

std::vector<MarketListing> ListingStore::GetAllListings(int limit)
{
    std::vector<MarketListing> results;

    std::unique_ptr<CDBIterator> iter(NewIterator());
    std::pair<char, uint256> startKey{DB_LISTING, uint256()};
    iter->Seek(startKey);

    while (iter->Valid() && results.size() < static_cast<size_t>(limit)) {
        std::pair<char, uint256> key;
        if (!iter->GetKey(key)) break;

        if (key.first != DB_LISTING) break;

        MarketListing listing;
        if (iter->GetValue(listing)) {
            results.push_back(listing);
        }

        iter->Next();
    }

    return results;
}

bool ListingStore::PruneExpired(int64_t currentTime)
{
    auto all = GetAllListings(10000);
    int pruned = 0;

    for (const auto& listing : all) {
        if (listing.IsExpired(currentTime) && listing.status == ListingStatus::ACTIVE) {
            MarketListing updated = listing;
            updated.status = ListingStatus::EXPIRED;
            updated.updatedAt = currentTime;
            WriteListing(updated);
            pruned++;
        }
    }

    return pruned > 0;
}

size_t ListingStore::CountActive()
{
    size_t count = 0;

    std::unique_ptr<CDBIterator> iter(NewIterator());
    std::tuple<char, int64_t, uint256> startKey{DB_ACTIVE, 0, uint256()};
    iter->Seek(startKey);

    while (iter->Valid()) {
        std::tuple<char, int64_t, uint256> key;
        if (!iter->GetKey(key)) break;
        if (std::get<0>(key) != DB_ACTIVE) break;
        count++;
        iter->Next();
    }

    return count;
}

void ListingStore::RemoveIndexes(const MarketListing& listing)
{
    CDBBatch batch(*this);

    // Remove category index
    if (!listing.category.empty()) {
        std::tuple<char, std::string, uint256> catKey{DB_CATEGORY, listing.category, listing.id};
        batch.Erase(catKey);
    }

    // Remove seller index
    std::string sellerStr = EncodeDestination(listing.sellerAddress);
    std::tuple<char, std::string, uint256> sellerKey{DB_SELLER, sellerStr, listing.id};
    batch.Erase(sellerKey);

    // Remove active index
    std::tuple<char, int64_t, uint256> activeKey{DB_ACTIVE, listing.createdAt, listing.id};
    batch.Erase(activeKey);

    WriteBatch(batch);
}

} // namespace marketplace
