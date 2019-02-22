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

#include "config.h"

#include <stdbool.h>

#include <QtGlobal>
#include <QApplication>
#include <QDesktopServices>
#include <QClipboard>
#include <QVariant>
#include <QByteArray>
#include <QString>
#include <QUrl>
#include <QUrlQuery>
#include <QDateTime>
#include <QCryptographicHash>
#include <QMenu>
#include <QMessageBox>
#include <QSettings>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

#include "core/application.h"
#include "core/closure.h"
#include "core/network.h"
#include "core/song.h"
#include "core/timeconstants.h"
#include "core/logging.h"
#include "internet/localredirectserver.h"
#include "settings/settingsdialog.h"
#include "settings/scrobblersettingspage.h"

#include "audioscrobbler.h"
#include "scrobblerservices.h"
#include "scrobblingapi20.h"
#include "scrobblercache.h"
#include "scrobblercacheitem.h"

const char *ScrobblingAPI20::kApiKey = "211990b4c96782c05d1536e7219eb56e";
const char *ScrobblingAPI20::kSecret = "80fd738f49596e9709b1bf9319c444a8";
const char *ScrobblingAPI20::kRedirectUrl = "https://oauth.strawbs.net";
const int ScrobblingAPI20::kScrobblesPerRequest = 50;

ScrobblingAPI20::ScrobblingAPI20(const QString &name, const QString &settings_group, const QString &auth_url, const QString &api_url, const bool batch, Application *app, QObject *parent) :
  ScrobblerService(name, app, parent),
  name_(name),
  settings_group_(settings_group),
  auth_url_(auth_url),
  api_url_(api_url),
  batch_(batch),
  app_(app),
  enabled_(false),
  subscriber_(false),
  submitted_(false) {}

ScrobblingAPI20::~ScrobblingAPI20() {}

void ScrobblingAPI20::ReloadSettings() {

  QSettings s;
  s.beginGroup(settings_group_);
  enabled_ = s.value("enabled", false).toBool();
  s.endGroup();

}

void ScrobblingAPI20::LoadSession() {

  QSettings s;
  s.beginGroup(settings_group_);
  subscriber_ = s.value("subscriber", false).toBool();
  username_ = s.value("username").toString();
  session_key_ = s.value("session_key").toString();
  s.endGroup();

}

void ScrobblingAPI20::Logout() {

  subscriber_ = false;
  username_.clear();
  session_key_.clear();

  QSettings settings;
  settings.beginGroup(settings_group_);
  settings.remove("subscriber");
  settings.remove("username");
  settings.remove("session_key");
  settings.endGroup();

}

void ScrobblingAPI20::Authenticate() {

  QUrl url(auth_url_);

  LocalRedirectServer *server = new LocalRedirectServer(this);
  server->Listen();
  NewClosure(server, SIGNAL(Finished()), this, &ScrobblingAPI20::RedirectArrived, server);

  QUrl redirect_url(kRedirectUrl);
  QUrlQuery redirect_url_query;
  const QString port = QString::number(server->url().port());
  redirect_url_query.addQueryItem("port", port);  
  redirect_url.setQuery(redirect_url_query);

  QUrlQuery url_query;
  url_query.addQueryItem("api_key", kApiKey);
  url_query.addQueryItem("cb", redirect_url.toString());
  url.setQuery(url_query);

  QMessageBox messagebox(QMessageBox::Information, tr("%1 Scrobbler Authentication").arg(name_), tr("Open URL in web browser?<br /><a href=\"%1\">%1</a><br />Press \"Save\" to copy the URL to clipboard and manually open it in a web browser.").arg(url.toString()), QMessageBox::Open|QMessageBox::Save|QMessageBox::Cancel);
  messagebox.setTextFormat(Qt::RichText);
  int result = messagebox.exec();
  switch (result) {
  case QMessageBox::Open:{
      bool openurl_result = QDesktopServices::openUrl(url);
      if (openurl_result) {
        break;
      }
      QMessageBox messagebox_error(QMessageBox::Warning, tr("%1 Scrobbler Authentication").arg(name_), tr("Could not open URL. Please open this URL in your browser:<br /><a href=\"%1\">%1</a>").arg(url.toString()), QMessageBox::Ok);
      messagebox_error.setTextFormat(Qt::RichText);
      messagebox_error.exec();
    }
  case QMessageBox::Save:
    QApplication::clipboard()->setText(url.toString());
    break;
  case QMessageBox::Cancel:
    AuthError(tr("Authentication was cancelled."));
    break;
  default:
    break;
  }

}

