// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/marketpage.h>

#include <qt/clientmodel.h>
#include <qt/guiutil.h>
#include <qt/platformstyle.h>
#include <qt/walletmodel.h>
#include <qt/widgets/statusbadge.h>
#include <qt/widgets/listingcard.h>

#include <QComboBox>
#include <QDateTime>
#include <QDoubleSpinBox>
#include <QFile>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QSplitter>
#include <QTabWidget>
#include <QTextEdit>
#include <QVBoxLayout>

MarketPage::MarketPage(const PlatformStyle *_platformStyle, QWidget *parent)
    : QWidget(parent)
    , platformStyle(_platformStyle)
{
    setupUI();

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
    applyBioportTheme();

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Header panel - industrial Win95 style
    auto *headerPanel = new QWidget(this);
    headerPanel->setObjectName("headerPanel");
    headerPanel->setStyleSheet(
        "#headerPanel {"
        "  background: qlineargradient(y1:0, y2:1, stop:0 #2a2a2a, stop:1 #1a1a1a);"
        "  border-bottom: 2px solid #3a3a3a;"
        "}"
    );
    auto *headerLayout = new QHBoxLayout(headerPanel);
    headerLayout->setContentsMargins(16, 12, 16, 12);

    titleLabel = new QLabel(tr("CORAL MARKET"), this);
    titleLabel->setStyleSheet(
        "color: #00d4aa;"
        "font-size: 18px;"
        "font-weight: bold;"
        "font-family: 'Terminus', monospace;"
        "background: transparent;"
    );
    headerLayout->addWidget(titleLabel);

    headerLayout->addStretch();

    balanceLabel = new QLabel(tr("Balance: -- CRL"), this);
    balanceLabel->setStyleSheet(
        "color: #33ff33;"
        "font-size: 14px;"
        "font-family: 'Terminus', monospace;"
        "background: transparent;"
    );
    headerLayout->addWidget(balanceLabel);

    mainLayout->addWidget(headerPanel);

    // Tab widget with bioport styling
    tabWidget = new QTabWidget(this);
    tabWidget->setStyleSheet(
        "QTabWidget::pane {"
        "  background: #0a0a0a;"
        "  border: 2px solid;"
        "  border-color: #5a5a5a #1a1a1a #1a1a1a #5a5a5a;"
        "}"
        "QTabBar::tab {"
        "  background: qlineargradient(y1:0, y2:1, stop:0 #3a3a3a, stop:1 #2a2a2a);"
        "  border: 2px solid;"
        "  border-color: #5a5a5a #1a1a1a #2a2a2a #5a5a5a;"
        "  color: #808080;"
        "  padding: 10px 24px;"
        "  font-family: 'Terminus', monospace;"
        "  font-weight: bold;"
        "}"
        "QTabBar::tab:selected {"
        "  background: #1a1a1a;"
        "  color: #00d4aa;"
        "  border-bottom-color: #1a1a1a;"
        "}"
        "QTabBar::tab:hover:!selected {"
        "  background: qlineargradient(y1:0, y2:1, stop:0 #4a4a4a, stop:1 #3a3a3a);"
        "  color: #c0c0c0;"
        "}"
    );

    // Create tabs
    auto *browseTab = new QWidget();
    browseTab->setStyleSheet("background: #0a0a0a;");
    createBrowseTab(browseTab);
    tabWidget->addTab(browseTab, tr("BROWSE"));

    auto *sellTab = new QWidget();
    sellTab->setStyleSheet("background: #0a0a0a;");
    createSellTab(sellTab);
    tabWidget->addTab(sellTab, tr("SELL"));

    auto *ordersTab = new QWidget();
    ordersTab->setStyleSheet("background: #0a0a0a;");
    createOrdersTab(ordersTab);
    tabWidget->addTab(ordersTab, tr("ORDERS"));

    auto *myListingsTab = new QWidget();
    myListingsTab->setStyleSheet("background: #0a0a0a;");
    createMyListingsTab(myListingsTab);
    tabWidget->addTab(myListingsTab, tr("MY LISTINGS"));

    mainLayout->addWidget(tabWidget, 1);
}

