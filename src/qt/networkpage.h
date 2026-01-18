// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CORAL_QT_NETWORKPAGE_H
#define CORAL_QT_NETWORKPAGE_H

#include <QWidget>
#include <QTimer>

class ClientModel;
class PlatformStyle;
class PeerTableModel;
class PeerTableSortProxy;

QT_BEGIN_NAMESPACE
class QLabel;
class QTableView;
class QVBoxLayout;
class QHBoxLayout;
class QGroupBox;
class QProgressBar;
class QPushButton;
class QLineEdit;
QT_END_NAMESPACE

/**
 * Network page showing peer discovery and mempool status.
 * This provides a clean, bespoke interface for network monitoring.
 */
class NetworkPage : public QWidget
{
    Q_OBJECT

public:
    explicit NetworkPage(const PlatformStyle *platformStyle, QWidget *parent = nullptr);
    ~NetworkPage();

    void setClientModel(ClientModel *clientModel);

public Q_SLOTS:
    void updateNetworkStats();
    void updateMempoolStats();
    void updatePeerCount(int count);
    void onAddPeerClicked();

private:
    void setupUI();
    void setupPeerSection();
    void setupMempoolSection();
    void setupNetworkStatsSection();

    ClientModel *clientModel{nullptr};
    const PlatformStyle *platformStyle;

    // Peer discovery section
    QGroupBox *peerGroupBox{nullptr};
    QTableView *peerTableView{nullptr};
    QLabel *peerCountLabel{nullptr};
    QLabel *inboundLabel{nullptr};
    QLabel *outboundLabel{nullptr};
    QLineEdit *addPeerEdit{nullptr};
    QPushButton *addPeerButton{nullptr};

    // Mempool section
    QGroupBox *mempoolGroupBox{nullptr};
    QLabel *mempoolSizeLabel{nullptr};
    QLabel *mempoolBytesLabel{nullptr};
    QLabel *mempoolMinFeeLabel{nullptr};
    QProgressBar *mempoolUsageBar{nullptr};

    // Network stats section
    QGroupBox *networkGroupBox{nullptr};
    QLabel *bytesInLabel{nullptr};
    QLabel *bytesOutLabel{nullptr};
    QLabel *networkActiveLabel{nullptr};
    QLabel *blockHeightLabel{nullptr};
    QLabel *lastBlockTimeLabel{nullptr};

    QTimer *refreshTimer{nullptr};
};

#endif // CORAL_QT_NETWORKPAGE_H
