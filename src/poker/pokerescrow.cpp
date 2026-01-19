// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <poker/pokerescrow.h>
#include <script/sign.h>
#include <script/standard.h>
#include <policy/policy.h>
#include <hash.h>
#include <util/strencodings.h>

#include <algorithm>

namespace poker {

// PokerEscrow implementation

bool PokerEscrow::Initialize(const std::vector<CPubKey>& playerPubKeys,
                              CAmount buyInAmount,
                              int currentHeight) {
    if (playerPubKeys.size() < 2 || playerPubKeys.size() > 9) {
        return false;
    }

    m_creationHeight = currentHeight;
    m_players.clear();
    m_players.reserve(playerPubKeys.size());

    for (const auto& pubKey : playerPubKeys) {
        PlayerStake stake;
        stake.pubKey = pubKey;
        stake.amount = buyInAmount;
        m_players.push_back(stake);
    }

    // Create escrow script
    m_escrowScript = EscrowTxBuilder::CreateEscrowScript(playerPubKeys, m_timeoutBlocks);
    m_escrowScriptPubKey = GetScriptForDestination(ScriptHash(m_escrowScript));

    m_totalPot = 0;
    m_state = EscrowState::FUNDING;

    return true;
}

CTxDestination PokerEscrow::GetEscrowAddress() const {
    return ScriptHash(m_escrowScript);
}

bool PokerEscrow::AddPlayerFunding(const CPubKey& player,
                                    const COutPoint& utxo,
                                    CAmount amount) {
    for (auto& stake : m_players) {
        if (stake.pubKey == player) {
            stake.fundingUtxo = utxo;
            stake.amount = amount;
            return true;
        }
    }
    return false;
}

bool PokerEscrow::IsFullyFunded() const {
    for (const auto& stake : m_players) {
        if (stake.fundingUtxo.IsNull() || stake.amount <= 0) {
            return false;
        }
    }
    return true;
}

bool PokerEscrow::CreateFundingTransaction() {
    if (!IsFullyFunded()) {
        return false;
    }

    m_fundingTx = CMutableTransaction();
    m_fundingTx.nVersion = 2;
    m_fundingTx.nLockTime = 0;

    // Add inputs from each player
    for (const auto& stake : m_players) {
        CTxIn input(stake.fundingUtxo);
        m_fundingTx.vin.push_back(input);
        m_totalPot += stake.amount;
    }

    // Single output to escrow address
    CTxOut output(m_totalPot, m_escrowScriptPubKey);
    m_fundingTx.vout.push_back(output);

    return true;
}

bool PokerEscrow::SignFundingTransaction(const CKey& privateKey,
                                          std::vector<unsigned char>& signatureOut) {
    // Find the input for this key
    CPubKey pubKey = privateKey.GetPubKey();
    int inputIndex = -1;

    for (size_t i = 0; i < m_players.size(); ++i) {
        if (m_players[i].pubKey == pubKey) {
            inputIndex = i;
            break;
        }
    }

    if (inputIndex < 0) {
        return false;
    }

    // Sign the input
    // Note: This is simplified - real implementation needs proper script handling
    uint256 hash = SignatureHash(m_escrowScriptPubKey, m_fundingTx,
                                  inputIndex, SIGHASH_ALL, m_players[inputIndex].amount,
                                  SigVersion::WITNESS_V0);

    if (!privateKey.Sign(hash, signatureOut)) {
        return false;
    }

    signatureOut.push_back(SIGHASH_ALL);
    return true;
}

bool PokerEscrow::AddFundingSignature(const CPubKey& player,
                                       const std::vector<unsigned char>& signature) {
    for (auto& stake : m_players) {
        if (stake.pubKey == player) {
            stake.signature = signature;
            return true;
        }
    }
    return false;
}

bool PokerEscrow::CreateSettlementTransaction(const SettlementOutcome& outcome) {
    if (outcome.payouts.empty() || outcome.TotalPayout() > m_totalPot) {
        return false;
    }

    m_settlementTx = CMutableTransaction();
    m_settlementTx.nVersion = 2;
    m_settlementTx.nLockTime = 0;

    // Input from escrow
    CTxIn input;
    input.prevout = COutPoint(m_fundingTx.GetHash(), 0);
    m_settlementTx.vin.push_back(input);

    // Outputs to winners
    for (const auto& [pubKey, amount] : outcome.payouts) {
        if (amount > 0) {
            CTxOut output(amount, GetScriptForDestination(
                PKHash(pubKey.GetID())));
            m_settlementTx.vout.push_back(output);
        }
    }

    m_settlementSigs.clear();
    m_state = EscrowState::SETTLING;

    return true;
}

bool PokerEscrow::SignSettlementTransaction(const CKey& privateKey,
                                             std::vector<unsigned char>& signatureOut) {
    CPubKey pubKey = privateKey.GetPubKey();

    // Verify this player is part of the escrow
    bool isPlayer = false;
    for (const auto& stake : m_players) {
        if (stake.pubKey == pubKey) {
            isPlayer = true;
            break;
        }
    }

    if (!isPlayer) {
        return false;
    }

    // Sign the settlement transaction
    uint256 hash = SignatureHash(m_escrowScript, m_settlementTx,
                                  0, SIGHASH_ALL, m_totalPot,
                                  SigVersion::WITNESS_V0);

    if (!privateKey.Sign(hash, signatureOut)) {
        return false;
    }

    signatureOut.push_back(SIGHASH_ALL);
    return true;
}

bool PokerEscrow::AddSettlementSignature(const CPubKey& player,
                                          const std::vector<unsigned char>& signature) {
    // Verify player is part of escrow
    bool isPlayer = false;
    for (const auto& stake : m_players) {
        if (stake.pubKey == player) {
            isPlayer = true;
            break;
        }
    }

    if (!isPlayer) {
        return false;
    }

    m_settlementSigs[player] = signature;
    return true;
}

bool PokerEscrow::IsSettlementFullySigned() const {
    return m_settlementSigs.size() == m_players.size();
}

CTransaction PokerEscrow::GetSignedSettlementTransaction() const {
    CMutableTransaction tx = m_settlementTx;

    // Combine signatures into script witness
    std::vector<std::vector<unsigned char>> sigs;
    for (const auto& stake : m_players) {
        auto it = m_settlementSigs.find(stake.pubKey);
        if (it != m_settlementSigs.end()) {
            sigs.push_back(it->second);
        }
    }

    // Build witness: [sig1] [sig2] ... [sigN] [redeemScript]
    tx.vin[0].scriptWitness.stack.clear();
    tx.vin[0].scriptWitness.stack.push_back({}); // Empty for CHECKMULTISIG bug

    for (const auto& sig : sigs) {
        tx.vin[0].scriptWitness.stack.push_back(sig);
    }

    std::vector<unsigned char> scriptBytes(m_escrowScript.begin(),
                                            m_escrowScript.end());
    tx.vin[0].scriptWitness.stack.push_back(scriptBytes);

    return CTransaction(tx);
}

bool PokerEscrow::CreateTimeoutTransaction(int currentHeight,
                                            const CTxDestination& refundDest) {
    if (!CanTriggerTimeout(currentHeight)) {
        return false;
    }

    // Create transaction that spends using timeout path
    CMutableTransaction tx;
    tx.nVersion = 2;
    tx.nLockTime = 0;

    CTxIn input;
    input.prevout = COutPoint(m_fundingTx.GetHash(), 0);
    input.nSequence = m_timeoutBlocks; // Relative timelock
    tx.vin.push_back(input);

    // Refund to specified destination
    CScript refundScript = GetScriptForDestination(refundDest);
    CTxOut output(m_totalPot, refundScript);
    tx.vout.push_back(output);

    m_settlementTx = tx;
    m_state = EscrowState::TIMEOUT;

    return true;
}

bool PokerEscrow::CanTriggerTimeout(int currentHeight) const {
    return (currentHeight - m_creationHeight) >= m_timeoutBlocks;
}

CScript PokerEscrow::CreateMultisigScript(const std::vector<CPubKey>& pubKeys) const {
    return EscrowTxBuilder::CreateNofNMultisig(pubKeys);
}

CScript PokerEscrow::CreateTimeoutScript(const std::vector<CPubKey>& pubKeys,
                                          int timeoutBlocks) const {
    return EscrowTxBuilder::CreateEscrowScript(pubKeys, timeoutBlocks);
}

// EscrowTxBuilder implementation

CScript EscrowTxBuilder::CreateNofNMultisig(const std::vector<CPubKey>& pubKeys) {
    if (pubKeys.empty() || pubKeys.size() > 16) {
        return CScript();
    }

    CScript script;
    script << CScript::EncodeOP_N(pubKeys.size());

    for (const auto& pubKey : pubKeys) {
        script << ToByteVector(pubKey);
    }

    script << CScript::EncodeOP_N(pubKeys.size());
    script << OP_CHECKMULTISIG;

    return script;
}

CScript EscrowTxBuilder::CreateTimelockScript(const CScript& mainScript,
                                               int relativeBlocks) {
    CScript script;
    script << relativeBlocks << OP_CHECKSEQUENCEVERIFY << OP_DROP;
    script.insert(script.end(), mainScript.begin(), mainScript.end());
    return script;
}

CScript EscrowTxBuilder::CreateEscrowScript(const std::vector<CPubKey>& pubKeys,
                                             int timeoutBlocks) {
    // Create a script that allows either:
    // 1. N-of-N multisig (normal settlement)
    // 2. (N-1)-of-N multisig after timeout (for dispute resolution)

    CScript multisig = CreateNofNMultisig(pubKeys);

    // For simplicity, just use the N-of-N multisig
    // A more sophisticated version would use OP_IF branches
    return multisig;
}

bool EscrowTxBuilder::SignInput(CMutableTransaction& tx,
                                 size_t inputIndex,
                                 const CScript& scriptPubKey,
                                 const CKey& key,
                                 std::vector<unsigned char>& sigOut) {
    if (inputIndex >= tx.vin.size()) {
        return false;
    }

    uint256 hash = SignatureHash(scriptPubKey, tx, inputIndex,
                                  SIGHASH_ALL, 0, SigVersion::BASE);

    if (!key.Sign(hash, sigOut)) {
        return false;
    }

    sigOut.push_back(SIGHASH_ALL);
    return true;
}

CScript EscrowTxBuilder::CombineMultisigSignatures(
    const std::vector<std::vector<unsigned char>>& signatures,
    const CScript& redeemScript) {

    CScript script;
    script << OP_0; // CHECKMULTISIG bug workaround

    for (const auto& sig : signatures) {
        script << sig;
    }

    script << std::vector<unsigned char>(redeemScript.begin(), redeemScript.end());

    return script;
}

bool EscrowTxBuilder::VerifySignature(const CTransaction& tx,
                                       size_t inputIndex,
                                       const CScript& scriptPubKey,
                                       const std::vector<unsigned char>& sig,
                                       const CPubKey& pubKey) {
    if (inputIndex >= tx.vin.size() || sig.empty()) {
        return false;
    }

    // Extract SIGHASH type
    uint8_t sighashType = sig.back();
    std::vector<unsigned char> sigWithoutType(sig.begin(), sig.end() - 1);

    uint256 hash = SignatureHash(scriptPubKey, tx, inputIndex,
                                  sighashType, 0, SigVersion::BASE);

    return pubKey.Verify(hash, sigWithoutType);
}

// EscrowManager implementation

std::shared_ptr<PokerEscrow> EscrowManager::CreateEscrow(
    const GameId& gameId,
    const std::vector<CPubKey>& players,
    CAmount buyIn,
    int currentHeight) {

    auto escrow = std::make_shared<PokerEscrow>(gameId);

    if (!escrow->Initialize(players, buyIn, currentHeight)) {
        return nullptr;
    }

    m_escrows[gameId] = escrow;
    return escrow;
}

std::shared_ptr<PokerEscrow> EscrowManager::GetEscrow(const GameId& gameId) {
    auto it = m_escrows.find(gameId);
    if (it != m_escrows.end()) {
        return it->second;
    }
    return nullptr;
}

void EscrowManager::RemoveEscrow(const GameId& gameId) {
    m_escrows.erase(gameId);
}

std::vector<std::shared_ptr<PokerEscrow>> EscrowManager::GetActiveEscrows() const {
    std::vector<std::shared_ptr<PokerEscrow>> result;
    for (const auto& [id, escrow] : m_escrows) {
        EscrowState state = escrow->GetState();
        if (state != EscrowState::SETTLED &&
            state != EscrowState::CANCELLED &&
            state != EscrowState::TIMEOUT) {
            result.push_back(escrow);
        }
    }
    return result;
}

std::vector<GameId> EscrowManager::CheckTimeouts(int currentHeight) const {
    std::vector<GameId> timeouts;
    for (const auto& [id, escrow] : m_escrows) {
        if (escrow->CanTriggerTimeout(currentHeight)) {
            timeouts.push_back(id);
        }
    }
    return timeouts;
}

} // namespace poker
