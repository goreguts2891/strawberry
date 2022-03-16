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

#ifndef M3UPARSER_H
#define M3UPARSER_H

#include "config.h"

#include <QtGlobal>
#include <QObject>
#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QDir>

#include "core/song.h"
#include "parserbase.h"
#include "playlist/playlist.h"

class QIODevice;
class CollectionBackendInterface;

class M3UParser : public ParserBase {
  Q_OBJECT

 public:
  explicit M3UParser(CollectionBackendInterface *collection, QObject *parent = nullptr);

  QString name() const override { return "M3U"; }
  QStringList file_extensions() const override { return QStringList() << "m3u"
                                                                      << "m3u8"; }
  QString mime_type() const override { return "text/uri-list"; }

  bool TryMagic(const QByteArray &data) const override;

  SongList Load(QIODevice *device, const QString &playlist_path = "", const QDir &dir = QDir(), const bool collection_search = true) const override;
  void Save(const SongList &songs, QIODevice *device, const QDir &dir = QDir(), Playlist::Path path_type = Playlist::Path_Automatic) const override;

 private:
  enum M3UType {
    STANDARD = 0,
    EXTENDED,  // Includes extended info (track, artist, etc.)
    LINK,      // Points to a directory.
  };

  struct Metadata {
    Metadata() : length(-1) {}
    QString artist;
    QString title;
    qint64 length;
  };

  static bool ParseMetadata(const QString &line, Metadata *metadata);

};

#endif  // M3UPARSER_H
