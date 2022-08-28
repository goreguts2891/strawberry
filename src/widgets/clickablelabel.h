/* This file is part of Strawberry.
   Copyright 2010, Andrea Decorte <adecorte@gmail.com>

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

#ifndef CLICKABLELABEL_H
#define CLICKABLELABEL_H

#include <QObject>
#include <QString>
#include <QLabel>

class QWidget;
class QMouseEvent;

class ClickableLabel : public QLabel {
  Q_OBJECT

 public:
  explicit ClickableLabel(QWidget *parent = nullptr);

 signals:
  void Clicked();

 protected:
  void mousePressEvent(QMouseEvent *event) override;
};

#endif  // CLICKABLELABEL_H
