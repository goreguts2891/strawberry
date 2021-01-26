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

#ifndef DEVICESTATEFILTERMODEL_H
#define DEVICESTATEFILTERMODEL_H

#include "config.h"

#include <QObject>
#include <QString>
#include <QSortFilterProxyModel>

#include "devicemanager.h"

class QAbstractItemModel;
class QModelIndex;

class DeviceStateFilterModel : public QSortFilterProxyModel {
  Q_OBJECT

 public:
  explicit DeviceStateFilterModel(QObject *parent, DeviceManager::State state = DeviceManager::State_Remembered);

  void setSourceModel(QAbstractItemModel *sourceModel) override;

 signals:
  void IsEmptyChanged(bool is_empty);

 protected:
  bool filterAcceptsRow(int row, const QModelIndex &parent) const override;

 private slots:
  void ProxyReset();
  void ProxyRowCountChanged(QModelIndex idx, int first, int last);

 private:
  DeviceManager::State state_;
};

#endif // DEVICESTATEFILTERMODEL_H
