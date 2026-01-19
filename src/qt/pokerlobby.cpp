// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/pokerlobby.h>
#include <qt/platformstyle.h>
#include <qt/clientmodel.h>
#include <qt/walletmodel.h>
#include <qt/guiutil.h>

#include <poker/pokernet.h>

#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QLabel>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QMessageBox>

PokerLobby::PokerLobby(const PlatformStyle* _platformStyle, QWidget* parent)
    : QWidget(parent)
    , platformStyle(_platformStyle)
{
    setupUi();
    connectSignals();

    // Start refresh timer
    refreshTimer = new QTimer(this);
    refreshTimer->setInterval(30000); // 30 seconds
    connect(refreshTimer, &QTimer::timeout, this, &PokerLobby::refreshGameList);
    refreshTimer->start();
}

PokerLobby::~PokerLobby()
{
    refreshTimer->stop();
}

void PokerLobby::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Title
    QLabel* titleLabel = new QLabel(tr("Poker Lobby"), this);
    titleLabel->setStyleSheet("font-size: 24px; font-weight: bold; margin-bottom: 10px;");
    mainLayout->addWidget(titleLabel);

    // Available games section
    QGroupBox* gamesGroup = new QGroupBox(tr("Available Games"), this);
    QVBoxLayout* gamesLayout = new QVBoxLayout(gamesGroup);

    gamesTable = new QTableWidget(this);
    gamesTable->setColumnCount(6);
    gamesTable->setHorizontalHeaderLabels({
        tr("Host"), tr("Variant"), tr("Stakes"), tr("Buy-in"),
        tr("Players"), tr("Status")
    });
    gamesTable->horizontalHeader()->setStretchLastSection(true);
    gamesTable->setSelectionBehavior(QTableWidget::SelectRows);
    gamesTable->setSelectionMode(QTableWidget::SingleSelection);
    gamesTable->setEditTriggers(QTableWidget::NoEditTriggers);
    gamesLayout->addWidget(gamesTable);

    QHBoxLayout* gamesButtonLayout = new QHBoxLayout();
    refreshButton = new QPushButton(tr("Refresh"), this);
    joinButton = new QPushButton(tr("Join Game"), this);
    joinButton->setEnabled(false);
    gamesButtonLayout->addWidget(refreshButton);
    gamesButtonLayout->addStretch();
    gamesButtonLayout->addWidget(joinButton);
    gamesLayout->addLayout(gamesButtonLayout);

    mainLayout->addWidget(gamesGroup);

    // Create game section
    createGameGroup = new QGroupBox(tr("Create New Game"), this);
    QGridLayout* createLayout = new QGridLayout(createGameGroup);

    int row = 0;

    createLayout->addWidget(new QLabel(tr("Game Type:"), this), row, 0);
    gameVariantCombo = new QComboBox(this);
    gameVariantCombo->addItem(tr("Texas Hold'em"), static_cast<int>(poker::GameVariant::TEXAS_HOLDEM));
    gameVariantCombo->addItem(tr("5-Card Draw"), static_cast<int>(poker::GameVariant::FIVE_CARD_DRAW));
    createLayout->addWidget(gameVariantCombo, row++, 1);

    createLayout->addWidget(new QLabel(tr("Small Blind:"), this), row, 0);
    smallBlindSpin = new QSpinBox(this);
    smallBlindSpin->setRange(1, 1000000);
    smallBlindSpin->setValue(100);
    smallBlindSpin->setSuffix(" sat");
    createLayout->addWidget(smallBlindSpin, row++, 1);

    createLayout->addWidget(new QLabel(tr("Big Blind:"), this), row, 0);
    bigBlindSpin = new QSpinBox(this);
    bigBlindSpin->setRange(2, 2000000);
    bigBlindSpin->setValue(200);
    bigBlindSpin->setSuffix(" sat");
    createLayout->addWidget(bigBlindSpin, row++, 1);

    createLayout->addWidget(new QLabel(tr("Min Buy-in:"), this), row, 0);
    minBuyInSpin = new QSpinBox(this);
    minBuyInSpin->setRange(1000, 100000000);
    minBuyInSpin->setValue(10000);
    minBuyInSpin->setSuffix(" sat");
    createLayout->addWidget(minBuyInSpin, row++, 1);

    createLayout->addWidget(new QLabel(tr("Max Buy-in:"), this), row, 0);
    maxBuyInSpin = new QSpinBox(this);
    maxBuyInSpin->setRange(1000, 100000000);
    maxBuyInSpin->setValue(100000);
    maxBuyInSpin->setSuffix(" sat");
    createLayout->addWidget(maxBuyInSpin, row++, 1);

    createLayout->addWidget(new QLabel(tr("Max Players:"), this), row, 0);
    maxPlayersSpin = new QSpinBox(this);
    maxPlayersSpin->setRange(2, 9);
    maxPlayersSpin->setValue(6);
    createLayout->addWidget(maxPlayersSpin, row++, 1);

    createButton = new QPushButton(tr("Create Game"), this);
    createLayout->addWidget(createButton, row, 0, 1, 2);

    mainLayout->addWidget(createGameGroup);
    mainLayout->addStretch();
}

