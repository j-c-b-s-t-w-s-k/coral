// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CORAL_QT_WIDGETS_CHATBUBBLE_H
#define CORAL_QT_WIDGETS_CHATBUBBLE_H

#include <QWidget>
#include <QLabel>

/**
 * Atomic chat bubble widget for displaying messages.
 * Terminal/IRC style with timestamp and nickname.
 */
class ChatBubble : public QWidget
{
    Q_OBJECT

public:
    struct MessageData {
        QString messageId;
        QString nick;
        QString address;
        QString content;
        int64_t timestamp;
        bool isOwn;
        bool isSystem;
    };

    explicit ChatBubble(QWidget *parent = nullptr);
    explicit ChatBubble(const MessageData &data, QWidget *parent = nullptr);
    ~ChatBubble() = default;

    void setData(const MessageData &data);
    MessageData data() const { return m_data; }

Q_SIGNALS:
    void nickClicked(const QString &nick, const QString &address);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void setupUI();
    void updateDisplay();
    QString formatTimestamp(int64_t timestamp) const;

    MessageData m_data;
    QLabel *m_timestampLabel{nullptr};
    QLabel *m_nickLabel{nullptr};
    QLabel *m_contentLabel{nullptr};
};

#endif // CORAL_QT_WIDGETS_CHATBUBBLE_H
