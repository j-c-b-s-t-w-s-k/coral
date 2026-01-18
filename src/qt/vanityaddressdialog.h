// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CORAL_QT_VANITYADDRESSDIALOG_H
#define CORAL_QT_VANITYADDRESSDIALOG_H

#include <QDialog>
#include <QThread>
#include <atomic>

class WalletModel;
class PlatformStyle;

QT_BEGIN_NAMESPACE
class QLabel;
class QLineEdit;
class QPushButton;
class QProgressBar;
class QTextEdit;
QT_END_NAMESPACE

/**
 * Worker thread for vanity address generation
 */
class VanityWorker : public QThread
{
    Q_OBJECT

public:
    VanityWorker(const QString& prefix, QObject* parent = nullptr);
    void stop();

Q_SIGNALS:
    void foundAddress(const QString& address, const QString& privateKey);
    void progress(quint64 attempts);

protected:
    void run() override;

private:
    QString m_prefix;
    std::atomic<bool> m_stopped{false};
};

/**
 * Dialog for generating vanity addresses with custom prefixes.
 * Addresses start with "1" followed by the user-specified pattern.
 */
class VanityAddressDialog : public QDialog
{
    Q_OBJECT

public:
    explicit VanityAddressDialog(const PlatformStyle* platformStyle, QWidget* parent = nullptr);
    ~VanityAddressDialog();

    void setModel(WalletModel* model);

private Q_SLOTS:
    void onGenerateClicked();
    void onStopClicked();
    void onFoundAddress(const QString& address, const QString& privateKey);
    void onProgress(quint64 attempts);
    void onImportClicked();

private:
    void setupUI();

    WalletModel* walletModel{nullptr};
    const PlatformStyle* platformStyle;

    QLineEdit* prefixEdit{nullptr};
    QPushButton* generateButton{nullptr};
    QPushButton* stopButton{nullptr};
    QPushButton* importButton{nullptr};
    QProgressBar* progressBar{nullptr};
    QLabel* statusLabel{nullptr};
    QLabel* attemptsLabel{nullptr};
    QTextEdit* resultEdit{nullptr};

    VanityWorker* worker{nullptr};
    QString foundPrivateKey;
    QString foundAddress;
};

#endif // CORAL_QT_VANITYADDRESSDIALOG_H
