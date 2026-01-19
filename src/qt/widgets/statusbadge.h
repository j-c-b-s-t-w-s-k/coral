// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CORAL_QT_WIDGETS_STATUSBADGE_H
#define CORAL_QT_WIDGETS_STATUSBADGE_H

#include <QWidget>
#include <QLabel>

/**
 * Atomic status badge widget for displaying status indicators.
 * Supports: PENDING, ACTIVE, FUNDED, COMPLETED, DISPUTED, CANCELLED, EXPIRED, ERROR
 */
class StatusBadge : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(Status status READ status WRITE setStatus NOTIFY statusChanged)

public:
    enum Status {
        Pending = 0,
        Active,
        Funded,
        Completed,
        Disputed,
        Cancelled,
        Expired,
        Error
    };
    Q_ENUM(Status)

    explicit StatusBadge(QWidget *parent = nullptr);
    explicit StatusBadge(Status status, QWidget *parent = nullptr);
    ~StatusBadge() = default;

    Status status() const { return m_status; }
    void setStatus(Status status);

    void setCustomText(const QString &text);
    QString text() const;

    static QString statusToString(Status status);
    static QString statusToStyleClass(Status status);

Q_SIGNALS:
    void statusChanged(Status newStatus);
    void clicked();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;

private:
    void updateAppearance();

    Status m_status{Pending};
    QString m_customText;
    QLabel *m_label{nullptr};
};

#endif // CORAL_QT_WIDGETS_STATUSBADGE_H
