// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <marketplace/orderstore.h>

#include <util/time.h>

namespace marketplace {

OrderStore::OrderStore(const fs::path& path, size_t cacheSize,
                       bool memory, bool wipe)
    : CDBWrapper(path, cacheSize, memory, wipe)
{
}

bool OrderStore::WriteOrder(const MarketOrder& order)
{
    CDBBatch batch(*this);

    // Write main order record
    std::pair<char, uint256> key{DB_ORDER, order.orderId};
    batch.Write(key, order);

    // Buyer index
    std::string buyerStr = EncodeDestination(order.buyerAddress);
    std::tuple<char, std::string, uint256> buyerKey{DB_BUYER, buyerStr, order.orderId};
    batch.Write(buyerKey, order.orderId);

    // Seller index
    std::string sellerStr = EncodeDestination(order.sellerAddress);
    std::tuple<char, std::string, uint256> sellerKey{DB_SELLER_ORDER, sellerStr, order.orderId};
    batch.Write(sellerKey, order.orderId);

    // Listing index
    std::tuple<char, uint256, uint256> listingKey{DB_LISTING_ORDER, order.listingId, order.orderId};
    batch.Write(listingKey, order.orderId);

    // Escrow tx index (if funded)
    if (!order.escrowTxid.IsNull()) {
        std::pair<char, uint256> escrowKey{DB_ESCROW, order.escrowTxid};
        batch.Write(escrowKey, order.orderId);
    }

    return WriteBatch(batch);
}

bool OrderStore::ReadOrder(const uint256& orderId, MarketOrder& order) const
{
    std::pair<char, uint256> key{DB_ORDER, orderId};
    return Read(key, order);
}

bool OrderStore::UpdateOrderStatus(const uint256& orderId, OrderStatus status)
{
    MarketOrder order;
    if (!ReadOrder(orderId, order)) {
        return false;
    }

    order.status = status;
    order.completedAt = GetTime();

    return WriteOrder(order);
}

bool OrderStore::DeleteOrder(const uint256& orderId)
{
    MarketOrder order;
    if (!ReadOrder(orderId, order)) {
        return false;
    }

    CDBBatch batch(*this);

    // Remove main record
    std::pair<char, uint256> key{DB_ORDER, orderId};
    batch.Erase(key);

    // Remove indexes
    RemoveIndexes(order);

    return WriteBatch(batch);
}

bool OrderStore::OrderExists(const uint256& orderId) const
{
    std::pair<char, uint256> key{DB_ORDER, orderId};
    return Exists(key);
}

std::vector<MarketOrder> OrderStore::GetByBuyer(const CTxDestination& buyer, int limit)
{
    std::vector<MarketOrder> results;
    std::string buyerStr = EncodeDestination(buyer);

    std::unique_ptr<CDBIterator> iter(NewIterator());
    std::tuple<char, std::string, uint256> startKey{DB_BUYER, buyerStr, uint256()};
    iter->Seek(startKey);

    while (iter->Valid() && results.size() < static_cast<size_t>(limit)) {
        std::tuple<char, std::string, uint256> key;
        if (!iter->GetKey(key)) break;

        if (std::get<0>(key) != DB_BUYER || std::get<1>(key) != buyerStr) break;

        uint256 orderId;
        if (iter->GetValue(orderId)) {
            MarketOrder order;
            if (ReadOrder(orderId, order)) {
                results.push_back(order);
            }
        }

        iter->Next();
    }

    return results;
}

std::vector<MarketOrder> OrderStore::GetBySeller(const CTxDestination& seller, int limit)
{
    std::vector<MarketOrder> results;
    std::string sellerStr = EncodeDestination(seller);

    std::unique_ptr<CDBIterator> iter(NewIterator());
    std::tuple<char, std::string, uint256> startKey{DB_SELLER_ORDER, sellerStr, uint256()};
    iter->Seek(startKey);

    while (iter->Valid() && results.size() < static_cast<size_t>(limit)) {
        std::tuple<char, std::string, uint256> key;
        if (!iter->GetKey(key)) break;

        if (std::get<0>(key) != DB_SELLER_ORDER || std::get<1>(key) != sellerStr) break;

        uint256 orderId;
        if (iter->GetValue(orderId)) {
            MarketOrder order;
            if (ReadOrder(orderId, order)) {
                results.push_back(order);
            }
        }

        iter->Next();
    }

    return results;
}

std::vector<MarketOrder> OrderStore::GetByListing(const uint256& listingId, int limit)
{
    std::vector<MarketOrder> results;

    std::unique_ptr<CDBIterator> iter(NewIterator());
    std::tuple<char, uint256, uint256> startKey{DB_LISTING_ORDER, listingId, uint256()};
    iter->Seek(startKey);

    while (iter->Valid() && results.size() < static_cast<size_t>(limit)) {
        std::tuple<char, uint256, uint256> key;
        if (!iter->GetKey(key)) break;

        if (std::get<0>(key) != DB_LISTING_ORDER || std::get<1>(key) != listingId) break;

        uint256 orderId;
        if (iter->GetValue(orderId)) {
            MarketOrder order;
            if (ReadOrder(orderId, order)) {
                results.push_back(order);
            }
        }

        iter->Next();
    }

    return results;
}

std::vector<MarketOrder> OrderStore::GetPendingOrders(int limit)
{
    std::vector<MarketOrder> results;
    auto all = GetAllOrders(1000);

    for (const auto& order : all) {
        if (results.size() >= static_cast<size_t>(limit)) break;

        if (order.status == OrderStatus::CREATED ||
            order.status == OrderStatus::FUNDED ||
            order.status == OrderStatus::SHIPPED) {
            results.push_back(order);
        }
    }

    return results;
}

std::vector<MarketOrder> OrderStore::GetDisputed(int limit)
{
    std::vector<MarketOrder> results;
    auto all = GetAllOrders(1000);

    for (const auto& order : all) {
        if (results.size() >= static_cast<size_t>(limit)) break;

        if (order.status == OrderStatus::DISPUTED) {
            results.push_back(order);
        }
    }

    return results;
}

bool OrderStore::GetOrderByEscrowTx(const uint256& escrowTxid, MarketOrder& order) const
{
    std::pair<char, uint256> key{DB_ESCROW, escrowTxid};
    uint256 orderId;
    if (!Read(key, orderId)) {
        return false;
    }
    return ReadOrder(orderId, order);
}

std::vector<MarketOrder> OrderStore::GetAllOrders(int limit)
{
    std::vector<MarketOrder> results;

    std::unique_ptr<CDBIterator> iter(NewIterator());
    std::pair<char, uint256> startKey{DB_ORDER, uint256()};
    iter->Seek(startKey);

    while (iter->Valid() && results.size() < static_cast<size_t>(limit)) {
        std::pair<char, uint256> key;
        if (!iter->GetKey(key)) break;

        if (key.first != DB_ORDER) break;

        MarketOrder order;
        if (iter->GetValue(order)) {
            results.push_back(order);
        }

        iter->Next();
    }

    return results;
}

std::vector<uint256> OrderStore::GetExpiredOrders(int currentHeight)
{
    std::vector<uint256> expired;
    auto all = GetAllOrders(10000);

    for (const auto& order : all) {
        if (order.IsExpired(currentHeight) &&
            order.status != OrderStatus::COMPLETED &&
            order.status != OrderStatus::REFUNDED &&
            order.status != OrderStatus::CANCELLED) {
            expired.push_back(order.orderId);
        }
    }

    return expired;
}

void OrderStore::RemoveIndexes(const MarketOrder& order)
{
    CDBBatch batch(*this);

    // Remove buyer index
    std::string buyerStr = EncodeDestination(order.buyerAddress);
    std::tuple<char, std::string, uint256> buyerKey{DB_BUYER, buyerStr, order.orderId};
    batch.Erase(buyerKey);

    // Remove seller index
    std::string sellerStr = EncodeDestination(order.sellerAddress);
    std::tuple<char, std::string, uint256> sellerKey{DB_SELLER_ORDER, sellerStr, order.orderId};
    batch.Erase(sellerKey);

    // Remove listing index
    std::tuple<char, uint256, uint256> listingKey{DB_LISTING_ORDER, order.listingId, order.orderId};
    batch.Erase(listingKey);

    // Remove escrow tx index
    if (!order.escrowTxid.IsNull()) {
        std::pair<char, uint256> escrowKey{DB_ESCROW, order.escrowTxid};
        batch.Erase(escrowKey);
    }

    WriteBatch(batch);
}

} // namespace marketplace
