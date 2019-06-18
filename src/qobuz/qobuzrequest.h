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

#ifndef QOBUZREQUEST_H
#define QOBUZREQUEST_H

#include "config.h"

#include <QtGlobal>
#include <QObject>
#include <QPair>
#include <QList>
#include <QHash>
#include <QMap>
#include <QMultiMap>
#include <QQueue>
#include <QString>
#include <QUrl>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

#include "core/song.h"
#include "qobuzbaserequest.h"

class NetworkAccessManager;
class QobuzService;
class QobuzUrlHandler;

class QobuzRequest : public QobuzBaseRequest {
  Q_OBJECT

 public:

  QobuzRequest(QobuzService *service, QobuzUrlHandler *url_handler, NetworkAccessManager *network, QueryType type, QObject *parent);
  ~QobuzRequest();

  void ReloadSettings();

  void Process();
  void Search(const int search_id, const QString &search_text);

 signals:
  void Login();
  void Login(const QString &username, const QString &password, const QString &token);
  void LoginSuccess();
  void LoginFailure(QString failure_reason);
  void Results(SongList songs);
  void SearchResults(int id, SongList songs);
  void ErrorSignal(QString message);
  void ErrorSignal(int id, QString message);
  void UpdateStatus(QString text);
  void ProgressSetMaximum(int max);
  void UpdateProgress(int max);
  void StreamURLFinished(const QUrl original_url, const QUrl url, const Song::FileType, QString error = QString());

 private slots:
  //void LoginComplete(bool success, QString error = QString());

  void ArtistsReplyReceived(QNetworkReply *reply, const int limit_requested, const int offset_requested);

  void AlbumsReplyReceived(QNetworkReply *reply, const int limit_requested, const int offset_requested);
  void AlbumsReceived(QNetworkReply *reply, const int artist_id_requested, const int limit_requested, const int offset_requested);

  void SongsReplyReceived(QNetworkReply *reply, const int limit_requested, const int offset_requested);
  void SongsReceived(QNetworkReply *reply, const int artist_id_requested, const QString &album_id_requested, const int limit_requested, const int offset_requested, const QString &album_artist_requested = QString());

  void ArtistAlbumsReplyReceived(QNetworkReply *reply, const int artist_id, const int offset_requested);
  void AlbumSongsReplyReceived(QNetworkReply *reply, const int artist_id, const QString &album_id, const int offset_requested, const QString &album_artist);
  void AlbumCoverReceived(QNetworkReply *reply, const QUrl &cover_url, const QString &filename);

 private:
  typedef QPair<QString, QString> Param;
  typedef QList<Param> ParamList;

  struct Request {
    int artist_id = 0;
    QString album_id = 0;
    int song_id = 0;
    int offset = 0;
    int limit = 0;
    QString album_artist;
    QString album;
  };
  struct AlbumCoverRequest {
    //int artist_id = 0;
    QUrl url;
    QString filename;
  };

  const bool IsQuery() { return (type_ == QueryType_Artists || type_ == QueryType_Albums || type_ == QueryType_Songs); }
  const bool IsSearch() { return (type_ == QueryType_SearchArtists || type_ == QueryType_SearchAlbums || type_ == QueryType_SearchSongs); }

  void GetArtists();
  void GetAlbums();
  void GetSongs();

  void ArtistsSearch();
  void AlbumsSearch();
  void SongsSearch();

  void AddArtistsRequest(const int offset = 0, const int limit = 0);
  void AddArtistsSearchRequest(const int offset = 0);
  void FlushArtistsRequests();
  void AddAlbumsRequest(const int offset = 0, const int limit = 0);
  void AddAlbumsSearchRequest(const int offset = 0);
  void FlushAlbumsRequests();
  void AddSongsRequest(const int offset = 0, const int limit = 0);
  void AddSongsSearchRequest(const int offset = 0);
  void FlushSongsRequests();

  void ArtistsFinishCheck(const int limit = 0, const int offset = 0, const int artists_received = 0);
  void AlbumsFinishCheck(const int artist_id, const int limit = 0, const int offset = 0, const int albums_total = 0, const int albums_received = 0);
  void SongsFinishCheck(const int artist_id, const QString &album_id, const int limit, const int offset, const int songs_total, const int songs_received, const QString &album_artist);

  void AddArtistAlbumsRequest(const int artist_id, const int offset = 0);
  void FlushArtistAlbumsRequests();

  void AddAlbumSongsRequest(const int artist_id, const QString &album_id, const QString &album_artist, const int offset = 0);
  void FlushAlbumSongsRequests();

  int ParseSong(Song &song, const QJsonObject &json_obj, int artist_id, QString album_id, QString album_artist, QString album, QUrl cover_url);

  QString AlbumCoverFileName(const Song &song);

  void GetAlbumCovers();
  void AddAlbumCoverRequest(Song &song);
  void FlushAlbumCoverRequests();
  void AlbumCoverFinishCheck();

  void FinishCheck();
  void Warn(QString error, QVariant debug = QVariant());
  QString Error(QString error, QVariant debug = QVariant());

  static const int kMaxConcurrentArtistsRequests;
  static const int kMaxConcurrentAlbumsRequests;
  static const int kMaxConcurrentSongsRequests;
  static const int kMaxConcurrentArtistAlbumsRequests;
  static const int kMaxConcurrentAlbumSongsRequests;
  static const int kMaxConcurrentAlbumCoverRequests;

  QobuzService *service_;
  QobuzUrlHandler *url_handler_;
  NetworkAccessManager *network_;

  QueryType type_;

  int search_id_;
  QString search_text_;

  bool finished_;

  QQueue<Request> artists_requests_queue_;
  QQueue<Request> albums_requests_queue_;
  QQueue<Request> songs_requests_queue_;

  QQueue<Request> artist_albums_requests_queue_;
  QQueue<Request> album_songs_requests_queue_;
  QQueue<AlbumCoverRequest> album_cover_requests_queue_;

  QList<int> artist_albums_requests_pending_;
  QHash<QString, Request> album_songs_requests_pending_;
  QMultiMap<QUrl, Song*> album_covers_requests_sent_;

  int artists_requests_active_;
  int artists_total_;
  int artists_received_;

  int albums_requests_active_;
  int songs_requests_active_;

  int artist_albums_requests_active_;
  int artist_albums_requested_;
  int artist_albums_received_;

  int album_songs_requests_active_;
  int album_songs_requested_;
  int album_songs_received_;

  int album_covers_requests_active_;
  int album_covers_requested_;
  int album_covers_received_;

  SongList songs_;
  QString errors_;
  bool no_results_;
  QList<QNetworkReply*> album_cover_replies_;

};

#endif  // QOBUZREQUEST_H
