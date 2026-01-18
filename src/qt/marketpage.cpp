// Copyright (c) 2026 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/marketpage.h>

#include <qt/clientmodel.h>
#include <qt/guiutil.h>
#include <qt/platformstyle.h>
#include <qt/walletmodel.h>

#include <QComboBox>
#include <QDateTime>
#include <QDoubleSpinBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QSpinBox>
#include <QSplitter>
#include <QTabWidget>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QFont>
#include <QUuid>

MarketPage::MarketPage(const PlatformStyle *_platformStyle, QWidget *parent)
    : QWidget(parent)
    , platformStyle(_platformStyle)
{
    setupUI();
    addMockListings();

    // Refresh listings periodically
    refreshTimer = new QTimer(this);
    connect(refreshTimer, &QTimer::timeout, this, &MarketPage::refreshListings);
    refreshTimer->start(30000);  // Refresh every 30 seconds
}

MarketPage::~MarketPage()
{
    if (refreshTimer) {
        refreshTimer->stop();
    }
}

void MarketPage::setupUI()
{
    // Dark theme - white on black with serif fonts
    setStyleSheet(
        "MarketPage { background-color: #000000; }"
        "QLabel { color: #ffffff; font-family: Georgia, serif; }"
        "QGroupBox { color: #ffffff; background-color: #111111; border: 1px solid #333333; border-radius: 8px; margin-top: 10px; padding-top: 10px; font-family: Georgia, serif; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; color: #888888; }"
        "QTabWidget::pane { border: 1px solid #333333; background-color: #000000; }"
        "QTabBar::tab { background-color: #111111; color: #888888; padding: 10px 20px; border: 1px solid #333333; font-family: Georgia, serif; }"
        "QTabBar::tab:selected { background-color: #222222; color: #ffffff; border-bottom: 2px solid #ffffff; }"
        "QListWidget { background-color: #111111; color: #ffffff; border: 1px solid #333333; font-family: Georgia, serif; }"
        "QListWidget::item { padding: 10px; border-bottom: 1px solid #222222; }"
        "QListWidget::item:selected { background-color: #222222; }"
    );

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // Title
    QLabel *titleLabel = new QLabel(tr("Coral Market"));
    titleLabel->setStyleSheet("font-size: 28px; font-weight: bold; color: #ffffff; font-family: Georgia, serif;");
    mainLayout->addWidget(titleLabel);

    QLabel *subtitleLabel = new QLabel(tr("Decentralized peer-to-peer marketplace powered by Coral"));
    subtitleLabel->setStyleSheet("font-size: 14px; color: #888888; font-family: Georgia, serif; margin-bottom: 10px;");
    mainLayout->addWidget(subtitleLabel);

    // Tab widget for different sections
    tabWidget = new QTabWidget();
    tabWidget->setStyleSheet(
        "QTabWidget::pane { border: 1px solid #333333; background-color: #000000; }"
        "QTabBar::tab { background-color: #111111; color: #888888; padding: 12px 24px; border: 1px solid #333333; font-family: Georgia, serif; }"
        "QTabBar::tab:selected { background-color: #222222; color: #ffffff; border-bottom: 2px solid #ffffff; }"
    );

    // Browse tab
    QWidget *browseTab = new QWidget();
    QVBoxLayout *browseLayout = new QVBoxLayout(browseTab);
    browseLayout->setContentsMargins(10, 10, 10, 10);
    createBrowseTab(browseLayout);
    tabWidget->addTab(browseTab, tr("Browse"));

    // Sell tab
    QWidget *sellTab = new QWidget();
    QVBoxLayout *sellLayout = new QVBoxLayout(sellTab);
    sellLayout->setContentsMargins(10, 10, 10, 10);
    createSellTab(sellLayout);
    tabWidget->addTab(sellTab, tr("Sell"));

    // My Listings tab
    QWidget *myListingsTab = new QWidget();
    QVBoxLayout *myListingsLayout = new QVBoxLayout(myListingsTab);
    myListingsLayout->setContentsMargins(10, 10, 10, 10);
    createMyListingsTab(myListingsLayout);
    tabWidget->addTab(myListingsTab, tr("My Listings"));

    mainLayout->addWidget(tabWidget);
    setLayout(mainLayout);
}

