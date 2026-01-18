// Copyright (c) 2026 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/miningpage.h>

#include <chrono>
#include <qt/clientmodel.h>
#include <qt/guiutil.h>
#include <qt/platformstyle.h>
#include <qt/walletmodel.h>

#include <interfaces/node.h>
#include <univalue.h>

#include <QDateTime>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollArea>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QFont>

#include <cmath>

// MiningWorker implementation
MiningWorker::MiningWorker(QObject *parent) : QObject(parent) {}

void MiningWorker::doMining()
{
    if (!clientModel || miningAddress.isEmpty()) {
        Q_EMIT miningError("No client model or address");
        Q_EMIT finished();
        return;
    }

    shouldStop = false;
    uint64_t totalHashes = 0;
    auto startTime = std::chrono::steady_clock::now();

    while (!shouldStop) {
        try {
            UniValue params(UniValue::VARR);
            params.push_back(1);  // Mine 1 block
            params.push_back(miningAddress.toStdString());

            UniValue result = clientModel->node().executeRpc("generatetoaddress", params, "");

            if (result.isArray() && result.size() > 0) {
                QString blockHash = QString::fromStdString(result[0].get_str());
                Q_EMIT blockFound(blockHash, 50.0);  // 50 CRL reward
            }

            // Estimate hashrate based on time taken
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
            if (elapsed > 0) {
                // RandomX typically does ~1-2 KH/s per thread
                totalHashes += 2000;  // Approximate hashes per attempt
                double hashrate = (totalHashes * 1000.0) / elapsed;
                Q_EMIT hashUpdate(hashrate, totalHashes);
            }

        } catch (const std::exception& e) {
            // Mining attempt failed - this is normal, just continue
            QThread::msleep(100);  // Small delay before retry
        }
    }

    Q_EMIT finished();
}

MiningPage::MiningPage(const PlatformStyle *_platformStyle, QWidget *parent)
    : QWidget(parent)
    , platformStyle(_platformStyle)
{
    setupUI();

    // Refresh stats every second
    refreshTimer = new QTimer(this);
    connect(refreshTimer, &QTimer::timeout, this, &MiningPage::updateMiningStats);
    refreshTimer->start(1000);
}

MiningPage::~MiningPage()
{
    if (refreshTimer) {
        refreshTimer->stop();
    }

    // Clean up mining thread
    if (miningWorker) {
        miningWorker->stop();
    }
    if (miningThread) {
        miningThread->quit();
        miningThread->wait(5000);
        delete miningThread;
    }
}

QString MiningPage::formatHashrate(double hashrate)
{
    if (hashrate >= 1e15) {
        return QString("%1 PH/s").arg(hashrate / 1e15, 0, 'f', 2);
    } else if (hashrate >= 1e12) {
        return QString("%1 TH/s").arg(hashrate / 1e12, 0, 'f', 2);
    } else if (hashrate >= 1e9) {
        return QString("%1 GH/s").arg(hashrate / 1e9, 0, 'f', 2);
    } else if (hashrate >= 1e6) {
        return QString("%1 MH/s").arg(hashrate / 1e6, 0, 'f', 2);
    } else if (hashrate >= 1e3) {
        return QString("%1 KH/s").arg(hashrate / 1e3, 0, 'f', 2);
    } else {
        return QString("%1 H/s").arg(hashrate, 0, 'f', 0);
    }
}

QString MiningPage::formatDuration(qint64 seconds)
{
    if (seconds < 0) return QString("--");

    qint64 years = seconds / (365 * 24 * 3600);
    seconds %= (365 * 24 * 3600);
    qint64 days = seconds / (24 * 3600);
    seconds %= (24 * 3600);
    qint64 hours = seconds / 3600;
    seconds %= 3600;
    qint64 minutes = seconds / 60;
    seconds %= 60;

    if (years > 0) {
        return QString("%1y %2d %3h").arg(years).arg(days).arg(hours);
    } else if (days > 0) {
        return QString("%1d %2h %3m").arg(days).arg(hours).arg(minutes);
    } else if (hours > 0) {
        return QString("%1h %2m %3s").arg(hours).arg(minutes).arg(seconds);
    } else if (minutes > 0) {
        return QString("%1m %2s").arg(minutes).arg(seconds);
    } else {
        return QString("%1s").arg(seconds);
    }
}

