// Copyright (c) 2017 The Karbowanec developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "MnemonicSeedDialog.h"
#include "MnemonicSeedLanguage.h"
#include "ui_mnemonicseeddialog.h"
#include "CurrencyAdapter.h"
#include "WalletAdapter.h"
#include "Settings.h"

#include <QComboBox>

namespace WalletGui {

MnemonicSeedDialog::MnemonicSeedDialog(QWidget* _parent) : QDialog(_parent), m_ui(new Ui::MnemonicSeedDialog) {
  m_ui->setupUi(this);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletInitCompletedSignal, this, &MnemonicSeedDialog::walletOpened, Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletCloseCompletedSignal, this, &MnemonicSeedDialog::walletClosed, Qt::QueuedConnection);
  connect(m_ui->m_languageCombo, &QComboBox::currentTextChanged, this, &MnemonicSeedDialog::languageChanged);
  initLanguages();
  if (WalletAdapter::instance().isOpen()) {
    languageChanged();
  }
}

MnemonicSeedDialog::~MnemonicSeedDialog() {
}

void MnemonicSeedDialog::walletOpened() {
  languageChanged();
}

void MnemonicSeedDialog::walletClosed() {
  m_ui->m_mnemonicSeedEdit->clear();
}

void MnemonicSeedDialog::initLanguages() {
  MnemonicSeedLanguage::initLanguageCombo(m_ui->m_languageCombo, Settings::instance().getLanguage());
}

void MnemonicSeedDialog::languageChanged() {
  const QString languageName = m_ui->m_languageCombo->currentText();
  if (languageName.isEmpty()) {
    m_ui->m_mnemonicSeedEdit->clear();
    return;
  }

  QString mnemonicSeed = WalletAdapter::instance().getMnemonicSeed(languageName);
  m_ui->m_mnemonicSeedEdit->setText(mnemonicSeed);
}

}