void ScrobblingAPI20::RedirectArrived(LocalRedirectServer *server) {

  server->deleteLater();

  QUrl url = server->request_url();
  RequestSession(QUrlQuery(url).queryItemValue("token").toUtf8());

}

void ScrobblingAPI20::RequestSession(QString token) {

  QUrl session_url(api_url_);
  QUrlQuery session_url_query;
  session_url_query.addQueryItem("api_key", kApiKey);
  session_url_query.addQueryItem("method", "auth.getSession");
  session_url_query.addQueryItem("token", token);
  QString data_to_sign;
  for (QPair<QString, QString> param : session_url_query.queryItems()) {
    data_to_sign += param.first + param.second;
  }
  data_to_sign += kSecret;
  QByteArray const digest = QCryptographicHash::hash(data_to_sign.toUtf8(), QCryptographicHash::Md5);
  QString signature = QString::fromLatin1(digest.toHex()).rightJustified(32, '0').toLower();
  session_url_query.addQueryItem("api_sig", signature);
  session_url_query.addQueryItem(QUrl::toPercentEncoding("format"), QUrl::toPercentEncoding("json"));
  session_url.setQuery(session_url_query);

  QNetworkRequest req(session_url);
  QNetworkReply *reply = network()->get(req);
  NewClosure(reply, SIGNAL(finished()), this, SLOT(AuthenticateReplyFinished(QNetworkReply*)), reply);

}

void ScrobblingAPI20::AuthenticateReplyFinished(QNetworkReply *reply) {

  reply->deleteLater();

  QByteArray data;

  if (reply->error() == QNetworkReply::NoError && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200) {
    data = reply->readAll();
  }
  else {
    if (reply->error() < 200) {
      // This is a network error, there is nothing more to do.
      QString failure_reason = QString("%1 (%2)").arg(reply->errorString()).arg(reply->error());
      AuthError(failure_reason);
    }
    else {
      // See if there is Json data containing "error" and "message" - then use that instead.
      data = reply->readAll();
      QJsonParseError error;
      QJsonDocument json_doc = QJsonDocument::fromJson(data, &error);
      QString failure_reason;
      if (error.error == QJsonParseError::NoError && !json_doc.isNull() && !json_doc.isEmpty() && json_doc.isObject()) {
        QJsonObject json_obj = json_doc.object();
        if (json_obj.contains("error") && json_obj.contains("message")) {
          int error = json_obj["error"].toInt();
          QString message = json_obj["message"].toString();
          failure_reason = "Error: " + QString::number(error) + ": " + message;
        }
        else {
          failure_reason = QString("%1 (%2)").arg(reply->errorString()).arg(reply->error());
        }
      }
      else {
        failure_reason = QString("%1 (%2)").arg(reply->errorString()).arg(reply->error());
      }
      AuthError(failure_reason);
    }
    return;
  }
  
  QJsonObject json_obj = ExtractJsonObj(data);
  if (json_obj.isEmpty()) {
    AuthError("Json document from server was empty.");
    return;
  }
  
  if (json_obj.contains("error") && json_obj.contains("message")) {
    int error = json_obj["error"].toInt();
    QString message = json_obj["message"].toString();
    QString failure_reason = "Error: " + QString::number(error) + ": " + message;
    AuthError(failure_reason);
    return;
  }

  if (!json_obj.contains("session")) {
    AuthError("Json reply from server is missing session.");
    return;
  }

  QJsonValue json_session = json_obj["session"];
  if (!json_session.isObject()) {
    AuthError("Json session is not an object.");
    return;
  }
  json_obj = json_session.toObject();
  if (json_obj.isEmpty()) {
    AuthError("Json session object is empty.");
    return;
  }
  if (!json_obj.contains("subscriber") || !json_obj.contains("name") || !json_obj.contains("key")) {
    AuthError("Json session object is missing values.");
    return;
  }

  subscriber_ = json_obj["subscriber"].toBool();
  username_ = json_obj["name"].toString();
  session_key_ = json_obj["key"].toString();
  
  QSettings s;
  s.beginGroup(settings_group_);
  s.setValue("subscriber", subscriber_);
  s.setValue("username", username_);
  s.setValue("session_key", session_key_);
  s.endGroup();

  emit AuthenticationComplete(true);

  DoSubmit();

}

