// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/vanityaddressdialog.h>

#include <qt/guiutil.h>
#include <qt/platformstyle.h>
#include <qt/walletmodel.h>

#include <key.h>
#include <key_io.h>
#include <util/strencodings.h>

#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QTextEdit>
#include <QVBoxLayout>

// VanityWorker implementation
VanityWorker::VanityWorker(const QString& prefix, QObject* parent)
    : QThread(parent)
    , m_prefix(prefix)
{
}

void VanityWorker::stop()
{
    m_stopped = true;
}

void VanityWorker::run()
{
    quint64 attempts = 0;
    std::string targetPrefix = "1" + m_prefix.toStdString();

    while (!m_stopped) {
        // Generate a new random key
        CKey key;
        key.MakeNewKey(true);  // Compressed

        // Get the public key and derive the address
        CPubKey pubKey = key.GetPubKey();
        CTxDestination dest = PKHash(pubKey);
        std::string address = EncodeDestination(dest);

        attempts++;

        // Report progress every 1000 attempts
        if (attempts % 1000 == 0) {
            Q_EMIT progress(attempts);
        }

        // Check if address matches the prefix
        if (address.substr(0, targetPrefix.length()) == targetPrefix) {
            // Found a matching address
            std::string privKeyWIF = EncodeSecret(key);
            Q_EMIT foundAddress(QString::fromStdString(address), QString::fromStdString(privKeyWIF));
            return;
        }
    }
}

// VanityAddressDialog implementation
VanityAddressDialog::VanityAddressDialog(const PlatformStyle* _platformStyle, QWidget* parent)
    : QDialog(parent)
    , platformStyle(_platformStyle)
{
    setWindowTitle(tr("Vanity Address Generator"));
    setMinimumSize(500, 400);
    setupUI();
}

VanityAddressDialog::~VanityAddressDialog()
{
    if (worker) {
        worker->stop();
        worker->wait();
        delete worker;
    }
}

void VanityAddressDialog::setupUI()
{
    // Dark theme styling
    setStyleSheet(
        "QDialog { background-color: #1a1a2e; }"
        "QLabel { color: #eaeaea; }"
        "QGroupBox { color: #eaeaea; background-color: #16213e; border: 2px solid #FF6B35; border-radius: 8px; margin-top: 10px; padding-top: 10px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; color: #FF6B35; }"
        "QLineEdit { padding: 8px; border: 1px solid #FF6B35; border-radius: 4px; background-color: #0f3460; color: #eaeaea; }"
        "QTextEdit { padding: 8px; border: 1px solid #FF6B35; border-radius: 4px; background-color: #0f3460; color: #eaeaea; font-family: monospace; }"
        "QPushButton { background-color: #FF6B35; color: white; padding: 10px 20px; border: none; border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background-color: #e55a2b; }"
        "QPushButton:pressed { background-color: #cc4f26; }"
        "QPushButton:disabled { background-color: #666; }"
    );

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // Title
    QLabel* titleLabel = new QLabel(tr("Generate Vanity Address"));
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #FF6B35;");
    mainLayout->addWidget(titleLabel);

    // Description
    QLabel* descLabel = new QLabel(tr("Generate a Coral address starting with your chosen prefix.\nAddresses always start with '1', followed by your prefix."));
    descLabel->setWordWrap(true);
    mainLayout->addWidget(descLabel);

    // Input section
    QGroupBox* inputGroup = new QGroupBox(tr("Prefix"));
    QHBoxLayout* inputLayout = new QHBoxLayout(inputGroup);

    QLabel* prefixLabel = new QLabel("1");
    prefixLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #FF6B35;");

    prefixEdit = new QLineEdit();
    prefixEdit->setPlaceholderText(tr("Enter desired prefix (e.g., Coral, ABC)"));
    prefixEdit->setMaxLength(6);  // Limit prefix length for reasonable generation time

    inputLayout->addWidget(prefixLabel);
    inputLayout->addWidget(prefixEdit, 1);
    mainLayout->addWidget(inputGroup);

    // Note about case sensitivity
    QLabel* noteLabel = new QLabel(tr("Note: Base58 addresses are case-sensitive. Longer prefixes take exponentially longer to find.\nAvoid characters: 0, O, I, l (not used in Base58)"));
    noteLabel->setStyleSheet("color: #888; font-size: 11px;");
    noteLabel->setWordWrap(true);
    mainLayout->addWidget(noteLabel);

    // Buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();

    generateButton = new QPushButton(tr("Generate"));
    connect(generateButton, &QPushButton::clicked, this, &VanityAddressDialog::onGenerateClicked);

    stopButton = new QPushButton(tr("Stop"));
    stopButton->setEnabled(false);
    connect(stopButton, &QPushButton::clicked, this, &VanityAddressDialog::onStopClicked);

    buttonLayout->addWidget(generateButton);
    buttonLayout->addWidget(stopButton);
    buttonLayout->addStretch();
    mainLayout->addLayout(buttonLayout);

    // Progress
    progressBar = new QProgressBar();
    progressBar->setRange(0, 0);  // Indeterminate
    progressBar->setVisible(false);
    progressBar->setStyleSheet(
        "QProgressBar { border: 1px solid #FF6B35; border-radius: 4px; background-color: #0f3460; }"
        "QProgressBar::chunk { background-color: #FF6B35; }"
    );
    mainLayout->addWidget(progressBar);

    // Status
    statusLabel = new QLabel(tr("Ready"));
    mainLayout->addWidget(statusLabel);

    attemptsLabel = new QLabel();
    attemptsLabel->setStyleSheet("color: #888;");
    mainLayout->addWidget(attemptsLabel);

    // Result section
    QGroupBox* resultGroup = new QGroupBox(tr("Result"));
    QVBoxLayout* resultLayout = new QVBoxLayout(resultGroup);

    resultEdit = new QTextEdit();
    resultEdit->setReadOnly(true);
    resultEdit->setMinimumHeight(100);

    importButton = new QPushButton(tr("Import to Wallet"));
    importButton->setEnabled(false);
    connect(importButton, &QPushButton::clicked, this, &VanityAddressDialog::onImportClicked);

    resultLayout->addWidget(resultEdit);
    resultLayout->addWidget(importButton);
    mainLayout->addWidget(resultGroup);

    mainLayout->addStretch();
}

