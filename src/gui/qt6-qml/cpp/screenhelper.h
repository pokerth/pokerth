#pragma once
#include <QObject>

// Plattformübergreifender Helper: hält den Bildschirm während Spiel und
// Warteraum wach.
//   Android: FLAG_KEEP_SCREEN_ON auf dem Activity-Window (JNI).
//   iOS:     UIApplication.idleTimerDisabled.
// Auf allen anderen Plattformen ist setKeepScreenOn() ein No-op.
//
// Auf den Mobilplattformen ist das mehr als Komfort: Nach der Bildschirmsperre
// wird die App suspendiert, das Socket-I/O steht still und die Server-
// Verbindung stirbt still – das Spiel wirkt eingefroren, obwohl die GUI noch
// bedienbar ist.
class ScreenHelper : public QObject
{
    Q_OBJECT
public:
    explicit ScreenHelper(QObject *parent = nullptr);
    Q_INVOKABLE void setKeepScreenOn(bool keep);
};
