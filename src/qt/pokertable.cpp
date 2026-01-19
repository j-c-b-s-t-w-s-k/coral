// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/pokertable.h>
#include <qt/platformstyle.h>
#include <qt/clientmodel.h>
#include <qt/walletmodel.h>

#include <poker/pokernet.h>

#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPainter>
#include <QMessageBox>

PokerTable::PokerTable(const PlatformStyle* _platformStyle, QWidget* parent)
    : QWidget(parent)
    , platformStyle(_platformStyle)
{
    setupUi();
    connectSignals();

    updateTimer = new QTimer(this);
    updateTimer->setInterval(1000);
    connect(updateTimer, &QTimer::timeout, this, &PokerTable::updateDisplay);
}

PokerTable::~PokerTable()
{
    updateTimer->stop();
}

void PokerTable::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Header with game info
    QHBoxLayout* headerLayout = new QHBoxLayout();
    phaseLabel = new QLabel(tr("Waiting..."), this);
    phaseLabel->setStyleSheet("font-size: 18px; font-weight: bold;");
    potLabel = new QLabel(tr("Pot: 0"), this);
    potLabel->setStyleSheet("font-size: 16px;");
    leaveButton = new QPushButton(tr("Leave Table"), this);

    headerLayout->addWidget(phaseLabel);
    headerLayout->addStretch();
    headerLayout->addWidget(potLabel);
    headerLayout->addWidget(leaveButton);
    mainLayout->addLayout(headerLayout);

    // Table area
    setupTableLayout();

    // Action buttons
    setupActionButtons();
}

void PokerTable::setupTableLayout()
{
    tableWidget = new QWidget(this);
    tableWidget->setMinimumSize(600, 400);
    tableWidget->setStyleSheet("background-color: #1a5f1a; border-radius: 100px;");

    QVBoxLayout* tableLayout = new QVBoxLayout(tableWidget);

    // Community cards (center)
    QHBoxLayout* communityLayout = new QHBoxLayout();
    communityLayout->addStretch();
    for (int i = 0; i < 5; ++i) {
        PokerCardWidget* card = new PokerCardWidget(this);
        communityCards.push_back(card);
        communityLayout->addWidget(card);
    }
    communityLayout->addStretch();
    tableLayout->addStretch();
    tableLayout->addLayout(communityLayout);
    tableLayout->addStretch();

    // Player positions around the table
    // Simplified: show players in a row at the bottom for now
    QHBoxLayout* playersLayout = new QHBoxLayout();
    for (int i = 0; i < 9; ++i) {
        QWidget* playerWidget = new QWidget(this);
        playerWidget->setFixedSize(100, 120);
        playerWidget->setStyleSheet("background-color: rgba(0,0,0,0.3); border-radius: 5px;");

        QVBoxLayout* playerLayout = new QVBoxLayout(playerWidget);

        QLabel* nameLabel = new QLabel(QString("Seat %1").arg(i + 1), playerWidget);
        nameLabel->setAlignment(Qt::AlignCenter);
        nameLabel->setStyleSheet("color: white; font-weight: bold;");

        QLabel* stackLabel = new QLabel("", playerWidget);
        stackLabel->setAlignment(Qt::AlignCenter);
        stackLabel->setStyleSheet("color: #ffd700;");

        QLabel* betLabel = new QLabel("", playerWidget);
        betLabel->setAlignment(Qt::AlignCenter);
        betLabel->setStyleSheet("color: #00ff00;");

        QHBoxLayout* cardsLayout = new QHBoxLayout();
        std::vector<PokerCardWidget*> cards;
        for (int j = 0; j < 2; ++j) {
            PokerCardWidget* card = new PokerCardWidget(playerWidget);
            card->setFixedSize(30, 45);
            cards.push_back(card);
            cardsLayout->addWidget(card);
        }
        playerCards.push_back(cards);

        playerLayout->addWidget(nameLabel);
        playerLayout->addLayout(cardsLayout);
        playerLayout->addWidget(stackLabel);
        playerLayout->addWidget(betLabel);

        playerWidgets.push_back(playerWidget);
        playerNameLabels.push_back(nameLabel);
        playerStackLabels.push_back(stackLabel);
        playerBetLabels.push_back(betLabel);

        playersLayout->addWidget(playerWidget);
    }

    tableLayout->addLayout(playersLayout);

    layout()->addWidget(tableWidget);
}

