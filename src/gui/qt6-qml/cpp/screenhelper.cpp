#include "screenhelper.h"

#ifdef Q_OS_ANDROID
#include <QJniObject>
#include <QCoreApplication>
#endif

#ifdef Q_OS_IOS
#import <UIKit/UIKit.h>
#endif

ScreenHelper::ScreenHelper(QObject *parent) : QObject(parent) {}

void ScreenHelper::setKeepScreenOn(bool keep)
{
#ifdef Q_OS_ANDROID
	auto applyFlag = [keep]() {
		QJniObject activity = QJniObject::callStaticObjectMethod(
								  "org/qtproject/qt/android/QtNative",
								  "activity",
								  "()Landroid/app/Activity;");
		if (!activity.isValid())
			return;

		QJniObject window = activity.callObjectMethod(
								"getWindow", "()Landroid/view/Window;");
		if (!window.isValid())
			return;

		// android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON = 0x00000080
		const jint FLAG_KEEP_SCREEN_ON = 0x00000080;
		if (keep)
			window.callMethod<void>("addFlags",   "(I)V", FLAG_KEEP_SCREEN_ON);
		else
			window.callMethod<void>("clearFlags", "(I)V", FLAG_KEEP_SCREEN_ON);

		// Also set via the decor view — this is how media views (VideoView etc.)
		// maintain the flag internally, and survives some lifecycle transitions.
		QJniObject decorView = window.callObjectMethod(
								   "getDecorView", "()Landroid/view/View;");
		if (decorView.isValid())
			decorView.callMethod<void>("setKeepScreenOn", "(Z)V", static_cast<jboolean>(keep));
	};

	// QNativeInterface::QAndroidApplication::runOnAndroidMainThread ensures
	// the flag is applied on the Android UI thread even if Qt's main thread
	// diverges (e.g. during early initialisation or after a lifecycle event).
	if (auto *iface = qApp->nativeInterface<QNativeInterface::QAndroidApplication>())
		iface->runOnAndroidMainThread(applyFlag);
	else
		applyFlag(); // non-Android or Qt < 6.2 fallback

#elif defined(Q_OS_IOS)
	// iOS counterpart to FLAG_KEEP_SCREEN_ON: the "idle timer" is the clock
	// after which iOS dims and locks the screen. Switched off, the display
	// stays on during the game.
	//
	// This is NOT just comfort here, it prevents a dropped connection: after
	// the screen lock iOS suspends the app, the socket I/O stalls and the TCP
	// connection to the server dies silently. For the user this looks like a
	// frozen game with an operable GUI - exactly the observed behaviour, even
	// though the app was in the foreground. Whoever thinks longer without
	// touching the screen used to run into exactly this trap.
	//
	// UIKit expects the main thread; setKeepScreenOn() is called from QML (GUI
	// thread), the check is only a safeguard.
	{
		const bool disableIdleTimer = keep;
		dispatch_block_t apply = ^ {
			[UIApplication sharedApplication].idleTimerDisabled = disableIdleTimer;
		};
		if ([NSThread isMainThread])
			apply();
		else
			dispatch_async(dispatch_get_main_queue(), apply);
	}
#else
	Q_UNUSED(keep)
#endif
}
