/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2026 Felix Hammer, Florian Thauer, Lothar May          *
 *****************************************************************************/

#ifndef ANDROIDCONNECTIONSERVICE_H
#define ANDROIDCONNECTIONSERVICE_H

// Start/stop of the Android foreground service (ConnectionService.java), which
// protects the app process from doze/app freezer during an active online
// session, so that the server connection survives background phases.
// On all other platforms both functions are no-ops.
//
// Threading: callable from arbitrary threads (including the network thread,
// e.g. from QmlGuiInterface callbacks) - the JNI call is moved onto the
// Android UI thread.
namespace AndroidConnectionService
{
void start();
void stop();
}

#endif // ANDROIDCONNECTIONSERVICE_H