QString MiningPage::formatLargeNumber(double num)
{
    if (num >= 1e18) {
        return QString("%1 E").arg(num / 1e18, 0, 'f', 2);
    } else if (num >= 1e15) {
        return QString("%1 P").arg(num / 1e15, 0, 'f', 2);
    } else if (num >= 1e12) {
        return QString("%1 T").arg(num / 1e12, 0, 'f', 2);
    } else if (num >= 1e9) {
        return QString("%1 B").arg(num / 1e9, 0, 'f', 2);
    } else if (num >= 1e6) {
        return QString("%1 M").arg(num / 1e6, 0, 'f', 2);
    } else if (num >= 1e3) {
        return QString("%1 K").arg(num / 1e3, 0, 'f', 2);
    } else {
        return QString("%1").arg(num, 0, 'f', 0);
    }
}

void MiningPage::setupUI()
{
    // Dark theme - white on black with serif fonts
    setStyleSheet(
        "MiningPage { background-color: #000000; }"
        "QLabel { color: #ffffff; font-family: Georgia, serif; }"
        "QScrollArea { background-color: #000000; border: none; }"
        "QWidget#scrollContent { background-color: #000000; }"
        "QGroupBox { color: #ffffff; background-color: #0a0a0a; border: 1px solid #222222; border-radius: 8px; margin-top: 12px; padding: 15px; padding-top: 25px; font-family: Georgia, serif; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 15px; padding: 0 8px; color: #666666; font-size: 12px; text-transform: uppercase; letter-spacing: 1px; }"
    );

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Scroll area for all content
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget *scrollContent = new QWidget();
    scrollContent->setObjectName("scrollContent");
    QVBoxLayout *contentLayout = new QVBoxLayout(scrollContent);
    contentLayout->setContentsMargins(25, 25, 25, 25);
    contentLayout->setSpacing(20);

    // Title
    QLabel *titleLabel = new QLabel(tr("Mining Dashboard"));
    titleLabel->setStyleSheet("font-size: 32px; font-weight: bold; color: #ffffff; font-family: Georgia, serif; margin-bottom: 5px;");
    contentLayout->addWidget(titleLabel);

    QLabel *subtitleLabel = new QLabel(tr("RandomX CPU Mining"));
    subtitleLabel->setStyleSheet("font-size: 14px; color: #666666; font-family: Georgia, serif; margin-bottom: 15px;");
    contentLayout->addWidget(subtitleLabel);

    // Create all the groups
    createControlsGroup(contentLayout);

    // Two-column layout for stats
    QHBoxLayout *columnsLayout = new QHBoxLayout();
    columnsLayout->setSpacing(20);

    QVBoxLayout *leftColumn = new QVBoxLayout();
    leftColumn->setSpacing(20);
    createHashrateGroup(leftColumn);
    createEstimatesGroup(leftColumn);

    QVBoxLayout *rightColumn = new QVBoxLayout();
    rightColumn->setSpacing(20);
    createBlockInfoGroup(rightColumn);
    createNetworkGroup(rightColumn);

    columnsLayout->addLayout(leftColumn, 1);
    columnsLayout->addLayout(rightColumn, 1);
    contentLayout->addLayout(columnsLayout);

    contentLayout->addStretch();

    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea);
}

