/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2026 Felix Hammer, Florian Thauer, Lothar May          *
 *****************************************************************************/

#include "androidconnectionservice.h"

#include <QCoreApplication>

#ifdef Q_OS_ANDROID
#include <QJniEnvironment>
#include <QJniObject>

namespace
{

// Pass the notification texts to Java localized from the Qt translations
// (the Java service itself has no access to the Qt translator).
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

		// Otherwise a thrown Java exception only ends up in the logcat and the
		// call looks successful from here - that is exactly how it went
		// unnoticed that the generated manifest did not declare the service at
		// all (startForegroundService then throws immediately). The result
		// belongs in the app log (~/.pokerth/pokerth-debug.log) so that a player
		// report can be evaluated without adb.
		QJniEnvironment env;
		if (env.checkAndClearExceptions()) {
			qWarning() << "[ANDROID-FGS]" << (start ? "start" : "stop")
					   << "threw - foreground service NOT running, the"
					   << "connection is unprotected in the background";
		} else {
			qInfo() << "[ANDROID-FGS]" << (start ? "started" : "stopped");
		}
	};

	// Run on the Android UI thread: callers may run on arbitrary threads
	// (network thread on connect/error callbacks).
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
