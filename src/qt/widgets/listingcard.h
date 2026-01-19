// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CORAL_QT_WIDGETS_LISTINGCARD_H
#define CORAL_QT_WIDGETS_LISTINGCARD_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>

#include <qt/widgets/statusbadge.h>
#include <consensus/amount.h>
#include <uint256.h>

/**
 * Atomic listing card widget for displaying marketplace items.
 * Shows title, price, category, seller, and status badge.
 */
class ListingCard : public QWidget
{
    Q_OBJECT

public:
    struct ListingData {
        uint256 id;
        QString title;
        QString description;
        QString category;
        CAmount price;
        QString sellerAddress;
        StatusBadge::Status status;
        int64_t timestamp;
    };

    explicit ListingCard(QWidget *parent = nullptr);
    explicit ListingCard(const ListingData &data, QWidget *parent = nullptr);
    ~ListingCard() = default;

    void setData(const ListingData &data);
    ListingData data() const { return m_data; }

    void setCompact(bool compact);
    bool isCompact() const { return m_compact; }

    void setShowBuyButton(bool show);

Q_SIGNALS:
    void clicked(const QString &listingId);
    void buyClicked(const QString &listingId);
    void moreInfoClicked(const QString &listingId);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private Q_SLOTS:
    void onBuyClicked();

private:
    void setupUI();
    void updateDisplay();
    QString formatPrice(CAmount amount) const;
    QString formatTimestamp(int64_t timestamp) const;

    ListingData m_data;
    bool m_compact{false};
    bool m_hovered{false};

    QLabel *m_titleLabel{nullptr};
    QLabel *m_priceLabel{nullptr};
    QLabel *m_categoryLabel{nullptr};
    QLabel *m_sellerLabel{nullptr};
    QLabel *m_timeLabel{nullptr};
    QLabel *m_descLabel{nullptr};
    StatusBadge *m_statusBadge{nullptr};
    QPushButton *m_buyButton{nullptr};
};

#endif // CORAL_QT_WIDGETS_LISTINGCARD_H
