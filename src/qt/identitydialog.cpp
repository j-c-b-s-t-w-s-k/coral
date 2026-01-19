// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/identitydialog.h>
#include <qt/walletmodel.h>

#include <QApplication>
#include <QClipboard>
#include <QMessageBox>
#include <QScrollArea>

IdentityDialog::IdentityDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUI();
    applyStyles();
}

void IdentityDialog::setWalletModel(WalletModel *model)
{
    m_walletModel = model;
}

void IdentityDialog::setAddress(const QString &address)
{
    m_address = address;
    if (m_addressLabel) {
        m_addressLabel->setText(address);
    }
    refreshProofsList();
}

void IdentityDialog::setupUI()
{
    setWindowTitle(tr("Identity & Verification"));
    setMinimumSize(600, 500);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(16, 16, 16, 16);

    // Tab widget
    m_tabWidget = new QTabWidget(this);

    // Profile tab
    auto *profileTab = new QWidget();
    setupProfileTab(profileTab);
    m_tabWidget->addTab(profileTab, tr("PROFILE"));

    // Identity Proofs tab
    auto *proofsTab = new QWidget();
    setupProofsTab(proofsTab);
    m_tabWidget->addTab(proofsTab, tr("IDENTITY PROOFS"));

    // PGP Key tab
    auto *pgpTab = new QWidget();
    setupPGPTab(pgpTab);
    m_tabWidget->addTab(pgpTab, tr("PGP KEY"));

    mainLayout->addWidget(m_tabWidget);

    // Bottom buttons
    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_cancelBtn = new QPushButton(tr("CANCEL"), this);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
    buttonLayout->addWidget(m_cancelBtn);

    m_saveBtn = new QPushButton(tr("SAVE"), this);
    connect(m_saveBtn, &QPushButton::clicked, this, &IdentityDialog::onSaveClicked);
    buttonLayout->addWidget(m_saveBtn);

    mainLayout->addLayout(buttonLayout);
}

void IdentityDialog::setupProfileTab(QWidget *tab)
{
    auto *layout = new QVBoxLayout(tab);
    layout->setSpacing(12);

    // Address display
    auto *addressGroup = new QGroupBox(tr("CORAL ADDRESS"), tab);
    auto *addressLayout = new QVBoxLayout(addressGroup);

    m_addressLabel = new QLabel(m_address, tab);
    m_addressLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    addressLayout->addWidget(m_addressLabel);

    layout->addWidget(addressGroup);

    // Trust score
    auto *trustGroup = new QGroupBox(tr("TRUST SCORE"), tab);
    auto *trustLayout = new QHBoxLayout(trustGroup);

    m_trustScoreLabel = new QLabel("0/100", tab);
    trustLayout->addWidget(m_trustScoreLabel);
    trustLayout->addStretch();

    auto *trustInfo = new QLabel(tr("Add identity proofs to increase your trust score"), tab);
    trustLayout->addWidget(trustInfo);

    layout->addWidget(trustGroup);

    // Display name
    auto *nameGroup = new QGroupBox(tr("DISPLAY NAME (OPTIONAL)"), tab);
    auto *nameLayout = new QVBoxLayout(nameGroup);

    m_displayNameEdit = new QLineEdit(tab);
    m_displayNameEdit->setPlaceholderText(tr("Enter a display name..."));
    m_displayNameEdit->setMaxLength(32);
    nameLayout->addWidget(m_displayNameEdit);

    layout->addWidget(nameGroup);

    // Bio
    auto *bioGroup = new QGroupBox(tr("BIO (OPTIONAL)"), tab);
    auto *bioLayout = new QVBoxLayout(bioGroup);

    m_bioEdit = new QTextEdit(tab);
    m_bioEdit->setPlaceholderText(tr("Tell others about yourself..."));
    m_bioEdit->setMaximumHeight(100);
    bioLayout->addWidget(m_bioEdit);

    layout->addWidget(bioGroup);

    layout->addStretch();
}