void MarketPage::createBrowseTab(QVBoxLayout *layout)
{
    // Search and filter bar
    QHBoxLayout *filterLayout = new QHBoxLayout();

    QLabel *categoryLabel = new QLabel(tr("Category:"));
    categoryLabel->setStyleSheet("font-size: 14px;");
    categoryFilter = new QComboBox();
    categoryFilter->setStyleSheet("padding: 8px; border: 1px solid #333333; border-radius: 4px; background-color: #111111; color: #ffffff;");
    categoryFilter->addItem(tr("All Categories"), "all");
    categoryFilter->addItem(tr("Electronics"), "electronics");
    categoryFilter->addItem(tr("Clothing"), "clothing");
    categoryFilter->addItem(tr("Collectibles"), "collectibles");
    categoryFilter->addItem(tr("Art & NFTs"), "art");
    categoryFilter->addItem(tr("Services"), "services");
    categoryFilter->addItem(tr("Other"), "other");
    connect(categoryFilter, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MarketPage::onCategoryChanged);

    searchEdit = new QLineEdit();
    searchEdit->setPlaceholderText(tr("Search listings..."));
    searchEdit->setStyleSheet("padding: 10px; border: 1px solid #333333; border-radius: 4px; background-color: #111111; color: #ffffff; font-family: Georgia, serif;");

    searchButton = new QPushButton(tr("Search"));
    searchButton->setStyleSheet(
        "QPushButton { background-color: #222222; color: #ffffff; padding: 10px 20px; border: 1px solid #444444; border-radius: 4px; font-family: Georgia, serif; }"
        "QPushButton:hover { background-color: #333333; border-color: #ffffff; }"
    );
    connect(searchButton, &QPushButton::clicked, this, &MarketPage::onSearchClicked);

    filterLayout->addWidget(categoryLabel);
    filterLayout->addWidget(categoryFilter);
    filterLayout->addWidget(searchEdit, 1);
    filterLayout->addWidget(searchButton);
    layout->addLayout(filterLayout);

    // Main content area with splitter
    QSplitter *splitter = new QSplitter(Qt::Horizontal);
    splitter->setStyleSheet("QSplitter::handle { background-color: #333333; }");

    // Listings list on the left
    listingsWidget = new QListWidget();
    listingsWidget->setMinimumWidth(300);
    connect(listingsWidget, &QListWidget::itemClicked, this, &MarketPage::onListingSelected);
    splitter->addWidget(listingsWidget);

    // Item details on the right
    QWidget *detailsWidget = new QWidget();
    QVBoxLayout *detailsLayout = new QVBoxLayout(detailsWidget);
    detailsLayout->setSpacing(15);

    itemTitleLabel = new QLabel(tr("Select an item to view details"));
    itemTitleLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #ffffff;");
    itemTitleLabel->setWordWrap(true);

    itemPriceLabel = new QLabel();
    itemPriceLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #ffffff;");

    itemDescriptionLabel = new QLabel();
    itemDescriptionLabel->setStyleSheet("font-size: 14px; color: #cccccc;");
    itemDescriptionLabel->setWordWrap(true);

    itemSellerLabel = new QLabel();
    itemSellerLabel->setStyleSheet("font-size: 12px; color: #888888; font-family: monospace;");

    buyButton = new QPushButton(tr("Buy Now"));
    buyButton->setEnabled(false);
    buyButton->setStyleSheet(
        "QPushButton { background-color: #222222; color: #ffffff; padding: 15px 30px; border: 1px solid #444444; border-radius: 4px; font-weight: bold; font-size: 16px; font-family: Georgia, serif; }"
        "QPushButton:hover { background-color: #333333; border-color: #ffffff; }"
        "QPushButton:disabled { background-color: #1a1a1a; color: #666666; }"
    );
    connect(buyButton, &QPushButton::clicked, this, &MarketPage::onBuyItemClicked);

    detailsLayout->addWidget(itemTitleLabel);
    detailsLayout->addWidget(itemPriceLabel);
    detailsLayout->addWidget(itemDescriptionLabel);
    detailsLayout->addWidget(itemSellerLabel);
    detailsLayout->addStretch();
    detailsLayout->addWidget(buyButton);

    splitter->addWidget(detailsWidget);
    splitter->setSizes({400, 600});

    layout->addWidget(splitter, 1);
}

