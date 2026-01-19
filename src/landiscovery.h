// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CORAL_LANDISCOVERY_H
#define CORAL_LANDISCOVERY_H

#include <netaddress.h>
#include <sync.h>
#include <threadinterrupt.h>

#include <atomic>
#include <thread>
#include <vector>
#include <functional>

/**
 * Local Area Network peer discovery using UDP broadcast.
 * Allows Coral nodes on the same LAN to find each other automatically.
 */
class LANDiscovery
{
public:
    // Callback when a peer is discovered
    using PeerCallback = std::function<void(const CService&)>;

    LANDiscovery();
    ~LANDiscovery();

    // Start discovery (broadcaster + listener)
    bool Start(uint16_t port, PeerCallback callback);

    // Stop discovery
    void Stop();

    // Check if running
    bool IsRunning() const { return m_running; }

    // Get discovered peers
    std::vector<CService> GetDiscoveredPeers() const;

    // Manually trigger a broadcast
    void Broadcast();

    // Discovery magic bytes: "CORAL" + version
    static constexpr uint8_t DISCOVERY_MAGIC[8] = {'C', 'O', 'R', 'A', 'L', 'D', 'I', 'S'};
    static constexpr uint16_t DISCOVERY_PORT = 8335; // One above mainnet P2P port

private:
    void BroadcastThread();
    void ListenerThread();
    bool SendBroadcast();

    std::atomic<bool> m_running{false};
    uint16_t m_port{0};
    PeerCallback m_callback;

    std::thread m_broadcast_thread;
    std::thread m_listener_thread;
    CThreadInterrupt m_interrupt;

    mutable Mutex m_peers_mutex;
    std::vector<CService> m_discovered_peers GUARDED_BY(m_peers_mutex);

    // UDP socket for broadcast
    int m_socket{-1};
};

#endif // CORAL_LANDISCOVERY_H