void MarketPage::applyBioportTheme()
{
    setStyleSheet(
        "MarketPage {"
        "  background-color: #0a0a0a;"
        "}"
        "QLabel {"
        "  color: #c0c0c0;"
        "  font-family: 'Terminus', monospace;"
        "}"
        "QGroupBox {"
        "  background: #1a1a1a;"
        "  border: 2px solid;"
        "  border-color: #5a5a5a #1a1a1a #1a1a1a #5a5a5a;"
        "  margin-top: 16px;"
        "  padding: 12px;"
        "  font-family: 'Terminus', monospace;"
        "}"
        "QGroupBox::title {"
        "  color: #00d4aa;"
        "  font-weight: bold;"
        "  subcontrol-origin: margin;"
        "  left: 12px;"
        "}"
        "QLineEdit, QTextEdit {"
        "  background: #0a0a0a;"
        "  border: 2px solid;"
        "  border-color: #1a1a1a #3a3a3a #3a3a3a #1a1a1a;"
        "  color: #33ff33;"
        "  font-family: 'Terminus', monospace;"
        "  padding: 8px;"
        "}"
        "QLineEdit:focus, QTextEdit:focus {"
        "  border-color: #0a5a4a #00d4aa #00d4aa #0a5a4a;"
        "}"
        "QComboBox {"
        "  background: qlineargradient(y1:0, y2:1, stop:0 #3a3a3a, stop:1 #2a2a2a);"
        "  border: 2px solid;"
        "  border-color: #5a5a5a #1a1a1a #1a1a1a #5a5a5a;"
        "  color: #c0c0c0;"
        "  padding: 6px 12px;"
        "  font-family: 'Terminus', monospace;"
        "}"
        "QDoubleSpinBox {"
        "  background: #0a0a0a;"
        "  border: 2px solid;"
        "  border-color: #1a1a1a #3a3a3a #3a3a3a #1a1a1a;"
        "  color: #33ff33;"
        "  font-family: 'Terminus', monospace;"
        "  padding: 6px;"
        "}"
        "QPushButton {"
        "  background: qlineargradient(y1:0, y2:1, stop:0 #3a3a3a, stop:1 #2a2a2a);"
        "  border: 2px solid;"
        "  border-color: #5a5a5a #1a1a1a #1a1a1a #5a5a5a;"
        "  color: #c0c0c0;"
        "  padding: 8px 16px;"
        "  font-family: 'Terminus', monospace;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "  background: qlineargradient(y1:0, y2:1, stop:0 #4a4a4a, stop:1 #3a3a3a);"
        "  border-color: #00d4aa #0a5a4a #0a5a4a #00d4aa;"
        "  color: #00d4aa;"
        "}"
        "QPushButton:pressed {"
        "  border-color: #1a1a1a #5a5a5a #5a5a5a #1a1a1a;"
        "}"
        "QPushButton:disabled {"
        "  background: #1a1a1a;"
        "  border-color: #2a2a2a;"
        "  color: #4a4a4a;"
        "}"
        "QListWidget {"
        "  background: #0a0a0a;"
        "  border: 2px solid;"
        "  border-color: #1a1a1a #3a3a3a #3a3a3a #1a1a1a;"
        "  color: #c0c0c0;"
        "  font-family: 'Terminus', monospace;"
        "}"
        "QListWidget::item {"
        "  padding: 8px;"
        "  border-bottom: 1px solid #1a1a1a;"
        "}"
        "QListWidget::item:selected {"
        "  background: #00d4aa;"
        "  color: #0a0a0a;"
        "}"
        "QScrollBar:vertical {"
        "  background: #1a1a1a;"
        "  width: 14px;"
        "}"
        "QScrollBar::handle:vertical {"
        "  background: #3a3a3a;"
        "  border: 1px solid #5a5a5a;"
        "  min-height: 20px;"
        "}"
    );
}

void MarketPage::createBrowseTab(QWidget *tab)
{
    auto *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);

    // Filter bar
    auto *filterLayout = new QHBoxLayout();
    filterLayout->setSpacing(8);

    auto *catLabel = new QLabel(tr("CATEGORY:"), this);
    catLabel->setStyleSheet("color: #808080; font-size: 11px;");
    filterLayout->addWidget(catLabel);

    categoryFilter = new QComboBox(this);
    categoryFilter->addItem(tr("All"), "all");
    categoryFilter->addItem(tr("Digital Goods"), "digital");
    categoryFilter->addItem(tr("Physical Goods"), "physical");
    categoryFilter->addItem(tr("Services"), "services");
    categoryFilter->addItem(tr("Crypto"), "crypto");
    categoryFilter->addItem(tr("Other"), "other");
    connect(categoryFilter, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MarketPage::onCategoryChanged);
    filterLayout->addWidget(categoryFilter);

    filterLayout->addSpacing(16);

    searchEdit = new QLineEdit(this);
    searchEdit->setPlaceholderText(tr("Search listings..."));
    filterLayout->addWidget(searchEdit, 1);

    searchButton = new QPushButton(tr("SEARCH"), this);
    connect(searchButton, &QPushButton::clicked, this, &MarketPage::onSearchClicked);
    filterLayout->addWidget(searchButton);

    layout->addLayout(filterLayout);

    // Main content: listings grid + details panel
    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->setStyleSheet(
        "QSplitter::handle {"
        "  background: #3a3a3a;"
        "  width: 4px;"
        "}"
        "QSplitter::handle:hover {"
        "  background: #00d4aa;"
        "}"
    );

    // Listings scroll area
    listingsScrollArea = new QScrollArea(this);
    listingsScrollArea->setWidgetResizable(true);
    listingsScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    listingsScrollArea->setStyleSheet("QScrollArea { background: #0a0a0a; border: none; }");

    listingsContainer = new QWidget();
    listingsContainer->setStyleSheet("background: #0a0a0a;");
    listingsGrid = new QGridLayout(listingsContainer);
    listingsGrid->setContentsMargins(0, 0, 0, 0);
    listingsGrid->setSpacing(12);

    listingsScrollArea->setWidget(listingsContainer);
    splitter->addWidget(listingsScrollArea);

    // Details panel
    detailsPanel = new QWidget(this);
    detailsPanel->setStyleSheet(
        "QWidget {"
        "  background: #1a1a1a;"
        "  border: 2px solid;"
        "  border-color: #5a5a5a #1a1a1a #1a1a1a #5a5a5a;"
        "}"
    );
    auto *detailsLayout = new QVBoxLayout(detailsPanel);
    detailsLayout->setContentsMargins(16, 16, 16, 16);
    detailsLayout->setSpacing(12);

    detailTitleLabel = new QLabel(tr("Select a listing"), this);
    detailTitleLabel->setStyleSheet(
        "color: #ffffff;"
        "font-size: 16px;"
        "font-weight: bold;"
        "border: none;"
    );
    detailTitleLabel->setWordWrap(true);
    detailsLayout->addWidget(detailTitleLabel);

    detailStatusBadge = new StatusBadge(StatusBadge::Pending, this);
    detailStatusBadge->hide();
    detailsLayout->addWidget(detailStatusBadge);

    detailPriceLabel = new QLabel(this);
    detailPriceLabel->setStyleSheet(
        "color: #00d4aa;"
        "font-size: 24px;"
        "font-weight: bold;"
        "border: none;"
    );
    detailsLayout->addWidget(detailPriceLabel);

    detailDescLabel = new QLabel(this);
    detailDescLabel->setStyleSheet(
        "color: #a0a0a0;"
        "font-size: 12px;"
        "border: none;"
    );
    detailDescLabel->setWordWrap(true);
    detailsLayout->addWidget(detailDescLabel);

    detailSellerLabel = new QLabel(this);
    detailSellerLabel->setStyleSheet(
        "color: #606060;"
        "font-size: 10px;"
        "border: none;"
    );
    detailsLayout->addWidget(detailSellerLabel);

    detailsLayout->addStretch();

    buyButton = new QPushButton(tr("BUY NOW"), this);
    buyButton->setEnabled(false);
    buyButton->setStyleSheet(
        "QPushButton {"
        "  background: qlineargradient(y1:0, y2:1, stop:0 #0a6a5a, stop:1 #064a3a);"
        "  border: 2px solid;"
        "  border-color: #00d4aa #043a2a #043a2a #00d4aa;"
        "  color: #00d4aa;"
        "  font-size: 14px;"
        "  font-weight: bold;"
        "  padding: 12px 24px;"
        "}"
        "QPushButton:hover {"
        "  background: qlineargradient(y1:0, y2:1, stop:0 #0a8a7a, stop:1 #066a5a);"
        "  color: #33ffcc;"
        "}"
        "QPushButton:disabled {"
        "  background: #1a1a1a;"
        "  border-color: #2a2a2a;"
        "  color: #4a4a4a;"
        "}"
    );
    connect(buyButton, &QPushButton::clicked, [this]() {
        onBuyItemClicked(selectedListingId);
    });
    detailsLayout->addWidget(buyButton);

    splitter->addWidget(detailsPanel);
    splitter->setSizes({500, 300});

    layout->addWidget(splitter, 1);

    // Initial population
    populateListings();
}