void MarketPage::createSellTab(QVBoxLayout *layout)
{
    QGroupBox *createGroup = new QGroupBox(tr("Create New Listing"));
    QVBoxLayout *createLayout = new QVBoxLayout(createGroup);
    createLayout->setSpacing(15);

    // Title
    QHBoxLayout *titleLayout = new QHBoxLayout();
    QLabel *titleLbl = new QLabel(tr("Title:"));
    titleLbl->setStyleSheet("font-size: 14px; min-width: 80px;");
    listingTitleEdit = new QLineEdit();
    listingTitleEdit->setPlaceholderText(tr("Enter item title"));
    listingTitleEdit->setStyleSheet("padding: 10px; border: 1px solid #333333; border-radius: 4px; background-color: #111111; color: #ffffff;");
    titleLayout->addWidget(titleLbl);
    titleLayout->addWidget(listingTitleEdit, 1);
    createLayout->addLayout(titleLayout);

    // Category
    QHBoxLayout *catLayout = new QHBoxLayout();
    QLabel *catLbl = new QLabel(tr("Category:"));
    catLbl->setStyleSheet("font-size: 14px; min-width: 80px;");
    listingCategoryCombo = new QComboBox();
    listingCategoryCombo->setStyleSheet("padding: 8px; border: 1px solid #333333; border-radius: 4px; background-color: #111111; color: #ffffff;");
    listingCategoryCombo->addItem(tr("Electronics"), "electronics");
    listingCategoryCombo->addItem(tr("Clothing"), "clothing");
    listingCategoryCombo->addItem(tr("Collectibles"), "collectibles");
    listingCategoryCombo->addItem(tr("Art & NFTs"), "art");
    listingCategoryCombo->addItem(tr("Services"), "services");
    listingCategoryCombo->addItem(tr("Other"), "other");
    catLayout->addWidget(catLbl);
    catLayout->addWidget(listingCategoryCombo, 1);
    createLayout->addLayout(catLayout);

    // Price
    QHBoxLayout *priceLayout = new QHBoxLayout();
    QLabel *priceLbl = new QLabel(tr("Price (CRL):"));
    priceLbl->setStyleSheet("font-size: 14px; min-width: 80px;");
    listingPriceSpinBox = new QDoubleSpinBox();
    listingPriceSpinBox->setRange(0.00000001, 1000000.0);
    listingPriceSpinBox->setDecimals(8);
    listingPriceSpinBox->setValue(1.0);
    listingPriceSpinBox->setStyleSheet("padding: 8px; border: 1px solid #333333; border-radius: 4px; background-color: #111111; color: #ffffff;");
    priceLayout->addWidget(priceLbl);
    priceLayout->addWidget(listingPriceSpinBox, 1);
    createLayout->addLayout(priceLayout);

    // Description
    QLabel *descLbl = new QLabel(tr("Description:"));
    descLbl->setStyleSheet("font-size: 14px;");
    listingDescriptionEdit = new QTextEdit();
    listingDescriptionEdit->setPlaceholderText(tr("Describe your item in detail..."));
    listingDescriptionEdit->setStyleSheet("padding: 10px; border: 1px solid #333333; border-radius: 4px; background-color: #111111; color: #ffffff; font-family: Georgia, serif;");
    listingDescriptionEdit->setMaximumHeight(150);
    createLayout->addWidget(descLbl);
    createLayout->addWidget(listingDescriptionEdit);

    // Create button
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    createListingButton = new QPushButton(tr("Create Listing"));
    createListingButton->setStyleSheet(
        "QPushButton { background-color: #222222; color: #ffffff; padding: 12px 24px; border: 1px solid #444444; border-radius: 4px; font-weight: bold; font-family: Georgia, serif; }"
        "QPushButton:hover { background-color: #333333; border-color: #ffffff; }"
    );
    connect(createListingButton, &QPushButton::clicked, this, &MarketPage::onCreateListingClicked);
    buttonLayout->addStretch();
    buttonLayout->addWidget(createListingButton);
    createLayout->addLayout(buttonLayout);

    listingStatusLabel = new QLabel();
    listingStatusLabel->setStyleSheet("font-size: 14px; color: #888888;");
    createLayout->addWidget(listingStatusLabel);

    layout->addWidget(createGroup);

    // Info box
    QGroupBox *infoGroup = new QGroupBox(tr("How It Works"));
    QVBoxLayout *infoLayout = new QVBoxLayout(infoGroup);
    QLabel *infoLabel = new QLabel(
        tr("1. Create a listing with your item details and price in CRL\n"
           "2. Your listing is broadcast to the Coral network\n"
           "3. Buyers can purchase directly using Coral\n"
           "4. Funds are held in escrow until delivery is confirmed\n"
           "5. Disputes are resolved by Coral moderators\n\n"
           "Listing fee: 0.001 CRL (to prevent spam)")
    );
    infoLabel->setStyleSheet("font-size: 13px; color: #888888; line-height: 1.5;");
    infoLabel->setWordWrap(true);
    infoLayout->addWidget(infoLabel);
    layout->addWidget(infoGroup);

    layout->addStretch();
}

