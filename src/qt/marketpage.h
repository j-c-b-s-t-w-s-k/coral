// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CORAL_QT_MARKETPAGE_H
#define CORAL_QT_MARKETPAGE_H

#include <QWidget>
#include <QTimer>

class ClientModel;
class WalletModel;
class PlatformStyle;
class ListingCard;
class StatusBadge;

QT_BEGIN_NAMESPACE
class QLabel;
class QLineEdit;
class QTextEdit;
class QPushButton;
class QGroupBox;
class QDoubleSpinBox;
class QComboBox;
class QListWidget;
class QListWidgetItem;
class QTabWidget;
class QVBoxLayout;
class QScrollArea;
class QGridLayout;
QT_END_NAMESPACE

/**
 * Marketplace page with bioport terminal aesthetic.
 * Browse, buy, and sell items using CRL with escrow protection.
 */
class MarketPage : public QWidget
{
    Q_OBJECT

public:
    explicit MarketPage(const PlatformStyle *platformStyle, QWidget *parent = nullptr);
    ~MarketPage();

    void setClientModel(ClientModel *model);
    void setWalletModel(WalletModel *model);

private Q_SLOTS:
    void onCreateListingClicked();
    void onBuyItemClicked(const QString &listingId);
    void onListingClicked(const QString &listingId);
    void onCategoryChanged(int index);
    void onSearchClicked();
    void refreshListings();
    void updateBalance();

Q_SIGNALS:
    void message(const QString &title, const QString &message, unsigned int style);

private:
    void setupUI();
    void applyBioportTheme();
    void createBrowseTab(QWidget *tab);
    void createSellTab(QWidget *tab);
    void createOrdersTab(QWidget *tab);
    void createMyListingsTab(QWidget *tab);
    void populateListings();
    void showListingDetails(const QString &listingId);
    QString formatPrice(double amount) const;

    const PlatformStyle *platformStyle;
    ClientModel *clientModel{nullptr};
    WalletModel *walletModel{nullptr};
    QTimer *refreshTimer{nullptr};
    QTabWidget *tabWidget{nullptr};

    // Header
    QLabel *titleLabel{nullptr};
    QLabel *balanceLabel{nullptr};

    // Browse tab
    QScrollArea *listingsScrollArea{nullptr};
    QWidget *listingsContainer{nullptr};
    QGridLayout *listingsGrid{nullptr};
    QComboBox *categoryFilter{nullptr};
    QLineEdit *searchEdit{nullptr};
    QPushButton *searchButton{nullptr};
    QWidget *detailsPanel{nullptr};
    QLabel *detailTitleLabel{nullptr};
    QLabel *detailPriceLabel{nullptr};
    QLabel *detailDescLabel{nullptr};
    QLabel *detailSellerLabel{nullptr};
    StatusBadge *detailStatusBadge{nullptr};
    QPushButton *buyButton{nullptr};

    // Sell tab
    QLineEdit *listingTitleEdit{nullptr};
    QTextEdit *listingDescriptionEdit{nullptr};
    QComboBox *listingCategoryCombo{nullptr};
    QDoubleSpinBox *listingPriceSpinBox{nullptr};
    QPushButton *createListingButton{nullptr};
    QLabel *listingStatusLabel{nullptr};

    // Orders tab
    QListWidget *ordersWidget{nullptr};

    // My Listings tab
    QListWidget *myListingsWidget{nullptr};
    QPushButton *removeListingButton{nullptr};

    // Current selection
    QString selectedListingId;
};

#endif // CORAL_QT_MARKETPAGE_H
