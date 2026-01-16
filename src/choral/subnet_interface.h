// Copyright (c) 2025 The Choral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CHORAL_SUBNET_INTERFACE_H
#define CHORAL_SUBNET_INTERFACE_H

#include <choral/choral_primitives.h>
#include <uint256.h>
#include <string>
#include <memory>
#include <optional>

/**
 * Subnet Interface SDK
 *
 * Defines the strict interface for subnet implementations.
 * Subnets must implement this interface to be compatible with Choral L1.
 */

/** Result type for subnet operations */
template <typename T>
struct SubnetResult {
    bool success;
    T value;
    std::string error;

    static SubnetResult<T> Ok(T val) {
        return {true, val, ""};
    }

    static SubnetResult<T> Err(const std::string& err) {
        return {false, T(), err};
    }
};

/** Subnet state handle - opaque pointer to subnet-specific state */
struct SubnetStateHandle {
    std::shared_ptr<void> data;
    SubnetId subnet_id;
    uint64_t height;
    uint256 state_root;

    SubnetStateHandle() : height(0) {}
};

/**
 * ISubnet - Abstract interface for all subnets
 *
 * Every subnet implementation must inherit from this interface
 * and implement all required methods.
 */
class ISubnet {
public:
    virtual ~ISubnet() = default;

    /** Get the subnet ID */
    virtual SubnetId GetSubnetId() const = 0;

    /** Get the subnet name/type */
    virtual std::string GetSubnetType() const = 0;

    /**
     * Validate a subnet update envelope against prior state
     *
     * @param envelope The update to validate
     * @param prior_state_root The state root before this update
     * @return Result containing new state root or error
     */
    virtual SubnetResult<uint256> ValidateUpdate(
        const SubnetUpdateEnvelope& envelope,
        const uint256& prior_state_root) const = 0;

    /**
     * Apply a validated update to the state
     *
     * @param envelope The update to apply
     * @param state The current state handle
     * @return Result containing new state handle or error
     */
    virtual SubnetResult<SubnetStateHandle> ApplyUpdate(
        const SubnetUpdateEnvelope& envelope,
        const SubnetStateHandle& state) const = 0;

    /**
     * Compute state root from state handle
     *
     * @param state The state handle
     * @return The Merkle root of the state
     */
    virtual uint256 ComputeStateRoot(const SubnetStateHandle& state) const = 0;

    /**
     * Encode anchor commitment
     *
     * @param subnet_id The subnet identifier
     * @param subnet_height The subnet's internal height
     * @param state_root The state root
     * @param tx_root The transaction/update root
     * @param prev_anchor The previous anchor
     * @return The anchor hash
     */
    virtual uint256 EncodeAnchor(
        const SubnetId& subnet_id,
        uint64_t subnet_height,
        const uint256& state_root,
        const uint256& tx_root,
        const uint256& prev_anchor) const = 0;

    /**
     * Verify a message proof (optional for v0)
     *
     * @param msg The message to verify
     * @param proof The proof data
     * @return true if proof is valid
     */
    virtual bool VerifyMessageProof(
        const CrossSubnetMessage& msg,
        const std::vector<unsigned char>& proof) const {
        return true; // Optional in v0
    }

    /**
     * Get genesis state for this subnet
     *
     * @return The genesis state handle
     */
    virtual SubnetStateHandle GetGenesisState() const = 0;

    /**
     * Serialize state to bytes for storage
     *
     * @param state The state to serialize
     * @return Serialized bytes
     */
    virtual std::vector<unsigned char> SerializeState(const SubnetStateHandle& state) const = 0;

    /**
     * Deserialize state from bytes
     *
     * @param data The serialized data
     * @return The state handle or error
     */
    virtual SubnetResult<SubnetStateHandle> DeserializeState(
        const std::vector<unsigned char>& data) const = 0;
};

/**
 * Subnet Registry
 *
 * Manages all registered subnets and provides lookup functionality.
 */
class SubnetRegistry {
private:
    std::map<SubnetId, std::shared_ptr<ISubnet>> subnets;

public:
    /**
     * Register a subnet implementation
     *
     * @param subnet The subnet to register
     * @return true if registered successfully
     */
    bool RegisterSubnet(std::shared_ptr<ISubnet> subnet);

    /**
     * Get subnet by ID
     *
     * @param subnet_id The subnet identifier
     * @return The subnet or nullptr if not found
     */
    std::shared_ptr<ISubnet> GetSubnet(const SubnetId& subnet_id) const;

    /**
     * Check if subnet is registered
     *
     * @param subnet_id The subnet identifier
     * @return true if subnet is registered
     */
    bool HasSubnet(const SubnetId& subnet_id) const;

    /**
     * Get all registered subnet IDs
     *
     * @return Vector of subnet IDs
     */
    std::vector<SubnetId> GetAllSubnetIds() const;

    /**
     * Get global subnet registry instance
     *
     * @return Reference to global registry
     */
    static SubnetRegistry& GetGlobalRegistry();
};

#endif // CHORAL_SUBNET_INTERFACE_H
