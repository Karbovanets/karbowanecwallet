// Copyright (c) 2017 The Karbowanec developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "VerifyMnemonicSeedDialog.h"
#include "MnemonicSeedLanguage.h"
#include "ui_verifymnemonicseeddialog.h"
#include "CurrencyAdapter.h"
#include "WalletAdapter.h"
#include "Settings.h"

#include <QComboBox>

namespace WalletGui {

VerifyMnemonicSeedDialog::VerifyMnemonicSeedDialog(QWidget* _parent) : QDialog(_parent), m_ui(new Ui::VerifyMnemonicSeedDialog) {
  setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::CustomizeWindowHint);
  m_ui->setupUi(this);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletCloseCompletedSignal, this, &VerifyMnemonicSeedDialog::walletClosed, Qt::QueuedConnection);
  connect(m_ui->m_languageCombo, &QComboBox::currentTextChanged, this, &VerifyMnemonicSeedDialog::languageChanged);
  initLanguages();
  languageChanged();
}

VerifyMnemonicSeedDialog::~VerifyMnemonicSeedDialog() {
}

void VerifyMnemonicSeedDialog::walletClosed() {
  m_ui->m_seedEdit->clear();
  m_ui->m_seedRepeat->clear();
}

void VerifyMnemonicSeedDialog::onTextChanged() {
  if (QString::compare(m_ui->m_seedEdit->toPlainText().trimmed(), m_ui->m_seedRepeat->toPlainText().trimmed(), Qt::CaseInsensitive) == 0) {
    m_ui->m_okButton->setEnabled(true);
    m_seedsMatch = true;
  } else {
    m_ui->m_okButton->setEnabled(false);
    m_seedsMatch = false;
  }
}

void VerifyMnemonicSeedDialog::reject() {
  if(m_seedsMatch == true) done(0);
}

void VerifyMnemonicSeedDialog::initLanguages() {
  MnemonicSeedLanguage::initLanguageCombo(m_ui->m_languageCombo, Settings::instance().getLanguage());
}

void VerifyMnemonicSeedDialog::languageChanged() {
  const QString languageName = m_ui->m_languageCombo->currentText();
  if (languageName.isEmpty()) {
    m_ui->m_seedEdit->clear();
    m_ui->m_okButton->setEnabled(false);
    m_seedsMatch = false;
    return;
  }

  QString mnemonicSeed = WalletAdapter::instance().getMnemonicSeed(languageName);
  m_ui->m_seedEdit->setText(mnemonicSeed);
  if (QString::compare(m_ui->m_seedEdit->toPlainText().trimmed(), m_ui->m_seedRepeat->toPlainText().trimmed(), Qt::CaseInsensitive) == 0) {
    m_ui->m_okButton->setEnabled(true);
    m_seedsMatch = true;
  } else {
    m_ui->m_okButton->setEnabled(false);
    m_seedsMatch = false;
  }
}

}
