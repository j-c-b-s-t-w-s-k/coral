// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <landiscovery.h>

#include <logging.h>
#include <netbase.h>
#include <util/time.h>

#ifndef WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif
#include <cstring>

constexpr uint8_t LANDiscovery::DISCOVERY_MAGIC[8];

LANDiscovery::LANDiscovery() = default;

LANDiscovery::~LANDiscovery()
{
    Stop();
}

bool LANDiscovery::Start(uint16_t port, PeerCallback callback)
{
    if (m_running) return false;

    m_port = port;
    m_callback = callback;

    // Create UDP socket
    m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (m_socket < 0) {
        LogPrintf("LANDiscovery: Failed to create socket\n");
        return false;
    }

    // Enable broadcast
    int broadcast = 1;
    if (setsockopt(m_socket, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
        LogPrintf("LANDiscovery: Failed to enable broadcast\n");
        close(m_socket);
        m_socket = -1;
        return false;
    }

    // Allow address reuse
    int reuse = 1;
    setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
#ifdef SO_REUSEPORT
    setsockopt(m_socket, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
#endif

    // Bind to discovery port
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(DISCOVERY_PORT);

    if (bind(m_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        LogPrintf("LANDiscovery: Failed to bind to port %d\n", DISCOVERY_PORT);
        close(m_socket);
        m_socket = -1;
        return false;
    }

    m_running = true;
    m_interrupt.reset();

    // Start threads
    m_broadcast_thread = std::thread(&LANDiscovery::BroadcastThread, this);
    m_listener_thread = std::thread(&LANDiscovery::ListenerThread, this);

    LogPrintf("LANDiscovery: Started on port %d (P2P port %d)\n", DISCOVERY_PORT, m_port);
    return true;
}

void LANDiscovery::Stop()
{
    if (!m_running) return;

    m_running = false;
    m_interrupt();

    if (m_socket >= 0) {
        close(m_socket);
        m_socket = -1;
    }

    if (m_broadcast_thread.joinable()) m_broadcast_thread.join();
    if (m_listener_thread.joinable()) m_listener_thread.join();

    LogPrintf("LANDiscovery: Stopped\n");
}

std::vector<CService> LANDiscovery::GetDiscoveredPeers() const
{
    LOCK(m_peers_mutex);
    return m_discovered_peers;
}

void LANDiscovery::Broadcast()
{
    SendBroadcast();
}

bool LANDiscovery::SendBroadcast()
{
    if (m_socket < 0) return false;

    // Build discovery packet: MAGIC (8) + PORT (2) = 10 bytes
    uint8_t packet[10];
    memcpy(packet, DISCOVERY_MAGIC, 8);
    packet[8] = (m_port >> 8) & 0xFF;
    packet[9] = m_port & 0xFF;

    // Broadcast address
    struct sockaddr_in broadcast_addr;
    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_addr.s_addr = INADDR_BROADCAST;
    broadcast_addr.sin_port = htons(DISCOVERY_PORT);

    ssize_t sent = sendto(m_socket, packet, sizeof(packet), 0,
                          (struct sockaddr*)&broadcast_addr, sizeof(broadcast_addr));

    if (sent < 0) {
        LogPrint(BCLog::NET, "LANDiscovery: Broadcast failed\n");
        return false;
    }

    LogPrint(BCLog::NET, "LANDiscovery: Sent broadcast (port %d)\n", m_port);
    return true;
}

void LANDiscovery::BroadcastThread()
{
    LogPrintf("LANDiscovery: Broadcast thread started\n");

    // Initial broadcast
    SendBroadcast();

    // Broadcast every 60 seconds
    while (m_running && !m_interrupt.sleep_for(std::chrono::seconds(60))) {
        SendBroadcast();
    }
}

void LANDiscovery::ListenerThread()
{
    LogPrintf("LANDiscovery: Listener thread started\n");

    uint8_t buffer[64];
    struct sockaddr_in sender_addr;
    socklen_t sender_len = sizeof(sender_addr);

    // Set receive timeout
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    while (m_running) {
        ssize_t received = recvfrom(m_socket, buffer, sizeof(buffer), 0,
                                    (struct sockaddr*)&sender_addr, &sender_len);

        if (received < 0) {
            // Timeout or error, just continue
            continue;
        }

        if (received < 10) continue;

        // Verify magic
        if (memcmp(buffer, DISCOVERY_MAGIC, 8) != 0) {
            continue;
        }

        // Extract port
        uint16_t peer_port = (buffer[8] << 8) | buffer[9];

        // Get sender IP
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &sender_addr.sin_addr, ip_str, sizeof(ip_str));

        // Skip our own broadcasts
        // (This is a simple check - could be improved)
        if (peer_port == m_port) {
            // Could be us, but also could be another node on same port
            // For now, still add it - the connection logic will handle duplicates
        }

        // Create service address
        CNetAddr netaddr;
        if (!LookupHost(ip_str, netaddr, false)) {
            continue;
        }
        CService service(netaddr, peer_port);

        // Check if already known
        bool already_known = false;
        {
            LOCK(m_peers_mutex);
            for (const auto& peer : m_discovered_peers) {
                if (peer == service) {
                    already_known = true;
                    break;
                }
            }
            if (!already_known) {
                m_discovered_peers.push_back(service);
                LogPrintf("LANDiscovery: Found peer %s:%d\n", ip_str, peer_port);
            }
        }

        // Notify callback
        if (!already_known && m_callback) {
            m_callback(service);
        }
    }
}
