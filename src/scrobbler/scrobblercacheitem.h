/*
 * Strawberry Music Player
 * Copyright 2018-2021, Jonas Kvinge <jonas@jkvinge.net>
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

#ifndef SCROBBLERCACHEITEM_H
#define SCROBBLERCACHEITEM_H

#include "config.h"

#include <memory>
#include <optional>

#include <QtGlobal>
#include <QObject>
#include <QString>

#include "core/song.h"

class ScrobblerCacheItem : public QObject {
  Q_OBJECT

 public:
  explicit ScrobblerCacheItem(const QString &artist, const QString &album, const QString &song, const QString &albumartist, const std::optional<uint> track, const std::optional<quint64> duration, const quint64 timestamp, QObject *parent = nullptr);

  QString effective_albumartist() const { return albumartist_.isEmpty() || albumartist_.compare(Song::kVariousArtists, Qt::CaseInsensitive) == 0 ? artist_ : albumartist_; }

 public:
  QString artist_;
  QString album_;
  QString song_;
  QString albumartist_;
  std::optional<quint64> track_;
  std::optional<quint64> duration_;
  quint64 timestamp_;
  bool sent_;

};

typedef std::shared_ptr<ScrobblerCacheItem> ScrobblerCacheItemPtr;
typedef QList<ScrobblerCacheItemPtr> ScrobblerCacheItemList;

Q_DECLARE_METATYPE(ScrobblerCacheItemPtr)
Q_DECLARE_METATYPE(ScrobblerCacheItemList)

#endif  // SCROBBLERCACHEITEM_H
