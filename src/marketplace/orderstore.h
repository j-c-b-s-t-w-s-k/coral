// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CORAL_MARKETPLACE_ORDERSTORE_H
#define CORAL_MARKETPLACE_ORDERSTORE_H

#include <marketplace/order.h>
#include <dbwrapper.h>

#include <vector>

namespace marketplace {

// Database key prefixes
static constexpr char DB_ORDER = 'O';          // O + orderId -> MarketOrder
static constexpr char DB_BUYER = 'B';          // B + buyerAddr + orderId -> orderId
static constexpr char DB_SELLER_ORDER = 'R';   // R + sellerAddr + orderId -> orderId
static constexpr char DB_LISTING_ORDER = 'X';  // X + listingId + orderId -> orderId
static constexpr char DB_ESCROW = 'E';         // E + escrowTxid -> orderId

/**
 * LevelDB-backed storage for marketplace orders.
 * Provides indexed access by buyer, seller, listing, and escrow transaction.
 */
class OrderStore : public CDBWrapper
{
public:
    OrderStore(const fs::path& path, size_t cacheSize,
               bool memory = false, bool wipe = false);

    // CRUD Operations
    bool WriteOrder(const MarketOrder& order);
    bool ReadOrder(const uint256& orderId, MarketOrder& order) const;
    bool UpdateOrderStatus(const uint256& orderId, OrderStatus status);
    bool DeleteOrder(const uint256& orderId);
    bool OrderExists(const uint256& orderId) const;

    // Query Operations
    std::vector<MarketOrder> GetByBuyer(const CTxDestination& buyer, int limit = 100);
    std::vector<MarketOrder> GetBySeller(const CTxDestination& seller, int limit = 100);
    std::vector<MarketOrder> GetByListing(const uint256& listingId, int limit = 100);
    std::vector<MarketOrder> GetPendingOrders(int limit = 100);
    std::vector<MarketOrder> GetDisputed(int limit = 100);

    // Escrow lookups
    bool GetOrderByEscrowTx(const uint256& escrowTxid, MarketOrder& order) const;

    // Get all orders
    std::vector<MarketOrder> GetAllOrders(int limit = 1000);

    // Maintenance
    std::vector<uint256> GetExpiredOrders(int currentHeight);

private:
    void UpdateIndexes(const MarketOrder& order, bool remove = false);
    void RemoveIndexes(const MarketOrder& order);
};

} // namespace marketplace

#endif // CORAL_MARKETPLACE_ORDERSTORE_H
