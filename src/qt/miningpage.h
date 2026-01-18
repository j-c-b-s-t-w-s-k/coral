// Copyright (c) 2026 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CORAL_QT_MININGPAGE_H
#define CORAL_QT_MININGPAGE_H

#include <QWidget>
#include <QTimer>
#include <QThread>
#include <atomic>

class ClientModel;
class WalletModel;
class PlatformStyle;

QT_BEGIN_NAMESPACE
class QLabel;
class QLineEdit;
class QPushButton;
class QProgressBar;
class QGroupBox;
class QSpinBox;
class QGridLayout;
class QVBoxLayout;
class QString;
QT_END_NAMESPACE

// Worker class for background mining
class MiningWorker : public QObject
{
    Q_OBJECT

public:
    explicit MiningWorker(QObject *parent = nullptr);
    void setClientModel(ClientModel *model) { clientModel = model; }
    void setAddress(const QString &addr) { miningAddress = addr; }
    void stop() { shouldStop = true; }

public Q_SLOTS:
    void doMining();

Q_SIGNALS:
    void blockFound(const QString &hash, double reward);
    void miningError(const QString &error);
    void hashUpdate(double hashrate, uint64_t totalHashes);
    void finished();

private:
    ClientModel *clientModel{nullptr};
    QString miningAddress;
    std::atomic<bool> shouldStop{false};
};

class MiningPage : public QWidget
{
    Q_OBJECT

public:
    explicit MiningPage(const PlatformStyle *platformStyle, QWidget *parent = nullptr);
    ~MiningPage();

    void setClientModel(ClientModel *model);
    void setWalletModel(WalletModel *model);

private Q_SLOTS:
    void onStartMiningClicked();
    void onStopMiningClicked();
    void updateMiningStats();
    void updateBlockCount(int count);
    void onBlockFound(const QString &hash, double reward);
    void onMiningError(const QString &error);
    void onHashUpdate(double hashrate, uint64_t hashes);
    void onMiningFinished();

private:
    void setupUI();
    void createControlsGroup(QVBoxLayout *layout);
    void createHashrateGroup(QVBoxLayout *layout);
    void createBlockInfoGroup(QVBoxLayout *layout);
    void createNetworkGroup(QVBoxLayout *layout);
    void createEstimatesGroup(QVBoxLayout *layout);
    QString formatHashrate(double hashrate);
    QString formatDuration(qint64 seconds);
    QString formatLargeNumber(double num);

    const PlatformStyle *platformStyle;
    ClientModel *clientModel{nullptr};
    WalletModel *walletModel{nullptr};
    QTimer *refreshTimer{nullptr};
    QString miningAddress;

    // Threading
    QThread *miningThread{nullptr};
    MiningWorker *miningWorker{nullptr};

    // Mining controls
    QLineEdit *addressEdit{nullptr};
    QSpinBox *threadsSpinBox{nullptr};
    QPushButton *startButton{nullptr};
    QPushButton *stopButton{nullptr};

    // Status display
    QLabel *statusLabel{nullptr};
    QProgressBar *miningProgress{nullptr};

    // Hashrate stats
    QLabel *currentHashrateLabel{nullptr};
    QLabel *averageHashrateLabel{nullptr};
    QLabel *peakHashrateLabel{nullptr};
    QLabel *totalHashesLabel{nullptr};
    QLabel *hashesPerSecLabel{nullptr};
    QLabel *uptimeLabel{nullptr};

    // Block info
    QLabel *currentBlockLabel{nullptr};
    QLabel *difficultyLabel{nullptr};
    QLabel *targetLabel{nullptr};
    QLabel *lastBlockTimeLabel{nullptr};
    QLabel *blockRewardLabel{nullptr};
    QLabel *blocksFoundLabel{nullptr};
    QLabel *totalEarnedLabel{nullptr};

    // Network stats
    QLabel *networkHashrateLabel{nullptr};
    QLabel *networkDifficultyLabel{nullptr};
    QLabel *connectedPeersLabel{nullptr};
    QLabel *mempoolSizeLabel{nullptr};
    QLabel *chainworkLabel{nullptr};

    // Estimates
    QLabel *estimatedTimeLabel{nullptr};
    QLabel *probabilityLabel{nullptr};
    QLabel *dailyEstimateLabel{nullptr};
    QLabel *weeklyEstimateLabel{nullptr};
    QLabel *monthlyEstimateLabel{nullptr};
    QLabel *shareOfNetworkLabel{nullptr};

    // Stats tracking
    bool isMining{false};
    int blocksFound{0};
    uint64_t totalHashes{0};
    double peakHashrate{0};
    double currentHashrate{0};
    double totalHashrateSum{0};
    int hashrateSamples{0};
    double totalEarned{0};
    std::chrono::steady_clock::time_point miningStartTime;
};

#endif // CORAL_QT_MININGPAGE_H