void IdentityDialog::setupProofsTab(QWidget *tab)
{
    auto *layout = new QVBoxLayout(tab);
    layout->setSpacing(12);

    // Existing proofs list
    auto *existingGroup = new QGroupBox(tr("VERIFIED IDENTITIES"), tab);
    auto *existingLayout = new QVBoxLayout(existingGroup);

    m_proofsList = new QListWidget(tab);
    m_proofsList->setMinimumHeight(100);
    existingLayout->addWidget(m_proofsList);

    auto *proofBtnLayout = new QHBoxLayout();
    m_removeProofBtn = new QPushButton(tr("REMOVE"), tab);
    connect(m_removeProofBtn, &QPushButton::clicked, this, &IdentityDialog::onRemoveProofClicked);
    proofBtnLayout->addWidget(m_removeProofBtn);
    proofBtnLayout->addStretch();
    existingLayout->addLayout(proofBtnLayout);

    layout->addWidget(existingGroup);

    // Add new proof
    auto *addGroup = new QGroupBox(tr("ADD NEW PROOF"), tab);
    auto *addLayout = new QVBoxLayout(addGroup);

    // Service selection
    auto *serviceLayout = new QHBoxLayout();
    serviceLayout->addWidget(new QLabel(tr("Service:"), tab));

    m_serviceCombo = new QComboBox(tab);
    m_serviceCombo->addItem(tr("X (Twitter)"), "x");
    m_serviceCombo->addItem(tr("Reddit"), "reddit");
    m_serviceCombo->addItem(tr("GitHub"), "github");
    m_serviceCombo->addItem(tr("Keybase"), "keybase");
    m_serviceCombo->addItem(tr("Nostr"), "nostr");
    m_serviceCombo->addItem(tr("Domain (DNS)"), "dns");
    connect(m_serviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &IdentityDialog::onProofServiceChanged);
    serviceLayout->addWidget(m_serviceCombo, 1);
    addLayout->addLayout(serviceLayout);

    // Username/identifier
    auto *identifierLayout = new QHBoxLayout();
    identifierLayout->addWidget(new QLabel(tr("Username:"), tab));

    m_identifierEdit = new QLineEdit(tab);
    m_identifierEdit->setPlaceholderText(tr("Your username on this service"));
    connect(m_identifierEdit, &QLineEdit::textChanged, this, &IdentityDialog::updateProofPreview);
    identifierLayout->addWidget(m_identifierEdit, 1);
    addLayout->addLayout(identifierLayout);

    // Proof URL
    auto *urlLayout = new QHBoxLayout();
    urlLayout->addWidget(new QLabel(tr("Proof URL:"), tab));

    m_proofUrlEdit = new QLineEdit(tab);
    m_proofUrlEdit->setPlaceholderText(tr("URL to your proof post"));
    urlLayout->addWidget(m_proofUrlEdit, 1);
    addLayout->addLayout(urlLayout);

    // Proof preview
    auto *previewLabel = new QLabel(tr("Post this to verify:"), tab);
    addLayout->addWidget(previewLabel);

    m_proofPreview = new QTextEdit(tab);
    m_proofPreview->setReadOnly(true);
    m_proofPreview->setMaximumHeight(120);
    addLayout->addWidget(m_proofPreview);

    // Action buttons
    auto *actionLayout = new QHBoxLayout();

    m_copyProofBtn = new QPushButton(tr("COPY PROOF"), tab);
    connect(m_copyProofBtn, &QPushButton::clicked, this, &IdentityDialog::onCopyProofClicked);
    actionLayout->addWidget(m_copyProofBtn);

    actionLayout->addStretch();

    m_verifyBtn = new QPushButton(tr("VERIFY"), tab);
    connect(m_verifyBtn, &QPushButton::clicked, this, &IdentityDialog::onVerifyClicked);
    actionLayout->addWidget(m_verifyBtn);

    m_addProofBtn = new QPushButton(tr("ADD PROOF"), tab);
    connect(m_addProofBtn, &QPushButton::clicked, this, &IdentityDialog::onAddProofClicked);
    actionLayout->addWidget(m_addProofBtn);

    addLayout->addLayout(actionLayout);

    layout->addWidget(addGroup);

    updateProofPreview();
}

