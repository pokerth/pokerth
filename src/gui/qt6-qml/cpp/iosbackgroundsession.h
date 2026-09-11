/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2026 Felix Hammer, Florian Thauer, Lothar May          *
 *****************************************************************************/

#ifndef IOSBACKGROUNDSESSION_H
#define IOSBACKGROUNDSESSION_H

// iOS counterpart to AndroidConnectionService.
//
// IMPORTANT - the platforms differ fundamentally:
// Android can keep the process alive permanently via a foreground service;
// the server connection thus survives arbitrarily long background phases.
// Such a construct does NOT exist on iOS. If the user switches apps
// (e.g. briefly to WhatsApp) or locks the device, the process is frozen after
// a short while: no timers, no socket I/O, and the TCP connection
// then dies silently - exactly the observed "freeze" in which the GUI still
// reacts but nothing comes from the server any more.
//
// The only thing iOS offers is beginBackgroundTask: a grace period of
// typically ~30 seconds during which the app may keep running after the
// switch. That covers the most frequent case - looking into another app
// briefly and coming back. It can NOT cover a longer absence; that needs
// the detection on returning (resume probe in pokerth.qml).
//
// start()/stop() mark "there is an active online session" and are called at
// the same places as AndroidConnectionService. Requesting and releasing the
// grace period is done by the implementation itself, based on the
// UIApplication notifications.
//
// On all other platforms both functions are no-ops. They are here in the
// header as inline definitions because iosbackgroundsession.mm only belongs
// to the target in the iOS branch of the CMakeLists (a .mm cannot be compiled
// elsewhere) - otherwise the callers would find no symbols when linking.
#include <QtGlobal>

namespace IosBackgroundSession
{
#ifdef Q_OS_IOS
void start();
void stop();
#else
inline void start() {}
inline void stop() {}
#endif
}

#endif // IOSBACKGROUNDSESSION_H
