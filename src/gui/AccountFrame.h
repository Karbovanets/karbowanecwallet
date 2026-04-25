// Copyright (c) 2011-2015 The Cryptonote developers
// Copyright (c) 2016-2020 The Karbo developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QFrame>

class QEvent;

namespace Ui {
class AccountFrame;
}

namespace WalletGui {

class AccountFrame : public QFrame {
  Q_OBJECT
  Q_DISABLE_COPY(AccountFrame)

public:
  AccountFrame(QWidget* _parent);
  ~AccountFrame();

protected:
  bool eventFilter(QObject* _object, QEvent* _event) override;
  void changeEvent(QEvent* _event) override;

private:
  QScopedPointer<Ui::AccountFrame> m_ui;
  QString m_accountNumber;
  bool m_accountNumberResolved;
  bool m_accountNumberFetchInProgress;
  // Set to true the moment the user confirms a registration. Used to hide
  // the Register button immediately so a single click can't be repeated
  // while the registration tx is still unconfirmed (otherwise users send
  // multiple duplicate registration txs in the same block, and only the
  // first one is honored by consensus). Reset on wallet close, on address
  // change, and once a real account number arrives from the daemon.
  bool m_registrationPending;

  void applyFramePalette();
  void updateWalletAddress(const QString& _address);
  void updateActualBalance(quint64 _balance);
  void updatePendingBalance(quint64 _balance);
  void updateUnmixableBalance(quint64 _balance);
  void reset();
  void fetchAccountNumber(const QString& _address);
  void updateAccountNumberDisplay();

  QStringList divideAmount(quint64 _val);

  Q_SLOT void copyAddress();
  Q_SLOT void showQR();
  Q_SLOT void copyAccountNumber();
  Q_SLOT void registerAccountNumber();

Q_SIGNALS:
  void showQRcodeSignal();

};

}