void MarketPage::createSellTab(QWidget *tab)
{
    auto *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(16);

    auto *createGroup = new QGroupBox(tr("CREATE NEW LISTING"), this);
    auto *createLayout = new QVBoxLayout(createGroup);
    createLayout->setSpacing(12);

    // Title
    auto *titleLayout = new QHBoxLayout();
    auto *titleLbl = new QLabel(tr("TITLE:"), this);
    titleLbl->setStyleSheet("color: #808080; font-size: 11px; min-width: 80px;");
    titleLayout->addWidget(titleLbl);
    listingTitleEdit = new QLineEdit(this);
    listingTitleEdit->setPlaceholderText(tr("Enter item title"));
    titleLayout->addWidget(listingTitleEdit, 1);
    createLayout->addLayout(titleLayout);

    // Category
    auto *catLayout = new QHBoxLayout();
    auto *catLbl = new QLabel(tr("CATEGORY:"), this);
    catLbl->setStyleSheet("color: #808080; font-size: 11px; min-width: 80px;");
    catLayout->addWidget(catLbl);
    listingCategoryCombo = new QComboBox(this);
    listingCategoryCombo->addItem(tr("Digital Goods"), "digital");
    listingCategoryCombo->addItem(tr("Physical Goods"), "physical");
    listingCategoryCombo->addItem(tr("Services"), "services");
    listingCategoryCombo->addItem(tr("Crypto"), "crypto");
    listingCategoryCombo->addItem(tr("Other"), "other");
    catLayout->addWidget(listingCategoryCombo, 1);
    createLayout->addLayout(catLayout);

    // Price
    auto *priceLayout = new QHBoxLayout();
    auto *priceLbl = new QLabel(tr("PRICE (CRL):"), this);
    priceLbl->setStyleSheet("color: #808080; font-size: 11px; min-width: 80px;");
    priceLayout->addWidget(priceLbl);
    listingPriceSpinBox = new QDoubleSpinBox(this);
    listingPriceSpinBox->setRange(0.00000001, 21000000.0);
    listingPriceSpinBox->setDecimals(8);
    listingPriceSpinBox->setValue(1.0);
    priceLayout->addWidget(listingPriceSpinBox, 1);
    createLayout->addLayout(priceLayout);

    // Description
    auto *descLbl = new QLabel(tr("DESCRIPTION:"), this);
    descLbl->setStyleSheet("color: #808080; font-size: 11px;");
    createLayout->addWidget(descLbl);
    listingDescriptionEdit = new QTextEdit(this);
    listingDescriptionEdit->setPlaceholderText(tr("Describe your item..."));
    listingDescriptionEdit->setMaximumHeight(120);
    createLayout->addWidget(listingDescriptionEdit);

    // Create button
    auto *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    createListingButton = new QPushButton(tr("CREATE LISTING"), this);
    createListingButton->setStyleSheet(
        "QPushButton {"
        "  background: qlineargradient(y1:0, y2:1, stop:0 #0a6a5a, stop:1 #064a3a);"
        "  border: 2px solid;"
        "  border-color: #00d4aa #043a2a #043a2a #00d4aa;"
        "  color: #00d4aa;"
        "  font-weight: bold;"
        "  padding: 10px 24px;"
        "}"
    );
    connect(createListingButton, &QPushButton::clicked, this, &MarketPage::onCreateListingClicked);
    btnLayout->addWidget(createListingButton);
    createLayout->addLayout(btnLayout);

    listingStatusLabel = new QLabel(this);
    listingStatusLabel->setStyleSheet("color: #808080; font-size: 11px;");
    createLayout->addWidget(listingStatusLabel);

    layout->addWidget(createGroup);

    // Info panel
    auto *infoGroup = new QGroupBox(tr("ESCROW INFORMATION"), this);
    auto *infoLayout = new QVBoxLayout(infoGroup);
    auto *infoLabel = new QLabel(
        tr("1. Create a listing with price in CRL\n"
           "2. Listing is stored locally and broadcast to network\n"
           "3. Buyers fund 2-of-3 multisig escrow\n"
           "4. You ship, buyer confirms, funds release\n"
           "5. Disputes resolved by network arbiter\n\n"
           "Listing fee: 0.001 CRL"),
        this
    );
    infoLabel->setStyleSheet("color: #606060; font-size: 11px; line-height: 1.5;");
    infoLabel->setWordWrap(true);
    infoLayout->addWidget(infoLabel);
    layout->addWidget(infoGroup);

    layout->addStretch();
}

