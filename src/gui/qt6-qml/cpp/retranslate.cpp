#include "retranslate.h"
#include <QCoreApplication>

LanguageManager::LanguageManager(QQmlApplicationEngine *engine, QObject *parent)
    : QObject(parent)
    , m_engine(engine)
{
}

void LanguageManager::switchLanguage(const QString &langCode)
{
    // Remove the currently installed translator
    QCoreApplication::removeTranslator(&m_translator);

    // Build the QM file path (ensure it matches your RESOURCE_PREFIX)
    const QString qmFile = QStringLiteral(":/i18n/pokerth_%1.qm").arg(langCode);
    if (m_translator.load(qmFile)) {
        QCoreApplication::installTranslator(&m_translator);
        qDebug() << "Installed new Translator with code " << langCode;
    } else {
        qWarning() << "Failed to load translation file:" << qmFile;
    }

    // Retranslate all QML bindings
    m_engine->retranslate();
}
