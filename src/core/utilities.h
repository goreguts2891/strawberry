/*
 * Strawberry Music Player
 * This file was part of Clementine.
 * Copyright 2010, David Sansome <me@davidsansome.com>
 * Copyright 2018-2021, Jonas Kvinge <jonas@jkvinge.net>
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

#ifndef UTILITIES_H
#define UTILITIES_H

#include "config.h"

#include <memory>

#include <QtGlobal>
#include <QWindow>
#include <QByteArray>
#include <QFile>
#include <QSize>
#include <QDateTime>
#include <QLocale>
#include <QList>
#include <QMetaObject>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QColor>
#include <QRegion>
#include <QtEvents>
#include <QCryptographicHash>

#include "core/song.h"

class QWidget;
class QIODevice;
class QXmlStreamReader;

namespace Utilities {
QString PrettyTime(int seconds);
QString PrettyTimeDelta(const int seconds);
QString PrettyTimeNanosec(const qint64 nanoseconds);
QString PrettySize(const quint64 bytes);
QString PrettySize(const QSize size);
QString WordyTime(const quint64 seconds);
QString WordyTimeNanosec(const quint64 nanoseconds);
QString Ago(const qint64 seconds_since_epoch, const QLocale &locale);
QString PrettyFutureDate(const QDate date);

QString ColorToRgba(const QColor &color);

quint64 FileSystemCapacity(const QString &path);
quint64 FileSystemFreeSpace(const QString &path);

bool MoveToTrashRecursive(const QString &path);
bool RemoveRecursive(const QString &path);
bool CopyRecursive(const QString &source, const QString &destination);
bool Copy(QIODevice *source, QIODevice *destination);

void OpenInFileBrowser(const QList<QUrl> &urls);

QByteArray Hmac(const QByteArray &key, const QByteArray &data, const QCryptographicHash::Algorithm method);
QByteArray HmacMd5(const QByteArray &key, const QByteArray &data);
QByteArray HmacSha256(const QByteArray &key, const QByteArray &data);
QByteArray HmacSha1(const QByteArray &key, const QByteArray &data);
QByteArray Sha1CoverHash(const QString &artist, const QString &album);

// Reads all children of the current element,
// and returns with the stream reader either on the EndElement for the current element, or the end of the file - whichever came first.
void ConsumeCurrentElement(QXmlStreamReader *reader);

// Advances the stream reader until it finds an element with the given name.
// Returns false if the end of the document was reached before finding a matching element.
bool ParseUntilElement(QXmlStreamReader *reader, const QString &name);
bool ParseUntilElementCI(QXmlStreamReader *reader, const QString &name);

// Parses a string containing an RFC822 time and date.
QDateTime ParseRFC822DateTime(const QString &text);

// Replaces some HTML entities with their normal characters.
QString DecodeHtmlEntities(const QString &text);

// Shortcut for getting a Qt-aware enum value as a string.
// Pass in the QMetaObject of the class that owns the enum, the string name of the enum and a valid value from that enum.
const char *EnumToString(const QMetaObject &meta, const char *name, int value);

QStringList Prepend(const QString &text, const QStringList &list);
QStringList Updateify(const QStringList &list);

// Get the path without the filename extension
QString PathWithoutFilenameExtension(const QString &filename);
QString FiddleFileExtension(const QString &filename, const QString &new_extension);

QString GetEnv(const QString &key);
void SetEnv(const char *key, const QString &value);
void IncreaseFDLimit();

// Borrowed from schedutils
enum IoPriority {
  IOPRIO_CLASS_NONE = 0,
  IOPRIO_CLASS_RT,
  IOPRIO_CLASS_BE,
  IOPRIO_CLASS_IDLE,

};
enum {
  IOPRIO_WHO_PROCESS = 1,
  IOPRIO_WHO_PGRP,
  IOPRIO_WHO_USER,

};
static const int IOPRIO_CLASS_SHIFT = 13;

long SetThreadIOPriority(const IoPriority priority);
long GetThreadId();

QString GetRandomStringWithChars(const int len);
QString GetRandomStringWithCharsAndNumbers(const int len);
QString CryptographicRandomString(const int len);
QString GetRandomString(const int len, const QString &UseCharacters);

QString DesktopEnvironment();

QString UnicodeToAscii(QString unicode);

QString MacAddress();

QString ReplaceMessage(const QString &message, const Song &song, const QString &newline, const bool html_escaped = false);
QString ReplaceVariable(const QString &variable, const Song &song, const QString &newline, const bool html_escaped = false);

bool IsColorDark(const QColor &color);

QByteArray ReadDataFromFile(const QString &filename);
QString MimeTypeFromData(const QByteArray &data);

#ifdef Q_OS_WIN
void enableBlurBehindWindow(QWindow *window, const QRegion &region);
#endif


}  // namespace Utilities

class ScopedWCharArray {
 public:
  explicit ScopedWCharArray(const QString &str);

  QString ToString() const { return QString::fromWCharArray(data_.get()); }

  wchar_t *get() const { return data_.get(); }
  explicit operator wchar_t *() const { return get(); }

  qint64 characters() const { return chars_; }
  qint64 bytes() const { return (chars_ + 1) * sizeof(wchar_t); }

 private:
  Q_DISABLE_COPY(ScopedWCharArray)

  qint64 chars_;
  std::unique_ptr<wchar_t[]> data_;

};

#endif  // UTILITIES_H
