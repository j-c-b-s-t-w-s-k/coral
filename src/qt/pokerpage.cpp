// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/pokerpage.h>
#include <qt/pokerlobby.h>
#include <qt/pokertable.h>
#include <qt/platformstyle.h>
#include <qt/clientmodel.h>
#include <qt/walletmodel.h>

#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

PokerPage::PokerPage(const PlatformStyle* _platformStyle, QWidget* parent)
    : QWidget(parent)
    , platformStyle(_platformStyle)
{
    setupUi();
    connectSignals();
}

PokerPage::~PokerPage() = default;

void PokerPage::setupUi()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    stackedWidget = new QStackedWidget(this);

    // Create lobby widget
    lobbyWidget = new PokerLobby(platformStyle, this);
    stackedWidget->addWidget(lobbyWidget);

    // Create table widget
    tableWidget = new PokerTable(platformStyle, this);
    stackedWidget->addWidget(tableWidget);

    mainLayout->addWidget(stackedWidget);

    // Start with lobby view
    showLobby();
}

void PokerPage::connectSignals()
{
    // Connect lobby signals
    connect(lobbyWidget, &PokerLobby::gameCreated, this, &PokerPage::onGameCreated);
    connect(lobbyWidget, &PokerLobby::gameJoined, this, &PokerPage::onGameJoined);

    // Connect table signals
    connect(tableWidget, &PokerTable::gameLeft, this, &PokerPage::onGameLeft);
}

void PokerPage::setClientModel(ClientModel* model)
{
    clientModel = model;
    if (lobbyWidget) {
        lobbyWidget->setClientModel(model);
    }
    if (tableWidget) {
        tableWidget->setClientModel(model);
    }
}

void PokerPage::setWalletModel(WalletModel* model)
{
    walletModel = model;
    if (lobbyWidget) {
        lobbyWidget->setWalletModel(model);
    }
    if (tableWidget) {
        tableWidget->setWalletModel(model);
    }
}

void PokerPage::showLobby()
{
    stackedWidget->setCurrentWidget(lobbyWidget);
}

void PokerPage::showTable(const poker::GameId& gameId)
{
    tableWidget->setGame(gameId);
    stackedWidget->setCurrentWidget(tableWidget);
}

void PokerPage::onGameCreated(const poker::GameId& gameId)
{
    showTable(gameId);
}

void PokerPage::onGameJoined(const poker::GameId& gameId)
{
    showTable(gameId);
}

void PokerPage::onGameLeft()
{
    showLobby();
}
