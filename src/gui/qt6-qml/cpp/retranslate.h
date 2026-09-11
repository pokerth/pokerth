#pragma once

#include <QObject>
#include <QString>
#include <QQmlApplicationEngine>
#include <QTranslator>

class LanguageManager : public QObject
{
	Q_OBJECT
public:
	explicit LanguageManager(QQmlApplicationEngine *engine, QObject *parent = nullptr);

	// Invokable from QML to switch the app language at runtime
	Q_INVOKABLE void switchLanguage(const QString &langCode);

private:
	QQmlApplicationEngine *m_engine;
	QTranslator m_translator;
};