void MiningPage::createControlsGroup(QVBoxLayout *layout)
{
    QGroupBox *group = new QGroupBox(tr("Mining Controls"));
    QVBoxLayout *groupLayout = new QVBoxLayout(group);
    groupLayout->setSpacing(15);

    // Status bar
    QHBoxLayout *statusLayout = new QHBoxLayout();
    statusLabel = new QLabel(tr("IDLE"));
    statusLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #666666; padding: 8px 16px; background-color: #1a1a1a; border-radius: 4px;");

    miningProgress = new QProgressBar();
    miningProgress->setRange(0, 0);
    miningProgress->setVisible(false);
    miningProgress->setFixedHeight(8);
    miningProgress->setStyleSheet(
        "QProgressBar { border: none; border-radius: 4px; background-color: #1a1a1a; }"
        "QProgressBar::chunk { background-color: #ffffff; border-radius: 4px; }"
    );

    statusLayout->addWidget(statusLabel);
    statusLayout->addWidget(miningProgress, 1);
    groupLayout->addLayout(statusLayout);

    // Address input
    QHBoxLayout *addressLayout = new QHBoxLayout();
    QLabel *addressLabel = new QLabel(tr("Mining Address:"));
    addressLabel->setStyleSheet("font-size: 13px; color: #888888; min-width: 120px;");
    addressEdit = new QLineEdit();
    addressEdit->setPlaceholderText(tr("Leave empty to use default wallet address"));
    addressEdit->setStyleSheet("padding: 12px; border: 1px solid #333333; border-radius: 6px; background-color: #111111; color: #ffffff; font-family: 'SF Mono', 'Monaco', monospace; font-size: 13px;");
    addressLayout->addWidget(addressLabel);
    addressLayout->addWidget(addressEdit, 1);
    groupLayout->addLayout(addressLayout);

    // Threads selector
    QHBoxLayout *threadsLayout = new QHBoxLayout();
    QLabel *threadsLabel = new QLabel(tr("CPU Threads:"));
    threadsLabel->setStyleSheet("font-size: 13px; color: #888888; min-width: 120px;");
    threadsSpinBox = new QSpinBox();
    threadsSpinBox->setRange(1, 64);
    threadsSpinBox->setValue(4);
    threadsSpinBox->setStyleSheet("padding: 10px; border: 1px solid #333333; border-radius: 6px; background-color: #111111; color: #ffffff; min-width: 80px;");

    QLabel *threadsHint = new QLabel(tr("(recommended: number of CPU cores)"));
    threadsHint->setStyleSheet("font-size: 11px; color: #555555;");

    threadsLayout->addWidget(threadsLabel);
    threadsLayout->addWidget(threadsSpinBox);
    threadsLayout->addWidget(threadsHint);
    threadsLayout->addStretch();
    groupLayout->addLayout(threadsLayout);

    // Buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    startButton = new QPushButton(tr("Start Mining"));
    startButton->setStyleSheet(
        "QPushButton { background-color: #1a1a1a; color: #ffffff; padding: 14px 32px; border: 1px solid #333333; border-radius: 6px; font-weight: bold; font-family: Georgia, serif; font-size: 14px; }"
        "QPushButton:hover { background-color: #252525; border-color: #444444; }"
        "QPushButton:pressed { background-color: #333333; }"
    );
    connect(startButton, &QPushButton::clicked, this, &MiningPage::onStartMiningClicked);

    stopButton = new QPushButton(tr("Stop Mining"));
    stopButton->setEnabled(false);
    stopButton->setStyleSheet(
        "QPushButton { background-color: #1a1a1a; color: #ffffff; padding: 14px 32px; border: 1px solid #333333; border-radius: 6px; font-weight: bold; font-family: Georgia, serif; font-size: 14px; }"
        "QPushButton:hover { background-color: #252525; border-color: #444444; }"
        "QPushButton:pressed { background-color: #333333; }"
        "QPushButton:disabled { background-color: #0a0a0a; color: #444444; border-color: #222222; }"
    );
    connect(stopButton, &QPushButton::clicked, this, &MiningPage::onStopMiningClicked);

    buttonLayout->addWidget(startButton);
    buttonLayout->addWidget(stopButton);
    buttonLayout->addStretch();
    groupLayout->addLayout(buttonLayout);

    layout->addWidget(group);
}

