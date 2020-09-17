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

#ifndef SUBSONICSERVICE_H
#define SUBSONICSERVICE_H

#include "config.h"

#include <memory>

#include <QObject>
#include <QPair>
#include <QSet>
#include <QList>
#include <QVariant>
#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QSslError>
#include <QDateTime>

#include "core/song.h"
#include "internet/internetservice.h"

class QSortFilterProxyModel;
class QNetworkReply;

class Application;
class SubsonicUrlHandler;
class SubsonicRequest;
class SubsonicScrobbleRequest;
class CollectionBackend;
class CollectionModel;

class SubsonicService : public InternetService {
  Q_OBJECT

 public:
  explicit SubsonicService(Application *app, QObject *parent);
  ~SubsonicService() override;

  static const Song::Source kSource;

  void ReloadSettings() override;
  void Exit() override;

  Application *app() { return app_; }

  QString client_name() { return kClientName; }
  QString api_version() { return kApiVersion; }
  QUrl server_url() { return server_url_; }
  QString username() { return username_; }
  QString password() { return password_; }
  bool verify_certificate() { return verify_certificate_; }
  bool download_album_covers() { return download_album_covers_; }

  CollectionBackend *collection_backend() { return collection_backend_; }
  CollectionModel *collection_model() { return collection_model_; }
  QSortFilterProxyModel *collection_sort_model() { return collection_sort_model_; }

  CollectionBackend *songs_collection_backend() override { return collection_backend_; }
  CollectionModel *songs_collection_model() override { return collection_model_; }
  QSortFilterProxyModel *songs_collection_sort_model() override { return collection_sort_model_; }

  void CheckConfiguration();
  void Scrobble(QString song_id, bool submission, QDateTime time);

 public slots:
  void ShowConfig() override;
  void SendPing();
  void SendPing(QUrl url, const QString &username, const QString &password, const bool redirect = false);
  void GetSongs() override;
  void ResetSongsRequest() override;

 private slots:
  void HandlePingSSLErrors(QList<QSslError> ssl_errors);
  void HandlePingReply(QNetworkReply *reply, const QUrl &url, const QString &username, const QString &password);
  void SongsResultsReceived(const SongList &songs, const QString &error);

 private:
  typedef QPair<QString, QString> Param;
  typedef QList<Param> ParamList;

  typedef QPair<QByteArray, QByteArray> EncodedParam;
  typedef QList<EncodedParam> EncodedParamList;

  void PingError(const QString &error = QString(), const QVariant &debug = QVariant());

  static const char *kClientName;
  static const char *kApiVersion;
  static const char *kSongsTable;
  static const char *kSongsFtsTable;
  static const int kMaxRedirects;

  Application *app_;
  std::unique_ptr<QNetworkAccessManager> network_;
  SubsonicUrlHandler *url_handler_;

  CollectionBackend *collection_backend_;
  CollectionModel *collection_model_;
  QSortFilterProxyModel *collection_sort_model_;

  std::shared_ptr<SubsonicRequest> songs_request_;
  std::shared_ptr<SubsonicScrobbleRequest> scrobble_request_;

  QUrl server_url_;
  QString username_;
  QString password_;
  bool verify_certificate_;
  bool download_album_covers_;

  QStringList errors_;
  int ping_redirects_;

  QList<QNetworkReply*> replies_;

};

#endif  // SUBSONICSERVICE_H
