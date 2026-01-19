// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CORAL_QT_CHATPAGE_H
#define CORAL_QT_CHATPAGE_H

#include <QWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QScrollArea>
#include <QVBoxLayout>

class ClientModel;
class WalletModel;
class PlatformStyle;

/**
 * Network chat page for P2P communication between nodes.
 * Messages are signed with wallet keys for authentication.
 */
class ChatPage : public QWidget
{
    Q_OBJECT

public:
    explicit ChatPage(const PlatformStyle *platformStyle, QWidget *parent = nullptr);
    ~ChatPage();

    void setClientModel(ClientModel *model);
    void setWalletModel(WalletModel *model);

public Q_SLOTS:
    void refreshChat();

private Q_SLOTS:
    void onSendClicked();
    void onReturnPressed();
    void updateNodeCount();

Q_SIGNALS:
    void message(const QString &title, const QString &message, unsigned int style);

private:
    void setupUI();
    void addMessage(const QString &nick, const QString &address,
                    const QString &content, int64_t timestamp,
                    bool isOwn, bool isSystem);
    void addSystemMessage(const QString &content);
    void scrollToBottom();
    QString getMyNick() const;

    const PlatformStyle *platformStyle;
    ClientModel *clientModel{nullptr};
    WalletModel *walletModel{nullptr};

    // UI elements
    QLabel *headerLabel{nullptr};
    QLabel *nodeCountLabel{nullptr};
    QScrollArea *chatScrollArea{nullptr};
    QWidget *chatContainer{nullptr};
    QVBoxLayout *chatLayout{nullptr};
    QLineEdit *messageInput{nullptr};
    QPushButton *sendButton{nullptr};

    // Settings
    QString myNick;
    QTimer *refreshTimer{nullptr};
};

#endif // CORAL_QT_CHATPAGE_H