void IdentityDialog::setupPGPTab(QWidget *tab)
{
    auto *layout = new QVBoxLayout(tab);
    layout->setSpacing(12);

    // Info
    auto *infoLabel = new QLabel(
        tr("Add your PGP public key to enable encrypted messaging and prove your identity. "
           "Your key fingerprint will be displayed publicly."),
        tab);
    infoLabel->setWordWrap(true);
    layout->addWidget(infoLabel);

    // PGP Key input
    auto *keyGroup = new QGroupBox(tr("PGP PUBLIC KEY"), tab);
    auto *keyLayout = new QVBoxLayout(keyGroup);

    m_pgpKeyEdit = new QTextEdit(tab);
    m_pgpKeyEdit->setPlaceholderText(
        tr("-----BEGIN PGP PUBLIC KEY BLOCK-----\n"
           "Paste your ASCII-armored public key here...\n"
           "-----END PGP PUBLIC KEY BLOCK-----"));
    connect(m_pgpKeyEdit, &QTextEdit::textChanged, this, &IdentityDialog::onPGPKeyChanged);
    keyLayout->addWidget(m_pgpKeyEdit);

    layout->addWidget(keyGroup);

    // Fingerprint display
    auto *fpGroup = new QGroupBox(tr("KEY INFORMATION"), tab);
    auto *fpLayout = new QVBoxLayout(fpGroup);

    auto *fpRow = new QHBoxLayout();
    fpRow->addWidget(new QLabel(tr("Fingerprint:"), tab));
    m_pgpFingerprintLabel = new QLabel(tr("No key imported"), tab);
    m_pgpFingerprintLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    fpRow->addWidget(m_pgpFingerprintLabel, 1);
    fpLayout->addLayout(fpRow);

    auto *statusRow = new QHBoxLayout();
    statusRow->addWidget(new QLabel(tr("Status:"), tab));
    m_pgpStatusLabel = new QLabel(tr("-"), tab);
    statusRow->addWidget(m_pgpStatusLabel, 1);
    fpLayout->addLayout(statusRow);

    layout->addWidget(fpGroup);

    // Buttons
    auto *btnLayout = new QHBoxLayout();

    m_importPGPBtn = new QPushButton(tr("IMPORT KEY"), tab);
    connect(m_importPGPBtn, &QPushButton::clicked, this, [this]() {
        QString keyText = m_pgpKeyEdit->toPlainText().trimmed();
        if (keyText.isEmpty()) {
            QMessageBox::warning(this, tr("Error"), tr("Please paste your PGP public key first."));
            return;
        }
        // TODO: Parse and import PGP key
        m_pgpStatusLabel->setText(tr("Key imported (verification pending)"));
        QMessageBox::information(this, tr("Success"),
            tr("PGP key imported. Verification support coming soon."));
    });
    btnLayout->addWidget(m_importPGPBtn);

    m_verifyPGPBtn = new QPushButton(tr("VERIFY KEY"), tab);
    connect(m_verifyPGPBtn, &QPushButton::clicked, this, [this]() {
        QMessageBox::information(this, tr("PGP Verification"),
            tr("To verify your PGP key:\n\n"
               "1. Sign a message containing your Coral address with your PGP key\n"
               "2. Post the signed message publicly\n"
               "3. Enter the URL to your signed proof\n\n"
               "Full PGP verification support coming soon."));
    });
    btnLayout->addWidget(m_verifyPGPBtn);

    btnLayout->addStretch();
    layout->addLayout(btnLayout);

    layout->addStretch();
}

