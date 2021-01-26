/* This file is part of Strawberry.
   Copyright 2011, David Sansome <me@davidsansome.com>

   Strawberry is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Strawberry is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Strawberry.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <QObject>
#include <QList>
#include <QTimer>
#include <QGenericArgument>
#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
#  include <QRandomGenerator>
#endif

#include "closure.h"

#include "core/timeconstants.h"

namespace _detail {

ClosureBase::ClosureBase(ObjectHelper *helper) : helper_(helper) {}

ClosureBase::~ClosureBase() {}

CallbackClosure::CallbackClosure(QObject *sender, const char *signal, std::function<void()> callback)
    : ClosureBase(new ObjectHelper(sender, signal, this)),
    callback_(callback) {
}

void CallbackClosure::Invoke() {
  callback_();
}

ObjectHelper* ClosureBase::helper() const {
  return helper_;
}

ObjectHelper::ObjectHelper(QObject *sender, const char *signal, ClosureBase *closure) : closure_(closure) {

  QObject::connect(sender, signal, SLOT(Invoked()));
  QObject::connect(sender, &QObject::destroyed, this, &ObjectHelper::deleteLater);

}

ObjectHelper::~ObjectHelper() {}

void ObjectHelper::Invoked() {
  closure_->Invoke();
  deleteLater();
}

void Unpack(QList<QGenericArgument>*) {}

}  // namespace _detail

_detail::ClosureBase* NewClosure(QObject *sender, const char *signal, std::function<void()> callback) {
  return new _detail::CallbackClosure(sender, signal, callback);
}

void DoAfter(QObject *receiver, const char *slot, int msec) {
  QTimer::singleShot(msec, receiver, slot);
}

void DoInAMinuteOrSo(QObject *receiver, const char *slot) {

#if QT_VERSION >= QT_VERSION_CHECK(5, 10, 0)
  int msec = (60 + QRandomGenerator::global()->bounded(1, 60)) * kMsecPerSec;
#else
  int msec = (60 + (qrand() % 60)) * kMsecPerSec;
#endif

  DoAfter(receiver, slot, msec);

}
