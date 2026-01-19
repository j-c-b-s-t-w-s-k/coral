// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/widgets/addresswidget.h>

#include <QHBoxLayout>
#include <QClipboard>
#include <QApplication>
#include <QMouseEvent>
#include <QTimer>
#include <QToolTip>

AddressWidget::AddressWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

AddressWidget::AddressWidget(const QString &address, QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    setAddress(address);
}

void AddressWidget::setupUI()
{
    auto *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    m_validIcon = new QLabel(this);
    m_validIcon->setFixedSize(12, 12);
    m_validIcon->hide();
    layout->addWidget(m_validIcon);

    m_labelLabel = new QLabel(this);
    m_labelLabel->setStyleSheet("color: #808080; font-size: 10px;");
    m_labelLabel->hide();
    layout->addWidget(m_labelLabel);

    m_addressLabel = new QLabel(this);
    m_addressLabel->setStyleSheet(
        "color: #c0c0c0;"
        "font-family: 'Terminus', monospace;"
        "font-size: 12px;"
    );
    m_addressLabel->setCursor(Qt::PointingHandCursor);
    layout->addWidget(m_addressLabel, 1);

    m_copyButton = new QPushButton(tr("COPY"), this);
    m_copyButton->setFixedSize(50, 20);
    m_copyButton->setStyleSheet(
        "QPushButton {"
        "  background: #2a2a2a;"
        "  border: 1px solid #3a3a3a;"
        "  color: #808080;"
        "  font-size: 9px;"
        "  font-weight: bold;"
        "  padding: 2px 6px;"
        "}"
        "QPushButton:hover {"
        "  background: #3a3a3a;"
        "  border-color: #00d4aa;"
        "  color: #00d4aa;"
        "}"
    );
    connect(m_copyButton, &QPushButton::clicked, this, &AddressWidget::onCopyClicked);
    layout->addWidget(m_copyButton);

    setStyleSheet(
        "AddressWidget {"
        "  background: transparent;"
        "}"
    );
}

void AddressWidget::setAddress(const QString &address)
{
    if (m_address != address) {
        m_address = address;
        updateDisplay();
        Q_EMIT addressChanged(address);
    }
}

void AddressWidget::setLabel(const QString &label)
{
    m_label = label;
    if (label.isEmpty()) {
        m_labelLabel->hide();
    } else {
        m_labelLabel->setText(label + ":");
        m_labelLabel->show();
    }
}

void AddressWidget::setShowCopyButton(bool show)
{
    m_showCopyButton = show;
    m_copyButton->setVisible(show);
}

void AddressWidget::setTruncateLength(int length)
{
    m_truncateLength = length;
    updateDisplay();
}

void AddressWidget::setValid(bool valid)
{
    m_valid = valid;
    if (valid) {
        m_validIcon->setStyleSheet(
            "background: #00d4aa;"
            "border-radius: 6px;"
        );
    } else {
        m_validIcon->setStyleSheet(
            "background: #aa0000;"
            "border-radius: 6px;"
        );
    }
    m_validIcon->show();

    m_addressLabel->setStyleSheet(QString(
        "color: %1;"
        "font-family: 'Terminus', monospace;"
        "font-size: 12px;"
    ).arg(valid ? "#c0c0c0" : "#aa0000"));
}

void AddressWidget::updateDisplay()
{
    m_addressLabel->setText(truncatedAddress());
    m_addressLabel->setToolTip(m_address);
}

QString AddressWidget::truncatedAddress() const
{
    if (m_address.length() <= m_truncateLength * 2 + 3) {
        return m_address;
    }
    return m_address.left(m_truncateLength) + "..." + m_address.right(m_truncateLength);
}

void AddressWidget::onCopyClicked()
{
    QApplication::clipboard()->setText(m_address);
    Q_EMIT copyClicked(m_address);

    // Brief visual feedback
    m_copyButton->setText(tr("OK!"));
    m_copyButton->setStyleSheet(
        "QPushButton {"
        "  background: #0a5a4a;"
        "  border: 1px solid #00d4aa;"
        "  color: #00d4aa;"
        "  font-size: 9px;"
        "  font-weight: bold;"
        "}"
    );

    QTimer::singleShot(1000, this, [this]() {
        m_copyButton->setText(tr("COPY"));
        m_copyButton->setStyleSheet(
            "QPushButton {"
            "  background: #2a2a2a;"
            "  border: 1px solid #3a3a3a;"
            "  color: #808080;"
            "  font-size: 9px;"
            "  font-weight: bold;"
            "}"
            "QPushButton:hover {"
            "  background: #3a3a3a;"
            "  border-color: #00d4aa;"
            "  color: #00d4aa;"
            "}"
        );
    });
}

void AddressWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        Q_EMIT clicked(m_address);
    }
    QWidget::mousePressEvent(event);
}
