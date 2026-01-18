// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/networkpage.h>

#include <qt/clientmodel.h>
#include <qt/guiutil.h>
#include <qt/peertablemodel.h>
#include <qt/peertablesortproxy.h>
#include <qt/platformstyle.h>

#include <QDateTime>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QTableView>
#include <QVBoxLayout>

NetworkPage::NetworkPage(const PlatformStyle *_platformStyle, QWidget *parent)
    : QWidget(parent)
    , platformStyle(_platformStyle)
{
    setupUI();

    // Refresh stats every 2 seconds
    refreshTimer = new QTimer(this);
    connect(refreshTimer, &QTimer::timeout, this, &NetworkPage::updateNetworkStats);
    connect(refreshTimer, &QTimer::timeout, this, &NetworkPage::updateMempoolStats);
    refreshTimer->start(2000);
}

NetworkPage::~NetworkPage()
{
    if (refreshTimer) {
        refreshTimer->stop();
    }
}

void NetworkPage::setupUI()
{
    // Dark theme for the entire page
    setStyleSheet(
        "NetworkPage { background-color: #000000; }"
        "QLabel { color: #ffffff; }"
        "QGroupBox { color: #ffffff; background-color: #111111; }"
    );

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(20);

    // Title
    QLabel *titleLabel = new QLabel(tr("Network Dashboard"));
    titleLabel->setStyleSheet("font-size: 24px; font-weight: bold; color: #ffffff;");
    mainLayout->addWidget(titleLabel);

    // Create horizontal layout for top sections
    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->setSpacing(20);

    // Setup the three main sections
    setupNetworkStatsSection();
    setupMempoolSection();

    topLayout->addWidget(networkGroupBox);
    topLayout->addWidget(mempoolGroupBox);

    mainLayout->addLayout(topLayout);

    // Peer section takes the rest
    setupPeerSection();
    mainLayout->addWidget(peerGroupBox, 1);

    setLayout(mainLayout);
}

void NetworkPage::setupNetworkStatsSection()
{
    networkGroupBox = new QGroupBox(tr("Network Status"));
    networkGroupBox->setStyleSheet(
        "QGroupBox { font-weight: bold; border: 1px solid #333333; border-radius: 8px; margin-top: 10px; padding-top: 10px; background-color: #111111; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; color: #888888; }"
    );

    QVBoxLayout *layout = new QVBoxLayout(networkGroupBox);
    layout->setSpacing(10);

    networkActiveLabel = new QLabel(tr("Status: Connecting..."));
    networkActiveLabel->setStyleSheet("font-size: 14px;");

    blockHeightLabel = new QLabel(tr("Block Height: 0"));
    blockHeightLabel->setStyleSheet("font-size: 14px;");

    lastBlockTimeLabel = new QLabel(tr("Last Block: --"));
    lastBlockTimeLabel->setStyleSheet("font-size: 14px;");

    bytesInLabel = new QLabel(tr("Received: 0 bytes"));
    bytesInLabel->setStyleSheet("font-size: 12px; color: #28a745;");

    bytesOutLabel = new QLabel(tr("Sent: 0 bytes"));
    bytesOutLabel->setStyleSheet("font-size: 12px; color: #dc3545;");

    layout->addWidget(networkActiveLabel);
    layout->addWidget(blockHeightLabel);
    layout->addWidget(lastBlockTimeLabel);
    layout->addWidget(bytesInLabel);
    layout->addWidget(bytesOutLabel);
    layout->addStretch();
}

