// Copyright (c) 2025 The Choral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <choral/subnet_interface.h>
#include <logging.h>

bool SubnetRegistry::RegisterSubnet(std::shared_ptr<ISubnet> subnet)
{
    if (!subnet) {
        LogPrintf("SubnetRegistry: Attempted to register null subnet\n");
        return false;
    }

    SubnetId id = subnet->GetSubnetId();
    if (subnets.count(id) > 0) {
        LogPrintf("SubnetRegistry: Subnet %s already registered\n", id.ToString());
        return false;
    }

    subnets[id] = subnet;
    LogPrintf("SubnetRegistry: Registered subnet %s (type: %s)\n",
              id.ToString(), subnet->GetSubnetType());
    return true;
}

std::shared_ptr<ISubnet> SubnetRegistry::GetSubnet(const SubnetId& subnet_id) const
{
    auto it = subnets.find(subnet_id);
    if (it == subnets.end()) {
        return nullptr;
    }
    return it->second;
}

bool SubnetRegistry::HasSubnet(const SubnetId& subnet_id) const
{
    return subnets.count(subnet_id) > 0;
}

std::vector<SubnetId> SubnetRegistry::GetAllSubnetIds() const
{
    std::vector<SubnetId> ids;
    for (const auto& pair : subnets) {
        ids.push_back(pair.first);
    }
    return ids;
}

SubnetRegistry& SubnetRegistry::GetGlobalRegistry()
{
    static SubnetRegistry registry;
    return registry;
}
