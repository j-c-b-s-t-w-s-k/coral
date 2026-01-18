// Copyright (c) 2026 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CORAL_QT_MARKETPAGE_H
#define CORAL_QT_MARKETPAGE_H

#include <QWidget>
#include <QTimer>

class ClientModel;
class WalletModel;
class PlatformStyle;

QT_BEGIN_NAMESPACE
class QLabel;
class QLineEdit;
class QTextEdit;
class QPushButton;
class QProgressBar;
class QGroupBox;
class QSpinBox;
class QDoubleSpinBox;
class QComboBox;
class QListWidget;
class QListWidgetItem;
class QTabWidget;
class QVBoxLayout;
QT_END_NAMESPACE

// Listing structure for marketplace items
struct MarketListing {
    QString id;
    QString title;
    QString description;
    QString category;
    double price;
    QString sellerAddress;
    QString imageHash;  // IPFS hash for image
    qint64 timestamp;
    bool isActive;
};

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
    void onBuyItemClicked();
    void onRefreshListingsClicked();
    void onListingSelected(QListWidgetItem *item);
    void onCategoryChanged(int index);
    void onSearchClicked();
    void refreshListings();

private:
    void setupUI();
    void createBrowseTab(QVBoxLayout *layout);
    void createSellTab(QVBoxLayout *layout);
    void createMyListingsTab(QVBoxLayout *layout);
    void addMockListings();  // Temporary: add demo listings

    const PlatformStyle *platformStyle;
    ClientModel *clientModel{nullptr};
    WalletModel *walletModel{nullptr};
    QTimer *refreshTimer{nullptr};
    QTabWidget *tabWidget{nullptr};

    // Browse tab
    QListWidget *listingsWidget{nullptr};
    QComboBox *categoryFilter{nullptr};
    QLineEdit *searchEdit{nullptr};
    QPushButton *searchButton{nullptr};
    QPushButton *buyButton{nullptr};
    QLabel *itemTitleLabel{nullptr};
    QLabel *itemPriceLabel{nullptr};
    QLabel *itemDescriptionLabel{nullptr};
    QLabel *itemSellerLabel{nullptr};

    // Sell tab
    QLineEdit *listingTitleEdit{nullptr};
    QTextEdit *listingDescriptionEdit{nullptr};
    QComboBox *listingCategoryCombo{nullptr};
    QDoubleSpinBox *listingPriceSpinBox{nullptr};
    QPushButton *createListingButton{nullptr};
    QLabel *listingStatusLabel{nullptr};

    // My Listings tab
    QListWidget *myListingsWidget{nullptr};
    QPushButton *removeListingButton{nullptr};

    // Mock data storage (temporary)
    QList<MarketListing> mockListings;
    MarketListing *selectedListing{nullptr};
};

#endif // CORAL_QT_MARKETPAGE_H
