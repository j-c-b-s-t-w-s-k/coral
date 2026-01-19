// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CORAL_QT_POKERTABLE_H
#define CORAL_QT_POKERTABLE_H

#include <QWidget>
#include <QTimer>

#include <poker/pokertypes.h>
#include <poker/poker.h>

class ClientModel;
class WalletModel;
class PlatformStyle;
class PokerCardWidget;

QT_BEGIN_NAMESPACE
class QLabel;
class QPushButton;
class QSlider;
class QSpinBox;
class QGridLayout;
QT_END_NAMESPACE

/**
 * Poker table widget showing the active game.
 */
class PokerTable : public QWidget
{
    Q_OBJECT

public:
    explicit PokerTable(const PlatformStyle* platformStyle, QWidget* parent = nullptr);
    ~PokerTable();

    void setClientModel(ClientModel* clientModel);
    void setWalletModel(WalletModel* walletModel);

    void setGame(const poker::GameId& gameId);

public Q_SLOTS:
    void updateDisplay();

    void onFoldClicked();
    void onCheckCallClicked();
    void onBetRaiseClicked();
    void onAllInClicked();
    void onLeaveClicked();

Q_SIGNALS:
    void gameLeft();
    void message(const QString& title, const QString& message, unsigned int style);

private Q_SLOTS:
    void onBetSliderChanged(int value);
    void onBetSpinChanged(int value);

private:
    const PlatformStyle* platformStyle;
    ClientModel* clientModel{nullptr};
    WalletModel* walletModel{nullptr};

    poker::GameId currentGameId;
    std::shared_ptr<poker::PokerGame> currentGame;

    // Table display
    QWidget* tableWidget;
    QLabel* potLabel;
    QLabel* phaseLabel;

    // Community cards
    std::vector<PokerCardWidget*> communityCards;

    // Player positions (up to 9 players)
    std::vector<QWidget*> playerWidgets;
    std::vector<QLabel*> playerNameLabels;
    std::vector<QLabel*> playerStackLabels;
    std::vector<QLabel*> playerBetLabels;
    std::vector<std::vector<PokerCardWidget*>> playerCards;

    // Action buttons
    QPushButton* foldButton;
    QPushButton* checkCallButton;
    QPushButton* betRaiseButton;
    QPushButton* allInButton;
    QPushButton* leaveButton;

    // Bet sizing
    QSlider* betSlider;
    QSpinBox* betSpin;

    // Update timer
    QTimer* updateTimer;

    void setupUi();
    void setupTableLayout();
    void setupActionButtons();
    void connectSignals();

    void updatePlayerDisplay(size_t index);
    void updateCommunityCards();
    void updateActionButtons();
    void updatePotDisplay();

    QString formatAmount(int64_t satoshis) const;
};

/**
 * Widget for displaying a single playing card.
 */
class PokerCardWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PokerCardWidget(QWidget* parent = nullptr);

    void setCard(const poker::Card& card);
    void setFaceDown(bool faceDown);
    void clear();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    bool m_hasCard{false};
    bool m_faceDown{false};
    poker::Card m_card;

    QColor getSuitColor() const;
    QString getSuitSymbol() const;
    QString getRankString() const;
};

#endif // CORAL_QT_POKERTABLE_H
