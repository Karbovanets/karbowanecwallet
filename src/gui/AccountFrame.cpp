// Copyright (c) 2011-2015 The Cryptonote developers
// Copyright (c) 2016-2026 The Karbo developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <QClipboard>
#include <QEvent>
#include <QKeyEvent>
#include <QKeySequence>
#include <QTimer>
#include <QFontDatabase>
#include <QMessageBox>
#include <QRegularExpression>
#include <future>
#include "AccountFrame.h"
#include "WalletAdapter.h"
#include "NodeAdapter.h"
#include "CurrencyAdapter.h"
#include "Settings.h"
#include "QRCodeDialog.h"

#include "ui_accountframe.h"

namespace WalletGui {

namespace {

constexpr int CAPTION_FONT_SIZE = 10;
constexpr int ADDRESS_FONT_SIZE = 16;
constexpr int ACCOUNT_NUMBER_VALUE_FONT_SIZE = 26;
constexpr int ADDRESS_CHUNK_SIZE = 13;
constexpr int ADDRESS_CHUNKS_PER_ROW = 4;

QString stripVisualAddressSeparators(QString text) {
  text.remove(QRegularExpression(QStringLiteral("[\\s\\x{00A0}\\x{2028}\\x{2029}]")));
  return text;
}

QString formatDisplayAddress(const QString& address) {
  if (address.isEmpty()) {
    return QString();
  }

  QStringList rows;
  QStringList rowChunks;

  for (int i = 0; i < address.size(); i += ADDRESS_CHUNK_SIZE) {
    rowChunks << QString("<span>%1</span>").arg(address.mid(i, ADDRESS_CHUNK_SIZE).toHtmlEscaped());
    if (rowChunks.size() == ADDRESS_CHUNKS_PER_ROW) {
      rows << rowChunks.join(QStringLiteral("&nbsp;&nbsp;"));
      rowChunks.clear();
    }
  }

  if (!rowChunks.isEmpty()) {
    rows << rowChunks.join(QStringLiteral("&nbsp;&nbsp;"));
  }

  return QString("<div style=\"line-height:1.22;\">%1</div>").arg(rows.join(QStringLiteral("<br/>")));
}

QString formatBalanceLabel(const QString& title, const QStringList& amountParts, const QString& ticker, int majorSize, int minorSize) {
  return QString(
    "<div style=\"line-height:1.0;\">"
      "<span style=\"font-size:%1px;\">%2</span>"
      "<span style=\"font-size:%1px;\">: </span>"
      "<span style=\"font-size:%3px; font-weight:600;\">%4</span>"
      "<span style=\"font-size:%5px;\"> %6 %7</span>"
    "</div>")
    .arg(CAPTION_FONT_SIZE)
    .arg(title.toHtmlEscaped())
    .arg(majorSize)
    .arg(amountParts.first().toHtmlEscaped())
    .arg(minorSize)
    .arg(amountParts.last().toHtmlEscaped())
    .arg(ticker.toHtmlEscaped());
}

}

QStringList AccountFrame::divideAmount(quint64 _val) {
  QStringList list;
  QString str = CurrencyAdapter::instance().formatAmount(_val).remove(',');

  quint32 offset = str.indexOf(".") + 3; // add two digits .00
  QString before = str.left(offset);
  QString after  = str.mid(offset);

  list << before << after;

  return list;
}

AccountFrame::AccountFrame(QWidget* _parent) : QFrame(_parent), m_ui(new Ui::AccountFrame),
  m_accountNumberResolved(false), m_accountNumberFetchInProgress(false) {
  m_ui->setupUi(this);
  connect(&WalletAdapter::instance(), &WalletAdapter::updateWalletAddressSignal, this, &AccountFrame::updateWalletAddress);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletActualBalanceUpdatedSignal, this, &AccountFrame::updateActualBalance,
    Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletPendingBalanceUpdatedSignal, this, &AccountFrame::updatePendingBalance,
    Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletUnmixableBalanceUpdatedSignal, this, &AccountFrame::updateUnmixableBalance,
    Qt::QueuedConnection);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletCloseCompletedSignal, this, &AccountFrame::reset);
  connect(&WalletAdapter::instance(), &WalletAdapter::walletSynchronizationCompletedSignal, this, [this](int _error, const QString&) {
    if (_error != 0 || !WalletAdapter::instance().isOpen() || m_accountNumberResolved) {
      return;
    }

    fetchAccountNumber(WalletAdapter::instance().getAddress());
  });

  // Style the account frame with a slightly brighter background
  applyFramePalette();

  m_ui->m_unmixableBalanceLabel->setVisible(false);
  m_ui->m_accountNumberLabel->setVisible(false);
  m_ui->m_copyAccountNumberButton->setVisible(false);
  m_ui->m_registerAccountButton->setVisible(false);

  QFont addressFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
  addressFont.setPixelSize(ADDRESS_FONT_SIZE);
  addressFont.setWeight(QFont::Bold);

  QFont accountNumberFont = QFontDatabase::systemFont(QFontDatabase::FixedFont);
  accountNumberFont.setPixelSize(ACCOUNT_NUMBER_VALUE_FONT_SIZE);
  accountNumberFont.setBold(true);

  m_ui->m_addressLabel->setFont(addressFont);
  m_ui->m_addressLabel->setWordWrap(true);
  m_ui->m_addressLabel->setTextFormat(Qt::RichText);
  m_ui->m_addressLabel->installEventFilter(this);
  m_ui->m_accountNumberLabel->setFont(accountNumberFont);
  m_ui->m_accountNumberLabel->setTextFormat(Qt::PlainText);
  m_ui->m_copyButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
  m_ui->m_qrButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
  m_ui->m_copyAccountNumberButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
  m_ui->m_copyButton->setIconSize(QSize(16, 16));
  m_ui->m_qrButton->setIconSize(QSize(16, 16));
  m_ui->m_copyAccountNumberButton->setIconSize(QSize(16, 16));
  m_ui->m_copyButton->setText(QString());
  m_ui->m_qrButton->setText(QString());
  m_ui->m_copyAccountNumberButton->setText(QString());
  m_ui->m_copyButton->setFocusPolicy(Qt::NoFocus);
  m_ui->m_qrButton->setFocusPolicy(Qt::NoFocus);
  m_ui->m_copyAccountNumberButton->setFocusPolicy(Qt::NoFocus);
}