void VanityAddressDialog::setModel(WalletModel* model)
{
    walletModel = model;
}

void VanityAddressDialog::onGenerateClicked()
{
    QString prefix = prefixEdit->text().trimmed();
    if (prefix.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Please enter a prefix."));
        return;
    }

    // Validate prefix characters (Base58 only)
    static const QString validChars = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
    for (const QChar& c : prefix) {
        if (!validChars.contains(c)) {
            QMessageBox::warning(this, tr("Error"),
                tr("Invalid character '%1'. Base58 does not include 0, O, I, or l.").arg(c));
            return;
        }
    }

    // Clear previous results
    resultEdit->clear();
    foundPrivateKey.clear();
    foundAddress.clear();
    importButton->setEnabled(false);

    // Start worker thread
    if (worker) {
        worker->stop();
        worker->wait();
        delete worker;
    }

    worker = new VanityWorker(prefix, this);
    connect(worker, &VanityWorker::foundAddress, this, &VanityAddressDialog::onFoundAddress);
    connect(worker, &VanityWorker::progress, this, &VanityAddressDialog::onProgress);
    connect(worker, &VanityWorker::finished, this, [this]() {
        generateButton->setEnabled(true);
        stopButton->setEnabled(false);
        progressBar->setVisible(false);
    });

    generateButton->setEnabled(false);
    stopButton->setEnabled(true);
    progressBar->setVisible(true);
    statusLabel->setText(tr("Searching..."));
    attemptsLabel->setText(tr("Attempts: 0"));

    worker->start();
}

void VanityAddressDialog::onStopClicked()
{
    if (worker) {
        worker->stop();
        statusLabel->setText(tr("Stopped"));
    }
}

void VanityAddressDialog::onFoundAddress(const QString& address, const QString& privateKey)
{
    foundAddress = address;
    foundPrivateKey = privateKey;

    QString result = tr("Address: %1\n\nPrivate Key (WIF): %2\n\nWARNING: Keep your private key safe! Anyone with this key can spend your funds.")
        .arg(address)
        .arg(privateKey);

    resultEdit->setText(result);
    statusLabel->setText(tr("Found matching address!"));
    statusLabel->setStyleSheet("color: #28a745; font-weight: bold;");
    importButton->setEnabled(walletModel != nullptr);
}

void VanityAddressDialog::onProgress(quint64 attempts)
{
    attemptsLabel->setText(tr("Attempts: %1").arg(attempts));
}

void VanityAddressDialog::onImportClicked()
{
    if (!walletModel || foundPrivateKey.isEmpty()) return;

    // Import the private key to the wallet
    // This would use wallet->importPrivateKey or similar
    // For now, show instructions
    QMessageBox::information(this, tr("Import Instructions"),
        tr("To import this address:\n\n"
           "1. Open the Debug Console (Help > Debug Window > Console)\n"
           "2. Run: importprivkey \"%1\"\n\n"
           "The address will be added to your wallet.")
        .arg(foundPrivateKey));
}
