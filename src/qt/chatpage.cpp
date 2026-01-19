// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/chatpage.h>
#include <qt/clientmodel.h>
#include <qt/walletmodel.h>
#include <qt/platformstyle.h>
#include <qt/widgets/chatbubble.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollBar>
#include <QDateTime>
#include <QSettings>

ChatPage::ChatPage(const PlatformStyle *_platformStyle, QWidget *parent)
    : QWidget(parent)
    , platformStyle(_platformStyle)
{
    setupUI();

    // Refresh timer for node count
    refreshTimer = new QTimer(this);
    connect(refreshTimer, &QTimer::timeout, this, &ChatPage::updateNodeCount);
    refreshTimer->start(5000); // Update every 5 seconds

    // Load saved nickname
    QSettings settings;
    myNick = settings.value("chat/nickname", "").toString();
}

ChatPage::~ChatPage()
{
    if (refreshTimer) {
        refreshTimer->stop();
    }
}

void ChatPage::setupUI()
{
    // Main dark theme
    setStyleSheet(
        "ChatPage {"
        "  background-color: #0a0a0a;"
        "}"
    );

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Header panel with terminal aesthetic
    auto *headerPanel = new QWidget(this);
    headerPanel->setStyleSheet(
        "QWidget {"
        "  background: qlineargradient(y1:0, y2:1, stop:0 #2a2a2a, stop:1 #1a1a1a);"
        "  border-bottom: 2px solid #3a3a3a;"
        "}"
    );
    auto *headerLayout = new QHBoxLayout(headerPanel);
    headerLayout->setContentsMargins(16, 12, 16, 12);

    headerLabel = new QLabel(tr("CORAL NETWORK CHAT"), this);
    headerLabel->setStyleSheet(
        "color: #00d4aa;"
        "font-size: 16px;"
        "font-weight: bold;"
        "font-family: 'Terminus', monospace;"
        "background: transparent;"
    );
    headerLayout->addWidget(headerLabel);

    headerLayout->addStretch();

    nodeCountLabel = new QLabel(tr("0 nodes online"), this);
    nodeCountLabel->setStyleSheet(
        "color: #808080;"
        "font-size: 12px;"
        "font-family: 'Terminus', monospace;"
        "background: transparent;"
    );
    headerLayout->addWidget(nodeCountLabel);

    mainLayout->addWidget(headerPanel);

    // Chat area - terminal style scrolling text
    auto *chatPanel = new QWidget(this);
    chatPanel->setStyleSheet(
        "QWidget {"
        "  background-color: #0a0a0a;"
        "  border: 2px solid;"
        "  border-color: #1a1a1a #3a3a3a #3a3a3a #1a1a1a;"
        "}"
    );
    auto *chatPanelLayout = new QVBoxLayout(chatPanel);
    chatPanelLayout->setContentsMargins(2, 2, 2, 2);

    chatScrollArea = new QScrollArea(this);
    chatScrollArea->setWidgetResizable(true);
    chatScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    chatScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    chatScrollArea->setStyleSheet(
        "QScrollArea {"
        "  background: #0a0a0a;"
        "  border: none;"
        "}"
    );

    chatContainer = new QWidget();
    chatContainer->setStyleSheet("background: #0a0a0a;");
    chatLayout = new QVBoxLayout(chatContainer);
    chatLayout->setContentsMargins(8, 8, 8, 8);
    chatLayout->setSpacing(0);
    chatLayout->addStretch();

    chatScrollArea->setWidget(chatContainer);
    chatPanelLayout->addWidget(chatScrollArea);

    mainLayout->addWidget(chatPanel, 1);

    // Input area - beveled Win95 style
    auto *inputPanel = new QWidget(this);
    inputPanel->setStyleSheet(
        "QWidget {"
        "  background: #1a1a1a;"
        "  border-top: 2px solid;"
        "  border-top-color: #5a5a5a;"
        "}"
    );
    auto *inputLayout = new QHBoxLayout(inputPanel);
    inputLayout->setContentsMargins(12, 8, 12, 8);
    inputLayout->setSpacing(8);

    messageInput = new QLineEdit(this);
    messageInput->setPlaceholderText(tr("Type a message..."));
    messageInput->setStyleSheet(
        "QLineEdit {"
        "  background: #0a0a0a;"
        "  border: 2px solid;"
        "  border-color: #1a1a1a #3a3a3a #3a3a3a #1a1a1a;"
        "  color: #33ff33;"
        "  font-family: 'Terminus', monospace;"
        "  font-size: 12px;"
        "  padding: 8px 12px;"
        "}"
        "QLineEdit:focus {"
        "  border-color: #0a5a4a #00d4aa #00d4aa #0a5a4a;"
        "}"
    );
    connect(messageInput, &QLineEdit::returnPressed, this, &ChatPage::onReturnPressed);
    inputLayout->addWidget(messageInput, 1);

    sendButton = new QPushButton(tr("SEND"), this);
    sendButton->setStyleSheet(
        "QPushButton {"
        "  background: qlineargradient(y1:0, y2:1, stop:0 #0a6a5a, stop:1 #064a3a);"
        "  border: 2px solid;"
        "  border-color: #00d4aa #043a2a #043a2a #00d4aa;"
        "  color: #00d4aa;"
        "  font-size: 12px;"
        "  font-weight: bold;"
        "  font-family: 'Terminus', monospace;"
        "  padding: 8px 24px;"
        "  min-width: 80px;"
        "}"
        "QPushButton:hover {"
        "  background: qlineargradient(y1:0, y2:1, stop:0 #0a8a7a, stop:1 #066a5a);"
        "  color: #33ffcc;"
        "}"
        "QPushButton:pressed {"
        "  border-color: #043a2a #00d4aa #00d4aa #043a2a;"
        "}"
    );
    connect(sendButton, &QPushButton::clicked, this, &ChatPage::onSendClicked);
    inputLayout->addWidget(sendButton);

    mainLayout->addWidget(inputPanel);

    // Add welcome message
    addSystemMessage(tr("Welcome to Coral Network Chat"));
    addSystemMessage(tr("Messages are broadcast to all connected nodes"));
}