QNetworkReply *ScrobblingAPI20::CreateRequest(const ParamList &request_params) {

  ParamList params = ParamList()
    << Param("api_key", kApiKey)
    << Param("sk", session_key_)
    << Param("lang", QLocale().name().left(2).toLower())
    << request_params;

  std::sort(params.begin(), params.end());

  QUrlQuery url_query;
  QString data_to_sign;
  for (const Param &param : params) {
    EncodedParam encoded_param(QUrl::toPercentEncoding(param.first), QUrl::toPercentEncoding(param.second));
    url_query.addQueryItem(encoded_param.first, encoded_param.second);
    data_to_sign += param.first + param.second;
  }
  data_to_sign += kSecret;

  QByteArray const digest = QCryptographicHash::hash(data_to_sign.toUtf8(), QCryptographicHash::Md5);
  QString signature = QString::fromLatin1(digest.toHex()).rightJustified(32, '0').toLower();

  url_query.addQueryItem("api_sig", QUrl::toPercentEncoding(signature));
  url_query.addQueryItem("format", QUrl::toPercentEncoding("json"));

  QUrl url(api_url_);
  QNetworkRequest req(url);
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
  QByteArray query = url_query.toString(QUrl::FullyEncoded).toUtf8();
  QNetworkReply *reply = network()->post(req, query);

  //qLog(Debug) << name_ << "Sending request" << query;

  return reply;

}

QByteArray ScrobblingAPI20::GetReplyData(QNetworkReply *reply) {

  QByteArray data;

  if (reply->error() == QNetworkReply::NoError && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200) {
    data = reply->readAll();
  }
  else {
    if (reply->error() < 200) {
      // This is a network error, there is nothing more to do.
      QString error_reason = QString("%1 (%2)").arg(reply->errorString()).arg(reply->error());
      Error(error_reason);
    }
    else {
      // See if there is Json data containing "error" and "message" - then use that instead.
      data = reply->readAll();
      QJsonParseError error;
      QJsonDocument json_doc = QJsonDocument::fromJson(data, &error);
      int error_code = -1;
      QString error_reason;
      if (error.error == QJsonParseError::NoError && !json_doc.isNull() && !json_doc.isEmpty() && json_doc.isObject()) {
        QJsonObject json_obj = json_doc.object();
        if (json_obj.contains("error") && json_obj.contains("message")) {
          error_code = json_obj["error"].toInt();
          QString error_message = json_obj["message"].toString();
          error_reason = QString("%1 (%2)").arg(error_message).arg(error_code);
        }
        else {
          error_reason = QString("%1 (%2)").arg(reply->errorString()).arg(reply->error());
        }
      }
      else {
        error_reason = QString("%1 (%2)").arg(reply->errorString()).arg(reply->error());
      }
      if (reply->error() == QNetworkReply::ContentAccessDenied ||
          reply->error() == QNetworkReply::ContentOperationNotPermittedError ||
          reply->error() == QNetworkReply::AuthenticationRequiredError ||
          error_code == ScrobbleErrorCode::InvalidSessionKey ||
          error_code == ScrobbleErrorCode::AuthenticationFailed
        ){
        // Session is probably expired
        Logout();
        Error(error_reason);
      }
      else if (reply->error() == QNetworkReply::ContentNotFoundError) { // Ignore this error
        Error(error_reason);
      }
      else { // Fail
        Error(error_reason);
      }
    }
    return QByteArray();
  }

  return data;
  
}

void ScrobblingAPI20::UpdateNowPlaying(const Song &song) {

  song_playing_ = song;
  timestamp_ = QDateTime::currentDateTime().toTime_t();

  if (!IsAuthenticated() || !song.is_metadata_good()) return;

  QString album = song.album();
  album = album.remove(Song::kAlbumRemoveDisc);
  album = album.remove(Song::kAlbumRemoveMisc);

  ParamList params = ParamList()
    << Param("method", "track.updateNowPlaying")
    << Param("artist", song.artist())
    << Param("track", song.title())
    << Param("album", album);

  QNetworkReply *reply = CreateRequest(params);
  NewClosure(reply, SIGNAL(finished()), this, SLOT(UpdateNowPlayingRequestFinished(QNetworkReply*)), reply);

}