void PokerTable::setupActionButtons()
{
    QHBoxLayout* actionLayout = new QHBoxLayout();

    foldButton = new QPushButton(tr("Fold"), this);
    foldButton->setStyleSheet("background-color: #cc0000; color: white; padding: 10px 20px;");

    checkCallButton = new QPushButton(tr("Check"), this);
    checkCallButton->setStyleSheet("background-color: #0066cc; color: white; padding: 10px 20px;");

    betRaiseButton = new QPushButton(tr("Bet"), this);
    betRaiseButton->setStyleSheet("background-color: #00cc00; color: white; padding: 10px 20px;");

    allInButton = new QPushButton(tr("All In"), this);
    allInButton->setStyleSheet("background-color: #cc6600; color: white; padding: 10px 20px;");

    betSlider = new QSlider(Qt::Horizontal, this);
    betSlider->setRange(0, 100);

    betSpin = new QSpinBox(this);
    betSpin->setRange(0, 100000000);
    betSpin->setSuffix(" sat");

    actionLayout->addWidget(foldButton);
    actionLayout->addWidget(checkCallButton);
    actionLayout->addWidget(betSlider);
    actionLayout->addWidget(betSpin);
    actionLayout->addWidget(betRaiseButton);
    actionLayout->addWidget(allInButton);

    layout()->addItem(actionLayout);

    // Disable all actions initially
    foldButton->setEnabled(false);
    checkCallButton->setEnabled(false);
    betRaiseButton->setEnabled(false);
    allInButton->setEnabled(false);
}

void PokerTable::connectSignals()
{
    connect(foldButton, &QPushButton::clicked, this, &PokerTable::onFoldClicked);
    connect(checkCallButton, &QPushButton::clicked, this, &PokerTable::onCheckCallClicked);
    connect(betRaiseButton, &QPushButton::clicked, this, &PokerTable::onBetRaiseClicked);
    connect(allInButton, &QPushButton::clicked, this, &PokerTable::onAllInClicked);
    connect(leaveButton, &QPushButton::clicked, this, &PokerTable::onLeaveClicked);

    connect(betSlider, &QSlider::valueChanged, this, &PokerTable::onBetSliderChanged);
    connect(betSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &PokerTable::onBetSpinChanged);
}

void PokerTable::setClientModel(ClientModel* model)
{
    clientModel = model;
}

void PokerTable::setWalletModel(WalletModel* model)
{
    walletModel = model;
}

void PokerTable::setGame(const poker::GameId& gameId)
{
    currentGameId = gameId;

    if (poker::g_poker_net) {
        currentGame = poker::g_poker_net->GetGame(gameId);
    }

    updateTimer->start();
    updateDisplay();
}

void PokerTable::updateDisplay()
{
    if (!currentGame) {
        return;
    }

    // Update phase
    QString phaseStr;
    switch (currentGame->GetPhase()) {
        case poker::GamePhase::WAITING: phaseStr = tr("Waiting for players..."); break;
        case poker::GamePhase::PREFLOP: phaseStr = tr("Pre-Flop"); break;
        case poker::GamePhase::FLOP: phaseStr = tr("Flop"); break;
        case poker::GamePhase::TURN: phaseStr = tr("Turn"); break;
        case poker::GamePhase::RIVER: phaseStr = tr("River"); break;
        case poker::GamePhase::SHOWDOWN: phaseStr = tr("Showdown"); break;
        default: phaseStr = tr("In Progress"); break;
    }
    phaseLabel->setText(phaseStr);

    updatePotDisplay();
    updateCommunityCards();

    // Update player displays
    const auto& players = currentGame->GetPlayers();
    for (size_t i = 0; i < playerWidgets.size(); ++i) {
        if (i < players.size()) {
            updatePlayerDisplay(i);
            playerWidgets[i]->setVisible(true);
        } else {
            playerWidgets[i]->setVisible(false);
        }
    }

    updateActionButtons();
}

