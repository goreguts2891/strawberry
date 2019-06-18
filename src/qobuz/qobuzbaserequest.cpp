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

#include "config.h"

#include <QObject>
#include <QByteArray>
#include <QPair>
#include <QList>
#include <QString>
#include <QUrl>
#include <QUrlQuery>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>

#include "core/logging.h"
#include "core/network.h"
#include "qobuzservice.h"
#include "qobuzbaserequest.h"

const char *QobuzBaseRequest::kApiUrl = "http://www.qobuz.com/api.json/0.2";

QobuzBaseRequest::QobuzBaseRequest(QobuzService *service, NetworkAccessManager *network, QObject *parent) :
      QObject(parent),
      service_(service),
      network_(network)
      {}

QobuzBaseRequest::~QobuzBaseRequest() {

  while (!replies_.isEmpty()) {
    QNetworkReply *reply = replies_.takeFirst();
    disconnect(reply, 0, nullptr, 0);
    if (reply->isRunning()) reply->abort();
    reply->deleteLater();
  }

}

QNetworkReply *QobuzBaseRequest::CreateRequest(const QString &ressource_name, const QList<Param> &params_provided) {

  ParamList params = ParamList() << params_provided
                                 << Param("app_id", app_id());

  std::sort(params.begin(), params.end());

  QUrlQuery url_query;
  for (const Param& param : params) {
    EncodedParam encoded_param(QUrl::toPercentEncoding(param.first), QUrl::toPercentEncoding(param.second));
    url_query.addQueryItem(encoded_param.first, encoded_param.second);
  }

  QUrl url(kApiUrl + QString("/") + ressource_name);
  url.setQuery(url_query);
  QNetworkRequest req(url);
  req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

  QNetworkReply *reply = network_->get(req);
  replies_ << reply;

  //qLog(Debug) << "Qobuz: Sending request" << url;

  return reply;

}

QByteArray QobuzBaseRequest::GetReplyData(QNetworkReply *reply, QString &error) {

  if (replies_.contains(reply)) {
    replies_.removeAll(reply);
    reply->deleteLater();
  }

  QByteArray data;

  if (reply->error() == QNetworkReply::NoError) {
    int http_code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (http_code == 200) {
      data = reply->readAll();
    }
    else {
      error = Error(QString("Received HTTP code %1").arg(http_code));
    }
  }
  else {
    if (reply->error() < 200) {
      // This is a network error, there is nothing more to do.
      error = Error(QString("%1 (%2)").arg(reply->errorString()).arg(reply->error()));
    }
    else {
      // See if there is Json data containing "status", "code" and "message" - then use that instead.
      data = reply->readAll();
      QJsonParseError parse_error;
      QJsonDocument json_doc = QJsonDocument::fromJson(data, &parse_error);
      QString failure_reason;
      if (parse_error.error == QJsonParseError::NoError && !json_doc.isNull() && !json_doc.isEmpty() && json_doc.isObject()) {
        QJsonObject json_obj = json_doc.object();
        if (!json_obj.isEmpty() && json_obj.contains("status") && json_obj.contains("code") && json_obj.contains("message")) {
          QString status = json_obj["status"].toString();
          int code = json_obj["code"].toInt();
          QString message = json_obj["message"].toString();
          failure_reason = QString("%1 (%2)").arg(message).arg(code);
        }
      }
      if (failure_reason.isEmpty()) {
        failure_reason = QString("%1 (%2)").arg(reply->errorString()).arg(reply->error());
      }
      error = Error(failure_reason);
    }
    return QByteArray();
  }

  return data;

}

QJsonObject QobuzBaseRequest::ExtractJsonObj(QByteArray &data, QString &error) {

  QJsonParseError json_error;
  QJsonDocument json_doc = QJsonDocument::fromJson(data, &json_error);

  if (json_error.error != QJsonParseError::NoError) {
    error = Error("Reply from server missing Json data.", data);
    return QJsonObject();
  }

  if (json_doc.isNull() || json_doc.isEmpty()) {
    error = Error("Received empty Json document.", data);
    return QJsonObject();
  }

  if (!json_doc.isObject()) {
    error = Error("Json document is not an object.", json_doc);
    return QJsonObject();
  }

  QJsonObject json_obj = json_doc.object();
  if (json_obj.isEmpty()) {
    error = Error("Received empty Json object.", json_doc);
    return QJsonObject();
  }

  return json_obj;

}

QJsonValue QobuzBaseRequest::ExtractItems(QByteArray &data, QString &error) {

  QJsonObject json_obj = ExtractJsonObj(data, error);
  if (json_obj.isEmpty()) return QJsonValue();
  return ExtractItems(json_obj, error);

}

QJsonValue QobuzBaseRequest::ExtractItems(QJsonObject &json_obj, QString &error) {

  if (!json_obj.contains("items")) {
    error = Error("Json reply is missing items.", json_obj);
    return QJsonArray();
  }
  QJsonValue json_items = json_obj["items"];
  return json_items;

}

QString QobuzBaseRequest::Error(QString error, QVariant debug) {

  qLog(Error) << "Qobuz:" << error;
  if (debug.isValid()) qLog(Debug) << debug;

  return error;

}