void ScrobblingAPI20::UpdateNowPlayingRequestFinished(QNetworkReply *reply) {

  reply->deleteLater();

  QByteArray data = GetReplyData(reply);
  if (data.isEmpty()) {
    return;
  }

  QJsonObject json_obj = ExtractJsonObj(data);
  if (json_obj.isEmpty()) {
    return;
  }

  if (json_obj.contains("error") && json_obj.contains("message")) {
    int error_code = json_obj["error"].toInt();
    QString error_message = json_obj["message"].toString();
    QString error_reason = QString("%1 (%2)").arg(error_message).arg(error_code);
    Error(error_reason);
    return;
  }

  if (!json_obj.contains("nowplaying")) {
    Error("Json reply from server is missing nowplaying.", json_obj);
    return;
  }

}

void ScrobblingAPI20::Scrobble(const Song &song) {

  if (song.id() != song_playing_.id() || song.url() != song_playing_.url() || !song.is_metadata_good()) return;

  cache()->Add(song, timestamp_);

  if (app_->scrobbler()->IsOffline()) return;

  if (!IsAuthenticated()) {
    emit ErrorMessage(tr("Scrobbler %1 is not authenticated!").arg(name_));
    return;
  }

  if (!submitted_) {
    submitted_ = true;
    if (!batch_ || app_->scrobbler()->SubmitDelay() <= 0) {
      Submit();
    }
    else {
      qint64 msec = (app_->scrobbler()->SubmitDelay() * 60 * kMsecPerSec);
      DoAfter(this, SLOT(Submit()), msec);
    }
  }

}

void ScrobblingAPI20::DoSubmit() {

  if (!submitted_ && cache()->Count() > 0) {
    submitted_ = true;
    qint64 msec = 30000ll;
    if (app_->scrobbler()->SubmitDelay() != 0) msec = (app_->scrobbler()->SubmitDelay() * 60 * kMsecPerSec);
    DoAfter(this, SLOT(Submit()), msec);
  }

}

void ScrobblingAPI20::Submit() {

  qLog(Debug) << __PRETTY_FUNCTION__ << name_;

  submitted_ = false;

  if (!IsEnabled() || !IsAuthenticated() || app_->scrobbler()->IsOffline()) return;

  ParamList params = ParamList()
    << Param("method", "track.scrobble");

  int i(0);
  QList<quint64> list;
  for (ScrobblerCacheItem *item : cache()->List()) {
    if (item->sent_) continue;
    item->sent_ = true;
    if (!batch_) { SendSingleScrobble(item); continue; }
    i++;
    list << item->timestamp_;
    params << Param(QString("%1[%2]").arg("artist").arg(i), item->artist_);
    params << Param(QString("%1[%2]").arg("album").arg(i), item->album_);
    params << Param(QString("%1[%2]").arg("track").arg(i), item->song_);
    params << Param(QString("%1[%2]").arg("timestamp").arg(i), QString::number(item->timestamp_));
    params << Param(QString("%1[%2]").arg("duration").arg(i), QString::number(item->duration_ / kNsecPerSec));
    if (!item->albumartist_.isEmpty()) params << Param(QString("%1[%2]").arg("albumArtist").arg(i), item->albumartist_);
    if (item->track_ > 0) params << Param(QString("%1[%2]").arg(i).arg("trackNumber"), QString::number(item->track_));
    if (i >= kScrobblesPerRequest) break;
  }

  if (!batch_ || i <= 0) return;

  QNetworkReply *reply = CreateRequest(params);
  NewClosure(reply, SIGNAL(finished()), this, SLOT(ScrobbleRequestFinished(QNetworkReply*, QList<quint64>)), reply, list);

}