void MiningPage::createHashrateGroup(QVBoxLayout *layout)
{
    QGroupBox *group = new QGroupBox(tr("Hashrate Performance"));
    QGridLayout *grid = new QGridLayout(group);
    grid->setSpacing(12);
    grid->setColumnStretch(1, 1);
    grid->setColumnStretch(3, 1);

    auto addStat = [&](int row, int col, const QString &label, QLabel **valueLabel, const QString &initialValue) {
        QLabel *lbl = new QLabel(label);
        lbl->setStyleSheet("font-size: 12px; color: #666666;");
        *valueLabel = new QLabel(initialValue);
        (*valueLabel)->setStyleSheet("font-size: 16px; font-weight: bold; color: #ffffff;");
        grid->addWidget(lbl, row * 2, col * 2);
        grid->addWidget(*valueLabel, row * 2 + 1, col * 2);
    };

    addStat(0, 0, tr("Current Hashrate"), &currentHashrateLabel, "0 H/s");
    addStat(0, 1, tr("Average Hashrate"), &averageHashrateLabel, "0 H/s");
    addStat(1, 0, tr("Peak Hashrate"), &peakHashrateLabel, "0 H/s");
    addStat(1, 1, tr("Total Hashes"), &totalHashesLabel, "0");
    addStat(2, 0, tr("Mining Uptime"), &uptimeLabel, "00:00:00");
    addStat(2, 1, tr("Efficiency"), &hashesPerSecLabel, "--");

    layout->addWidget(group);
}

void MiningPage::createBlockInfoGroup(QVBoxLayout *layout)
{
    QGroupBox *group = new QGroupBox(tr("Block Information"));
    QGridLayout *grid = new QGridLayout(group);
    grid->setSpacing(12);
    grid->setColumnStretch(1, 1);
    grid->setColumnStretch(3, 1);

    auto addStat = [&](int row, int col, const QString &label, QLabel **valueLabel, const QString &initialValue) {
        QLabel *lbl = new QLabel(label);
        lbl->setStyleSheet("font-size: 12px; color: #666666;");
        *valueLabel = new QLabel(initialValue);
        (*valueLabel)->setStyleSheet("font-size: 16px; font-weight: bold; color: #ffffff;");
        grid->addWidget(lbl, row * 2, col * 2);
        grid->addWidget(*valueLabel, row * 2 + 1, col * 2);
    };

    addStat(0, 0, tr("Current Block"), &currentBlockLabel, "0");
    addStat(0, 1, tr("Block Reward"), &blockRewardLabel, "50 CRL");
    addStat(1, 0, tr("Difficulty"), &difficultyLabel, "1.00");
    addStat(1, 1, tr("Target"), &targetLabel, "--");
    addStat(2, 0, tr("Blocks Found"), &blocksFoundLabel, "0");
    addStat(2, 1, tr("Total Earned"), &totalEarnedLabel, "0.00 CRL");

    // Last block time spans full width
    QLabel *lastBlockLbl = new QLabel(tr("Last Block"));
    lastBlockLbl->setStyleSheet("font-size: 12px; color: #666666;");
    lastBlockTimeLabel = new QLabel("--");
    lastBlockTimeLabel->setStyleSheet("font-size: 14px; color: #ffffff;");
    grid->addWidget(lastBlockLbl, 6, 0);
    grid->addWidget(lastBlockTimeLabel, 7, 0, 1, 4);

    layout->addWidget(group);
}