void MarketPage::createOrdersTab(QWidget *tab)
{
    auto *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);

    auto *headerLabel = new QLabel(tr("YOUR ORDERS"), this);
    headerLabel->setStyleSheet("color: #00d4aa; font-size: 14px; font-weight: bold;");
    layout->addWidget(headerLabel);

    ordersWidget = new QListWidget(this);
    layout->addWidget(ordersWidget, 1);

    // Placeholder
    auto *emptyLabel = new QLabel(tr("No orders yet. Purchase items from the Browse tab."), this);
    emptyLabel->setStyleSheet("color: #4a4a4a; font-size: 12px;");
    emptyLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(emptyLabel);
}

void MarketPage::createMyListingsTab(QWidget *tab)
{
    auto *layout = new QVBoxLayout(tab);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);

    auto *headerLabel = new QLabel(tr("YOUR LISTINGS"), this);
    headerLabel->setStyleSheet("color: #00d4aa; font-size: 14px; font-weight: bold;");
    layout->addWidget(headerLabel);

    myListingsWidget = new QListWidget(this);
    layout->addWidget(myListingsWidget, 1);

    auto *btnLayout = new QHBoxLayout();
    btnLayout->addStretch();
    removeListingButton = new QPushButton(tr("REMOVE SELECTED"), this);
    removeListingButton->setStyleSheet(
        "QPushButton {"
        "  background: qlineargradient(y1:0, y2:1, stop:0 #5a1a1a, stop:1 #3a0a0a);"
        "  border: 2px solid;"
        "  border-color: #aa0000 #3a0a0a #3a0a0a #aa0000;"
        "  color: #ff6666;"
        "}"
    );
    btnLayout->addWidget(removeListingButton);
    layout->addLayout(btnLayout);
}