void ScrobblingAPI20::ScrobbleRequestFinished(QNetworkReply *reply, QList<quint64> list) {

  reply->deleteLater();

  QByteArray data = GetReplyData(reply);
  if (data.isEmpty()) {
    cache()->ClearSent(list);
    DoSubmit();
    return;
  }

  QJsonObject json_obj = ExtractJsonObj(data);
  if (json_obj.isEmpty()) {
    cache()->ClearSent(list);
    DoSubmit();
    return;
  }

  if (json_obj.contains("error") && json_obj.contains("message")) {
    int error_code = json_obj["error"].toInt();
    QString error_message = json_obj["message"].toString();
    QString error_reason = QString("%1 (%2)").arg(error_message).arg(error_code);
    Error(error_reason);
    cache()->ClearSent(list);
    DoSubmit();
    return;
  }

  if (!json_obj.contains("scrobbles")) {
    Error("Json reply from server is missing scrobbles.", json_obj);
    cache()->ClearSent(list);
    DoSubmit();
    return;
  }

  cache()->Flush(list);

  QJsonValue json_scrobbles = json_obj["scrobbles"];
  if (!json_scrobbles.isObject()) {
    Error("Json scrobbles is not an object.", json_obj);
    DoSubmit();
    return;
  }
  json_obj = json_scrobbles.toObject();
  if (json_obj.isEmpty()) {
    Error("Json scrobbles object is empty.", json_scrobbles);
    DoSubmit();
    return;
  }
  if (!json_obj.contains("@attr") || !json_obj.contains("scrobble")) {
    Error("Json scrobbles object is missing values.", json_obj);
    DoSubmit();
    return;
  }

  QJsonValue json_attr = json_obj["@attr"];
  if (!json_attr.isObject()) {
    Error("Json scrobbles attr is not an object.", json_attr);
    DoSubmit();
    return;
  }
  QJsonObject json_obj_attr = json_attr.toObject();
  if (json_obj_attr.isEmpty()) {
    Error("Json scrobbles attr is empty.", json_attr);
    DoSubmit();
    return;
  }
  if (!json_obj_attr.contains("accepted") || !json_obj_attr.contains("ignored")) {
    Error("Json scrobbles attr is missing values.", json_obj_attr);
    DoSubmit();
    return;
  }
  int accepted = json_obj_attr["accepted"].toInt();
  int ignored = json_obj_attr["ignored"].toInt();
  qLog(Debug) << name_ << "Scrobbles accepted:" << accepted << "ignored:" << ignored;

  QJsonValue json_scrobble = json_obj["scrobble"];
  if (!json_scrobble.isArray()) {
    Error("Json scrobbles scrobble is not array.", json_scrobble);
    DoSubmit();
    return;
  }
  QJsonArray json_array_scrobble = json_scrobble.toArray();
  if (json_array_scrobble.isEmpty()) {
    Error("Json scrobbles scrobble array is empty.", json_scrobble);
    DoSubmit();
    return;
  }
  
  for (const QJsonValue &value : json_array_scrobble) {

    if (!value.isObject()) {
      Error("Json scrobbles scrobble array value is not an object.", value);
      continue;
    }
    QJsonObject json_track = value.toObject();
    if (json_track.isEmpty()) {
      continue;
    }

    if (!json_track.contains("artist") ||
        !json_track.contains("album") ||
        !json_track.contains("albumArtist") ||
        !json_track.contains("track") ||
        !json_track.contains("timestamp") ||
        !json_track.contains("ignoredMessage")
    ) {
      Error("Json scrobbles scrobble is missing values.", json_track);
      continue;
    }

    QJsonValue json_value_artist = json_track["artist"];
    QJsonValue json_value_album = json_track["album"];
    QJsonValue json_value_song = json_track["track"];
    QJsonValue json_value_ignoredmessage = json_track["ignoredMessage"];
    //quint64 timestamp = json_track["timestamp"].toVariant().toULongLong();

    if (!json_value_artist.isObject() || !json_value_album.isObject() || !json_value_song.isObject() || !json_value_ignoredmessage.isObject()) {
      Error("Json scrobbles scrobble values are not objects.", json_track);
      continue;
    }

    QJsonObject json_obj_artist = json_value_artist.toObject();
    QJsonObject json_obj_album = json_value_album.toObject();
    QJsonObject json_obj_song = json_value_song.toObject();
    QJsonObject json_obj_ignoredmessage = json_value_ignoredmessage.toObject();

    if (json_obj_artist.isEmpty() || json_obj_album.isEmpty() || json_obj_song.isEmpty() || json_obj_ignoredmessage.isEmpty()) {
      Error("Json scrobbles scrobble values objects are empty.", json_track);
      continue;
    }

    if (!json_obj_artist.contains("#text") || !json_obj_album.contains("#text") || !json_obj_song.contains("#text")) {
      // Just ignore this, as Last.fm seem to return 1 ignored scrobble for a blank song for each requst (no idea why).
      continue;
    }

    QString artist = json_obj_artist["#text"].toString();
    QString album = json_obj_album["#text"].toString();
    QString song = json_obj_song["#text"].toString();
    bool ignoredmessage = json_obj_ignoredmessage["code"].toVariant().toBool();
    QString ignoredmessage_text = json_obj_ignoredmessage["#text"].toString();

    if (ignoredmessage) {
      Error(QString("Scrobble for \"%1\" ignored: %2").arg(song).arg(ignoredmessage_text));
    }
    else {
      qLog(Debug) << name_ << "Scrobble for" << song << "accepted";
    }

 }

  DoSubmit();

}

