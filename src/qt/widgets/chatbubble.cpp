// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/widgets/chatbubble.h>

#include <QHBoxLayout>
#include <QPainter>
#include <QDateTime>
#include <QMouseEvent>

ChatBubble::ChatBubble(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

ChatBubble::ChatBubble(const MessageData &data, QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    setData(data);
}

void ChatBubble::setupUI()
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(8);

    // IRC-style: [HH:MM] <nick> message
    m_timestampLabel = new QLabel(this);
    m_timestampLabel->setStyleSheet(
        "color: #4a4a4a;"
        "font-family: 'Terminus', monospace;"
        "font-size: 11px;"
    );
    m_timestampLabel->setAlignment(Qt::AlignTop);
    layout->addWidget(m_timestampLabel);

    m_nickLabel = new QLabel(this);
    m_nickLabel->setStyleSheet(
        "color: #00d4aa;"
        "font-family: 'Terminus', monospace;"
        "font-size: 11px;"
        "font-weight: bold;"
    );
    m_nickLabel->setAlignment(Qt::AlignTop);
    m_nickLabel->setCursor(Qt::PointingHandCursor);
    layout->addWidget(m_nickLabel);

    m_contentLabel = new QLabel(this);
    m_contentLabel->setStyleSheet(
        "color: #c0c0c0;"
        "font-family: 'Terminus', monospace;"
        "font-size: 11px;"
    );
    m_contentLabel->setWordWrap(true);
    m_contentLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    layout->addWidget(m_contentLabel, 1);

    setMinimumHeight(24);
}

void ChatBubble::setData(const MessageData &data)
{
    m_data = data;
    updateDisplay();
}

void ChatBubble::updateDisplay()
{
    m_timestampLabel->setText(formatTimestamp(m_data.timestamp));

    if (m_data.isSystem) {
        // System message: * system: message
        m_nickLabel->setText("*");
        m_nickLabel->setStyleSheet(
            "color: #d4aa00;"
            "font-family: 'Terminus', monospace;"
            "font-size: 11px;"
            "font-weight: bold;"
        );
        m_contentLabel->setText("system: " + m_data.content);
        m_contentLabel->setStyleSheet(
            "color: #808080;"
            "font-family: 'Terminus', monospace;"
            "font-size: 11px;"
            "font-style: italic;"
        );
    } else {
        // Regular message: <nick> message
        QString nick = m_data.nick.isEmpty()
            ? QString("anon_%1").arg(m_data.address.left(4).toLower())
            : m_data.nick;
        m_nickLabel->setText(QString("<%1>").arg(nick));

        if (m_data.isOwn) {
            m_nickLabel->setStyleSheet(
                "color: #33ff33;"
                "font-family: 'Terminus', monospace;"
                "font-size: 11px;"
                "font-weight: bold;"
            );
            m_contentLabel->setStyleSheet(
                "color: #c0c0c0;"
                "font-family: 'Terminus', monospace;"
                "font-size: 11px;"
            );
        } else {
            m_nickLabel->setStyleSheet(
                "color: #00d4aa;"
                "font-family: 'Terminus', monospace;"
                "font-size: 11px;"
                "font-weight: bold;"
            );
            m_contentLabel->setStyleSheet(
                "color: #c0c0c0;"
                "font-family: 'Terminus', monospace;"
                "font-size: 11px;"
            );
        }

        m_contentLabel->setText(m_data.content);
    }
}

QString ChatBubble::formatTimestamp(int64_t timestamp) const
{
    if (timestamp == 0) return "[--:--]";
    QDateTime dt = QDateTime::fromSecsSinceEpoch(timestamp);
    return dt.toString("[HH:mm]");
}

void ChatBubble::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);

    // Background
    if (m_data.isOwn) {
        p.fillRect(rect(), QColor("#0a2a2a"));
    } else if (m_data.isSystem) {
        p.fillRect(rect(), QColor("#1a1a0a"));
    } else {
        p.fillRect(rect(), QColor("#0a0a0a"));
    }

    // Bottom border
    p.setPen(QColor("#1a1a1a"));
    p.drawLine(rect().bottomLeft(), rect().bottomRight());
}