AccountFrame::~AccountFrame() {
}

void AccountFrame::changeEvent(QEvent* _event) {
  QFrame::changeEvent(_event);
  if (_event->type() == QEvent::PaletteChange || _event->type() == QEvent::StyleChange) {
    applyFramePalette();
  }
}

void AccountFrame::applyFramePalette() {
  const QColor borderColor = palette().color(QPalette::Mid);

  // Account number panel — border only
  m_ui->m_accountNumberPanel->setStyleSheet(
    QString("QFrame#m_accountNumberPanel { border: 2px solid %1; border-radius: 8px; }")
    .arg(borderColor.name()));
}

bool AccountFrame::eventFilter(QObject* _object, QEvent* _event) {
  if (_object == m_ui->m_addressLabel && _event->type() == QEvent::KeyPress) {
    const auto* keyEvent = static_cast<QKeyEvent*>(_event);
    if (keyEvent->matches(QKeySequence::Copy) && m_ui->m_addressLabel->hasSelectedText()) {
      QApplication::clipboard()->setText(stripVisualAddressSeparators(m_ui->m_addressLabel->selectedText()));
      return true;
    }
  }

  return QFrame::eventFilter(_object, _event);
}

void AccountFrame::updateWalletAddress(const QString& _address) {
  m_ui->m_addressLabel->setText(formatDisplayAddress(_address));
  m_ui->m_addressLabel->setToolTip(_address);
  m_accountNumber.clear();
  m_accountNumberResolved = false;
  m_accountNumberFetchInProgress = false;
  updateAccountNumberDisplay();
  fetchAccountNumber(_address);
}

void AccountFrame::copyAddress() {
  QApplication::clipboard()->setText(WalletAdapter::instance().getAddress());
}

void AccountFrame::showQR() {
  const QString address = WalletAdapter::instance().getAddress();
  if (address.isEmpty()) {
    return;
  }

  QRCodeDialog dlg(tr("QR Code"), address, this);
  dlg.exec();
}

void AccountFrame::updateActualBalance(quint64 _balance) {
  QStringList actualList = divideAmount(_balance);
  const QString ticker = CurrencyAdapter::instance().getCurrencyTicker().toUpper();
  m_ui->m_actualBalanceLabel->setText(formatBalanceLabel(tr("Available"), actualList, ticker, 18, 10));

  quint64 pendingBalance = WalletAdapter::instance().getPendingBalance();

  QStringList pendingList = divideAmount(_balance + pendingBalance);
  m_ui->m_totalBalanceLabel->setText(formatBalanceLabel(tr("Total"), pendingList, ticker, 20, 10));
}

void AccountFrame::updatePendingBalance(quint64 _balance) {
  QStringList pendingList = divideAmount(_balance);
  const QString ticker = CurrencyAdapter::instance().getCurrencyTicker().toUpper();
  m_ui->m_pendingBalanceLabel->setText(formatBalanceLabel(tr("Pending"), pendingList, ticker, 18, 10));

  quint64 actualBalance = WalletAdapter::instance().getActualBalance();

  QStringList totalList = divideAmount(_balance + actualBalance);
  m_ui->m_totalBalanceLabel->setText(formatBalanceLabel(tr("Total"), totalList, ticker, 20, 10));
}