void NetworkPage::setupMempoolSection()
{
    mempoolGroupBox = new QGroupBox(tr("Mempool"));
    mempoolGroupBox->setStyleSheet(
        "QGroupBox { font-weight: bold; border: 1px solid #333333; border-radius: 8px; margin-top: 10px; padding-top: 10px; background-color: #111111; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; color: #888888; }"
    );

    QVBoxLayout *layout = new QVBoxLayout(mempoolGroupBox);
    layout->setSpacing(10);

    mempoolSizeLabel = new QLabel(tr("Transactions: 0"));
    mempoolSizeLabel->setStyleSheet("font-size: 14px;");

    mempoolBytesLabel = new QLabel(tr("Size: 0 bytes"));
    mempoolBytesLabel->setStyleSheet("font-size: 14px;");

    mempoolMinFeeLabel = new QLabel(tr("Min Fee: 0 sat/vB"));
    mempoolMinFeeLabel->setStyleSheet("font-size: 12px;");

    mempoolUsageBar = new QProgressBar();
    mempoolUsageBar->setRange(0, 100);
    mempoolUsageBar->setValue(0);
    mempoolUsageBar->setFormat(tr("Memory Usage: %p%"));
    mempoolUsageBar->setStyleSheet(
        "QProgressBar { border: 1px solid #333333; border-radius: 4px; text-align: center; background-color: #111111; color: #ffffff; }"
        "QProgressBar::chunk { background-color: #ffffff; border-radius: 3px; }"
    );

    layout->addWidget(mempoolSizeLabel);
    layout->addWidget(mempoolBytesLabel);
    layout->addWidget(mempoolMinFeeLabel);
    layout->addWidget(mempoolUsageBar);
    layout->addStretch();
}

void NetworkPage::setupPeerSection()
{
    peerGroupBox = new QGroupBox(tr("Connected Peers"));
    peerGroupBox->setStyleSheet(
        "QGroupBox { font-weight: bold; border: 1px solid #333333; border-radius: 8px; margin-top: 10px; padding-top: 10px; background-color: #111111; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; color: #888888; }"
    );

    QVBoxLayout *layout = new QVBoxLayout(peerGroupBox);

    // Peer count summary
    QHBoxLayout *summaryLayout = new QHBoxLayout();

    peerCountLabel = new QLabel(tr("Total Peers: 0"));
    peerCountLabel->setStyleSheet("font-size: 16px; font-weight: bold;");

    inboundLabel = new QLabel(tr("Inbound: 0"));
    inboundLabel->setStyleSheet("font-size: 12px; color: #28a745;");

    outboundLabel = new QLabel(tr("Outbound: 0"));
    outboundLabel->setStyleSheet("font-size: 12px; color: #007bff;");

    summaryLayout->addWidget(peerCountLabel);
    summaryLayout->addStretch();
    summaryLayout->addWidget(inboundLabel);
    summaryLayout->addWidget(outboundLabel);

    layout->addLayout(summaryLayout);

    // Add peer manually
    QHBoxLayout *addPeerLayout = new QHBoxLayout();

    addPeerEdit = new QLineEdit();
    addPeerEdit->setPlaceholderText(tr("Enter peer address (e.g., 192.168.1.1:8334)"));
    addPeerEdit->setStyleSheet("padding: 8px; border: 1px solid #333333; border-radius: 4px; background-color: #111111; color: #ffffff;");

    addPeerButton = new QPushButton(tr("Add Peer"));
    addPeerButton->setStyleSheet(
        "QPushButton { background-color: #222222; color: #ffffff; padding: 8px 16px; border: 1px solid #444444; border-radius: 4px; }"
        "QPushButton:hover { background-color: #333333; border-color: #ffffff; }"
        "QPushButton:pressed { background-color: #444444; }"
    );
    connect(addPeerButton, &QPushButton::clicked, this, &NetworkPage::onAddPeerClicked);

    addPeerLayout->addWidget(addPeerEdit, 1);
    addPeerLayout->addWidget(addPeerButton);

    layout->addLayout(addPeerLayout);

    // Peer table with dark theme
    peerTableView = new QTableView();
    peerTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    peerTableView->setSelectionMode(QAbstractItemView::SingleSelection);
    peerTableView->setAlternatingRowColors(true);
    peerTableView->setSortingEnabled(true);
    peerTableView->verticalHeader()->hide();
    peerTableView->setShowGrid(false);
    peerTableView->setStyleSheet(
        "QTableView { border: 1px solid #333333; border-radius: 4px; background-color: #111111; color: #ffffff; }"
        "QTableView::item { padding: 8px; }"
        "QTableView::item:selected { background-color: #333333; color: #ffffff; }"
        "QTableView::item:alternate { background-color: #0a0a0a; }"
        "QHeaderView::section { background-color: #000000; padding: 8px; border: none; border-bottom: 1px solid #333333; font-weight: bold; color: #888888; }"
    );

    layout->addWidget(peerTableView, 1);
}

