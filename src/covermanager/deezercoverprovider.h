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

#ifndef DEEZERCOVERPROVIDER_H
#define DEEZERCOVERPROVIDER_H

#include "config.h"

#include <stdbool.h>

#include <QObject>
#include <QVariant>
#include <QByteArray>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonValue>
#include <QJsonObject>
#include <QJsonArray>

#include "coverprovider.h"

class Application;

class DeezerCoverProvider : public CoverProvider {
  Q_OBJECT

 public:
  explicit DeezerCoverProvider(Application *app, QObject *parent = nullptr);
  bool StartSearch(const QString &artist, const QString &album, int id);
  void CancelSearch(int id);

 private slots:
  void HandleSearchReply(QNetworkReply *reply, int id);

 private:
  static const char *kApiUrl;
  static const int kLimit;

  QByteArray GetReplyData(QNetworkReply *reply);
  QJsonObject ExtractJsonObj(QByteArray &data);
  QJsonValue ExtractData(QByteArray &data);
  void Error(QString error, QVariant debug = QVariant());

  QNetworkAccessManager *network_;

};

#endif  // DEEZERCOVERPROVIDER_H