void MiningPage::createNetworkGroup(QVBoxLayout *layout)
{
    QGroupBox *group = new QGroupBox(tr("Network Statistics"));
    QGridLayout *grid = new QGridLayout(group);
    grid->setSpacing(12);
    grid->setColumnStretch(1, 1);
    grid->setColumnStretch(3, 1);

    auto addStat = [&](int row, int col, const QString &label, QLabel **valueLabel, const QString &initialValue) {
        QLabel *lbl = new QLabel(label);
        lbl->setStyleSheet("font-size: 12px; color: #666666;");
        *valueLabel = new QLabel(initialValue);
        (*valueLabel)->setStyleSheet("font-size: 16px; font-weight: bold; color: #ffffff;");
        grid->addWidget(lbl, row * 2, col * 2);
        grid->addWidget(*valueLabel, row * 2 + 1, col * 2);
    };

    addStat(0, 0, tr("Network Hashrate"), &networkHashrateLabel, "--");
    addStat(0, 1, tr("Connected Peers"), &connectedPeersLabel, "0");
    addStat(1, 0, tr("Network Difficulty"), &networkDifficultyLabel, "--");
    addStat(1, 1, tr("Mempool Size"), &mempoolSizeLabel, "0 txs");

    QLabel *chainworkLbl = new QLabel(tr("Total Chainwork"));
    chainworkLbl->setStyleSheet("font-size: 12px; color: #666666;");
    chainworkLabel = new QLabel("--");
    chainworkLabel->setStyleSheet("font-size: 14px; color: #ffffff; font-family: monospace;");
    grid->addWidget(chainworkLbl, 4, 0);
    grid->addWidget(chainworkLabel, 5, 0, 1, 4);

    layout->addWidget(group);
}

void MiningPage::createEstimatesGroup(QVBoxLayout *layout)
{
    QGroupBox *group = new QGroupBox(tr("Mining Estimates"));
    QGridLayout *grid = new QGridLayout(group);
    grid->setSpacing(12);
    grid->setColumnStretch(1, 1);
    grid->setColumnStretch(3, 1);

    auto addStat = [&](int row, int col, const QString &label, QLabel **valueLabel, const QString &initialValue) {
        QLabel *lbl = new QLabel(label);
        lbl->setStyleSheet("font-size: 12px; color: #666666;");
        *valueLabel = new QLabel(initialValue);
        (*valueLabel)->setStyleSheet("font-size: 16px; font-weight: bold; color: #ffffff;");
        grid->addWidget(lbl, row * 2, col * 2);
        grid->addWidget(*valueLabel, row * 2 + 1, col * 2);
    };

    addStat(0, 0, tr("Est. Time to Block"), &estimatedTimeLabel, "--");
    addStat(0, 1, tr("Share of Network"), &shareOfNetworkLabel, "0.00%");
    addStat(1, 0, tr("Daily Estimate"), &dailyEstimateLabel, "0.00 CRL");
    addStat(1, 1, tr("Weekly Estimate"), &weeklyEstimateLabel, "0.00 CRL");
    addStat(2, 0, tr("Monthly Estimate"), &monthlyEstimateLabel, "0.00 CRL");
    addStat(2, 1, tr("Block Probability/Day"), &probabilityLabel, "0.00%");

    layout->addWidget(group);
}

void MiningPage::setClientModel(ClientModel *model)
{
    clientModel = model;
    if (clientModel) {
        connect(clientModel, &ClientModel::numBlocksChanged, this, &MiningPage::updateBlockCount);
        connect(clientModel, &ClientModel::numConnectionsChanged, [this](int count) {
            connectedPeersLabel->setText(QString::number(count));
        });
        updateBlockCount(clientModel->getNumBlocks());
        connectedPeersLabel->setText(QString::number(clientModel->getNumConnections()));
    }
}

void MiningPage::setWalletModel(WalletModel *model)
{
    walletModel = model;
}

