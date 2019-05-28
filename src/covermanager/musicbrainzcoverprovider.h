/*
 * Strawberry Music Player
 * Copyright 2018, Jonas Kvinge <jonas@jkvinge.net>
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

#ifndef MUSICBRAINZCOVERPROVIDER_H
#define MUSICBRAINZCOVERPROVIDER_H

#include "config.h"

#include <stdbool.h>

#include <QObject>
#include <QByteArray>
#include <QVariant>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>

#include "coverprovider.h"
#include "albumcoverfetcher.h"

class Application;

class MusicbrainzCoverProvider : public CoverProvider {
  Q_OBJECT
 public:
  explicit MusicbrainzCoverProvider(Application *app, QObject *parent = nullptr);

  bool StartSearch(const QString &artist, const QString &album, int id);
  void CancelSearch(int id);

 private slots:
  void HandleSearchReply(QNetworkReply *reply, int search_id);

 private:
  QByteArray GetReplyData(QNetworkReply *reply);
  QJsonObject ExtractJsonObj(const QByteArray &data);
  void Error(QString error, QVariant debug = QVariant());

 private:
  static const char *kReleaseSearchUrl;
  static const char *kAlbumCoverUrl;
  static const int kLimit;
  QNetworkAccessManager *network_;

};

#endif  // MUSICBRAINZCOVERPROVIDER_H