void IdentityDialog::applyStyles()
{
    setStyleSheet(
        "QDialog {"
        "  background: #0a0a0a;"
        "}"
        "QGroupBox {"
        "  background: #1a1a1a;"
        "  border: 2px solid;"
        "  border-color: #5a5a5a #1a1a1a #1a1a1a #5a5a5a;"
        "  margin-top: 12px;"
        "  padding: 12px;"
        "  padding-top: 20px;"
        "  font-family: 'Terminus', monospace;"
        "  font-weight: bold;"
        "  color: #808080;"
        "}"
        "QGroupBox::title {"
        "  subcontrol-origin: margin;"
        "  left: 10px;"
        "  padding: 0 5px;"
        "  color: #00d4aa;"
        "  font-size: 10px;"
        "}"
        "QLabel {"
        "  color: #c0c0c0;"
        "  font-family: 'Terminus', monospace;"
        "  font-size: 12px;"
        "}"
        "QLineEdit, QTextEdit {"
        "  background: #0a0a0a;"
        "  border: 2px solid;"
        "  border-color: #1a1a1a #3a3a3a #3a3a3a #1a1a1a;"
        "  color: #33ff33;"
        "  font-family: 'Terminus', monospace;"
        "  font-size: 12px;"
        "  padding: 6px;"
        "  selection-background-color: #00d4aa;"
        "}"
        "QLineEdit:focus, QTextEdit:focus {"
        "  border-color: #00d4aa;"
        "}"
        "QPushButton {"
        "  background: qlineargradient(y1:0, y2:1, stop:0 #3a3a3a, stop:1 #2a2a2a);"
        "  border: 2px solid;"
        "  border-color: #5a5a5a #1a1a1a #1a1a1a #5a5a5a;"
        "  color: #c0c0c0;"
        "  font-family: 'Terminus', monospace;"
        "  font-weight: bold;"
        "  font-size: 11px;"
        "  padding: 8px 16px;"
        "  min-width: 80px;"
        "}"
        "QPushButton:hover {"
        "  background: qlineargradient(y1:0, y2:1, stop:0 #4a4a4a, stop:1 #3a3a3a);"
        "  border-color: #00d4aa #0a5a4a #0a5a4a #00d4aa;"
        "  color: #00d4aa;"
        "}"
        "QPushButton:pressed {"
        "  border-color: #1a1a1a #5a5a5a #5a5a5a #1a1a1a;"
        "}"
        "QTabWidget::pane {"
        "  border: 2px solid;"
        "  border-color: #5a5a5a #1a1a1a #1a1a1a #5a5a5a;"
        "  background: #1a1a1a;"
        "}"
        "QTabBar::tab {"
        "  background: #2a2a2a;"
        "  border: 2px solid;"
        "  border-color: #5a5a5a #1a1a1a #1a1a1a #5a5a5a;"
        "  color: #808080;"
        "  font-family: 'Terminus', monospace;"
        "  font-weight: bold;"
        "  font-size: 10px;"
        "  padding: 8px 20px;"
        "  margin-right: 2px;"
        "}"
        "QTabBar::tab:selected {"
        "  background: #1a1a1a;"
        "  border-bottom-color: #1a1a1a;"
        "  color: #00d4aa;"
        "}"
        "QTabBar::tab:hover:!selected {"
        "  color: #c0c0c0;"
        "}"
        "QListWidget {"
        "  background: #0a0a0a;"
        "  border: 2px solid;"
        "  border-color: #1a1a1a #3a3a3a #3a3a3a #1a1a1a;"
        "  color: #c0c0c0;"
        "  font-family: 'Terminus', monospace;"
        "  font-size: 11px;"
        "}"
        "QListWidget::item {"
        "  padding: 8px;"
        "  border-bottom: 1px solid #2a2a2a;"
        "}"
        "QListWidget::item:selected {"
        "  background: #00d4aa;"
        "  color: #0a0a0a;"
        "}"
        "QComboBox {"
        "  background: #2a2a2a;"
        "  border: 2px solid;"
        "  border-color: #5a5a5a #1a1a1a #1a1a1a #5a5a5a;"
        "  color: #c0c0c0;"
        "  font-family: 'Terminus', monospace;"
        "  font-size: 11px;"
        "  padding: 6px;"
        "}"
        "QComboBox:hover {"
        "  border-color: #00d4aa #0a5a4a #0a5a4a #00d4aa;"
        "}"
        "QComboBox::drop-down {"
        "  border: none;"
        "  width: 20px;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background: #1a1a1a;"
        "  border: 1px solid #3a3a3a;"
        "  color: #c0c0c0;"
        "  selection-background-color: #00d4aa;"
        "  selection-color: #0a0a0a;"
        "}"
    );
}

void IdentityDialog::onAddProofClicked()
{
    QString identifier = m_identifierEdit->text().trimmed();
    QString proofUrl = m_proofUrlEdit->text().trimmed();
    QString service = m_serviceCombo->currentData().toString();

    if (identifier.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Please enter your username for this service."));
        return;
    }

    if (proofUrl.isEmpty()) {
        QMessageBox::warning(this, tr("Error"),
            tr("Please enter the URL to your proof post.\n\n"
               "First, copy the proof text and post it on %1, then enter the URL here.")
               .arg(m_serviceCombo->currentText()));
        return;
    }

    // Add to list (verification would happen on save)
    QString displayText = QString("%1: @%2").arg(m_serviceCombo->currentText(), identifier);
    auto *item = new QListWidgetItem(displayText, m_proofsList);
    item->setData(Qt::UserRole, service);
    item->setData(Qt::UserRole + 1, identifier);
    item->setData(Qt::UserRole + 2, proofUrl);

    // Clear inputs
    m_identifierEdit->clear();
    m_proofUrlEdit->clear();

    QMessageBox::information(this, tr("Proof Added"),
        tr("Identity proof added. Click SAVE to finalize."));
}

void IdentityDialog::onRemoveProofClicked()
{
    auto *item = m_proofsList->currentItem();
    if (item) {
        delete item;
    }
}