void ScrobblingAPI20::SendSingleScrobble(ScrobblerCacheItem *item) {

  ParamList params = ParamList()
    << Param("method", "track.scrobble")
    << Param("artist", item->artist_)
    << Param("album", item->album_)
    << Param("track", item->song_)
    << Param("timestamp", QString::number(item->timestamp_))
    << Param("duration", QString::number(item->duration_ / kNsecPerSec));

  if (!item->albumartist_.isEmpty()) params << Param("albumArtist", item->albumartist_);
  if (item->track_ > 0) params << Param("trackNumber", QString::number(item->track_));

  QNetworkReply *reply = CreateRequest(params);
  NewClosure(reply, SIGNAL(finished()), this, SLOT(SingleScrobbleRequestFinished(QNetworkReply*, quint64)), reply, item->timestamp_);

}

void ScrobblingAPI20::SingleScrobbleRequestFinished(QNetworkReply *reply, quint64 timestamp) {

  reply->deleteLater();

  ScrobblerCacheItem *item = cache()->Get(timestamp);
  if (!item) {
    Error(QString("Received reply for non-existing cache entry %1.").arg(timestamp));
    return;
  }

  QByteArray data = GetReplyData(reply);
  if (data.isEmpty()) {
    item->sent_ = false;
    return;
  }

  QJsonObject json_obj = ExtractJsonObj(data);
  if (json_obj.isEmpty()) {
    item->sent_ = false;
    return;
  }

  if (json_obj.contains("error") && json_obj.contains("message")) {
    int error_code = json_obj["error"].toInt();
    QString error_message = json_obj["message"].toString();
    QString error_reason = QString("%1 (%2)").arg(error_message).arg(error_code);
    Error(error_reason);
    item->sent_ = false;
    return;
  }

  if (!json_obj.contains("scrobbles")) {
    Error("Json reply from server is missing scrobbles.", json_obj);
    item->sent_ = false;
    return;
  }

  cache()->Remove(timestamp);
  item = nullptr;

  QJsonValue json_scrobbles = json_obj["scrobbles"];
  if (!json_scrobbles.isObject()) {
    Error("Json scrobbles is not an object.", json_obj);
    return;
  }
  json_obj = json_scrobbles.toObject();
  if (json_obj.isEmpty()) {
    Error("Json scrobbles object is empty.", json_scrobbles);
    return;
  }
  if (!json_obj.contains("@attr") || !json_obj.contains("scrobble")) {
    Error("Json scrobbles object is missing values.", json_obj);
    return;
  }

  QJsonValue json_attr = json_obj["@attr"];
  if (!json_attr.isObject()) {
    Error("Json scrobbles attr is not an object.", json_attr);
    return;
  }
  QJsonObject json_obj_attr = json_attr.toObject();
  if (json_obj_attr.isEmpty()) {
    Error("Json scrobbles attr is empty.", json_attr);
    return;
  }

  QJsonValue json_scrobble = json_obj["scrobble"];
  if (!json_scrobble.isObject()) {
    Error("Json scrobbles scrobble is not an object.", json_scrobble);
    return;
  }
  QJsonObject json_obj_scrobble = json_scrobble.toObject();
  if (json_obj_scrobble.isEmpty()) {
    Error("Json scrobbles scrobble is empty.", json_scrobble);
    return;
  }

  if (!json_obj_attr.contains("accepted") || !json_obj_attr.contains("ignored")) {
    Error("Json scrobbles attr is missing values.", json_obj_attr);
    return;
  }

  if (!json_obj_scrobble.contains("artist") || !json_obj_scrobble.contains("album") || !json_obj_scrobble.contains("albumArtist") || !json_obj_scrobble.contains("track") || !json_obj_scrobble.contains("timestamp")) {
    Error("Json scrobbles scrobble is missing values.", json_obj_scrobble);
    return;
  }

  QJsonValue json_value_artist = json_obj_scrobble["artist"];
  QJsonValue json_value_album = json_obj_scrobble["album"];
  QJsonValue json_value_song = json_obj_scrobble["track"];

  if (!json_value_artist.isObject() || !json_value_album.isObject() || !json_value_song.isObject()) {
    Error("Json scrobbles scrobble values are not objects.", json_obj_scrobble);
    return;
  }

  QJsonObject json_obj_artist = json_value_artist.toObject();
  QJsonObject json_obj_album = json_value_album.toObject();
  QJsonObject json_obj_song = json_value_song.toObject();

  if (json_obj_artist.isEmpty() || json_obj_album.isEmpty() || json_obj_song.isEmpty()) {
    Error("Json scrobbles scrobble values objects are empty.", json_obj_scrobble);
    return;
  }

  if (!json_obj_artist.contains("#text") || !json_obj_album.contains("#text") || !json_obj_song.contains("#text")) {
    Error("Json scrobbles scrobble values objects are missing #text.", json_obj_artist);
    return;
  }

  QString artist = json_obj_artist["#text"].toString();
  QString album = json_obj_album["#text"].toString();
  QString song = json_obj_song["#text"].toString();

  int accepted = json_obj_attr["accepted"].toVariant().toInt();
  if (accepted == 1) {
    qLog(Debug) << name_ << "Scrobble for" << song << "accepted";
  }
  else {
    Error(QString("Scrobble for \"%1\" not accepted").arg(song));
  }

}

