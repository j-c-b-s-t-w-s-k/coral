// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/widgets/statusbadge.h>

#include <QHBoxLayout>
#include <QMouseEvent>

StatusBadge::StatusBadge(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_label = new QLabel(this);
    m_label->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_label);

    setFixedHeight(20);
    setCursor(Qt::PointingHandCursor);

    updateAppearance();
}

StatusBadge::StatusBadge(Status status, QWidget *parent)
    : StatusBadge(parent)
{
    setStatus(status);
}

void StatusBadge::setStatus(Status status)
{
    if (m_status != status) {
        m_status = status;
        m_customText.clear();
        updateAppearance();
        Q_EMIT statusChanged(status);
    }
}

void StatusBadge::setCustomText(const QString &text)
{
    m_customText = text;
    updateAppearance();
}

QString StatusBadge::text() const
{
    return m_customText.isEmpty() ? statusToString(m_status) : m_customText;
}

QString StatusBadge::statusToString(Status status)
{
    switch (status) {
    case Pending:   return tr("PENDING");
    case Active:    return tr("ACTIVE");
    case Funded:    return tr("FUNDED");
    case Completed: return tr("COMPLETED");
    case Disputed:  return tr("DISPUTED");
    case Cancelled: return tr("CANCELLED");
    case Expired:   return tr("EXPIRED");
    case Error:     return tr("ERROR");
    }
    return tr("UNKNOWN");
}

QString StatusBadge::statusToStyleClass(Status status)
{
    switch (status) {
    case Pending:   return "pending";
    case Active:    return "active";
    case Funded:    return "funded";
    case Completed: return "completed";
    case Disputed:  return "disputed";
    case Cancelled: return "cancelled";
    case Expired:   return "expired";
    case Error:     return "error";
    }
    return "pending";
}

void StatusBadge::updateAppearance()
{
    m_label->setText(text());

    // Colors based on status
    QString bgColor, textColor, borderColor;

    switch (m_status) {
    case Pending:
        bgColor = "#3a3a3a"; textColor = "#808080"; borderColor = "#4a4a4a";
        break;
    case Active:
        bgColor = "#0a4a6a"; textColor = "#00aaff"; borderColor = "#00aaff";
        break;
    case Funded:
        bgColor = "#0a5a4a"; textColor = "#00d4aa"; borderColor = "#00d4aa";
        break;
    case Completed:
        bgColor = "#2a2a2a"; textColor = "#ffffff"; borderColor = "#5a5a5a";
        break;
    case Disputed:
        bgColor = "#5a4a0a"; textColor = "#d4aa00"; borderColor = "#d4aa00";
        break;
    case Cancelled:
    case Expired:
        bgColor = "#2a2a2a"; textColor = "#808080"; borderColor = "#3a3a3a";
        break;
    case Error:
        bgColor = "#4a0a0a"; textColor = "#ff4444"; borderColor = "#aa0000";
        break;
    }

    setStyleSheet(QString(
        "StatusBadge {"
        "  background-color: %1;"
        "  border: 1px solid %3;"
        "  border-radius: 2px;"
        "  padding: 2px 8px;"
        "}"
        "StatusBadge QLabel {"
        "  color: %2;"
        "  font-size: 10px;"
        "  font-weight: bold;"
        "  font-family: 'Terminus', monospace;"
        "  background: transparent;"
        "}"
    ).arg(bgColor, textColor, borderColor));
}

void StatusBadge::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        Q_EMIT clicked();
    }
    QWidget::mousePressEvent(event);
}

void StatusBadge::enterEvent(QEvent *event)
{
    Q_UNUSED(event);
    // Could add hover effect here
}

void StatusBadge::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    // Could remove hover effect here
}
