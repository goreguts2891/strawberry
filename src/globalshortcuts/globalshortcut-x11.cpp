/*
 * Strawberry Music Player
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

#include "config.h"

#include <QtGlobal>
#include <QApplication>
#include <QMap>
#include <QVector>
#include <QByteArray>
#include <QString>
#include <QKeySequence>
#include <QFlags>
#include <QScreen>

#include "globalshortcut.h"
#include "keymapper_x11.h"

#include <X11/X.h>
#include <X11/Xlib.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>

#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)
#  include <QGuiApplication>
#elif defined(HAVE_X11EXTRAS)
#  include <QX11Info>
#elif defined(HAVE_QPA_QPLATFORMNATIVEINTERFACE_H)
#  include <qpa/qplatformnativeinterface.h>
#else
#  error "Missing Qt >= 6.2, X11Extras or qpa/qplatformnativeinterface.h header."
#endif

const QVector<quint32> GlobalShortcut::mask_modifiers_ = QVector<quint32>() << 0 << Mod2Mask << LockMask << (Mod2Mask | LockMask);

namespace {

Display *X11Display() {

#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)  // 6.2: Use the new native interface.

  if (!qApp) return nullptr;

  if (QNativeInterface::QX11Application *x11_app = qApp->nativeInterface<QNativeInterface::QX11Application>()) {
   return x11_app->display();
  }
  else {
    return nullptr;
  }

#elif defined(HAVE_X11EXTRAS)  // Qt 5: Use X11Extras

  return QX11Info::display();

#elif defined(HAVE_QPA_QPLATFORMNATIVEINTERFACE_H)  // Use private headers.

  if (!qApp) return nullptr;

  QPlatformNativeInterface *native = QGuiApplication::platformNativeInterface();
  if (!native) return nullptr;

  void *display = native->nativeResourceForIntegration("display");
  return reinterpret_cast<Display*>(display);

#else

#  error "Missing Qt >= 6.2, X11Extras or qpa/qplatformnativeinterface.h header."

#endif

}

quint32 AppRootWindow() {

#if QT_VERSION >= QT_VERSION_CHECK(6, 2, 0)  // 6.2: Use the new native interface.

  if (QNativeInterface::QX11Application *x11_app = qApp->nativeInterface<QNativeInterface::QX11Application>()) {
    return static_cast<xcb_window_t>(reinterpret_cast<quintptr>(x11_app->connection()));
  }
  else {
    return 0;
  }

#elif defined(HAVE_X11EXTRAS)  // Qt 5: Use X11Extras

  return QX11Info::appRootWindow();

#elif defined(HAVE_QPA_QPLATFORMNATIVEINTERFACE_H)  // Use private headers.

  if (!qApp) return 0;

  QPlatformNativeInterface *native = QGuiApplication::platformNativeInterface();
  if (!native) return 0;

  QScreen *screen = QGuiApplication::primaryScreen();
  if (!screen) return 0;

  return static_cast<xcb_window_t>(reinterpret_cast<quintptr>(native->nativeResourceForScreen("rootwindow", screen)));

#else

#  error "Missing Qt >= 6.2, X11Extras or qpa/qplatformnativeinterface.h header."

#endif

}

}  // namespace

quint32 GlobalShortcut::nativeModifiers(Qt::KeyboardModifiers qt_mods) {

  quint32 native_mods = 0;
  if (qt_mods & Qt::ShiftModifier)    native_mods |= ShiftMask;
  if (qt_mods & Qt::ControlModifier)  native_mods |= ControlMask;
  if (qt_mods & Qt::AltModifier)      native_mods |= Mod1Mask;
  if (qt_mods & Qt::MetaModifier)     native_mods |= Mod4Mask;
  return native_mods;

}

quint32 GlobalShortcut::nativeKeycode(Qt::Key qt_key) {

  Display *disp = X11Display();
  if (!disp) return false;

  quint32 keysym = 0;
  if (KeyMapperX11::keymapper_x11_.contains(qt_key)) {
    keysym = KeyMapperX11::keymapper_x11_.value(qt_key);
  }
  else {
    keysym = XStringToKeysym(QKeySequence(qt_key).toString().toLatin1().data());
    if (keysym == NoSymbol) return 0;
  }
  return XKeysymToKeycode(disp, keysym);

}

bool GlobalShortcut::registerShortcut(quint32 native_key, quint32 native_mods) {

  Display *disp = X11Display();
  if (!disp) return false;

  for (quint32 mask_mods : mask_modifiers_) {
    XGrabKey(disp, native_key, (native_mods | mask_mods), AppRootWindow(), True, GrabModeAsync, GrabModeAsync);
  }
  return true;

}

bool GlobalShortcut::unregisterShortcut(quint32 native_key, quint32 native_mods) {

  Display *disp = X11Display();
  if (!disp) return false;

  for (quint32 mask_mods : mask_modifiers_) {
    XUngrabKey(disp, native_key, native_mods | mask_mods, AppRootWindow());
  }
  return true;

}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
bool GlobalShortcut::nativeEventFilter(const QByteArray &eventtype, void *message, qintptr *result) {
#else
bool GlobalShortcut::nativeEventFilter(const QByteArray &eventtype, void *message, long *result) {
#endif

  Q_UNUSED(eventtype);
  Q_UNUSED(result);

  xcb_generic_event_t *event = static_cast<xcb_generic_event_t *>(message);
  if ((event->response_type & 127) != XCB_KEY_PRESS) return false;

  xcb_key_press_event_t *key_press_event = static_cast<xcb_key_press_event_t *>(message);
  if (!key_press_event) return false;

  quint32 keycode = key_press_event->detail;
  unsigned int keystate = 0;
  if (key_press_event->state & XCB_MOD_MASK_1) keystate |= Mod1Mask;
  if (key_press_event->state & XCB_MOD_MASK_CONTROL) keystate |= ControlMask;
  if (key_press_event->state & XCB_MOD_MASK_4) keystate |= Mod4Mask;
  if (key_press_event->state & XCB_MOD_MASK_SHIFT) keystate |= ShiftMask;
  activateShortcut(keycode, keystate & (ShiftMask | ControlMask | Mod1Mask | Mod4Mask));

  return false;

}
