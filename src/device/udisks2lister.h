/*
 * Strawberry Music Player
 * This file was part of Clementine.
 * Copyright 2016, Valeriy Malov <jazzvoid@gmail.com>
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

#ifndef UDISKS2LISTER_H
#define UDISKS2LISTER_H

#include "config.h"

#include <memory>
#include <stdbool.h>

#include <QtGlobal>
#include <QObject>
#include <QMutex>
#include <QList>
#include <QMap>
#include <QMetaType>
#include <QReadWriteLock>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QDBusObjectPath>
#include <QDBusArgument>
#include <QJsonArray>
#include <QJsonObject>

#include "dbus/metatypes.h"
#include "devicelister.h"

class OrgFreedesktopDBusObjectManagerInterface;
class OrgFreedesktopUDisks2JobInterface;

class Udisks2Lister : public DeviceLister {
  Q_OBJECT

 public:
  Udisks2Lister();
  ~Udisks2Lister();

  QStringList DeviceUniqueIDs() override;
  QVariantList DeviceIcons(const QString &id) override;
  QString DeviceManufacturer(const QString &id) override;
  QString DeviceModel(const QString &id) override;
  quint64 DeviceCapacity(const QString &id) override;
  quint64 DeviceFreeSpace(const QString &id) override;
  QVariantMap DeviceHardwareInfo(const QString &id) override;

  QString MakeFriendlyName(const QString &id) override;
  QList<QUrl> MakeDeviceUrls(const QString &id) override;

  void UnmountDevice(const QString &id) override;

 public slots:
  void UpdateDeviceFreeSpace(const QString &id) override;

 protected:
  bool Init() override;

 private slots:
  void DBusInterfaceAdded(const QDBusObjectPath &path, const InterfacesAndProperties &ifaces);
  void DBusInterfaceRemoved(const QDBusObjectPath &path, const QStringList &ifaces);
  void JobCompleted(bool success, const QString &message);

 private:
  bool isPendingJob(const QDBusObjectPath &job_path);
  void RemoveDevice(const QDBusObjectPath &device_path);
  QList<QDBusObjectPath> GetMountedPartitionsFromDBusArgument( const QDBusArgument &input);

  struct Udisks2Job {
    Udisks2Job();
    bool is_mount;
    QList<QDBusObjectPath> mounted_partitions;
    std::shared_ptr<OrgFreedesktopUDisks2JobInterface> dbus_interface;
  };

  QMutex jobs_lock_;
  QMap<QDBusObjectPath, Udisks2Job> mounting_jobs_;

 private:
  struct PartitionData {
    PartitionData();

    QString unique_id() const;

    QString dbus_path;
    QString friendly_name;

    // Device
    QString serial;
    QString vendor;
    QString model;
    quint64 capacity;
    QString dbus_drive_path;

    // Paritition
    QString label;
    QString uuid;
    quint64 free_space;
    QStringList mount_paths;
  };

  PartitionData ReadPartitionData(const QDBusObjectPath &path);
  void HandleFinishedMountJob(const Udisks2Lister::PartitionData &partition_data);
  void HandleFinishedUnmountJob(const Udisks2Lister::PartitionData &partition_data, const QDBusObjectPath &mounted_object);

  QReadWriteLock device_data_lock_;
  QMap<QString, PartitionData> device_data_;

 private:
  std::unique_ptr<OrgFreedesktopDBusObjectManagerInterface> udisks2_interface_;

  static constexpr char udisks2_service_[] = "org.freedesktop.UDisks2";
};

#endif  // UDISKS2LISTER_H