void NetworkPage::setClientModel(ClientModel *model)
{
    clientModel = model;

    if (clientModel) {
        // Connect to peer table model
        peerTableView->setModel(clientModel->peerTableSortProxy());
        peerTableView->horizontalHeader()->setStretchLastSection(true);

        // Hide some columns for cleaner view
        peerTableView->hideColumn(PeerTableModel::NetNodeId);

        // Connect signals
        connect(clientModel, &ClientModel::numConnectionsChanged, this, &NetworkPage::updatePeerCount);
        connect(clientModel, &ClientModel::mempoolSizeChanged, this, [this](long count, size_t bytes) {
            mempoolSizeLabel->setText(tr("Transactions: %1").arg(count));
            mempoolBytesLabel->setText(tr("Size: %1").arg(GUIUtil::formatBytes(bytes)));

            // Calculate usage percentage (assuming 300MB max mempool)
            const size_t maxMempool = 300 * 1024 * 1024;
            int percentage = static_cast<int>((bytes * 100) / maxMempool);
            mempoolUsageBar->setValue(std::min(percentage, 100));
        });

        connect(clientModel, &ClientModel::bytesChanged, this, [this](quint64 bytesIn, quint64 bytesOut) {
            bytesInLabel->setText(tr("Received: %1").arg(GUIUtil::formatBytes(bytesIn)));
            bytesOutLabel->setText(tr("Sent: %1").arg(GUIUtil::formatBytes(bytesOut)));
        });

        connect(clientModel, &ClientModel::networkActiveChanged, this, [this](bool active) {
            if (active) {
                networkActiveLabel->setText(tr("Status: Online"));
                networkActiveLabel->setStyleSheet("font-size: 14px; color: #ffffff;");
            } else {
                networkActiveLabel->setText(tr("Status: Offline"));
                networkActiveLabel->setStyleSheet("font-size: 14px; color: #ff4444;");
            }
        });

        // Initial update
        updatePeerCount(clientModel->getNumConnections());
        updateNetworkStats();
    }
}

void NetworkPage::updateNetworkStats()
{
    if (!clientModel) return;

    int numBlocks = clientModel->getNumBlocks();
    blockHeightLabel->setText(tr("Block Height: %1").arg(numBlocks));

    int64_t headerTime = clientModel->getHeaderTipTime();
    if (headerTime > 0) {
        QDateTime blockTime = QDateTime::fromSecsSinceEpoch(headerTime);
        lastBlockTimeLabel->setText(tr("Last Block: %1").arg(blockTime.toString("yyyy-MM-dd hh:mm:ss")));
    }
}

void NetworkPage::updateMempoolStats()
{
    // Mempool stats are updated via signal from clientModel
}

void NetworkPage::updatePeerCount(int count)
{
    peerCountLabel->setText(tr("Total Peers: %1").arg(count));

    if (clientModel) {
        int inbound = clientModel->getNumConnections(CONNECTIONS_IN);
        int outbound = clientModel->getNumConnections(CONNECTIONS_OUT);
        inboundLabel->setText(tr("Inbound: %1").arg(inbound));
        outboundLabel->setText(tr("Outbound: %1").arg(outbound));
    }
}

void NetworkPage::onAddPeerClicked()
{
    QString peerAddress = addPeerEdit->text().trimmed();
    if (peerAddress.isEmpty()) return;

    if (clientModel) {
        // Use RPC to add peer: addnode "address" "add"
        // For now, just clear the field as a placeholder
        // The actual implementation would use clientModel->node().addNode()
        addPeerEdit->clear();
    }
}
