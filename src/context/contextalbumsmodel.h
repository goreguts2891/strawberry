/*
 * Strawberry Music Player
 * This code was part of Clementine.
 * Copyright 2010, David Sansome <me@davidsansome.com>
 * Copyright 2013-2021, Jonas Kvinge <jonas@jkvinge.net>
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

#ifndef CONTEXTALBUMSMODEL_H
#define CONTEXTALBUMSMODEL_H

#include "config.h"

#include <QtGlobal>
#include <QObject>
#include <QAbstractItemModel>
#include <QPair>
#include <QSet>
#include <QList>
#include <QMap>
#include <QVariant>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QImage>
#include <QPixmap>
#include <QIcon>

#include "core/simpletreemodel.h"
#include "core/song.h"
#include "collection/collectionquery.h"
#include "collection/collectionitem.h"
#include "covermanager/albumcoverloaderoptions.h"
#include "covermanager/albumcoverloaderresult.h"

class QMimeData;

class Application;
class CollectionBackend;
class CollectionItem;

class ContextAlbumsModel : public SimpleTreeModel<CollectionItem> {
  Q_OBJECT

 public:
  explicit ContextAlbumsModel(CollectionBackend *backend, Application *app, QObject *parent = nullptr);
  ~ContextAlbumsModel() override;

  static const int kPrettyCoverSize;

  enum Role {
    Role_Type = Qt::UserRole + 1,
    Role_ContainerType,
    Role_SortText,
    Role_Key,
    Role_Artist,
    Role_Editable,
    LastRole
  };

  void GetChildSongs(CollectionItem *item, QList<QUrl> *urls, SongList *songs, QSet<int> *song_ids) const;
  SongList GetChildSongs(const QModelIndex &idx) const;
  SongList GetChildSongs(const QModelIndexList &indexes) const;

  QVariant data(const QModelIndex &idx, int role = Qt::DisplayRole) const override;
  Qt::ItemFlags flags(const QModelIndex &idx) const override;
  QStringList mimeTypes() const override;
  QMimeData *mimeData(const QModelIndexList &indexes) const override;

  void Reset();
  void AddSongs(const SongList &songs);

 private slots:
  void AlbumCoverLoaded(const quint64 id, const AlbumCoverLoaderResult &result);

 private:
  CollectionItem *ItemFromSong(CollectionItem::Type item_type, const bool signal, CollectionItem *parent, const Song &s, const int container_level);

  static QString AlbumIconPixmapCacheKey(const QModelIndex &idx);
  QVariant AlbumIcon(const QModelIndex &idx);
  QVariant data(const CollectionItem *item, int role) const;
  bool CompareItems(const CollectionItem *a, const CollectionItem *b) const;

 private:
  CollectionBackend *backend_;
  Application *app_;
  QueryOptions query_options_;
  QMap<QString, CollectionItem *> container_nodes_;
  QMap<int, CollectionItem *> song_nodes_;
  QIcon album_icon_;
  QPixmap no_cover_icon_;
  AlbumCoverLoaderOptions cover_loader_options_;
  typedef QPair<CollectionItem *, QString> ItemAndCacheKey;
  QMap<quint64, ItemAndCacheKey> pending_art_;
  QSet<QString> pending_cache_keys_;

};

#endif  // CONTEXTALBUMSMODEL_H