void IdentityDialog::onVerifyClicked()
{
    QString proofUrl = m_proofUrlEdit->text().trimmed();
    if (proofUrl.isEmpty()) {
        QMessageBox::warning(this, tr("Error"), tr("Please enter a proof URL to verify."));
        return;
    }

    // TODO: Actually fetch and verify the proof
    QMessageBox::information(this, tr("Verification"),
        tr("Proof verification will check that your proof post:\n\n"
           "1. Contains your Coral address\n"
           "2. Contains a valid signature\n"
           "3. Was posted by the account you claim\n\n"
           "Automatic verification coming soon."));
}

void IdentityDialog::onCopyProofClicked()
{
    QString proof = generateProofText();
    QApplication::clipboard()->setText(proof);
    QMessageBox::information(this, tr("Copied"),
        tr("Proof text copied to clipboard.\n\nPost this on %1 to verify your identity.")
           .arg(m_serviceCombo->currentText()));
}

void IdentityDialog::onSaveClicked()
{
    // TODO: Save to IdentityManager
    Q_EMIT identityUpdated();
    accept();
}

void IdentityDialog::onProofServiceChanged(int index)
{
    Q_UNUSED(index);
    updateProofPreview();

    // Update placeholder based on service
    QString service = m_serviceCombo->currentData().toString();
    if (service == "x") {
        m_identifierEdit->setPlaceholderText(tr("Your X handle (without @)"));
        m_proofUrlEdit->setPlaceholderText(tr("https://x.com/yourhandle/status/..."));
    } else if (service == "reddit") {
        m_identifierEdit->setPlaceholderText(tr("Your Reddit username"));
        m_proofUrlEdit->setPlaceholderText(tr("https://reddit.com/user/.../comments/..."));
    } else if (service == "github") {
        m_identifierEdit->setPlaceholderText(tr("Your GitHub username"));
        m_proofUrlEdit->setPlaceholderText(tr("https://gist.github.com/yourusername/..."));
    } else if (service == "keybase") {
        m_identifierEdit->setPlaceholderText(tr("Your Keybase username"));
        m_proofUrlEdit->setPlaceholderText(tr("https://keybase.io/yourusername"));
    } else if (service == "nostr") {
        m_identifierEdit->setPlaceholderText(tr("Your npub or NIP-05 identifier"));
        m_proofUrlEdit->setPlaceholderText(tr("nostr:npub... or note URL"));
    } else if (service == "dns") {
        m_identifierEdit->setPlaceholderText(tr("Your domain (e.g., example.com)"));
        m_proofUrlEdit->setPlaceholderText(tr("Not needed for DNS - add TXT record"));
    }
}

void IdentityDialog::onPGPKeyChanged()
{
    QString keyText = m_pgpKeyEdit->toPlainText().trimmed();

    if (keyText.isEmpty()) {
        m_pgpFingerprintLabel->setText(tr("No key imported"));
        m_pgpStatusLabel->setText(tr("-"));
        return;
    }

    // Basic validation
    if (keyText.contains("-----BEGIN PGP PUBLIC KEY BLOCK-----")) {
        m_pgpStatusLabel->setText(tr("Key detected - click IMPORT KEY to add"));
        // TODO: Parse fingerprint from key
        m_pgpFingerprintLabel->setText(tr("(Import to see fingerprint)"));
    } else {
        m_pgpStatusLabel->setText(tr("Invalid key format"));
        m_pgpFingerprintLabel->setText(tr("No key imported"));
    }
}

void IdentityDialog::refreshProofsList()
{
    // TODO: Load from IdentityManager
    m_proofsList->clear();
}

void IdentityDialog::updateProofPreview()
{
    m_proofPreview->setText(generateProofText());
}

QString IdentityDialog::generateProofText()
{
    QString service = m_serviceCombo->currentData().toString();
    QString identifier = m_identifierEdit->text().trimmed();
    QString address = m_address;

    if (identifier.isEmpty()) {
        identifier = "[your_username]";
    }
    if (address.isEmpty()) {
        address = "[your_coral_address]";
    }

    // Generate proof message
    QString message = QString(
        "Verifying my Coral Network identity:\n\n"
        "Address: %1\n"
        "Service: %2\n"
        "Username: %3\n\n"
        "This is a cryptographic proof that I control this Coral address."
    ).arg(address, service, identifier);

    // Add signature placeholder
    // TODO: Actually sign with wallet key
    message += "\n\nSignature: [signature_will_be_generated]";

    return message;
}