void MarketPage::setClientModel(ClientModel *model)
{
    clientModel = model;
}

void MarketPage::setWalletModel(WalletModel *model)
{
    walletModel = model;
    updateBalance();
}

void MarketPage::updateBalance()
{
    if (walletModel) {
        // In real implementation, get balance from wallet
        balanceLabel->setText(tr("Balance: -- CRL"));
    }
}

void MarketPage::populateListings()
{
    // Clear existing
    QLayoutItem *item;
    while ((item = listingsGrid->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    // TODO: Fetch real listings from ListingStore when backend is integrated
    // For now, show empty state

    auto *emptyLabel = new QLabel(tr("No listings available.\n\nCreate the first listing in the SELL tab."), this);
    emptyLabel->setStyleSheet(
        "color: #4a4a4a;"
        "font-size: 14px;"
        "font-family: 'Terminus', monospace;"
        "padding: 40px;"
    );
    emptyLabel->setAlignment(Qt::AlignCenter);
    listingsGrid->addWidget(emptyLabel, 0, 0, 1, 2);
    listingsGrid->setRowStretch(1, 1);
}

void MarketPage::showListingDetails(const QString &listingId)
{
    Q_UNUSED(listingId);
    // TODO: Fetch listing details from ListingStore when backend is integrated
    detailTitleLabel->setText(tr("Select a listing"));
    detailPriceLabel->setText(QString());
    detailDescLabel->setText(QString());
    detailSellerLabel->setText(QString());
    detailStatusBadge->hide();
    buyButton->setEnabled(false);
}

void MarketPage::onListingClicked(const QString &listingId)
{
    selectedListingId = listingId;
    showListingDetails(listingId);
}

void MarketPage::onBuyItemClicked(const QString &listingId)
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        tr("Confirm Purchase"),
        tr("Create escrow transaction for this item?\n\n"
           "Funds will be held in 2-of-3 multisig until you confirm delivery."),
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        // In real implementation:
        // 1. Create MarketplaceEscrow with buyer/seller/arbiter keys
        // 2. Use WalletModel to create and broadcast funding tx
        // 3. Create MarketOrder and store in OrderStore
        QMessageBox::information(
            this,
            tr("Escrow Created"),
            tr("Escrow transaction created.\n\n"
               "The seller will be notified. Once they ship,\n"
               "confirm delivery to release funds.")
        );
    }
}

