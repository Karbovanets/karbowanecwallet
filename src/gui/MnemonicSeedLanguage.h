// Copyright (c) 2026 The Karbowanec developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <QComboBox>
#include <QSignalBlocker>
#include <QString>

#include <vector>

#include "Mnemonics/electrum-words.h"

namespace WalletGui {
namespace MnemonicSeedLanguage {

inline QString defaultLanguageName() {
  return QString::fromUtf8("English");
}

inline QString languageNameForLocale(const QString& locale) {
  if (locale == QLatin1String("en")) {
    return QString::fromUtf8("English");
  } else if (locale == QLatin1String("nl")) {
    return QString::fromUtf8("Nederlands");
  } else if (locale == QLatin1String("fr")) {
    return QString::fromUtf8("Français");
  } else if (locale == QLatin1String("es")) {
    return QString::fromUtf8("Español");
  } else if (locale == QLatin1String("pt")) {
    return QString::fromUtf8("Português");
  } else if (locale == QLatin1String("ja") || locale == QLatin1String("jp")) {
    return QString::fromUtf8("日本語");
  } else if (locale == QLatin1String("it")) {
    return QString::fromUtf8("Italiano");
  } else if (locale == QLatin1String("de")) {
    return QString::fromUtf8("Deutsch");
  } else if (locale == QLatin1String("ru")) {
    return QString::fromUtf8("русский язык");
  } else if (locale == QLatin1String("zh") || locale == QLatin1String("cn")) {
    return QString::fromUtf8("简体中文 (中国)");
  } else if (locale == QLatin1String("uk")) {
    return QString::fromUtf8("українська мова");
  } else if (locale == QLatin1String("pl")) {
    return QString::fromUtf8("język polski");
  } else if (locale == QLatin1String("be")) {
    return QString::fromUtf8("русский язык");
  }

  return QString();
}

inline void initLanguageCombo(QComboBox* combo, const QString& locale) {
  QSignalBlocker blocker(combo);

  combo->clear();

  std::vector<std::string> languages;
  Crypto::ElectrumWords::get_language_list(languages);
  for (const auto& language : languages) {
    combo->addItem(QString::fromStdString(language));
  }

  int languageIndex = combo->findText(languageNameForLocale(locale));
  if (languageIndex < 0) {
    languageIndex = combo->findText(defaultLanguageName());
  }
  if (languageIndex < 0 && combo->count() > 0) {
    languageIndex = 0;
  }
  if (languageIndex >= 0) {
    combo->setCurrentIndex(languageIndex);
  }
}

} // namespace MnemonicSeedLanguage
} // namespace WalletGui