void ScrobblingAPI20::Love(const Song &song) {

  if (!IsAuthenticated()) app_->scrobbler()->ShowConfig();

  ParamList params = ParamList()
    << Param("method", "track.love")
    << Param("artist", song.artist())
    << Param("track", song.title())
    << Param("album", song.album());

  QNetworkReply *reply = CreateRequest(params);
  NewClosure(reply, SIGNAL(finished()), this, SLOT(RequestFinished(QNetworkReply*)), reply);

}

void ScrobblingAPI20::AuthError(QString error) {
  emit AuthenticationComplete(false, error);
}

void ScrobblingAPI20::Error(QString error, QVariant debug) {

  qLog(Error) << name_ << error;
  if (debug.isValid()) qLog(Debug) << debug;

}

QString ScrobblingAPI20::ErrorString(ScrobbleErrorCode error) const {

  switch (error) {
    case ScrobbleErrorCode::InvalidService:
      return QString("Invalid service - This service does not exist");
    case ScrobbleErrorCode::InvalidMethod:
      return QString("Invalid Method - No method with that name in this package");
    case ScrobbleErrorCode::AuthenticationFailed:
      return QString("Authentication Failed - You do not have permissions to access the service");
    case ScrobbleErrorCode::InvalidFormat:
      return QString("Invalid format - This service doesn't exist in that format");
    case ScrobbleErrorCode::InvalidParameters:
      return QString("Invalid parameters - Your request is missing a required parameter");
    case ScrobbleErrorCode::InvalidResourceSpecified:
      return QString("Invalid resource specified");
    case ScrobbleErrorCode::OperationFailed:
      return QString("Operation failed - Something else went wrong");
    case ScrobbleErrorCode::InvalidSessionKey:
      return QString("Invalid session key - Please re-authenticate");
    case ScrobbleErrorCode::InvalidApiKey:
      return QString("Invalid API key - You must be granted a valid key by last.fm");
    case ScrobbleErrorCode::ServiceOffline:
      return QString("Service Offline - This service is temporarily offline. Try again later.");
    case ScrobbleErrorCode::InvalidMethodSignature:
        return QString("Invalid method signature supplied");
    case ScrobbleErrorCode::TempError:
      return QString("There was a temporary error processing your request. Please try again");
    case ScrobbleErrorCode::SuspendedAPIKey:
      return QString("Suspended API key - Access for your account has been suspended, please contact Last.fm");
    case ScrobbleErrorCode::RateLimitExceeded:
      return QString("Rate limit exceeded - Your IP has made too many requests in a short period");
    default:
      return QString("Unknown error");
  }

}