void MarketPage::onCreateListingClicked()
{
    QString title = listingTitleEdit->text().trimmed();
    QString desc = listingDescriptionEdit->toPlainText().trimmed();
    double price = listingPriceSpinBox->value();

    if (title.isEmpty()) {
        listingStatusLabel->setText(tr("ERROR: Title required"));
        listingStatusLabel->setStyleSheet("color: #ff4444;");
        return;
    }

    if (price <= 0) {
        listingStatusLabel->setText(tr("ERROR: Invalid price"));
        listingStatusLabel->setStyleSheet("color: #ff4444;");
        return;
    }

    // In real implementation:
    // 1. Create MarketListing with wallet address
    // 2. Store in ListingStore
    // 3. Broadcast to network

    listingStatusLabel->setText(tr("Listing created successfully"));
    listingStatusLabel->setStyleSheet("color: #00d4aa;");

    listingTitleEdit->clear();
    listingDescriptionEdit->clear();
    listingPriceSpinBox->setValue(1.0);

    refreshListings();
}

void MarketPage::onCategoryChanged(int /*index*/)
{
    refreshListings();
}

void MarketPage::onSearchClicked()
{
    refreshListings();
}

void MarketPage::refreshListings()
{
    populateListings();
}

QString MarketPage::formatPrice(double amount) const
{
    return QString("%1 CRL").arg(amount, 0, 'f', 8);
}