void MarketPage::createMyListingsTab(QVBoxLayout *layout)
{
    QLabel *headerLabel = new QLabel(tr("Your Active Listings"));
    headerLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: #ffffff;");
    layout->addWidget(headerLabel);

    myListingsWidget = new QListWidget();
    layout->addWidget(myListingsWidget, 1);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    removeListingButton = new QPushButton(tr("Remove Selected"));
    removeListingButton->setStyleSheet(
        "QPushButton { background-color: #222222; color: #ffffff; padding: 10px 20px; border: 1px solid #444444; border-radius: 4px; font-family: Georgia, serif; }"
        "QPushButton:hover { background-color: #333333; border-color: #ffffff; }"
    );
    buttonLayout->addStretch();
    buttonLayout->addWidget(removeListingButton);
    layout->addLayout(buttonLayout);
}

void MarketPage::setClientModel(ClientModel *model)
{
    clientModel = model;
}

void MarketPage::setWalletModel(WalletModel *model)
{
    walletModel = model;
}

void MarketPage::addMockListings()
{
    // Add some demo listings for UI demonstration
    MarketListing l1;
    l1.id = QUuid::createUuid().toString();
    l1.title = "Vintage Bitcoin Hardware Wallet";
    l1.description = "Original Trezor Model One, never used, still in sealed packaging. A piece of crypto history!";
    l1.category = "electronics";
    l1.price = 0.5;
    l1.sellerAddress = "coral1qw508d6qejxtdg4y5r3zarvary0c5xw7kv8f3t4";
    l1.timestamp = QDateTime::currentSecsSinceEpoch();
    l1.isActive = true;
    mockListings.append(l1);

    MarketListing l2;
    l2.id = QUuid::createUuid().toString();
    l2.title = "Custom Crypto Art Print";
    l2.description = "Limited edition digital art print, signed by the artist. Physical canvas print shipped worldwide.";
    l2.category = "art";
    l2.price = 0.1;
    l2.sellerAddress = "coral1q9sxk9fv3r5g4r7hs8m6j2qz4k2h7y4d5w6e8c";
    l2.timestamp = QDateTime::currentSecsSinceEpoch() - 3600;
    l2.isActive = true;
    mockListings.append(l2);

    MarketListing l3;
    l3.id = QUuid::createUuid().toString();
    l3.title = "Freelance Smart Contract Audit";
    l3.description = "Professional smart contract security audit. 5+ years experience. Detailed report included.";
    l3.category = "services";
    l3.price = 2.0;
    l3.sellerAddress = "coral1q8v7m3f5k2j9h6g4s1p0o7i8u6y5t4r3e2w1q";
    l3.timestamp = QDateTime::currentSecsSinceEpoch() - 7200;
    l3.isActive = true;
    mockListings.append(l3);

    MarketListing l4;
    l4.id = QUuid::createUuid().toString();
    l4.title = "Rare Crypto Collectible Card";
    l4.description = "Physical trading card from the 2017 crypto boom era. Excellent condition, comes with certificate of authenticity.";
    l4.category = "collectibles";
    l4.price = 0.25;
    l4.sellerAddress = "coral1qm5k8v7n3j2h1g9f6d4s3a2p1o0i8u7y6t5r4";
    l4.timestamp = QDateTime::currentSecsSinceEpoch() - 10800;
    l4.isActive = true;
    mockListings.append(l4);

    refreshListings();
}

