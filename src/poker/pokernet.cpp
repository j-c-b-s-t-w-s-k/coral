// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <poker/pokernet.h>
#include <hash.h>
#include <netmessagemaker.h>
#include <random.h>
#include <util/time.h>

namespace poker {

std::unique_ptr<PokerNetManager> g_poker_net;

PokerNetManager::PokerNetManager() {}

bool PokerNetManager::Initialize(const CKey& signingKey) {
    LOCK(cs_poker);

    m_signingKey = signingKey;
    m_pubKey = signingKey.GetPubKey();

    return true;
}

void PokerNetManager::BroadcastGameAnnounce(const MsgAnnounce& announce) {
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << announce;

    // Broadcast to all connected peers
    // This would integrate with the connman
    // For now, store locally
    LOCK(cs_poker);
    m_availableGames[announce.gameId] = announce;
}

const std::map<GameId, MsgAnnounce>& PokerNetManager::GetAvailableGames() const {
    LOCK(cs_poker);
    return m_availableGames;
}

bool PokerNetManager::CreateGame(const GameConfig& config, GameId& gameIdOut) {
    LOCK(cs_poker);

    // Generate game ID from config and timestamp
    CHashWriter hasher(SER_GETHASH, 0);
    hasher << config.variant << config.smallBlind << config.bigBlind
           << config.minBuyIn << config.maxBuyIn << config.maxPlayers
           << GetTime() << m_pubKey;
    gameIdOut = hasher.GetHash();

    // Create the game
    auto game = PokerGameFactory::CreateGame(gameIdOut, config);
    if (!game) {
        return false;
    }

    m_activeGames[gameIdOut] = std::move(game);

    // Create and broadcast announcement
    MsgAnnounce announce;
    announce.gameId = gameIdOut;
    announce.config = config;
    announce.hostPubKey = m_pubKey;
    announce.hostName = "Host"; // TODO: Get from wallet
    announce.currentPlayers = 0;
    announce.timestamp = GetTime();

    uint256 hash = HashMessage(announce);
    SignMessage(announce.signature, hash);

    BroadcastGameAnnounce(announce);

    return true;
}

bool PokerNetManager::JoinGame(const GameId& gameId) {
    LOCK(cs_poker);

    auto it = m_availableGames.find(gameId);
    if (it == m_availableGames.end()) {
        return false;
    }

    MsgJoin joinMsg;
    joinMsg.gameId = gameId;
    joinMsg.playerPubKey = m_pubKey;
    joinMsg.playerName = "Player"; // TODO: Get from wallet
    joinMsg.timestamp = GetTime();

    uint256 hash = HashMessage(joinMsg);
    SignMessage(joinMsg.signature, hash);

    // Send to game host
    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << joinMsg;

    // Would send via connman to the host
    return true;
}

bool PokerNetManager::LeaveGame(const GameId& gameId, const std::string& reason) {
    LOCK(cs_poker);

    MsgLeave leaveMsg;
    leaveMsg.gameId = gameId;
    leaveMsg.playerPubKey = m_pubKey;
    leaveMsg.reason = reason;
    leaveMsg.timestamp = GetTime();

    uint256 hash = HashMessage(leaveMsg);
    SignMessage(leaveMsg.signature, hash);

    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << leaveMsg;

    SendToGame(gameId, NetMsgType::PKRLEAVE, ss);

    m_activeGames.erase(gameId);
    return true;
}

std::shared_ptr<PokerGame> PokerNetManager::GetGame(const GameId& gameId) {
    LOCK(cs_poker);
    auto it = m_activeGames.find(gameId);
    if (it != m_activeGames.end()) {
        return it->second;
    }
    return nullptr;
}

bool PokerNetManager::ProcessAnnounce(CNode* pfrom, const MsgAnnounce& msg) {
    LOCK(cs_poker);

    // Verify signature
    uint256 hash = HashMessage(msg);
    if (!VerifyMessage(msg.hostPubKey, msg.signature, hash)) {
        return false;
    }

    // Check timestamp (not too old)
    int64_t now = GetTime();
    if (msg.timestamp < now - 3600 || msg.timestamp > now + 60) {
        return false; // Too old or in the future
    }

    // Store announcement
    m_availableGames[msg.gameId] = msg;

    // Track the node for this game
    m_gameNodes[msg.gameId].insert(pfrom->GetId());

    // Notify UI
    if (m_onGameAnnounce) {
        m_onGameAnnounce(msg);
    }

    return true;
}

bool PokerNetManager::ProcessJoin(CNode* pfrom, const MsgJoin& msg) {
    LOCK(cs_poker);

    // Verify signature
    uint256 hash = HashMessage(msg);
    if (!VerifyMessage(msg.playerPubKey, msg.signature, hash)) {
        return false;
    }

    // Check if we're hosting this game
    auto gameIt = m_activeGames.find(msg.gameId);
    if (gameIt == m_activeGames.end()) {
        return false;
    }

    auto& game = gameIt->second;

    // Add player
    PlayerId playerId;
    memcpy(playerId.begin(), msg.playerPubKey.GetID().begin(), 20);

    Player player(playerId, msg.playerPubKey, msg.playerName);
    if (!game->AddPlayer(player)) {
        return false;
    }

    // Track node
    m_gameNodes[msg.gameId].insert(pfrom->GetId());

    // Send acceptance
    MsgAccept acceptMsg;
    acceptMsg.gameId = msg.gameId;
    acceptMsg.playerPubKey = msg.playerPubKey;
    acceptMsg.seatNumber = game->GetPlayers().size() - 1;
    acceptMsg.timestamp = GetTime();

    uint256 acceptHash = HashMessage(acceptMsg);
    SignMessage(acceptMsg.signature, acceptHash);

    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << acceptMsg;
    SendToPlayer(pfrom->GetId(), NetMsgType::PKRACCEPT, ss);

    // Notify UI
    if (m_onPlayerJoin) {
        m_onPlayerJoin(msg.gameId, msg);
    }

    return true;
}

bool PokerNetManager::ProcessAccept(CNode* pfrom, const MsgAccept& msg) {
    LOCK(cs_poker);

    // We received acceptance - add ourselves to the game
    auto gameIt = m_activeGames.find(msg.gameId);
    if (gameIt != m_activeGames.end()) {
        return true; // Already in game
    }

    // Get game config from announcement
    auto announceIt = m_availableGames.find(msg.gameId);
    if (announceIt == m_availableGames.end()) {
        return false;
    }

    // Create local game instance
    auto game = PokerGameFactory::CreateGame(msg.gameId, announceIt->second.config);
    if (!game) {
        return false;
    }

    m_activeGames[msg.gameId] = std::move(game);
    m_gameNodes[msg.gameId].insert(pfrom->GetId());

    return true;
}

bool PokerNetManager::ProcessReady(CNode* pfrom, const MsgReady& msg) {
    LOCK(cs_poker);

    auto gameIt = m_activeGames.find(msg.gameId);
    if (gameIt == m_activeGames.end()) {
        return false;
    }

    // Verify escrow transaction exists
    // TODO: Verify with mempool/chain

    return true;
}

bool PokerNetManager::ProcessStart(CNode* pfrom, const MsgStart& msg) {
    LOCK(cs_poker);

    auto gameIt = m_activeGames.find(msg.gameId);
    if (gameIt == m_activeGames.end()) {
        return false;
    }

    auto& game = gameIt->second;

    // Verify signature from host
    uint256 hash = HashMessage(msg);
    auto announceIt = m_availableGames.find(msg.gameId);
    if (announceIt == m_availableGames.end()) {
        return false;
    }

    if (!VerifyMessage(announceIt->second.hostPubKey, msg.signature, hash)) {
        return false;
    }

    // Start the game
    game->StartGame();

    if (m_onPhaseChange) {
        m_onPhaseChange(msg.gameId, game->GetPhase());
    }

    return true;
}

bool PokerNetManager::ProcessKey(CNode* pfrom, const MsgKey& msg) {
    LOCK(cs_poker);

    auto gameIt = m_activeGames.find(msg.gameId);
    if (gameIt == m_activeGames.end()) {
        return false;
    }

    auto& game = gameIt->second;
    auto& mentalPoker = game->GetMentalPoker();

    // Find player position
    size_t playerPos = 0;
    bool found = false;
    for (const auto& player : game->GetPlayers()) {
        if (player.GetPubKey() == msg.playerPubKey) {
            found = true;
            break;
        }
        playerPos++;
    }

    if (!found) {
        return false;
    }

    if (!msg.isReveal) {
        // Commitment phase
        mentalPoker.ReceiveCommitment(playerPos, msg.keyCommitment);
    } else {
        // Reveal phase
        mentalPoker.ReceivePublicKey(playerPos, msg.publicKey);
    }

    return true;
}

bool PokerNetManager::ProcessDeck(CNode* pfrom, const MsgDeck& msg) {
    LOCK(cs_poker);

    auto gameIt = m_activeGames.find(msg.gameId);
    if (gameIt == m_activeGames.end()) {
        return false;
    }

    auto& game = gameIt->second;
    game->GetMentalPoker().SetDeck(msg.deck);

    return true;
}

bool PokerNetManager::ProcessReveal(CNode* pfrom, const MsgReveal& msg) {
    LOCK(cs_poker);

    auto gameIt = m_activeGames.find(msg.gameId);
    if (gameIt == m_activeGames.end()) {
        return false;
    }

    auto& game = gameIt->second;
    auto& mentalPoker = game->GetMentalPoker();

    // Find player position
    size_t playerPos = 0;
    for (const auto& player : game->GetPlayers()) {
        if (player.GetPubKey() == msg.playerPubKey) {
            break;
        }
        playerPos++;
    }

    mentalPoker.ReceivePartialDecrypt(msg.cardIndex, playerPos, msg.partialDecrypt);

    return true;
}

bool PokerNetManager::ProcessAction(CNode* pfrom, const MsgAction& msg) {
    LOCK(cs_poker);

    auto gameIt = m_activeGames.find(msg.gameId);
    if (gameIt == m_activeGames.end()) {
        return false;
    }

    // Verify signature
    uint256 hash = HashMessage(msg);
    if (!VerifyMessage(msg.playerPubKey, msg.signature, hash)) {
        return false;
    }

    auto& game = gameIt->second;

    // Find player ID
    PlayerId playerId;
    memcpy(playerId.begin(), msg.playerPubKey.GetID().begin(), 20);

    // Process the action
    if (!game->ProcessAction(playerId, msg.action, msg.amount)) {
        return false;
    }

    // Notify UI
    if (m_onAction) {
        m_onAction(msg.gameId, msg);
    }

    return true;
}

bool PokerNetManager::ProcessState(CNode* pfrom, const MsgState& msg) {
    LOCK(cs_poker);

    auto gameIt = m_activeGames.find(msg.gameId);
    if (gameIt == m_activeGames.end()) {
        return false;
    }

    // Verify state hash matches our local state
    // TODO: Implement state verification

    return true;
}

bool PokerNetManager::ProcessSettle(CNode* pfrom, const MsgSettle& msg) {
    LOCK(cs_poker);

    auto gameIt = m_activeGames.find(msg.gameId);
    if (gameIt == m_activeGames.end()) {
        return false;
    }

    auto& game = gameIt->second;
    auto escrow = game->GetEscrow();

    if (!escrow) {
        return false;
    }

    // Add settlement signature
    escrow->AddSettlementSignature(msg.playerPubKey, msg.settlementSig);

    // Check if we can finalize
    if (escrow->IsSettlementFullySigned()) {
        CTransaction settleTx = escrow->GetSignedSettlementTransaction();
        // TODO: Broadcast settlement transaction
    }

    return true;
}

bool PokerNetManager::ProcessLeave(CNode* pfrom, const MsgLeave& msg) {
    LOCK(cs_poker);

    auto gameIt = m_activeGames.find(msg.gameId);
    if (gameIt == m_activeGames.end()) {
        return false;
    }

    // Verify signature
    uint256 hash = HashMessage(msg);
    if (!VerifyMessage(msg.playerPubKey, msg.signature, hash)) {
        return false;
    }

    auto& game = gameIt->second;

    PlayerId playerId;
    memcpy(playerId.begin(), msg.playerPubKey.GetID().begin(), 20);

    game->RemovePlayer(playerId);

    // Remove from node tracking
    if (pfrom) {
        m_gameNodes[msg.gameId].erase(pfrom->GetId());
    }

    return true;
}

void PokerNetManager::SendToGame(const GameId& gameId, const std::string& msgType,
                                  const CDataStream& data) {
    LOCK(cs_poker);

    auto nodeIt = m_gameNodes.find(gameId);
    if (nodeIt == m_gameNodes.end()) {
        return;
    }

    // Would use connman to send to all nodes
    // g_connman->ForNode(nodeId, [&](CNode* pnode) { ... });
}

void PokerNetManager::SendToPlayer(NodeId nodeId, const std::string& msgType,
                                    const CDataStream& data) {
    // Would use connman to send to specific node
}

bool PokerNetManager::SendAction(const GameId& gameId, Action action, CAmount amount) {
    LOCK(cs_poker);

    auto gameIt = m_activeGames.find(gameId);
    if (gameIt == m_activeGames.end()) {
        return false;
    }

    MsgAction msg;
    msg.gameId = gameId;
    msg.playerPubKey = m_pubKey;
    msg.action = action;
    msg.amount = amount;
    msg.timestamp = GetTime();

    uint256 hash = HashMessage(msg);
    SignMessage(msg.signature, hash);

    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << msg;

    SendToGame(gameId, NetMsgType::PKRACTION, ss);

    // Also process locally
    PlayerId playerId;
    memcpy(playerId.begin(), m_pubKey.GetID().begin(), 20);
    gameIt->second->ProcessAction(playerId, action, amount);

    return true;
}

bool PokerNetManager::SendReady(const GameId& gameId, const uint256& escrowTxId) {
    LOCK(cs_poker);

    MsgReady msg;
    msg.gameId = gameId;
    msg.playerPubKey = m_pubKey;
    msg.escrowTxId = escrowTxId;
    msg.timestamp = GetTime();

    uint256 hash = HashMessage(msg);
    SignMessage(msg.signature, hash);

    CDataStream ss(SER_NETWORK, PROTOCOL_VERSION);
    ss << msg;

    SendToGame(gameId, NetMsgType::PKRREADY, ss);

    return true;
}

bool PokerNetManager::SignMessage(std::vector<unsigned char>& sig, const uint256& hash) const {
    return m_signingKey.Sign(hash, sig);
}

bool PokerNetManager::VerifyMessage(const CPubKey& pubKey,
                                     const std::vector<unsigned char>& sig,
                                     const uint256& hash) const {
    return pubKey.Verify(hash, sig);
}

template<typename T>
uint256 PokerNetManager::HashMessage(const T& msg) const {
    CHashWriter hasher(SER_GETHASH, 0);
    hasher << msg;
    return hasher.GetHash();
}

void PokerNetManager::CleanupExpiredAnnouncements() {
    LOCK(cs_poker);

    int64_t now = GetTime();
    int64_t maxAge = 3600; // 1 hour

    for (auto it = m_availableGames.begin(); it != m_availableGames.end(); ) {
        if (it->second.timestamp < now - maxAge) {
            it = m_availableGames.erase(it);
        } else {
            ++it;
        }
    }
}

bool InitPokerNet() {
    g_poker_net = std::make_unique<PokerNetManager>();
    return true;
}

void ShutdownPokerNet() {
    g_poker_net.reset();
}

} // namespace poker
