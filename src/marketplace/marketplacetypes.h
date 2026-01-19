// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CORAL_MARKETPLACE_TYPES_H
#define CORAL_MARKETPLACE_TYPES_H

#include <cstdint>

namespace marketplace {

/**
 * Listing status enumeration
 */
enum class ListingStatus : uint8_t {
    DRAFT = 0,          // Not yet published
    ACTIVE = 1,         // Published and available
    PENDING_SALE = 2,   // Buyer initiated purchase
    SOLD = 3,           // Sale completed
    CANCELLED = 4,      // Cancelled by seller
    EXPIRED = 5         // Time expired
};

/**
 * Order status enumeration
 */
enum class OrderStatus : uint8_t {
    CREATED = 0,        // Order created, awaiting buyer funding
    FUNDED = 1,         // Buyer funded escrow
    SHIPPED = 2,        // Seller marked as shipped
    DELIVERED = 3,      // Buyer confirmed receipt
    COMPLETED = 4,      // Funds released to seller
    DISPUTED = 5,       // Dispute opened
    REFUNDED = 6,       // Funds returned to buyer
    CANCELLED = 7,      // Order cancelled before funding
    EXPIRED = 8         // Timed out
};

/**
 * Escrow state enumeration
 */
enum class EscrowState : uint8_t {
    NONE = 0,
    CREATED = 1,
    FUNDED = 2,
    RELEASING = 3,
    RELEASED = 4,
    REFUNDING = 5,
    REFUNDED = 6,
    DISPUTED = 7,
    TIMEOUT = 8
};

/**
 * Listing categories
 */
enum class ListingCategory : uint8_t {
    GENERAL = 0,
    DIGITAL_GOODS = 1,
    PHYSICAL_GOODS = 2,
    SERVICES = 3,
    CRYPTO = 4,
    OTHER = 255
};

inline const char* ListingCategoryToString(ListingCategory cat) {
    switch (cat) {
    case ListingCategory::GENERAL:       return "General";
    case ListingCategory::DIGITAL_GOODS: return "Digital Goods";
    case ListingCategory::PHYSICAL_GOODS: return "Physical Goods";
    case ListingCategory::SERVICES:      return "Services";
    case ListingCategory::CRYPTO:        return "Crypto";
    case ListingCategory::OTHER:         return "Other";
    }
    return "Unknown";
}

inline const char* ListingStatusToString(ListingStatus status) {
    switch (status) {
    case ListingStatus::DRAFT:        return "Draft";
    case ListingStatus::ACTIVE:       return "Active";
    case ListingStatus::PENDING_SALE: return "Pending Sale";
    case ListingStatus::SOLD:         return "Sold";
    case ListingStatus::CANCELLED:    return "Cancelled";
    case ListingStatus::EXPIRED:      return "Expired";
    }
    return "Unknown";
}

inline const char* OrderStatusToString(OrderStatus status) {
    switch (status) {
    case OrderStatus::CREATED:   return "Created";
    case OrderStatus::FUNDED:    return "Funded";
    case OrderStatus::SHIPPED:   return "Shipped";
    case OrderStatus::DELIVERED: return "Delivered";
    case OrderStatus::COMPLETED: return "Completed";
    case OrderStatus::DISPUTED:  return "Disputed";
    case OrderStatus::REFUNDED:  return "Refunded";
    case OrderStatus::CANCELLED: return "Cancelled";
    case OrderStatus::EXPIRED:   return "Expired";
    }
    return "Unknown";
}

// Constants
static constexpr int64_t DEFAULT_LISTING_EXPIRY_SECONDS = 7 * 24 * 60 * 60; // 1 week
static constexpr int DEFAULT_ESCROW_TIMEOUT_BLOCKS = 1008; // ~1 week
static constexpr double MARKETPLACE_FEE_PERCENT = 0.5; // 0.5% platform fee

} // namespace marketplace

#endif // CORAL_MARKETPLACE_TYPES_H
