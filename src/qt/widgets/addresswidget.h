// Copyright (c) 2024 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CORAL_QT_WIDGETS_ADDRESSWIDGET_H
#define CORAL_QT_WIDGETS_ADDRESSWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>

/**
 * Atomic address display widget with copy functionality.
 * Shows truncated address with full address in tooltip.
 */
class AddressWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(QString address READ address WRITE setAddress NOTIFY addressChanged)
    Q_PROPERTY(QString label READ label WRITE setLabel)
    Q_PROPERTY(bool showCopyButton READ showCopyButton WRITE setShowCopyButton)
    Q_PROPERTY(int truncateLength READ truncateLength WRITE setTruncateLength)

public:
    explicit AddressWidget(QWidget *parent = nullptr);
    explicit AddressWidget(const QString &address, QWidget *parent = nullptr);
    ~AddressWidget() = default;

    QString address() const { return m_address; }
    void setAddress(const QString &address);

    QString label() const { return m_label; }
    void setLabel(const QString &label);

    bool showCopyButton() const { return m_showCopyButton; }
    void setShowCopyButton(bool show);

    int truncateLength() const { return m_truncateLength; }
    void setTruncateLength(int length);

    void setValid(bool valid);

Q_SIGNALS:
    void addressChanged(const QString &address);
    void copyClicked(const QString &address);
    void clicked(const QString &address);

protected:
    void mousePressEvent(QMouseEvent *event) override;

private Q_SLOTS:
    void onCopyClicked();

private:
    void setupUI();
    void updateDisplay();
    QString truncatedAddress() const;

    QString m_address;
    QString m_label;
    bool m_showCopyButton{true};
    int m_truncateLength{16};
    bool m_valid{true};

    QLabel *m_addressLabel{nullptr};
    QLabel *m_labelLabel{nullptr};
    QPushButton *m_copyButton{nullptr};
    QLabel *m_validIcon{nullptr};
};

#endif // CORAL_QT_WIDGETS_ADDRESSWIDGET_H
