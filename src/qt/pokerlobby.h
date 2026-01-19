// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CORAL_QT_POKERLOBBY_H
#define CORAL_QT_POKERLOBBY_H

#include <QWidget>
#include <QTimer>

#include <poker/pokertypes.h>

class ClientModel;
class WalletModel;
class PlatformStyle;

QT_BEGIN_NAMESPACE
class QTableWidget;
class QPushButton;
class QComboBox;
class QSpinBox;
class QLabel;
class QGroupBox;
QT_END_NAMESPACE

/**
 * Poker lobby widget for discovering and creating games.
 */
class PokerLobby : public QWidget
{
    Q_OBJECT

public:
    explicit PokerLobby(const PlatformStyle* platformStyle, QWidget* parent = nullptr);
    ~PokerLobby();

    void setClientModel(ClientModel* clientModel);
    void setWalletModel(WalletModel* walletModel);

public Q_SLOTS:
    void refreshGameList();
    void createGame();
    void joinSelectedGame();

Q_SIGNALS:
    void gameCreated(const poker::GameId& gameId);
    void gameJoined(const poker::GameId& gameId);
    void message(const QString& title, const QString& message, unsigned int style);

private Q_SLOTS:
    void onGameSelectionChanged();
    void updateCreateButtonState();

private:
    const PlatformStyle* platformStyle;
    ClientModel* clientModel{nullptr};
    WalletModel* walletModel{nullptr};

    // Available games table
    QTableWidget* gamesTable;
    QPushButton* refreshButton;
    QPushButton* joinButton;

    // Create game section
    QGroupBox* createGameGroup;
    QComboBox* gameVariantCombo;
    QSpinBox* smallBlindSpin;
    QSpinBox* bigBlindSpin;
    QSpinBox* minBuyInSpin;
    QSpinBox* maxBuyInSpin;
    QSpinBox* maxPlayersSpin;
    QPushButton* createButton;

    // Refresh timer
    QTimer* refreshTimer;

    void setupUi();
    void connectSignals();
    void populateGameList();
};

#endif // CORAL_QT_POKERLOBBY_H