void ChatPage::setClientModel(ClientModel *model)
{
    clientModel = model;
    updateNodeCount();
}

void ChatPage::setWalletModel(WalletModel *model)
{
    walletModel = model;
}

void ChatPage::refreshChat()
{
    // In a full implementation, this would fetch messages from ChatManager
    // For now, just update node count
    updateNodeCount();
}

void ChatPage::updateNodeCount()
{
    if (clientModel) {
        int numConnections = clientModel->getNumConnections();
        nodeCountLabel->setText(tr("%1 nodes online").arg(numConnections));

        // Color based on connection status
        if (numConnections == 0) {
            nodeCountLabel->setStyleSheet(
                "color: #aa0000;"
                "font-size: 12px;"
                "font-family: 'Terminus', monospace;"
                "background: transparent;"
            );
        } else if (numConnections < 3) {
            nodeCountLabel->setStyleSheet(
                "color: #d4aa00;"
                "font-size: 12px;"
                "font-family: 'Terminus', monospace;"
                "background: transparent;"
            );
        } else {
            nodeCountLabel->setStyleSheet(
                "color: #00d4aa;"
                "font-size: 12px;"
                "font-family: 'Terminus', monospace;"
                "background: transparent;"
            );
        }
    }
}

void ChatPage::onSendClicked()
{
    QString text = messageInput->text().trimmed();
    if (text.isEmpty()) return;

    // In a full implementation, this would:
    // 1. Sign the message with the wallet key
    // 2. Broadcast to connected peers via ChatManager
    // 3. Add to local display

    // For now, just add locally as demonstration
    QString nick = getMyNick();
    QString address = walletModel ? "local" : "unknown";
    int64_t timestamp = QDateTime::currentSecsSinceEpoch();

    addMessage(nick, address, text, timestamp, true, false);

    messageInput->clear();
    messageInput->setFocus();
}

void ChatPage::onReturnPressed()
{
    onSendClicked();
}

void ChatPage::addMessage(const QString &nick, const QString &address,
                          const QString &content, int64_t timestamp,
                          bool isOwn, bool isSystem)
{
    ChatBubble::MessageData data;
    data.nick = nick;
    data.address = address;
    data.content = content;
    data.timestamp = timestamp;
    data.isOwn = isOwn;
    data.isSystem = isSystem;

    auto *bubble = new ChatBubble(data, chatContainer);

    // Insert before the stretch
    chatLayout->insertWidget(chatLayout->count() - 1, bubble);

    // Auto-scroll to bottom
    QTimer::singleShot(10, this, &ChatPage::scrollToBottom);
}

void ChatPage::addSystemMessage(const QString &content)
{
    addMessage("", "", content, QDateTime::currentSecsSinceEpoch(), false, true);
}

void ChatPage::scrollToBottom()
{
    QScrollBar *sb = chatScrollArea->verticalScrollBar();
    sb->setValue(sb->maximum());
}

QString ChatPage::getMyNick() const
{
    if (!myNick.isEmpty()) {
        return myNick;
    }

    // Generate anonymous nick from wallet address
    if (walletModel) {
        // In real implementation, get first address from wallet
        return QString("anon_%1").arg(QString::number(qrand() % 0xffff, 16));
    }

    return "anonymous";
}
