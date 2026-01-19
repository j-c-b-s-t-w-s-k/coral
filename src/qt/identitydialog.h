// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CORAL_QT_IDENTITYDIALOG_H
#define CORAL_QT_IDENTITYDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QTabWidget>
#include <QListWidget>
#include <QGroupBox>
#include <QComboBox>

#include <identity/useridentity.h>

class WalletModel;

/**
 * Dialog for managing user identity and proofs (PGP, X, Reddit, etc.)
 * Similar to Keybase identity verification system.
 */
class IdentityDialog : public QDialog
{
    Q_OBJECT

public:
    explicit IdentityDialog(QWidget *parent = nullptr);
    ~IdentityDialog() = default;

    void setWalletModel(WalletModel *model);
    void setAddress(const QString &address);

Q_SIGNALS:
    void identityUpdated();

private Q_SLOTS:
    void onAddProofClicked();
    void onRemoveProofClicked();
    void onVerifyClicked();
    void onCopyProofClicked();
    void onSaveClicked();
    void onProofServiceChanged(int index);
    void onPGPKeyChanged();
    void refreshProofsList();

private:
    void setupUI();
    void setupProfileTab(QWidget *tab);
    void setupProofsTab(QWidget *tab);
    void setupPGPTab(QWidget *tab);
    void updateProofPreview();
    QString generateProofText();
    void applyStyles();

    WalletModel *m_walletModel{nullptr};
    QString m_address;

    // Profile tab
    QLineEdit *m_displayNameEdit{nullptr};
    QTextEdit *m_bioEdit{nullptr};
    QLabel *m_addressLabel{nullptr};
    QLabel *m_trustScoreLabel{nullptr};

    // Proofs tab
    QListWidget *m_proofsList{nullptr};
    QComboBox *m_serviceCombo{nullptr};
    QLineEdit *m_identifierEdit{nullptr};
    QLineEdit *m_proofUrlEdit{nullptr};
    QTextEdit *m_proofPreview{nullptr};
    QPushButton *m_addProofBtn{nullptr};
    QPushButton *m_removeProofBtn{nullptr};
    QPushButton *m_verifyBtn{nullptr};
    QPushButton *m_copyProofBtn{nullptr};

    // PGP tab
    QTextEdit *m_pgpKeyEdit{nullptr};
    QLabel *m_pgpFingerprintLabel{nullptr};
    QLabel *m_pgpStatusLabel{nullptr};
    QPushButton *m_importPGPBtn{nullptr};
    QPushButton *m_verifyPGPBtn{nullptr};

    // Main buttons
    QPushButton *m_saveBtn{nullptr};
    QPushButton *m_cancelBtn{nullptr};

    // Tab widget
    QTabWidget *m_tabWidget{nullptr};
};

#endif // CORAL_QT_IDENTITYDIALOG_H
