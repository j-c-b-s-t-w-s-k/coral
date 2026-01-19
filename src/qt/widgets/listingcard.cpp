// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/widgets/listingcard.h>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QDateTime>

ListingCard::ListingCard(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

ListingCard::ListingCard(const ListingData &data, QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    setData(data);
}

void ListingCard::setupUI()
{
    setMinimumSize(250, 120);
    setCursor(Qt::PointingHandCursor);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(6);

    // Top row: title + status
    auto *topRow = new QHBoxLayout();
    topRow->setSpacing(8);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setStyleSheet(
        "color: #ffffff;"
        "font-size: 14px;"
        "font-weight: bold;"
        "font-family: 'Terminus', monospace;"
    );
    m_titleLabel->setWordWrap(true);
    topRow->addWidget(m_titleLabel, 1);

    m_statusBadge = new StatusBadge(StatusBadge::Active, this);
    topRow->addWidget(m_statusBadge);

    mainLayout->addLayout(topRow);

    // Category + time row
    auto *metaRow = new QHBoxLayout();

    m_categoryLabel = new QLabel(this);
    m_categoryLabel->setStyleSheet(
        "color: #808080;"
        "font-size: 10px;"
        "font-family: 'Terminus', monospace;"
    );
    metaRow->addWidget(m_categoryLabel);

    metaRow->addStretch();

    m_timeLabel = new QLabel(this);
    m_timeLabel->setStyleSheet(
        "color: #4a4a4a;"
        "font-size: 10px;"
        "font-family: 'Terminus', monospace;"
    );
    metaRow->addWidget(m_timeLabel);

    mainLayout->addLayout(metaRow);

    // Description (hidden in compact mode)
    m_descLabel = new QLabel(this);
    m_descLabel->setStyleSheet(
        "color: #a0a0a0;"
        "font-size: 11px;"
        "font-family: 'Terminus', monospace;"
    );
    m_descLabel->setWordWrap(true);
    m_descLabel->setMaximumHeight(40);
    mainLayout->addWidget(m_descLabel);

    // Seller address
    m_sellerLabel = new QLabel(this);
    m_sellerLabel->setStyleSheet(
        "color: #606060;"
        "font-size: 10px;"
        "font-family: 'Terminus', monospace;"
    );
    mainLayout->addWidget(m_sellerLabel);

    mainLayout->addStretch();

    // Bottom row: price + buy button
    auto *bottomRow = new QHBoxLayout();
    bottomRow->setSpacing(8);

    m_priceLabel = new QLabel(this);
    m_priceLabel->setStyleSheet(
        "color: #00d4aa;"
        "font-size: 18px;"
        "font-weight: bold;"
        "font-family: 'Terminus', monospace;"
    );
    bottomRow->addWidget(m_priceLabel);

    bottomRow->addStretch();

    m_buyButton = new QPushButton(tr("BUY"), this);
    m_buyButton->setStyleSheet(
        "QPushButton {"
        "  background: qlineargradient(y1:0, y2:1, stop:0 #0a6a5a, stop:1 #064a3a);"
        "  border: 2px solid;"
        "  border-color: #00d4aa #043a2a #043a2a #00d4aa;"
        "  color: #00d4aa;"
        "  font-size: 12px;"
        "  font-weight: bold;"
        "  font-family: 'Terminus', monospace;"
        "  padding: 6px 20px;"
        "}"
        "QPushButton:hover {"
        "  background: qlineargradient(y1:0, y2:1, stop:0 #0a8a7a, stop:1 #066a5a);"
        "  color: #33ffcc;"
        "}"
        "QPushButton:pressed {"
        "  border-color: #043a2a #00d4aa #00d4aa #043a2a;"
        "}"
    );
    connect(m_buyButton, &QPushButton::clicked, this, &ListingCard::onBuyClicked);
    bottomRow->addWidget(m_buyButton);

    mainLayout->addLayout(bottomRow);

    updateDisplay();
}

void ListingCard::setData(const ListingData &data)
{
    m_data = data;
    updateDisplay();
}

void ListingCard::setCompact(bool compact)
{
    m_compact = compact;
    m_descLabel->setVisible(!compact);
    if (compact) {
        setMinimumSize(200, 80);
        setMaximumHeight(100);
    } else {
        setMinimumSize(250, 120);
        setMaximumHeight(16777215);
    }
}

void ListingCard::setShowBuyButton(bool show)
{
    m_buyButton->setVisible(show);
}

void ListingCard::updateDisplay()
{
    m_titleLabel->setText(m_data.title.isEmpty() ? tr("Untitled") : m_data.title);
    m_priceLabel->setText(formatPrice(m_data.price));
    m_categoryLabel->setText(m_data.category.isEmpty() ? tr("General") : m_data.category.toUpper());
    m_timeLabel->setText(formatTimestamp(m_data.timestamp));
    m_statusBadge->setStatus(m_data.status);

    if (!m_data.description.isEmpty()) {
        QString desc = m_data.description;
        if (desc.length() > 80) {
            desc = desc.left(77) + "...";
        }
        m_descLabel->setText(desc);
    } else {
        m_descLabel->setText("");
    }

    if (!m_data.sellerAddress.isEmpty()) {
        QString addr = m_data.sellerAddress;
        if (addr.length() > 20) {
            addr = addr.left(8) + "..." + addr.right(8);
        }
        m_sellerLabel->setText(tr("Seller: %1").arg(addr));
    } else {
        m_sellerLabel->setText("");
    }
}

QString ListingCard::formatPrice(CAmount amount) const
{
    double crl = static_cast<double>(amount) / 100000000.0;
    return QString("%1 CRL").arg(crl, 0, 'f', 8);
}

QString ListingCard::formatTimestamp(int64_t timestamp) const
{
    if (timestamp == 0) return "";

    QDateTime dt = QDateTime::fromSecsSinceEpoch(timestamp);
    QDateTime now = QDateTime::currentDateTime();
    qint64 secs = dt.secsTo(now);

    if (secs < 60) return tr("just now");
    if (secs < 3600) return tr("%1m ago").arg(secs / 60);
    if (secs < 86400) return tr("%1h ago").arg(secs / 3600);
    if (secs < 604800) return tr("%1d ago").arg(secs / 86400);
    return dt.toString("MMM d");
}

void ListingCard::onBuyClicked()
{
    Q_EMIT buyClicked(QString::fromStdString(m_data.id.GetHex()));
}

void ListingCard::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        Q_EMIT clicked(QString::fromStdString(m_data.id.GetHex()));
    }
    QWidget::mousePressEvent(event);
}

void ListingCard::enterEvent(QEvent *event)
{
    Q_UNUSED(event);
    m_hovered = true;
    update();
}

void ListingCard::leaveEvent(QEvent *event)
{
    Q_UNUSED(event);
    m_hovered = false;
    update();
}

void ListingCard::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRect r = rect();

    // Background
    p.fillRect(r, QColor("#1a1a1a"));

    // Border with Win95 bevel effect
    if (m_hovered) {
        // Organic glow on hover
        p.setPen(QPen(QColor("#00d4aa"), 2));
        p.drawRect(r.adjusted(1, 1, -1, -1));
        p.setPen(QPen(QColor("#0a5a4a"), 1));
        p.drawRect(r.adjusted(2, 2, -2, -2));
    } else {
        // Standard Win95 bevel
        p.setPen(QColor("#5a5a5a"));
        p.drawLine(r.topLeft(), r.topRight());
        p.drawLine(r.topLeft(), r.bottomLeft());
        p.setPen(QColor("#1a1a1a"));
        p.drawLine(r.bottomLeft(), r.bottomRight());
        p.drawLine(r.topRight(), r.bottomRight());
    }
}
