/* 
   Strawberry Music Player
   This file was part of Clementine.
   Copyright 2004, Melchior FRANZ <mfranz@kde.org>
   Copyright 2009-2010, David Sansome <davidsansome@gmail.com>
   Copyright 2010, 2014, John Maguire <john.maguire@gmail.com>
   Copyright 2014-2015, Mark Furneaux <mark@furneaux.ca>
   Copyright 2014, Krzysztof Sobiecki <sobkas@gmail.com>

   Strawberry is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Strawberry is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Strawberry.  If not, see <http://www.gnu.org/licenses/>.
*/

/* Original Author:  Melchior FRANZ  <mfranz@kde.org>  2004
 */

#include "sonogram.h"

#include <QPainter>

using Analyzer::Scope;

const char* Sonogram::kName =
    QT_TRANSLATE_NOOP("AnalyzerContainer", "Sonogram");

Sonogram::Sonogram(QWidget* parent)
    : Analyzer::Base(parent, 9), scope_size_(128) {}

Sonogram::~Sonogram() {}

void Sonogram::resizeEvent(QResizeEvent* e) {
  QWidget::resizeEvent(e);

// only for gcc < 4.0
#if !(__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 0))
  resizeForBands(height() < 128 ? 128 : height());
#endif

  canvas_ = QPixmap(size());
  canvas_.fill(palette().color(QPalette::Window));
}

void Sonogram::analyze(QPainter &p, const Scope &s, bool new_frame) {
  if (!new_frame || engine_->state() == Engine::Paused) {
    p.drawPixmap(0, 0, canvas_);
    return;
  }

  int x = width() - 1;
  QColor c;

  QPainter canvas_painter(&canvas_);
  canvas_painter.drawPixmap(0, 0, canvas_, 1, 0, x, -1);

  Scope::const_iterator it = s.begin(), end = s.end();
  if (scope_size_ != s.size()) {
    scope_size_ = s.size();
  }
    for (int y = height() - 1; y;) {
      if (it >= end || *it < .005)
        c = palette().color(QPalette::Window);
      else if (*it < .05)
        c.setHsv(95, 255, 255 - static_cast<int>(*it * 4000.0));
      else if (*it < 1.0)
        c.setHsv(95 - static_cast<int>(*it * 90.0), 255, 255);
      else
        c = Qt::red;

      canvas_painter.setPen(c);
      canvas_painter.drawPoint(x, y--);

      if (it < end) ++it;
    }

  canvas_painter.end();

  p.drawPixmap(0, 0, canvas_);
}

void Sonogram::transform(Scope& scope) {
  fht_->power2(scope.data());
  fht_->scale(scope.data(), 1.0 / 256);
  scope.resize(fht_->size() / 2);
}

void Sonogram::demo(QPainter& p) {
  analyze(p, Scope(fht_->size(), 0), new_frame_);
}