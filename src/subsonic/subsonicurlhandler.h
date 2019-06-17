/*
 * Strawberry Music Player
 * Copyright 2019, Jonas Kvinge <jonas@jkvinge.net>
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

#ifndef SUBSONICURLHANDLER_H
#define SUBSONICURLHANDLER_H

#include <QObject>
#include <QPair>
#include <QList>
#include <QByteArray>
#include <QString>
#include <QUrl>

#include "core/urlhandler.h"
#include "core/song.h"
#include "subsonic/subsonicservice.h"

class Application;
class SubsonicService;

class SubsonicUrlHandler : public UrlHandler {
  Q_OBJECT

 public:
  SubsonicUrlHandler(Application *app, SubsonicService *service);

  QString scheme() const { return service_->url_scheme(); }
  LoadResult StartLoading(const QUrl &url);

 private:
  typedef QPair<QString, QString> Param;
  typedef QList<Param> ParamList;

  typedef QPair<QByteArray, QByteArray> EncodedParam;
  typedef QList<EncodedParam> EncodedParamList;

  SubsonicService *service_;

};

#endif  // SUBSONICURLHANDLER_H
