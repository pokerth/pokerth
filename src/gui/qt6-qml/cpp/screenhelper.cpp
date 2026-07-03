#include "screenhelper.h"

#ifdef Q_OS_ANDROID
#include <QJniObject>
#include <QCoreApplication>
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
#else
    Q_UNUSED(keep)
#endif
}