void AccountFrame::updateUnmixableBalance(quint64 _balance) {
  QStringList unmixableList = divideAmount(_balance);
  const QString ticker = CurrencyAdapter::instance().getCurrencyTicker().toUpper();

  m_ui->m_unmixableBalanceLabel->setText(formatBalanceLabel(tr("Unmixable"), unmixableList, ticker, 18, 10));
  if (_balance != 0) {
    m_ui->m_unmixableBalanceLabel->setVisible(true);
  } else {
    m_ui->m_unmixableBalanceLabel->setVisible(false);
  }
}

void AccountFrame::fetchAccountNumber(const QString& _address) {
  if (_address.isEmpty() || m_accountNumberFetchInProgress) {
    return;
  }

  m_accountNumberFetchInProgress = true;
  const QString requestedAddress = _address;
  std::string address = _address.toStdString();
  std::string* accountNumber = new std::string();

  NodeAdapter::instance().getAccountNumber(address, *accountNumber,
    [this, accountNumber, requestedAddress](std::error_code ec) {
      QString result;
      const bool hasResult = !ec;
      if (hasResult && !accountNumber->empty()) {
        result = QString::fromStdString(*accountNumber);
      }
      delete accountNumber;

      QMetaObject::invokeMethod(this, [this, result, hasResult, requestedAddress]() {
        if (WalletAdapter::instance().getAddress() != requestedAddress) {
          m_accountNumberFetchInProgress = false;
          return;
        }

      // In case Node lookups can transiently fail keep the current display and retry on next
      // synchronization completion instead of clearing it.
      if (!hasResult) {
        m_accountNumberFetchInProgress = false;
        return;
      }

      if (result.isEmpty()) {
        m_accountNumberResolved = true;
        m_accountNumber.clear();
        m_accountNumberFetchInProgress = false;
        updateAccountNumberDisplay();
        return;
      }

      m_accountNumberResolved = true;
      m_accountNumber = result;
      m_accountNumberFetchInProgress = false;
      updateAccountNumberDisplay();

    }, Qt::QueuedConnection);
  });
}

void AccountFrame::updateAccountNumberDisplay() {
  if (m_accountNumber.isEmpty()) {
    const bool canRegister = WalletAdapter::instance().isOpen() && !Settings::instance().isTrackingMode();
    m_ui->m_accountNumberLabel->clear();
    m_ui->m_accountNumberLabel->setVisible(!canRegister);
    if (!canRegister) {
      m_ui->m_accountNumberLabel->setText(tr("Not registered"));
      m_ui->m_accountNumberLabel->setToolTip(tr("This wallet does not have a registered account number."));
    }
    m_ui->m_copyAccountNumberButton->setVisible(false);
    m_ui->m_registerAccountButton->setVisible(canRegister);
  } else {
    m_ui->m_accountNumberLabel->setText(m_accountNumber);
    m_ui->m_accountNumberLabel->setToolTip(m_accountNumber);
    m_ui->m_accountNumberLabel->setVisible(true);
    m_ui->m_copyAccountNumberButton->setVisible(true);
    m_ui->m_registerAccountButton->setVisible(false);
  }
}

void AccountFrame::copyAccountNumber() {
  if (!m_accountNumber.isEmpty()) {
    QApplication::clipboard()->setText(m_accountNumber);
  }
}

void AccountFrame::registerAccountNumber() {
  if (!WalletAdapter::instance().isOpen()) {
    return;
  }

  if (Settings::instance().isTrackingMode()) {
    QMessageBox::critical(this, tr("Error"), tr("Cannot register account number from a tracking wallet."));
    return;
  }

  if (QMessageBox::question(this, tr("Register Account Number"),
      tr("Register an account number for easy payments?\nA small fee will be charged."),
      QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
    WalletAdapter::instance().registerAccountNumber();
  }
}

void AccountFrame::reset() {
  updateActualBalance(0);
  updatePendingBalance(0);
  updateUnmixableBalance(0);
  m_ui->m_addressLabel->clear();
  m_ui->m_addressLabel->setToolTip(tr("Your receiving address"));
  m_accountNumber.clear();
  m_ui->m_accountNumberLabel->clear();
  m_ui->m_accountNumberLabel->setVisible(false);
  m_ui->m_copyAccountNumberButton->setVisible(false);
  m_ui->m_registerAccountButton->setVisible(false);
}

}
