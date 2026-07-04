/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2026 Felix Hammer, Florian Thauer, Lothar May          *
 *****************************************************************************/

#include "androidconnectionservice.h"

#include <QCoreApplication>

#ifdef Q_OS_ANDROID
#include <QJniObject>

namespace {

// Notification-Texte lokalisiert aus den Qt-Übersetzungen an Java übergeben
// (der Java-Service selbst hat keinen Zugriff auf den Qt-Übersetzer).
void callService(bool start)
{
    auto invoke = [start]() {
        QJniObject context = QNativeInterface::QAndroidApplication::context();
        if (!context.isValid())
            return;
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