void PokerTable::updatePlayerDisplay(size_t index)
{
    if (!currentGame || index >= currentGame->GetPlayers().size()) {
        return;
    }

    const auto& player = currentGame->GetPlayers()[index];

    playerNameLabels[index]->setText(QString::fromStdString(player.GetName()));
    playerStackLabels[index]->setText(formatAmount(player.GetStack()));

    if (player.GetCurrentBet() > 0) {
        playerBetLabels[index]->setText(tr("Bet: ") + formatAmount(player.GetCurrentBet()));
    } else {
        playerBetLabels[index]->clear();
    }

    // Update card display
    const auto& holeCards = player.GetHoleCards();
    for (size_t j = 0; j < playerCards[index].size(); ++j) {
        if (j < holeCards.size()) {
            // Show face down for other players
            playerCards[index][j]->setFaceDown(true);
        } else {
            playerCards[index][j]->clear();
        }
    }

    // Highlight current player
    if (currentGame->GetCurrentPlayer() &&
        currentGame->GetCurrentPlayer()->GetId() == player.GetId()) {
        playerWidgets[index]->setStyleSheet(
            "background-color: rgba(255,215,0,0.5); border-radius: 5px; border: 2px solid gold;");
    } else if (player.GetState() == poker::PlayerState::FOLDED) {
        playerWidgets[index]->setStyleSheet(
            "background-color: rgba(100,100,100,0.5); border-radius: 5px;");
    } else {
        playerWidgets[index]->setStyleSheet(
            "background-color: rgba(0,0,0,0.3); border-radius: 5px;");
    }
}

void PokerTable::updateCommunityCards()
{
    if (!currentGame) {
        return;
    }

    const auto& cards = currentGame->GetCommunityCards();
    for (size_t i = 0; i < communityCards.size(); ++i) {
        if (i < cards.size()) {
            communityCards[i]->setCard(cards[i]);
        } else {
            communityCards[i]->clear();
        }
    }
}

void PokerTable::updateActionButtons()
{
    // Disable all by default
    foldButton->setEnabled(false);
    checkCallButton->setEnabled(false);
    betRaiseButton->setEnabled(false);
    allInButton->setEnabled(false);

    if (!currentGame || !poker::g_poker_net) {
        return;
    }

    // Get our player ID
    poker::PlayerId myId;
    memcpy(myId.begin(), poker::g_poker_net->GetPubKey().GetID().begin(), 20);

    auto validActions = currentGame->GetValidActions(myId);

    for (auto action : validActions) {
        switch (action) {
            case poker::Action::FOLD:
                foldButton->setEnabled(true);
                break;
            case poker::Action::CHECK:
                checkCallButton->setEnabled(true);
                checkCallButton->setText(tr("Check"));
                break;
            case poker::Action::CALL:
                checkCallButton->setEnabled(true);
                checkCallButton->setText(tr("Call"));
                break;
            case poker::Action::BET:
                betRaiseButton->setEnabled(true);
                betRaiseButton->setText(tr("Bet"));
                break;
            case poker::Action::RAISE:
                betRaiseButton->setEnabled(true);
                betRaiseButton->setText(tr("Raise"));
                break;
            case poker::Action::ALL_IN:
                allInButton->setEnabled(true);
                break;
        }
    }
}

void PokerTable::updatePotDisplay()
{
    if (currentGame) {
        potLabel->setText(tr("Pot: ") + formatAmount(currentGame->GetTotalPot()));
    }
}

QString PokerTable::formatAmount(int64_t satoshis) const
{
    if (satoshis >= 100000000) {
        return QString::number(satoshis / 100000000.0, 'f', 8) + " CORAL";
    }
    return QString::number(satoshis) + " sat";
}

void PokerTable::onFoldClicked()
{
    if (poker::g_poker_net) {
        poker::g_poker_net->SendAction(currentGameId, poker::Action::FOLD);
    }
}

void PokerTable::onCheckCallClicked()
{
    if (!poker::g_poker_net || !currentGame) {
        return;
    }

    poker::PlayerId myId;
    memcpy(myId.begin(), poker::g_poker_net->GetPubKey().GetID().begin(), 20);

    auto validActions = currentGame->GetValidActions(myId);
    bool canCheck = std::find(validActions.begin(), validActions.end(),
                              poker::Action::CHECK) != validActions.end();

    if (canCheck) {
        poker::g_poker_net->SendAction(currentGameId, poker::Action::CHECK);
    } else {
        poker::g_poker_net->SendAction(currentGameId, poker::Action::CALL);
    }
}

void PokerTable::onBetRaiseClicked()
{
    if (!poker::g_poker_net || !currentGame) {
        return;
    }

    int64_t amount = betSpin->value();

    poker::PlayerId myId;
    memcpy(myId.begin(), poker::g_poker_net->GetPubKey().GetID().begin(), 20);

    auto validActions = currentGame->GetValidActions(myId);
    bool canBet = std::find(validActions.begin(), validActions.end(),
                           poker::Action::BET) != validActions.end();

    if (canBet) {
        poker::g_poker_net->SendAction(currentGameId, poker::Action::BET, amount);
    } else {
        poker::g_poker_net->SendAction(currentGameId, poker::Action::RAISE, amount);
    }
}

