// Copyright (c) 2015-2017, The Bytecoin developers
//
// This file is part of Bytecoin.
//
// Karbovanets is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Karbovanets is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with Karbovanets.  If not, see <http://www.gnu.org/licenses/>.

#include <algorithm>
#include <QApplication>
#include <QPainter>
#include <QScreen>
#include "CurrencyAdapter.h"
#include "SendGlassFrame.h"

namespace WalletGui {

namespace {

const quint32 MAX_QUINT32 = std::numeric_limits<quint32>::max();

QColor withAlpha(QColor color, int alpha) {
  color.setAlpha(alpha);
  return color;
}

}

SendGlassFrame::SendGlassFrame(QWidget* _parent) : GlassFrame(_parent), m_currentHeight(MAX_QUINT32), m_totalHeight(MAX_QUINT32),
  m_pixmapBuffer(QSize(340, 340) * QApplication::primaryScreen()->devicePixelRatio()) {
}

SendGlassFrame::~SendGlassFrame() {
}

void SendGlassFrame::paintEvent(QPaintEvent* _event) {
  paintInBuffer();
  GlassFrame::paintEvent(_event);
  QRect image_rect(m_pixmapBuffer.rect());
  int left = rect().left() + (rect().width() - m_pixmapBuffer.width()) / 2;
  int top = rect().top() + (rect().height() - m_pixmapBuffer.height()) / 2;
  image_rect.moveTopLeft(QPoint(left, top));
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);
  painter.fillRect(rect(), withAlpha(palette().color(QPalette::Window), 192));
  painter.drawPixmap(image_rect, m_pixmapBuffer);
}

void SendGlassFrame::updateSynchronizationState(quint32 _current, quint32 _total) {
  if (m_currentHeight != _current || m_totalHeight != _total) {
    m_currentHeight = _current;
    m_totalHeight = _total;
    paintInBuffer();
    update();
  }
}

void SendGlassFrame::paintInBuffer() {
  m_pixmapBuffer.fill(Qt::transparent);

  QPainter painter(&m_pixmapBuffer);
  painter.setRenderHint(QPainter::Antialiasing);

  drawProgressGraph(painter);
  drawProgressLabel(painter);
  drawProgressValue(painter);
}

void SendGlassFrame::drawProgressGraph(QPainter &_painter) {
  const int degree = 16;
  const int startAngle = 90 * degree;
  const qreal progress = m_totalHeight == 0 || m_totalHeight == MAX_QUINT32 ? 0.0 :
    std::min<qreal>(1.0, static_cast<qreal>(m_currentHeight) / static_cast<qreal>(m_totalHeight));
  const int endAngle = -(360 * degree) * progress;
  QRect rect = m_pixmapBuffer.rect().marginsRemoved(QMargins(5, 5, 5, 5));

  QPen pen;
  pen.setWidth(10);

  pen.setColor(withAlpha(palette().color(QPalette::Mid), 140));
  _painter.setPen(pen);
  _painter.drawArc(rect, 0, 360 * degree);

  pen.setColor(palette().color(QPalette::Highlight));
  _painter.setPen(pen);
  _painter.drawArc(rect, startAngle, endAngle);
}

void SendGlassFrame::drawProgressLabel(QPainter &_painter) {
  QFont font;
  font.setBold(true);
  font.setPixelSize(14);
  QString msg = tr("You will be able to send %1\nwhen the wallet is synchronized").arg(CurrencyAdapter::instance().getCurrencyTicker().toUpper());
  QFontMetrics fm(font);
  QRect messageRect = fm.boundingRect(QRect(), Qt::AlignCenter, msg);
  messageRect.moveCenter(QPoint(m_pixmapBuffer.width() / 2, 163));

  QPen pen;
  pen.setColor(palette().color(QPalette::WindowText));
  _painter.setPen(pen);
  _painter.setFont(font);
  _painter.setRenderHint(QPainter::TextAntialiasing);
  _painter.drawText(messageRect, Qt::AlignCenter, msg);
}

void SendGlassFrame::drawProgressValue(QPainter &_painter) {
  QFont font;
  font.setBold(false);
  font.setPixelSize(10);
  QString msg = QString("%1 %2/%3").arg(tr("Synchronizing")).arg(m_currentHeight).arg(m_totalHeight);
  QFontMetrics fm(font);
  QRect messageRect = fm.boundingRect(QRect(), Qt::AlignCenter, msg);
  messageRect.moveCenter(QPoint(m_pixmapBuffer.width() / 2, 198));

  QPen pen;
  pen.setColor(withAlpha(palette().color(QPalette::WindowText), 180));
  _painter.setPen(pen);
  _painter.setFont(font);
  _painter.drawText(messageRect, Qt::AlignCenter, msg);
}

}
