/*
 * Strawberry Music Player
 * This file was part of Clementine.
 * Copyright 2010, David Sansome <me@davidsansome.com>
 *
 * Strawberry is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Strawberry is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Strawberry.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef MULTILOADINGINDICATOR_H
#define MULTILOADINGINDICATOR_H

#include "config.h"

#include <QObject>
#include <QWidget>
#include <QSize>
#include <QString>

class QPaintEvent;

class BusyIndicator;
class TaskManager;

class MultiLoadingIndicator : public QWidget {
  Q_OBJECT

 public:
  explicit MultiLoadingIndicator(QWidget *parent = nullptr);

  static const int kVerticalPadding;
  static const int kHorizontalPadding;
  static const int kSpacing;

  void SetTaskManager(TaskManager *task_manager);

  QSize sizeHint() const override;

 signals:
  void TaskCountChange(int tasks);

 protected:
  void paintEvent(QPaintEvent *) override;

 private slots:
  void UpdateText();

 private:
  TaskManager *task_manager_;

  BusyIndicator *spinner_;
  QString text_;
};

#endif  // MULTILOADINGINDICATOR_H
