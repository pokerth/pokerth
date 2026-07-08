/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2026 Felix Hammer, Florian Thauer, Lothar May          *
 *****************************************************************************/

#ifndef ANDROIDCONNECTIONSERVICE_H
#define ANDROIDCONNECTIONSERVICE_H

// Start/Stop des Android-Foreground-Service (ConnectionService.java), der
// während einer aktiven Online-Session den App-Prozess vor Doze/App-Freezer
// schützt, damit die Server-Verbindung Hintergrund-Phasen überlebt.
// Auf allen anderen Plattformen sind beide Funktionen No-ops.
//
// Threading: von beliebigen Threads aufrufbar (auch vom Netzwerk-Thread,
// z.B. aus QmlGuiInterface-Callbacks) - der JNI-Aufruf wird auf den
// Android-UI-Thread verlagert.
namespace AndroidConnectionService
{
void start();
void stop();
}

#endif // ANDROIDCONNECTIONSERVICE_H
