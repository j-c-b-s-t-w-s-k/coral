// Copyright (c) 2011-2020 The Coral Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef CORAL_QT_CORALADDRESSVALIDATOR_H
#define CORAL_QT_CORALADDRESSVALIDATOR_H

#include <QValidator>

/** Base58 entry widget validator, checks for valid characters and
 * removes some whitespace.
 */
class CoralAddressEntryValidator : public QValidator
{
    Q_OBJECT

public:
    explicit CoralAddressEntryValidator(QObject *parent);

    State validate(QString &input, int &pos) const override;
};

/** Coral address widget validator, checks for a valid coral address.
 */
class CoralAddressCheckValidator : public QValidator
{
    Q_OBJECT

public:
    explicit CoralAddressCheckValidator(QObject *parent);

    State validate(QString &input, int &pos) const override;
};

#endif // CORAL_QT_CORALADDRESSVALIDATOR_H