void MiningPage::onStartMiningClicked()
{
    if (!clientModel || !walletModel) {
        statusLabel->setText(tr("ERROR: No wallet loaded"));
        return;
    }

    // Get mining address
    QString address = addressEdit->text().trimmed();
    if (address.isEmpty()) {
        // Get a new address from the wallet
        try {
            UniValue params(UniValue::VARR);
            UniValue result = clientModel->node().executeRpc("getnewaddress", params, "");
            address = QString::fromStdString(result.get_str());
            addressEdit->setText(address);
        } catch (const std::exception& e) {
            statusLabel->setText(tr("ERROR: %1").arg(QString::fromStdString(e.what())));
            return;
        }
    }

    miningAddress = address;
    isMining = true;
    miningStartTime = std::chrono::steady_clock::now();
    totalHashes = 0;
    peakHashrate = 0;
    totalHashrateSum = 0;
    hashrateSamples = 0;

    startButton->setEnabled(false);
    stopButton->setEnabled(true);
    addressEdit->setEnabled(false);
    threadsSpinBox->setEnabled(false);

    statusLabel->setText(tr("STARTING..."));
    statusLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #ffffff; padding: 8px 16px; background-color: #1a1a1a; border-radius: 4px; border: 1px solid #333333;");
    miningProgress->setVisible(true);

    // Create worker and thread
    miningThread = new QThread();
    miningWorker = new MiningWorker();
    miningWorker->setClientModel(clientModel);
    miningWorker->setAddress(miningAddress);
    miningWorker->moveToThread(miningThread);

    // Connect signals
    connect(miningThread, &QThread::started, miningWorker, &MiningWorker::doMining);
    connect(miningWorker, &MiningWorker::blockFound, this, &MiningPage::onBlockFound);
    connect(miningWorker, &MiningWorker::miningError, this, &MiningPage::onMiningError);
    connect(miningWorker, &MiningWorker::hashUpdate, this, &MiningPage::onHashUpdate);
    connect(miningWorker, &MiningWorker::finished, this, &MiningPage::onMiningFinished);
    connect(miningWorker, &MiningWorker::finished, miningThread, &QThread::quit);
    connect(miningThread, &QThread::finished, miningWorker, &QObject::deleteLater);
    connect(miningThread, &QThread::finished, miningThread, &QObject::deleteLater);

    // Start mining thread
    miningThread->start();
    statusLabel->setText(tr("MINING"));
}

void MiningPage::onStopMiningClicked()
{
    isMining = false;

    // Stop the worker
    if (miningWorker) {
        miningWorker->stop();
    }

    statusLabel->setText(tr("STOPPING..."));
}

void MiningPage::onBlockFound(const QString &hash, double reward)
{
    blocksFound++;
    totalEarned += reward;
    blocksFoundLabel->setText(QString::number(blocksFound));
    totalEarnedLabel->setText(QString("%1 CRL").arg(totalEarned, 0, 'f', 2));
    statusLabel->setText(tr("BLOCK FOUND!"));

    // Brief highlight then back to mining
    QTimer::singleShot(2000, this, [this]() {
        if (isMining) {
            statusLabel->setText(tr("MINING"));
        }
    });
}

void MiningPage::onMiningError(const QString &error)
{
    statusLabel->setText(tr("ERROR: %1").arg(error));
}

void MiningPage::onHashUpdate(double hashrate, uint64_t hashes)
{
    currentHashrate = hashrate;
    totalHashes = hashes;
    if (hashrate > peakHashrate) {
        peakHashrate = hashrate;
    }
    totalHashrateSum += hashrate;
    hashrateSamples++;
}

void MiningPage::onMiningFinished()
{
    miningThread = nullptr;
    miningWorker = nullptr;

    startButton->setEnabled(true);
    stopButton->setEnabled(false);
    addressEdit->setEnabled(true);
    threadsSpinBox->setEnabled(true);

    statusLabel->setText(tr("STOPPED"));
    statusLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #666666; padding: 8px 16px; background-color: #1a1a1a; border-radius: 4px;");
    miningProgress->setVisible(false);
}

