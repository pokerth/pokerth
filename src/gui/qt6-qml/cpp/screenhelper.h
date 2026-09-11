#pragma once
#include <QObject>

// Cross-platform helper: keeps the screen awake during the game and the
// waiting room.
//   Android: FLAG_KEEP_SCREEN_ON on the activity window (JNI).
//   iOS:     UIApplication.idleTimerDisabled.
// On all other platforms setKeepScreenOn() is a no-op.
//
// On the mobile platforms this is more than comfort: after the screen lock
// the app is suspended, the socket I/O stalls and the server connection
// dies silently – the game appears frozen even though the GUI can still be
// operated.
class ScreenHelper : public QObject
{
	Q_OBJECT
public:
	explicit ScreenHelper(QObject *parent = nullptr);
	Q_INVOKABLE void setKeepScreenOn(bool keep);
};
