// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CORAL_MARKETPLACE_ORDER_H
#define CORAL_MARKETPLACE_ORDER_H

#include <marketplace/marketplacetypes.h>
#include <consensus/amount.h>
#include <key_io.h>
#include <uint256.h>
#include <script/standard.h>
#include <script/script.h>
#include <serialize.h>
#include <pubkey.h>

#include <string>
#include <vector>

namespace marketplace {

/**
 * Order message for buyer-seller communication (encrypted in future)
 */
struct OrderMessage
{
    uint256 messageId;
    CTxDestination sender;
    std::string content;
    int64_t timestamp;

    SERIALIZE_METHODS(OrderMessage, obj)
    {
        READWRITE(obj.messageId);
        std::string addrStr;
        if (!ser_action.ForRead()) {
            addrStr = EncodeDestination(obj.sender);
        }
        READWRITE(addrStr);
        if (ser_action.ForRead()) {
            obj.sender = DecodeDestination(addrStr);
        }
        READWRITE(obj.content);
        READWRITE(obj.timestamp);
    }
};

/**
 * MarketOrder represents a purchase transaction between buyer and seller.
 * Contains escrow information and state tracking.
 */
struct MarketOrder
{
    // Identifiers
    uint256 orderId;
    uint256 listingId;

    // Participants
    CTxDestination buyerAddress;
    CPubKey buyerPubKey;
    CTxDestination sellerAddress;
    CPubKey sellerPubKey;
    CTxDestination arbiterAddress;      // Optional for disputes
    CPubKey arbiterPubKey;

    // Financial
    CAmount amount{0};                  // Item price
    CAmount escrowAmount{0};            // Total in escrow (price + fees)
    CAmount networkFee{0};              // Mining fee
    CAmount serviceFee{0};              // Platform fee

    // Escrow
    uint256 escrowTxid;
    uint256 releaseTxid;
    CTxDestination escrowAddress;
    CScript escrowScript;

    // State
    OrderStatus status{OrderStatus::CREATED};
    int64_t createdAt{0};
    int64_t fundedAt{0};
    int64_t shippedAt{0};
    int64_t deliveredAt{0};
    int64_t completedAt{0};
    int creationHeight{0};
    int expirationBlocks{DEFAULT_ESCROW_TIMEOUT_BLOCKS};

    // Communication
    std::vector<OrderMessage> messages;

    // Dispute info
    std::string disputeReason;
    int64_t disputeOpenedAt{0};
    bool disputeResolvedForBuyer{false};

    MarketOrder() = default;

    // Compute order ID hash
    uint256 ComputeHash() const;

    // Validate order data
    bool IsValid() const;

    // Check if order is expired (based on block height)
    bool IsExpired(int currentHeight) const {
        return currentHeight > (creationHeight + expirationBlocks);
    }

    // Check if order can be funded
    bool CanFund() const {
        return status == OrderStatus::CREATED;
    }

    // Check if order can be marked as shipped
    bool CanShip() const {
        return status == OrderStatus::FUNDED;
    }

    // Check if order can be confirmed as delivered
    bool CanConfirmDelivery() const {
        return status == OrderStatus::SHIPPED;
    }

    // Check if order can be disputed
    bool CanDispute() const {
        return status == OrderStatus::FUNDED ||
               status == OrderStatus::SHIPPED;
    }

    SERIALIZE_METHODS(MarketOrder, obj)
    {
        READWRITE(obj.orderId);
        READWRITE(obj.listingId);

        // Serialize addresses as strings
        std::string buyerAddrStr, sellerAddrStr, arbiterAddrStr, escrowAddrStr;
        SER_WRITE(obj, buyerAddrStr = EncodeDestination(obj.buyerAddress));
        SER_WRITE(obj, sellerAddrStr = EncodeDestination(obj.sellerAddress));
        SER_WRITE(obj, arbiterAddrStr = EncodeDestination(obj.arbiterAddress));
        SER_WRITE(obj, escrowAddrStr = EncodeDestination(obj.escrowAddress));
        READWRITE(buyerAddrStr);
        READWRITE(sellerAddrStr);
        READWRITE(arbiterAddrStr);
        READWRITE(escrowAddrStr);
        SER_READ(obj, obj.buyerAddress = DecodeDestination(buyerAddrStr));
        SER_READ(obj, obj.sellerAddress = DecodeDestination(sellerAddrStr));
        SER_READ(obj, obj.arbiterAddress = DecodeDestination(arbiterAddrStr));
        SER_READ(obj, obj.escrowAddress = DecodeDestination(escrowAddrStr));

        READWRITE(obj.buyerPubKey);
        READWRITE(obj.sellerPubKey);
        READWRITE(obj.arbiterPubKey);

        READWRITE(obj.amount);
        READWRITE(obj.escrowAmount);
        READWRITE(obj.networkFee);
        READWRITE(obj.serviceFee);

        READWRITE(obj.escrowTxid);
        READWRITE(obj.releaseTxid);
        READWRITE(obj.escrowScript);

        // Serialize status as uint8_t
        uint8_t statusInt;
        SER_WRITE(obj, statusInt = static_cast<uint8_t>(obj.status));
        READWRITE(statusInt);
        SER_READ(obj, obj.status = static_cast<OrderStatus>(statusInt));

        READWRITE(obj.createdAt);
        READWRITE(obj.fundedAt);
        READWRITE(obj.shippedAt);
        READWRITE(obj.deliveredAt);
        READWRITE(obj.completedAt);
        READWRITE(obj.creationHeight);
        READWRITE(obj.expirationBlocks);

        READWRITE(obj.messages);

        READWRITE(obj.disputeReason);
        READWRITE(obj.disputeOpenedAt);
        READWRITE(obj.disputeResolvedForBuyer);
    }
};

} // namespace marketplace

#endif // CORAL_MARKETPLACE_ORDER_H
