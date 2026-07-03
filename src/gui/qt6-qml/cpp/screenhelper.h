#pragma once
#include <QObject>

// Plattformübergreifender Helper: hält den Android-Bildschirm wach,
// indem er FLAG_KEEP_SCREEN_ON auf dem Activity-Window setzt.
// Auf anderen Plattformen ist setKeepScreenOn() ein No-op.
class ScreenHelper : public QObject
{
    Q_OBJECT
public:
    explicit ScreenHelper(QObject *parent = nullptr);
    Q_INVOKABLE void setKeepScreenOn(bool keep);
};
