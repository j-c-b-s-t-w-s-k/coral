// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CORAL_QT_POKERPAGE_H
#define CORAL_QT_POKERPAGE_H

#include <QWidget>
#include <QStackedWidget>

#include <poker/pokertypes.h>
#include <poker/poker.h>

class ClientModel;
class WalletModel;
class PlatformStyle;
class PokerLobby;
class PokerTable;

QT_BEGIN_NAMESPACE
class QVBoxLayout;
class QLabel;
class QPushButton;
QT_END_NAMESPACE

/**
 * Main poker page widget.
 * Contains the lobby for finding/creating games and the game table.
 */
class PokerPage : public QWidget
{
    Q_OBJECT

public:
    explicit PokerPage(const PlatformStyle* platformStyle, QWidget* parent = nullptr);
    ~PokerPage();

    void setClientModel(ClientModel* clientModel);
    void setWalletModel(WalletModel* walletModel);

public Q_SLOTS:
    void showLobby();
    void showTable(const poker::GameId& gameId);

    void onGameCreated(const poker::GameId& gameId);
    void onGameJoined(const poker::GameId& gameId);
    void onGameLeft();

Q_SIGNALS:
    void message(const QString& title, const QString& message, unsigned int style);

private:
    const PlatformStyle* platformStyle;
    ClientModel* clientModel{nullptr};
    WalletModel* walletModel{nullptr};

    QStackedWidget* stackedWidget;
    PokerLobby* lobbyWidget;
    PokerTable* tableWidget;

    void setupUi();
    void connectSignals();
};

#endif // CORAL_QT_POKERPAGE_H
