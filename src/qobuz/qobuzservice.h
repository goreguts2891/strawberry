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

#ifndef QOBUZSERVICE_H
#define QOBUZSERVICE_H

#include "config.h"

#include <memory>
#include <stdbool.h>

#include <QtGlobal>
#include <QObject>
#include <QPair>
#include <QList>
#include <QString>
#include <QUrl>
#include <QNetworkReply>
#include <QTimer>
#include <QSortFilterProxyModel>

#include "core/song.h"
#include "internet/internetservice.h"
#include "internet/internetsearch.h"
#include "settings/qobuzsettingspage.h"

class Application;
class NetworkAccessManager;
class QobuzUrlHandler;
class QobuzRequest;
class QobuzFavoriteRequest;
class QobuzStreamURLRequest;
class CollectionBackend;
class CollectionModel;

using std::shared_ptr;

class QobuzService : public InternetService {
  Q_OBJECT

 public:
  QobuzService(Application *app, QObject *parent);
  ~QobuzService();

  static const Song::Source kSource;

  void ReloadSettings();
  QString CoverCacheDir();  

  void Logout();
  int Search(const QString &query, InternetSearch::SearchType type);
  void CancelSearch();

  const int max_login_attempts() { return kLoginAttempts; }

  QString app_id() { return app_id_; }
  QString app_secret() { return app_secret_; }
  QString username() { return username_; }
  QString password() { return password_; }
  int format() { return format_; }
  int search_delay() { return search_delay_; }
  int artistssearchlimit() { return artistssearchlimit_; }
  int albumssearchlimit() { return albumssearchlimit_; }
  int songssearchlimit() { return songssearchlimit_; }
  bool cache_album_covers() { return cache_album_covers_; }

  QString access_token() { return access_token_; }

  const bool authenticated() { return (!app_id_.isEmpty() && !app_secret_.isEmpty() && !access_token_.isEmpty()); }
  const bool login_sent() { return login_sent_; }
  const bool login_attempts() { return login_attempts_; }

  void GetStreamURL(const QUrl &url);

  CollectionBackend *artists_collection_backend() { return artists_collection_backend_; }
  CollectionBackend *albums_collection_backend() { return albums_collection_backend_; }
  CollectionBackend *songs_collection_backend() { return songs_collection_backend_; }

  CollectionModel *artists_collection_model() { return artists_collection_model_; }
  CollectionModel *albums_collection_model() { return albums_collection_model_; }
  CollectionModel *songs_collection_model() { return songs_collection_model_; }

  QSortFilterProxyModel *artists_collection_sort_model() { return artists_collection_sort_model_; }
  QSortFilterProxyModel *albums_collection_sort_model() { return albums_collection_sort_model_; }
  QSortFilterProxyModel *songs_collection_sort_model() { return songs_collection_sort_model_; }

  enum QueryType {
    QueryType_Artists,
    QueryType_Albums,
    QueryType_Songs,
    QueryType_SearchArtists,
    QueryType_SearchAlbums,
    QueryType_SearchSongs,
  };

 signals:

 public slots:
  void ShowConfig();
  void TryLogin();
  void SendLogin(const QString &username, const QString &password, const QString &token);
  void GetArtists();
  void GetAlbums();
  void GetSongs();
  void ResetArtistsRequest();
  void ResetAlbumsRequest();
  void ResetSongsRequest();

 private slots:
  void SendLogin();
  void HandleAuthReply(QNetworkReply *reply);
  void ResetLoginAttempts();
  void StartSearch();
  void ArtistsResultsReceived(SongList songs);
  void ArtistsErrorReceived(QString error);
  void AlbumsResultsReceived(SongList songs);
  void AlbumsErrorReceived(QString error);
  void SongsResultsReceived(SongList songs);
  void SongsErrorReceived(QString error);
  void HandleStreamURLFinished(const QUrl original_url, const QUrl stream_url, const Song::FileType filetype, QString error = QString());

 private:
  typedef QPair<QString, QString> Param;
  typedef QList<Param> ParamList;

  typedef QPair<QByteArray, QByteArray> EncodedParam;
  typedef QList<EncodedParam> EncodedParamList;

  void SendSearch();
  QString LoginError(QString error, QVariant debug = QVariant());

  static const char *kAuthUrl;
  static const int kLoginAttempts;
  static const int kTimeResetLoginAttempts;

  static const char *kArtistsSongsTable;
  static const char *kAlbumsSongsTable;
  static const char *kSongsTable;

  static const char *kArtistsSongsFtsTable;
  static const char *kAlbumsSongsFtsTable;
  static const char *kSongsFtsTable;

  Application *app_;
  NetworkAccessManager *network_;
  QobuzUrlHandler *url_handler_;

  CollectionBackend *artists_collection_backend_;
  CollectionBackend *albums_collection_backend_;
  CollectionBackend *songs_collection_backend_;

  CollectionModel *artists_collection_model_;
  CollectionModel *albums_collection_model_;
  CollectionModel *songs_collection_model_;

  QSortFilterProxyModel *artists_collection_sort_model_;
  QSortFilterProxyModel *albums_collection_sort_model_;
  QSortFilterProxyModel *songs_collection_sort_model_;

  QTimer *timer_search_delay_;
  QTimer *timer_login_attempt_;

  std::shared_ptr<QobuzRequest> artists_request_;
  std::shared_ptr<QobuzRequest> albums_request_;
  std::shared_ptr<QobuzRequest> songs_request_;
  std::shared_ptr<QobuzRequest> search_request_;
  QobuzFavoriteRequest *favorite_request_;

  QString app_id_;
  QString app_secret_;
  QString username_;
  QString password_;
  int format_;
  int search_delay_;
  int artistssearchlimit_;
  int albumssearchlimit_;
  int songssearchlimit_;
  bool cache_album_covers_;

  QString access_token_;

  int pending_search_id_;
  int next_pending_search_id_;
  QString pending_search_text_;
  InternetSearch::SearchType pending_search_type_;

  int search_id_;
  QString search_text_;
  bool login_sent_;
  int login_attempts_;

  QList<QobuzStreamURLRequest*> stream_url_requests_;

};

#endif  // QOBUZSERVICE_H