void MarketPage::onCreateListingClicked()
{
    QString title = listingTitleEdit->text().trimmed();
    QString description = listingDescriptionEdit->toPlainText().trimmed();
    double price = listingPriceSpinBox->value();

    if (title.isEmpty()) {
        listingStatusLabel->setText(tr("Please enter a title for your listing"));
        listingStatusLabel->setStyleSheet("font-size: 14px; color: #ff6666;");
        return;
    }

    if (description.isEmpty()) {
        listingStatusLabel->setText(tr("Please enter a description"));
        listingStatusLabel->setStyleSheet("font-size: 14px; color: #ff6666;");
        return;
    }

    if (price <= 0) {
        listingStatusLabel->setText(tr("Please enter a valid price"));
        listingStatusLabel->setStyleSheet("font-size: 14px; color: #ff6666;");
        return;
    }

    // Create new listing (in real implementation, this would broadcast to network)
    MarketListing newListing;
    newListing.id = QUuid::createUuid().toString();
    newListing.title = title;
    newListing.description = description;
    newListing.category = listingCategoryCombo->currentData().toString();
    newListing.price = price;
    newListing.sellerAddress = "your_address_here";  // Would come from wallet
    newListing.timestamp = QDateTime::currentSecsSinceEpoch();
    newListing.isActive = true;

    mockListings.prepend(newListing);

    // Clear form
    listingTitleEdit->clear();
    listingDescriptionEdit->clear();
    listingPriceSpinBox->setValue(1.0);

    listingStatusLabel->setText(tr("Listing created successfully! It will appear in the marketplace shortly."));
    listingStatusLabel->setStyleSheet("font-size: 14px; color: #ffffff;");

    refreshListings();
}

void MarketPage::onBuyItemClicked()
{
    if (!selectedListing) return;

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("Confirm Purchase"),
        tr("Are you sure you want to buy \"%1\" for %2 CRL?\n\n"
           "The funds will be held in escrow until you confirm receipt of the item.")
           .arg(selectedListing->title)
           .arg(selectedListing->price, 0, 'f', 8),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        // In real implementation, this would create an escrow transaction
        QMessageBox::information(
            this,
            tr("Purchase Initiated"),
            tr("Your purchase has been initiated!\n\n"
               "Transaction ID: %1\n\n"
               "Please contact the seller to arrange delivery. "
               "Once you receive the item, confirm receipt to release the funds from escrow.")
               .arg(QUuid::createUuid().toString().mid(1, 8))
        );
    }
}

void MarketPage::onRefreshListingsClicked()
{
    refreshListings();
}

void MarketPage::onListingSelected(QListWidgetItem *item)
{
    int index = listingsWidget->row(item);
    if (index < 0 || index >= mockListings.size()) return;

    selectedListing = &mockListings[index];

    itemTitleLabel->setText(selectedListing->title);
    itemPriceLabel->setText(QString("%1 CRL").arg(selectedListing->price, 0, 'f', 8));
    itemDescriptionLabel->setText(selectedListing->description);
    itemSellerLabel->setText(tr("Seller: %1").arg(selectedListing->sellerAddress));
    buyButton->setEnabled(true);
}

void MarketPage::onCategoryChanged(int index)
{
    refreshListings();
}

void MarketPage::onSearchClicked()
{
    refreshListings();
}

void MarketPage::refreshListings()
{
    listingsWidget->clear();
    QString filterCategory = categoryFilter->currentData().toString();
    QString searchText = searchEdit->text().toLower();

    for (const MarketListing &listing : mockListings) {
        if (!listing.isActive) continue;

        // Category filter
        if (filterCategory != "all" && listing.category != filterCategory) continue;

        // Search filter
        if (!searchText.isEmpty()) {
            if (!listing.title.toLower().contains(searchText) &&
                !listing.description.toLower().contains(searchText)) {
                continue;
            }
        }

        QListWidgetItem *item = new QListWidgetItem();
        item->setText(QString("%1\n%2 CRL").arg(listing.title).arg(listing.price, 0, 'f', 8));
        item->setData(Qt::UserRole, listing.id);
        listingsWidget->addItem(item);
    }

    // Reset selection
    selectedListing = nullptr;
    itemTitleLabel->setText(tr("Select an item to view details"));
    itemPriceLabel->clear();
    itemDescriptionLabel->clear();
    itemSellerLabel->clear();
    buyButton->setEnabled(false);
}
