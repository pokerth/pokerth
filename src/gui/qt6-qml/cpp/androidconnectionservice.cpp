/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2026 Felix Hammer, Florian Thauer, Lothar May          *
 *****************************************************************************/

#include "androidconnectionservice.h"

#include <QCoreApplication>

#ifdef Q_OS_ANDROID
#include <QJniEnvironment>
#include <QJniObject>

namespace {

// Notification-Texte lokalisiert aus den Qt-Übersetzungen an Java übergeben
// (der Java-Service selbst hat keinen Zugriff auf den Qt-Übersetzer).
void callService(bool start)
{
    auto invoke = [start]() {
        QJniObject context = QNativeInterface::QAndroidApplication::context();
        if (!context.isValid()) {
            qWarning() << "[ANDROID-FGS] no valid Android context - foreground"
                       << "service NOT started, connection is unprotected";
            return;
        }
        if (start) {
            const QJniObject text = QJniObject::fromString(
                QCoreApplication::translate("AndroidConnectionService",
                                            "Connected to the game server"));
            const QJniObject channelName = QJniObject::fromString(
                QCoreApplication::translate("AndroidConnectionService",
                                            "Online game connection"));
            QJniObject::callStaticMethod<void>(
                "org/pokerth/qml/ConnectionService", "start",
                "(Landroid/content/Context;Ljava/lang/String;Ljava/lang/String;)V",
                context.object(), text.object(), channelName.object());
        } else {
            QJniObject::callStaticMethod<void>(
                "org/pokerth/qml/ConnectionService", "stop",
                "(Landroid/content/Context;)V",
                context.object());
        }

        // Eine geworfene Java-Exception bleibt sonst nur im Logcat hängen und
        // der Aufruf sieht von hier aus erfolgreich aus - genau so blieb es
        // unbemerkt, dass das generierte Manifest den Service gar nicht
        // deklarierte (startForegroundService wirft dann sofort). Das Ergebnis
        // gehört in den App-Log (~/.pokerth/pokerth-debug.log), damit ein
        // Spieler-Report ohne adb auswertbar ist.
        QJniEnvironment env;
        if (env.checkAndClearExceptions()) {
            qWarning() << "[ANDROID-FGS]" << (start ? "start" : "stop")
                       << "threw - foreground service NOT running, the"
                       << "connection is unprotected in the background";
        } else {
            qInfo() << "[ANDROID-FGS]" << (start ? "started" : "stopped");
        }
    };

    // Auf dem Android-UI-Thread ausführen: Aufrufer können auf beliebigen
    // Threads laufen (Netzwerk-Thread bei Connect/Error-Callbacks).
    if (auto *iface = qApp->nativeInterface<QNativeInterface::QAndroidApplication>())
        iface->runOnAndroidMainThread(invoke);
    else
        invoke();
}

} // namespace

void AndroidConnectionService::start()
{
    callService(true);
}

void AndroidConnectionService::stop()
{
    callService(false);
}

#else // !Q_OS_ANDROID

void AndroidConnectionService::start()
{
}

void AndroidConnectionService::stop()
{
}

#endif
