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

#ifndef AUDDLYRICSPROVIDER_H
#define AUDDLYRICSPROVIDER_H

#include "config.h"

#include <stdbool.h>

#include <QObject>
#include <QHash>
#include <QMetaType>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "lyricsprovider.h"
#include "lyricsfetcher.h"

class AuddLyricsProvider : public LyricsProvider {
  Q_OBJECT

 public:
  explicit AuddLyricsProvider(QObject *parent = nullptr);

  bool StartSearch(const QString &artist, const QString &album, const QString &title, quint64 id);
  void CancelSearch(quint64 id);

 private slots:
  void HandleSearchReply(QNetworkReply *reply, quint64 id, const QString artist, const QString title);

 private:
  static const char *kUrlSearch;
  static const char *kAPITokenB64;
  static const int kMaxLength;
  QNetworkAccessManager *network_;
  void Error(quint64 id, QString error, QVariant debug = QVariant());

  QJsonObject ExtractJsonObj(QNetworkReply *reply, quint64 id);
  QJsonArray ExtractResult(QNetworkReply *reply, const quint64 id, const QString &artist, const QString &title);

};

#endif  // AUDDLYRICSPROVIDER_H