void PokerLobby::connectSignals()
{
    connect(refreshButton, &QPushButton::clicked, this, &PokerLobby::refreshGameList);
    connect(joinButton, &QPushButton::clicked, this, &PokerLobby::joinSelectedGame);
    connect(createButton, &QPushButton::clicked, this, &PokerLobby::createGame);

    connect(gamesTable, &QTableWidget::itemSelectionChanged,
            this, &PokerLobby::onGameSelectionChanged);

    connect(smallBlindSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &PokerLobby::updateCreateButtonState);
    connect(bigBlindSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &PokerLobby::updateCreateButtonState);
}

void PokerLobby::setClientModel(ClientModel* model)
{
    clientModel = model;
}

void PokerLobby::setWalletModel(WalletModel* model)
{
    walletModel = model;
    updateCreateButtonState();
}

void PokerLobby::refreshGameList()
{
    populateGameList();
}

void PokerLobby::populateGameList()
{
    gamesTable->setRowCount(0);

    if (!poker::g_poker_net) {
        return;
    }

    const auto& games = poker::g_poker_net->GetAvailableGames();

    for (const auto& [gameId, announce] : games) {
        int row = gamesTable->rowCount();
        gamesTable->insertRow(row);

        gamesTable->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(announce.hostName)));

        QString variant = announce.config.variant == poker::GameVariant::TEXAS_HOLDEM ?
                         tr("Texas Hold'em") : tr("5-Card Draw");
        gamesTable->setItem(row, 1, new QTableWidgetItem(variant));

        QString stakes = QString("%1/%2").arg(announce.config.smallBlind).arg(announce.config.bigBlind);
        gamesTable->setItem(row, 2, new QTableWidgetItem(stakes));

        QString buyIn = QString("%1-%2").arg(announce.config.minBuyIn).arg(announce.config.maxBuyIn);
        gamesTable->setItem(row, 3, new QTableWidgetItem(buyIn));

        QString players = QString("%1/%2").arg(announce.currentPlayers).arg(announce.config.maxPlayers);
        gamesTable->setItem(row, 4, new QTableWidgetItem(players));

        gamesTable->setItem(row, 5, new QTableWidgetItem(tr("Waiting")));

        // Store game ID in first item
        gamesTable->item(row, 0)->setData(Qt::UserRole, QByteArray((const char*)gameId.begin(), 32));
    }
}

void PokerLobby::createGame()
{
    if (!walletModel || !poker::g_poker_net) {
        QMessageBox::warning(this, tr("Error"), tr("Wallet not available"));
        return;
    }

    poker::GameConfig config;
    config.variant = static_cast<poker::GameVariant>(gameVariantCombo->currentData().toInt());
    config.smallBlind = smallBlindSpin->value();
    config.bigBlind = bigBlindSpin->value();
    config.minBuyIn = minBuyInSpin->value();
    config.maxBuyIn = maxBuyInSpin->value();
    config.maxPlayers = maxPlayersSpin->value();

    if (!config.IsValid()) {
        QMessageBox::warning(this, tr("Invalid Configuration"),
                            tr("Please check your game settings"));
        return;
    }

    poker::GameId gameId;
    if (poker::g_poker_net->CreateGame(config, gameId)) {
        Q_EMIT gameCreated(gameId);
    } else {
        QMessageBox::warning(this, tr("Error"), tr("Failed to create game"));
    }
}

void PokerLobby::joinSelectedGame()
{
    if (!poker::g_poker_net) {
        return;
    }

    QList<QTableWidgetItem*> selected = gamesTable->selectedItems();
    if (selected.isEmpty()) {
        return;
    }

    int row = selected.first()->row();
    QByteArray gameIdData = gamesTable->item(row, 0)->data(Qt::UserRole).toByteArray();

    poker::GameId gameId;
    if (gameIdData.size() == 32) {
        memcpy(gameId.begin(), gameIdData.constData(), 32);

        if (poker::g_poker_net->JoinGame(gameId)) {
            Q_EMIT gameJoined(gameId);
        } else {
            QMessageBox::warning(this, tr("Error"), tr("Failed to join game"));
        }
    }
}

void PokerLobby::onGameSelectionChanged()
{
    joinButton->setEnabled(!gamesTable->selectedItems().isEmpty());
}

void PokerLobby::updateCreateButtonState()
{
    bool valid = smallBlindSpin->value() > 0 &&
                 bigBlindSpin->value() >= smallBlindSpin->value() * 2 &&
                 minBuyInSpin->value() > 0 &&
                 maxBuyInSpin->value() >= minBuyInSpin->value();

    createButton->setEnabled(valid && walletModel != nullptr);
}
