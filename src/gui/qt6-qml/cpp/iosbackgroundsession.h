/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2026 Felix Hammer, Florian Thauer, Lothar May          *
 *****************************************************************************/

#ifndef IOSBACKGROUNDSESSION_H
#define IOSBACKGROUNDSESSION_H

// iOS-Gegenstueck zu AndroidConnectionService.
//
// WICHTIG - die Plattformen unterscheiden sich grundlegend:
// Android kann den Prozess ueber einen Foreground-Service dauerhaft am Leben
// halten; die Server-Verbindung uebersteht damit beliebig lange Hintergrund-
// Phasen. Ein solches Konstrukt gibt es auf iOS NICHT. Wechselt der Nutzer die
// App (z.B. kurz zu WhatsApp) oder sperrt das Geraet, wird der Prozess nach
// kurzer Zeit eingefroren: keine Timer, kein Socket-I/O, und die TCP-Verbindung
// stirbt anschliessend still - genau der beobachtete "Freeze", bei dem die GUI
// noch reagiert, vom Server aber nichts mehr kommt.
//
// Das einzige, was iOS anbietet, ist beginBackgroundTask: eine Gnadenfrist von
// typischerweise ~30 Sekunden, in der die App nach dem Wechsel weiterlaufen
// darf. Damit ueberlebt der haeufigste Fall - kurz in eine andere App schauen
// und zurueckkommen. Laengere Abwesenheit kann sie NICHT abdecken; dafuer
// braucht es die Erkennung beim Zurueckkehren (Resume-Probe in pokerth.qml).
//
// start()/stop() markieren "es gibt eine aktive Online-Session" und werden an
// denselben Stellen aufgerufen wie AndroidConnectionService. Das Anfordern und
// Freigeben der Gnadenfrist erledigt die Implementierung selbst anhand der
// UIApplication-Benachrichtigungen.
//
// Auf allen anderen Plattformen sind beide Funktionen No-ops.
namespace IosBackgroundSession
{
void start();
void stop();
}

#endif // IOSBACKGROUNDSESSION_H