void PokerTable::onAllInClicked()
{
    if (poker::g_poker_net) {
        poker::g_poker_net->SendAction(currentGameId, poker::Action::ALL_IN);
    }
}

void PokerTable::onLeaveClicked()
{
    if (poker::g_poker_net) {
        poker::g_poker_net->LeaveGame(currentGameId, "User requested");
    }
    updateTimer->stop();
    currentGame.reset();
    Q_EMIT gameLeft();
}

void PokerTable::onBetSliderChanged(int value)
{
    if (!currentGame) {
        return;
    }

    int64_t minBet = currentGame->GetConfig().bigBlind;
    int64_t maxBet = 1000000; // TODO: Get from player stack

    int64_t betAmount = minBet + (maxBet - minBet) * value / 100;
    betSpin->blockSignals(true);
    betSpin->setValue(betAmount);
    betSpin->blockSignals(false);
}

void PokerTable::onBetSpinChanged(int value)
{
    if (!currentGame) {
        return;
    }

    int64_t minBet = currentGame->GetConfig().bigBlind;
    int64_t maxBet = 1000000;

    int sliderValue = 0;
    if (maxBet > minBet) {
        sliderValue = (value - minBet) * 100 / (maxBet - minBet);
    }

    betSlider->blockSignals(true);
    betSlider->setValue(sliderValue);
    betSlider->blockSignals(false);
}

// PokerCardWidget implementation

PokerCardWidget::PokerCardWidget(QWidget* parent)
    : QWidget(parent)
{
    setFixedSize(50, 70);
}

void PokerCardWidget::setCard(const poker::Card& card)
{
    m_card = card;
    m_hasCard = true;
    m_faceDown = false;
    update();
}

void PokerCardWidget::setFaceDown(bool faceDown)
{
    m_faceDown = faceDown;
    update();
}

void PokerCardWidget::clear()
{
    m_hasCard = false;
    update();
}

void PokerCardWidget::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QRect cardRect = rect().adjusted(1, 1, -1, -1);

    if (!m_hasCard) {
        // Empty card slot
        painter.setPen(QPen(Qt::gray, 1, Qt::DashLine));
        painter.setBrush(Qt::transparent);
        painter.drawRoundedRect(cardRect, 3, 3);
        return;
    }

    if (m_faceDown) {
        // Face-down card
        painter.setPen(QPen(Qt::darkGray, 1));
        painter.setBrush(QColor(30, 30, 100));
        painter.drawRoundedRect(cardRect, 3, 3);

        // Pattern
        painter.setPen(QPen(QColor(50, 50, 150), 1));
        for (int i = 5; i < cardRect.width() - 5; i += 5) {
            painter.drawLine(cardRect.left() + i, cardRect.top() + 5,
                           cardRect.left() + i, cardRect.bottom() - 5);
        }
        return;
    }

    // Face-up card
    painter.setPen(QPen(Qt::black, 1));
    painter.setBrush(Qt::white);
    painter.drawRoundedRect(cardRect, 3, 3);

    // Suit and rank
    painter.setPen(getSuitColor());
    QFont font = painter.font();
    font.setBold(true);
    font.setPixelSize(14);
    painter.setFont(font);

    QString text = getRankString() + getSuitSymbol();
    painter.drawText(cardRect, Qt::AlignCenter, text);
}

QColor PokerCardWidget::getSuitColor() const
{
    switch (m_card.GetSuit()) {
        case poker::Suit::HEARTS:
        case poker::Suit::DIAMONDS:
            return Qt::red;
        default:
            return Qt::black;
    }
}

QString PokerCardWidget::getSuitSymbol() const
{
    switch (m_card.GetSuit()) {
        case poker::Suit::CLUBS: return QString::fromUtf8("\u2663");
        case poker::Suit::DIAMONDS: return QString::fromUtf8("\u2666");
        case poker::Suit::HEARTS: return QString::fromUtf8("\u2665");
        case poker::Suit::SPADES: return QString::fromUtf8("\u2660");
    }
    return "?";
}

QString PokerCardWidget::getRankString() const
{
    return QString::fromStdString(poker::RankToString(m_card.GetRank()));
}