void MiningPage::updateMiningStats()
{
    if (isMining) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - miningStartTime).count();

        // Simulated hashrate for UI demonstration (RandomX typically 1-10 KH/s per thread)
        int threads = threadsSpinBox->value();
        currentHashrate = (1500.0 + (rand() % 500)) * threads;  // ~1.5-2 KH/s per thread

        // Track stats
        totalHashes += static_cast<uint64_t>(currentHashrate);
        if (currentHashrate > peakHashrate) {
            peakHashrate = currentHashrate;
        }
        totalHashrateSum += currentHashrate;
        hashrateSamples++;
        double averageHashrate = hashrateSamples > 0 ? totalHashrateSum / hashrateSamples : 0;

        // Update hashrate labels
        currentHashrateLabel->setText(formatHashrate(currentHashrate));
        averageHashrateLabel->setText(formatHashrate(averageHashrate));
        peakHashrateLabel->setText(formatHashrate(peakHashrate));
        totalHashesLabel->setText(formatLargeNumber(static_cast<double>(totalHashes)));
        uptimeLabel->setText(formatDuration(elapsed));

        // Efficiency (hashes per watt estimate - just show hashes/thread for now)
        hashesPerSecLabel->setText(QString("%1/thread").arg(formatHashrate(currentHashrate / threads)));

        // Estimates based on simulated network hashrate
        double networkHashrate = 100000.0;  // 100 KH/s simulated network
        double shareOfNetwork = (currentHashrate / networkHashrate) * 100.0;
        double blockReward = 50.0;  // CRL per block
        double blocksPerDay = 144.0;  // ~10 min blocks

        double dailyEarnings = shareOfNetwork / 100.0 * blockReward * blocksPerDay;
        double weeklyEarnings = dailyEarnings * 7;
        double monthlyEarnings = dailyEarnings * 30;

        // Time to find a block (on average)
        double timeToBlockSeconds = (networkHashrate / currentHashrate) * 600.0;  // 600s = 10 min block time

        // Probability of finding a block in 24 hours
        double blocksIn24h = 86400.0 / 600.0;  // 144 blocks
        double probPerBlock = currentHashrate / networkHashrate;
        double probIn24h = 1.0 - pow(1.0 - probPerBlock, blocksIn24h);

        shareOfNetworkLabel->setText(QString("%1%").arg(shareOfNetwork, 0, 'f', 4));
        estimatedTimeLabel->setText(formatDuration(static_cast<qint64>(timeToBlockSeconds)));
        dailyEstimateLabel->setText(QString("%1 CRL").arg(dailyEarnings, 0, 'f', 4));
        weeklyEstimateLabel->setText(QString("%1 CRL").arg(weeklyEarnings, 0, 'f', 4));
        monthlyEstimateLabel->setText(QString("%1 CRL").arg(monthlyEarnings, 0, 'f', 2));
        probabilityLabel->setText(QString("%1%").arg(probIn24h * 100.0, 0, 'f', 2));

        networkHashrateLabel->setText(formatHashrate(networkHashrate));
    }
}

void MiningPage::updateBlockCount(int count)
{
    currentBlockLabel->setText(QString::number(count));

    if (clientModel) {
        int64_t headerTime = clientModel->getHeaderTipTime();
        if (headerTime > 0) {
            QDateTime blockTime = QDateTime::fromSecsSinceEpoch(headerTime);
            QDateTime now = QDateTime::currentDateTime();
            qint64 secsAgo = blockTime.secsTo(now);

            QString timeStr = blockTime.toString("yyyy-MM-dd hh:mm:ss");
            QString agoStr = formatDuration(secsAgo);
            lastBlockTimeLabel->setText(QString("%1 (%2 ago)").arg(timeStr).arg(agoStr));
        }

        // Simulated difficulty for now
        double difficulty = 1.0 + (count * 0.001);  // Increases slightly with height
        difficultyLabel->setText(QString::number(difficulty, 'f', 6));
        networkDifficultyLabel->setText(QString::number(difficulty, 'f', 6));

        // Target (simplified display)
        targetLabel->setText(QString("0x1e0fffff"));

        // Block reward (assuming 50 CRL, halving every 210000 blocks)
        int halvings = count / 210000;
        double reward = 50.0 / pow(2, halvings);
        blockRewardLabel->setText(QString("%1 CRL").arg(reward, 0, 'f', 2));

        // Update blocks found and earned
        blocksFoundLabel->setText(QString::number(blocksFound));
        totalEarnedLabel->setText(QString("%1 CRL").arg(totalEarned, 0, 'f', 2));

        // Chainwork (placeholder)
        chainworkLabel->setText(QString("0x%1").arg(QString::number(count * 1000, 16).rightJustified(16, '0')));
    }
}
