/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 * Copyright (C) 2006-2012 Felix Hammer, Florian Thauer, Lothar May          *
 *                                                                           *
 * This program is free software: you can redistribute it and/or modify      *
 * it under the terms of the GNU Affero General Public License as            *
 * published by the Free Software Foundation, either version 3 of the        *
 * License, or (at your option) any later version.                           *
 *                                                                           *
 * This program is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 * GNU Affero General Public License for more details.                       *
 *                                                                           *
 * You should have received a copy of the GNU Affero General Public License  *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.     *
 *                                                                           *
 *                                                                           *
 * Additional permission under GNU AGPL version 3 section 7                  *
 *                                                                           *
 * If you modify this program, or any covered work, by linking or            *
 * combining it with the OpenSSL project's OpenSSL library (or a             *
 * modified version of that library), containing parts covered by the        *
 * terms of the OpenSSL or SSLeay licenses, the authors of PokerTH           *
 * (Felix Hammer, Florian Thauer, Lothar May) grant you additional           *
 * permission to convey the resulting work.                                  *
 * Corresponding Source for a non-source form of such a combination          *
 * shall include the source code for the parts of OpenSSL used as well       *
 * as that of the covered work.                                              *
 *****************************************************************************/
#ifdef QML_CLIENT
//START THE QML SWITCH HERE
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <QtCore>
#include <QtQml>
#include <QApplication>
#include <QSettings>
#include <QQuickWindow>
#include <QIcon>
#include <boost/shared_ptr.hpp>
#include "configfile.h"
#include "session.h"
#include "log.h"
#include <QTranslator>
#include <QQmlContext>
#include <QDebug>
#include <retranslate.h>
#include <settingsmanager.h>
#include "gui/qt6-qml/cpp/styleprovider.h"
#include "gui/qt6-qml/cpp/serverconnectionhandler.h"
#include "gui/qt6-qml/cpp/lobbyhandler.h"
#include "gui/qt6-qml/cpp/gamehandler.h"
#include "gui/qt6-qml/cpp/loghandler.h"
#include "gui/qt6-qml/cpp/networkgamehandler.h"
#include "gui/qt6-qml/cpp/qmlguiinterface.h"
#include "gui/qt6-qml/cpp/screenhelper.h"
#include "gui/qt6-qml/cpp/webnetworkfactory.h"
#include <core/loghelper.h>
#include <cstdio>

// Routes every Qt/QML diagnostic message (qDebug, qWarning, and crucially the
// QML console.log "[NAV]" navigation traces) into the persistent client debug
// log file (~/.pokerth/pokerth-debug.log) opened by loghelper_init(), while
// still echoing to stderr so a terminal run looks unchanged. Combined with the
// engine/net LOG_* messages (which share the same file) this gives a single,
// thread-id-tagged timeline – essential for the rare end-of-round freeze where
// the GUI thread blocks inside the SQLite log flush.
static void pokerthQmlMessageHandler(QtMsgType type, const QMessageLogContext & /*ctx*/, const QString &msg)
{
    // Debug-Meldungen werden NICHT mehr verarbeitet: die per-Aktion-
    // Instrumentierung (z. B. [ACTDBG] im GameHandler) stammt aus der damaligen
    // Freeze-Untersuchung (gelöst) und schrieb pro Aktion synchron nach stderr
    // UND auf Platte (fflush je Zeile) → unnötige I/O im Spiel. Nur noch
    // Info/Warning/Critical/Fatal (selten, echte Diagnose) werden geloggt.
    //
    // Ausnahme: Diagnose-Build (-DPOKERTH_DIAG_LOG=ON, siehe qt6-qml/CMakeLists.txt).
    // Dort ist die per-Aktion-Instrumentierung genau das Gesuchte, also muessen
    // die Debug-Zeilen hier durchgelassen werden.
#ifndef POKERTH_DIAG_LOG
    if (type == QtDebugMsg)
        return;
#endif

    const char *tag = "[QML] ";
    switch (type) {
        case QtDebugMsg:    tag = "[QML-D] "; break;
        case QtInfoMsg:     tag = "[QML-I] "; break;
        case QtWarningMsg:  tag = "[QML-W] "; break;
        case QtCriticalMsg: tag = "[QML-C] "; break;
        case QtFatalMsg:    tag = "[QML-F] "; break;
    }
    const QByteArray local = msg.toLocal8Bit();
    fprintf(stderr, "%s%s\n", tag, local.constData());
    fflush(stderr);
    loghelper_write_raw(std::string(tag) + msg.toStdString());
    if (type == QtFatalMsg)
        abort();
}

// Erzwingt nach jeder Größenänderung einen vollständigen Neuaufbau des
// Fensterinhalts.
//
// Hintergrund (Forum-Bug „QML - Button 'hit zone' is sometimes wrong",
// Windows 10, 2.1.5): Zieht man das Fenster am UNTEREN Rand auf, präsentiert
// das D3D11-RHI-Backend weiterhin den alten Frame. Die Szene ist korrekt für
// die neue Größe layoutet – nur das Bild hängt hinterher: die obere App-Leiste
// fehlt darin, unten bleibt ein gleich hoher Streifen unbemalt (man sieht
// fremde Fenster durch). Da alle Klickzonen der GÜLTIGEN Geometrie folgen,
// wirkt es, als lägen die Buttons ~15–40 px zu tief – im schlimmsten Fall
// trifft ein zu hoch gezielter Fold-Klick den All-In-Button darüber.
// Verschiebt man das Fenster an der Titelleiste, ist alles wieder korrekt:
// EIN zusätzlicher Renderdurchlauf genügt, um den Zustand aufzulösen.
// Bestätigt hat das der Melder per Backend-Test: mit QSG_RHI_BACKEND=opengl
// tritt der Fehler nicht auf, QSG_RENDER_LOOP=basic ändert dagegen nichts.
//
// Genau diesen zusätzlichen Durchlauf stößt der Filter an. Das Nachzügler-
// Update ist nötig, weil Windows während des interaktiven Resize-Loops die
// Event-Schleife blockiert – der Timer feuert erst, wenn der Anwender die
// Maus losgelassen hat und die letzte Größe wirklich feststeht.
class ResizeRepaintGuard : public QObject
{
public:
    explicit ResizeRepaintGuard(QQuickWindow *window)
        : QObject(window), m_window(window)
    {
        m_settle.setSingleShot(true);
        m_settle.setInterval(80);
        QObject::connect(&m_settle, &QTimer::timeout, this, [this]() {
            m_window->requestUpdate();
        });
        m_window->installEventFilter(this);
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (watched == m_window
            && (event->type() == QEvent::Resize || event->type() == QEvent::Expose)) {
            m_window->requestUpdate();
            m_settle.start();
        }
        // Nur mitlesen, nie schlucken.
        return QObject::eventFilter(watched, event);
    }

private:
    QQuickWindow *m_window;
    QTimer m_settle;
};

int main(int argc, char *argv[])
{
    // QT_QUICK_CONTROLS_STYLE wird beim frühen Style-Resolve ausgewertet.
    // Unter KDE/Plasma ist oft bereits ein nativer Style im Environment
    // gesetzt ("Desktop" / "org.kde.desktop"), der im Deploy das fehlende
    // Modul verlangt. Daher problematische Werte früh auf Universal
    // normalisieren, noch vor Q(Gui)Application und vor dem ersten
    // QtQuick-Controls-Zugriff.
    const QByteArray quickControlsStyle = qgetenv("QT_QUICK_CONTROLS_STYLE");
    const QByteArray quickControlsStyleLower = quickControlsStyle.toLower();
    if (quickControlsStyle.isEmpty()
        || quickControlsStyleLower == "desktop"
        || quickControlsStyleLower.startsWith("org.kde")) {
        qputenv("QT_QUICK_CONTROLS_STYLE", "Universal");
    }

    // QML-Disk-Cache (.qmlc) zentral abschalten – vor jeder Engine-Nutzung, also
    // wirksam auf JEDEM Startpfad (Wrapper, mitgelieferte .desktop, direktes
    // Binary, AppImage) und jeder Plattform. Grund: Qt validiert den Cache über
    // Datei-mtimes; nach einem Update behält es teils den alten .qmlc einer
    // *bestehenden* QML-Datei, sodass NEU hinzugefügte Properties/Funktionen als
    // `undefined` gelesen werden (z. B. Config.Parameters.showCommunitySuggest →
    // Suggest-Button verschwindet). Bisher setzte das nur der pokerth-qml-Wrapper
    // bzw. das Flatpak-Manifest – hier gilt es garantiert überall. Der minimale
    // Neucompile-Aufwand beim Start ist vernachlässigbar.
    qputenv("QML_DISABLE_DISK_CACHE", "1");

    // Qt6 nutzt auf Wayland standardmäßig den THREADED Scene-Graph-Render-Loop.
    // Dessen Teardown beim Programmende kann mit der GUI-Thread-Zerstörung der
    // QML-Items kollidieren → Absturz in QQuickWindowPrivate::cleanup(QSGNode*).
    // Der BASIC-Render-Loop rendert auf dem GUI-Thread und baut synchron/ohne
    // Race ab. Nur erzwingen, wenn der User nichts gesetzt hat UND wir auf
    // Wayland laufen (X11 ist vom Problem nicht betroffen; Funktionsumfang/
    // Effekte bleiben identisch, nur die Render-Thread-Aufteilung ändert sich).
    if (qEnvironmentVariableIsEmpty("QSG_RENDER_LOOP")) {
        const bool onWayland = !qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY")
                               || qgetenv("XDG_SESSION_TYPE") == QByteArrayLiteral("wayland");
        if (onWayland)
            qputenv("QSG_RENDER_LOOP", "basic");
    }

    QGuiApplication::setApplicationName("PokerTH");
    QGuiApplication::setOrganizationName("PokerTH");
 	QGuiApplication::setOrganizationDomain("pokerth.net");

    QApplication app(argc, argv);

	// single instance check using QLockFile
    QString lockPath = QDir::temp().absoluteFilePath("pokerth_qml-client.lock");
    QLockFile lockFile(lockPath);
    lockFile.setStaleLockTime(0);
    if (!lockFile.tryLock()) {
        return 0;
    }


    QIcon::setThemeName("pokerth");
    // Set the application/window icon explicitly. On X11 (and on Wayland with
    // Qt >= 6.9 via the xdg-toplevel-icon protocol) this makes the window
    // manager / task-bar show the PokerTH logo instead of the generic "W".
    // IMPORTANT: build the icon from concrete rasterized pixmaps, NOT directly
    // from the scalable SVG. A QIcon backed only by an SVG reports no
    // availableSizes(); Qt's Wayland xdg-toplevel-icon path (Qt >= 6.9, used by
    // the AppImage) sends the icon as sized buffers, so with no sizes it sends
    // nothing and the compositor keeps its generic fallback. The widget client
    // works because it uses a raster PNG; here we render the SVG to fixed sizes.
    {
        const QIcon svgIcon(QStringLiteral(":/resources/pokerth.svg"));
        QIcon appIcon;
        for (int sz : {16, 24, 32, 48, 64, 128, 256})
            appIcon.addPixmap(svgIcon.pixmap(sz, sz));
        QGuiApplication::setWindowIcon(appIcon);
    }
    // On Wayland with the Qt 6.8 deploy the compositor (e.g. KWin) can NOT get
    // the icon from setWindowIcon; it resolves it from the matching .desktop
    // file via the surface app_id. Pin the app_id to the installed desktop-file
    // basename so the portable launcher's ~/.local/share entry is matched.
    QGuiApplication::setDesktopFileName(QStringLiteral("pokerth_qml"));

    boost::shared_ptr<ConfigFile> myConfig;
    myConfig.reset(new ConfigFile(argv[0], false));

    // Open the persistent client debug log (~/.pokerth/pokerth-debug.log) and
    // funnel all Qt/QML diagnostics through it. Done as early as possible so the
    // whole session – including the navigation traces around end-of-game – is
    // captured for after-the-fact analysis of the rare leave-game freeze.
    loghelper_init(myConfig->readConfigString("LogDir"), 1);
    qInstallMessageHandler(pokerthQmlMessageHandler);
    qInfo().noquote() << "[BOOT] PokerTH QML client starting; debug log in"
                      << QString::fromStdString(myConfig->readConfigString("LogDir"));

    // Der QtQuick-Controls-Style ist durch die QT_QUICK_CONTROLS_STYLE-
    // Normalisierung oben (vor QApplication) bereits vollständig bestimmt;
    // QtQuick liest die Env-Variable selbst. KEIN QSettings auf config.xml
    // mehr: der frühere XML-Handler (settingsxmlhandler) las nur die value-
    // Attribute und verwarf beim Zurückschreiben sämtliche Listen-Einträge
    // (PlayerTooltips = Spieler-Notizen/-Bewertungen, PlayerIgnoreList,
    // ManualBlindsList, ...). An config.xml schreibt ausschließlich ConfigFile.

	const QLocale locale;
	const QString baseName = "pokerth";
	QTranslator translator;
	if (translator.load(locale, "pokerth", "_", ":/i18n")) {
		// qDebug() << "Locale found!";
		app.installTranslator(&translator);
	} else {
		// qDebug() << "Locale not found in translations";
	}

    // Initialize Log
    Log *log = new Log(myConfig.get());
    
    // Create handlers
    ServerConnectionHandler *connectionHandler = new ServerConnectionHandler(&app);
    connectionHandler->setConfig(myConfig.get());
    
    LobbyHandler *lobbyHandler = new LobbyHandler(&app);
    lobbyHandler->setConfig(myConfig.get());

    // Automatische Wiederverbindung: Der ServerConnectionHandler schaltet das
    // Rejoin im LobbyHandler scharf, damit das Angebot des Servers (InitAck)
    // ohne Rückfrage angenommen wird und der Spieler wieder an seinem Tisch
    // landet statt auf der Login-Seite.
    QObject::connect(connectionHandler, &ServerConnectionHandler::autoRejoinArmed,
                     lobbyHandler, &LobbyHandler::setAutoRejoin);

    GameHandler *gameHandler = new GameHandler(&app);

    LogHandler *logHandler = new LogHandler(myConfig.get(), &app);

    NetworkGameHandler *networkGameHandler = new NetworkGameHandler(&app);

    // Create GUI Interface with handlers
    QmlGuiInterface *guiInterface = new QmlGuiInterface(myConfig.get(), connectionHandler, lobbyHandler);
    guiInterface->setGameHandler(gameHandler);
    
    // Create and initialize Session
    boost::shared_ptr<Session> session;
    try {
        session.reset(new Session(guiInterface, myConfig.get(), log));
        bool initResult = session->init();
        if (!initResult) {
            qWarning() << "Session initialization failed";
        }
        log->init();
        logHandler->setLog(log);
        logHandler->refresh();

        networkGameHandler->setSession(session);
        networkGameHandler->setConfig(myConfig.get());

        guiInterface->setSession(session);
        connectionHandler->setSession(session);
        lobbyHandler->setSession(session);
        gameHandler->setSession(session);
        gameHandler->setConfig(myConfig.get());
        
        // qDebug() << "Session initialized successfully";
    } catch (const std::exception &e) {
        qWarning() << "Exception during session init:" << e.what();
    }

    // Declared before the engine so it outlives it (the engine uses the factory
    // during its own destruction). Injects the allowlisted PokerTH User-Agent on
    // all QML network requests (rankings & player pages behind Cloudflare).
    WebNetworkAccessManagerFactory webNamFactory;

    QQmlApplicationEngine engine;
    engine.setNetworkAccessManagerFactory(&webNamFactory);

    SettingsManager settingsMgr(myConfig);
    StyleProvider styleProvider(myConfig);
    LanguageManager langMgr(&engine);
    ScreenHelper screenHelper;
    engine.rootContext()->setContextProperty("SettingsManager", &settingsMgr);
    engine.rootContext()->setContextProperty("StyleProvider", &styleProvider);
    engine.rootContext()->setContextProperty("LanguageManager", &langMgr);
    engine.rootContext()->setContextProperty("ScreenHelper", &screenHelper);
    engine.rootContext()->setContextProperty("ServerConnection", connectionHandler);
    engine.rootContext()->setContextProperty("Lobby", lobbyHandler);
    engine.rootContext()->setContextProperty("GameTable", gameHandler);
    engine.rootContext()->setContextProperty("LogStore", logHandler);
    engine.rootContext()->setContextProperty("NetworkGame", networkGameHandler);
	engine.load(QUrl(QStringLiteral("qrc:/pokerth.qml")));

	if (engine.rootObjects().isEmpty()) {
        // Session owns log – do NOT delete log here.
        // lobbyHandler/connectionHandler are parented to &app – Qt handles them.
        session.reset();
        delete guiInterface;
        return -1;
    }

    // Repaint-Absicherung für Größenänderungen (s. ResizeRepaintGuard). Der
    // Guard hängt am Fenster und wird mit ihm zerstört.
    for (QObject *root : engine.rootObjects()) {
        if (auto *w = qobject_cast<QQuickWindow *>(root))
            new ResizeRepaintGuard(w);
    }

    int result = app.exec();

    // Scene-Graph sauber herunterfahren, BEVOR die Engine (und damit die
    // QML-Items samt Fenster) beim Stack-Unwind zerstört wird. Auf Wayland mit
    // Threaded-Render-Loop stürzt sonst QQuickWindowPrivate::cleanup() beim
    // Zerstören der Items ab (derefWindow → Cleanup eines bereits halb
    // abgebauten SG). hide() stoppt den Render-Thread und gibt die SG-Ressourcen
    // frei, solange GUI-Thread und App noch voll leben.
    for (QObject *root : engine.rootObjects()) {
        if (auto *w = qobject_cast<QQuickWindow *>(root))
            w->hide();
    }

    // Cleanup order matters:
    // 1. Stop network thread first (before any owned objects are freed).
    // 2. Release session references held by all handlers BEFORE session.reset().
    //    gameHandler/lobbyHandler/connectionHandler are parented to &app, so
    //    their destructors run inside ~QApplication – at that point
    //    QCoreApplication::instance() is already nullptr.  If they still hold a
    //    shared_ptr<Session> when they are destroyed, ~Session() → ~Log() would
    //    execute SQL cleanup after QCoreApplication is gone → crash.
    //    Clearing the refs here ensures ~Session() runs while QCoreApplication
    //    is still fully alive.
    // 3. Then destroy session explicitly – ~Session() deletes myLog and
    //    myQtToolsInterface.  Do NOT delete log manually.
    // 4. Only then delete guiInterface (session no longer holds a raw ptr to it).
    if (session) {
        session->terminateNetworkClient();
    }
    // Drop shared_ptr<Session> copies held by QObject-parented handlers.
    connectionHandler->setSession(boost::shared_ptr<Session>());
    lobbyHandler->setSession(boost::shared_ptr<Session>());
    gameHandler->setSession(boost::shared_ptr<Session>());
    // Terminate + release the embedded network server (its ServerGuiWrapper
    // references guiInterface) before guiInterface is deleted below.
    networkGameHandler->shutdown();
    delete guiInterface; // releases guiInterface's session ref too
    // Commit any still-buffered SQLite log statements now, while the Session
    // (and thus the Log) is guaranteed alive and we are on the GUI thread.
    // flushLog() takes the Log's mutex, so if the network thread was leaked by
    // terminateNetworkClient() (Join timeout) and is mid-flush, we block until
    // it finishes rather than racing it. This matters because the Wayland path
    // below ends the process with std::_Exit(), which skips ~Log() entirely.
    if (session && session->getMyLog()) {
        session->getMyLog()->flushLog();
    }
    session.reset(); // ~Session() runs here; ~Log() (SQLite close) follows once
                     // the last shared_ptr<Log> ref drops (a leaked network
                     // thread may briefly hold one – the Log stays valid for it)

    // Wayland-Workaround gegen den reproduzierbaren Shutdown-Crash: der
    // QQmlApplicationEngine-Destruktor (Stack-Unwind am main-Ende) baut die
    // QtQuickLayouts-Items ab; auf Wayland dereferenziert dabei ein Item ein
    // bereits halb zerstörtes Fenster → SIGSEGV in QQuickWindowPrivate::cleanup
    // (auch mit Basic-Render-Loop und hide() nicht zuverlässig vermeidbar).
    // Da sämtliche Persistenz bereits erledigt ist – Config-XML eager via
    // writeBuffer(), Log-SQL via ~Session() oben, und die QtQuick-Settings
    // (Parameters.qml) sichern wir hier noch explizit auf Platte – beenden wir
    // den Prozess hart, ohne den fehlerhaften QML-/Qt-Teardown auszuführen.
    if (QGuiApplication::platformName().startsWith(QLatin1String("wayland"))) {
        QSettings().sync();   // QtQuick-Settings (gemeinsamer QConfFile-Store) flushen
        std::cout.flush();
        std::cerr.flush();
        std::_Exit(result);
    }

    return result;
}

#else
// START OF OLD QT-WIDGETS GUI SECTION


#include <boost/asio.hpp>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <qapplication.h>

#include <QtWidgets>
#include <QtGui>
#include <QtCore>
#include <QDebug>
#include <QScreen>
#include <QGuiApplication>

#include "session.h"
#include "startwindowimpl.h"
#include "configfile.h"
#include "log.h"
#include "startsplash.h"
#include "game_defs.h"
#include "darkmodehelper.h"
#include <net/socket_startup.h>


#ifdef _MSC_VER
#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>

#define ENABLE_LEAK_CHECK() \
{ \
    int tmpFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG); \
    tmpFlag |= _CRTDBG_LEAK_CHECK_DF; \
    _CrtSetDbgFlag(tmpFlag); \
    }
#endif
#endif

#ifndef ENABLE_LEAK_CHECK
#define ENABLE_LEAK_CHECK()
#endif

#ifdef ANDROID
#ifndef ANDROID_TEST
#include <QJniEnvironment>
#include <QJniObject>
#include <cmath>
#endif
#endif

using namespace std;

// In Qt6 HiDPI is always active. Low-resolution (1x) pixmaps are upscaled by
// the widget paint engine. Without the SmoothPixmapTransform hint Qt uses
// nearest-neighbor interpolation, which makes cards, avatars and action badges
// look blocky. This proxy-style enables smooth (bilinear) interpolation for all
// widget rendering that goes through QStyle::drawItemPixmap (i.e. every
// QLabel::setPixmap call throughout the application).
class SmoothPixmapStyle : public QProxyStyle {
public:
    explicit SmoothPixmapStyle(QStyle *base = nullptr) : QProxyStyle(base) {}
    void drawItemPixmap(QPainter *p, const QRect &r, int alignment,
                        const QPixmap &pm) const override
    {
        p->setRenderHint(QPainter::SmoothPixmapTransform, true);
        QProxyStyle::drawItemPixmap(p, r, alignment, pm);
    }
};

class startWindowImpl;
class Game;

int main( int argc, char **argv )
{
    //ENABLE_LEAK_CHECK();

    //_CrtSetBreakAlloc(49937);
    socket_startup();

    // High DPI support: Use PassThrough rounding to correctly handle fractional
    // scale factors (e.g. 125% = 1.25x) on Windows.  Must be set before
    // QApplication is constructed.
    // NOTE: Do NOT enable on Android – the platform handles DPI scaling natively.
    // PassThrough causes a non-integer devicePixelRatio on high-DPI phones
    // (e.g. Galaxy S23 at ~425 PPI → 2.65625× instead of 3×), which can crash
    // the EGL/SurfaceFlinger rendering pipeline on Android 16+.
#ifndef ANDROID
    QGuiApplication::setHighDpiScaleFactorRoundingPolicy(
        Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
#endif

#ifdef __APPLE__
	// The following needs to be done before the application is created, otherwise loading platforms plugin fails.
	QDir dir(argv[0]);
	dir.cdUp();
	dir.cdUp();
	dir.cd("plugins");
	QApplication::setLibraryPaths(QStringList(dir.absolutePath()));
#endif

	/////// can be removed for non-qt-guis ////////////
#ifdef ANDROID
	// The 800×480 mobile layout needs ≥480 logical px on the short screen edge.
	// Modern high-DPI phones only report ~360 logical dp, clipping dialogs.
	// Compute QT_SCALE_FACTOR dynamically from Android display metrics so the
	// fix works on ANY device (phones, tablets, foldables, varying densities).
	// Must be set BEFORE QApplication is constructed.
#ifndef ANDROID_TEST
	{
		QJniObject resources = QJniObject::callStaticObjectMethod(
			"android/content/res/Resources",
			"getSystem",
			"()Landroid/content/res/Resources;");
		if (resources.isValid()) {
			QJniObject dm = resources.callObjectMethod(
				"getDisplayMetrics",
				"()Landroid/util/DisplayMetrics;");
			if (dm.isValid()) {
				int wPx  = dm.getField<jint>("widthPixels");
				int hPx  = dm.getField<jint>("heightPixels");
				float density = dm.getField<jfloat>("density");
				if (wPx > 0 && hPx > 0 && density > 0.0f) {
					int shortPx  = qMin(wPx, hPx);
					int qtDpr    = qMax(1, static_cast<int>(std::round(density)));
					qreal logicalShort = static_cast<qreal>(shortPx) / qtDpr;
					if (logicalShort > 0.0 && logicalShort < 480.0) {
						qreal factor = logicalShort / 480.0;
						qputenv("QT_SCALE_FACTOR",
							QByteArray::number(static_cast<double>(factor), 'f', 4));
					}
				}
			}
		}
	}
#endif // !ANDROID_TEST

	QApplication a(argc, argv);
	a.setApplicationName("PokerTH");
#else
	QApplication a(argc, argv);
	// Wrap the platform style so that all QLabel::setPixmap / drawItemPixmap
	// calls use bilinear interpolation instead of nearest-neighbor when Qt6
	// upscales 1x assets on HiDPI screens.
	a.setStyle(new SmoothPixmapStyle(a.style()));

	// single instance check using QLockFile
    QString lockPath = QDir::temp().absoluteFilePath("pokerth_client.lock");
    QLockFile lockFile(lockPath);
    lockFile.setStaleLockTime(0);
    if (!lockFile.tryLock()) {
        return 0;
    }
#endif

	//create defaultconfig
	ConfigFile *myConfig = new ConfigFile(argv[0], false);
	Log *myLog = new Log(myConfig);

	// set PlastiqueStyle even for mac-version to prevent artefacts on styled widgets

	QString	myAppDataPath = QString::fromUtf8(myConfig->readConfigString("AppDataDir").c_str());

	// Set the application-wide window icon so surfaces that use the *application*
	// icon (e.g. the desktop shell's status/indicator area) show the PokerTH logo
	// instead of the generic default. The taskbar already gets the icon from the
	// per-window StartWindowImpl::setWindowIcon, but that does not cover the
	// application-level icon.
	QApplication::setWindowIcon(QIcon(myAppDataPath + "gfx/gui/misc/windowicon.png"));
	// On Wayland (Qt 6.8 deploy) setWindowIcon does not reach the compositor;
	// the icon is resolved from the matching .desktop file via the surface
	// app_id. Pin the app_id to the installed desktop-file basename so the
	// portable launcher's ~/.local/share entry is matched by KWin.
	QApplication::setDesktopFileName(QStringLiteral("pokerth"));

	//set QApplication default font

	QFontDatabase::addApplicationFont (myAppDataPath +"fonts/n019003l.pfb");
	QFontDatabase::addApplicationFont (myAppDataPath +"fonts/DejaVuSans-Bold.ttf");
	// Gebündelter Farb-Emoji-Font (Noto Color Emoji, CBDT/CBLC-Bitmap-Variante),
	// damit Emojis distributionsunabhängig erscheinen und nicht von der
	// System-FreeType-Version (COLRv1 erst ab 2.13) abhängen.
	QFontDatabase::addApplicationFont (myAppDataPath +"fonts/NotoColorEmoji.ttf");

#ifdef _WIN32
	QString font1String("QApplication, QWidget, QDialog { font-size: 12px; }");
#elif __APPLE__
	//            QString font1String("font-family: \"Lucida Grande\";");
	QString font1String("QApplication, QWidget, QDialog { font-size: 11px; }");
#elif ANDROID
	// Auto-scale fonts for high-DPI Android screens.
	// The 800×480 mobile layout was designed with 26px base font at exactly
	// 800×480 logical resolution.  Modern phones (e.g. Galaxy S23 at ~780×360
	// logical landscape) have less vertical space, so scale proportionally.
	int androidBaseFontPx = 26;
	{
		int userScale = myConfig->readConfigInt("AndroidUiScalePercent");
		if (userScale > 0) {
			// Manual override: percentage of the reference 26px size.
			androidBaseFontPx = qMax(10, 26 * userScale / 100);
		} else {
			// Auto: scale by min(screenW/800, screenH/480).
			QScreen *scr = QGuiApplication::primaryScreen();
			if (scr) {
				QRect geo = scr->availableGeometry();
				int sw = qMax(geo.width(), geo.height());   // landscape width
				int sh = qMin(geo.width(), geo.height());   // landscape height
				qreal scale = qMin(static_cast<qreal>(sw) / 800.0,
				                   static_cast<qreal>(sh) / 480.0);
				scale = qBound(0.5, scale, 1.0);
				androidBaseFontPx = qMax(10, static_cast<int>(26.0 * scale + 0.5));
			}
		}
	}
	QString font1String(QString("QApplication, QWidget, QDialog { font-family: \"Nimbus Sans L\"; font-size: %1px; }").arg(androidBaseFontPx));
	QPalette p = a.palette();
	p.setColor(QPalette::Button, QColor::fromRgb(80,80,80));
	p.setColor(QPalette::Base, QColor::fromRgb(80,80,80));
	p.setColor(QPalette::Window, QColor::fromRgb(50,50,50));
	p.setColor(QPalette::ButtonText, QColor::fromRgb(255,255,255));
	p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor::fromRgb(130,130,130));
	p.setColor(QPalette::WindowText, QColor::fromRgb(255,255,255));
	p.setColor(QPalette::Disabled, QPalette::WindowText, QColor::fromRgb(100,100,100));
	p.setColor(QPalette::Text, QColor::fromRgb(255,255,255));
	p.setColor(QPalette::Disabled, QPalette::Text, QColor::fromRgb(100,100,100));
	p.setColor(QPalette::Link, QColor::fromRgb(192,192,255));
	p.setColor(QPalette::LinkVisited, QColor::fromRgb(192,192,255));
	a.setPalette(p);
#elif MAEMO
	QString font1String("QApplication, QWidget, QDialog { font-family: \"Nimbus Sans L\"; font-size: 22px; }");
	QPalette p = a.palette();
	p.setColor(QPalette::Button, QColor::fromRgb(80,80,80));
	p.setColor(QPalette::Base, QColor::fromRgb(80,80,80));
	p.setColor(QPalette::Window, QColor::fromRgb(50,50,50));
	p.setColor(QPalette::ButtonText, QColor::fromRgb(255,255,255));
	p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor::fromRgb(100,100,100));
	p.setColor(QPalette::WindowText, QColor::fromRgb(255,255,255));
	p.setColor(QPalette::Disabled, QPalette::WindowText, QColor::fromRgb(100,100,100));
	p.setColor(QPalette::Text, QColor::fromRgb(255,255,255));
	p.setColor(QPalette::Disabled, QPalette::Text, QColor::fromRgb(100,100,100));
	p.setColor(QPalette::Link, QColor::fromRgb(192,192,255));
	p.setColor(QPalette::LinkVisited, QColor::fromRgb(192,192,255));
	a.setPalette(p);
#else
	QString font1String("QApplication, QWidget, QDialog { font-family: \"Nimbus Sans L\"; font-size: 12px; }");
	// Apply dark mode palette based on config setting
	DarkModeHelper::applyPalette(myConfig);
#endif
	qApp->setStyleSheet(font1String + " QDialogButtonBox, QMessageBox { dialogbuttonbox-buttons-have-icons: 1; dialog-ok-icon: url(:/gfx/dialog_ok_apply.png); dialog-cancel-icon: url(:/gfx/dialog_close.png); dialog-close-icon: url(:/gfx/dialog_close.png); dialog-yes-icon: url(:/gfx/dialog_ok_apply.png); dialog-no-icon: url(:/gfx/dialog_close.png) }");

#ifdef ANDROID
	//check if custom background pictures for the resolution are there. Otherwise create them!
	QString UserDataDir = QString::fromUtf8(myConfig->readConfigString("UserDataDir").c_str());
	QScreen *screen = QGuiApplication::primaryScreen();
	QRect screenGeometry = screen->geometry();
	int screenWidth = screenGeometry.width();
	int screenHeight = screenGeometry.height(); 
	QString customStartWindowBgFileString(UserDataDir+"/startwindowbg10_"+QString::number(screenWidth)+"x"+QString::number(screenHeight)+".png");
	QString customWelcomePokerTHFileString(UserDataDir+"/welcomepokerth10_"+QString::number(screenWidth)+"x"+QString::number(screenHeight)+".png");
	QFile customStartWindowBgFile(customStartWindowBgFileString);
	QFile customWelcomePokerTHFile(customWelcomePokerTHFileString);

	QSplashScreen preSplashFirstRun;
	if(!customStartWindowBgFile.exists()) {

		//load preSplashPix to show that PokerTH is already running during first time pics calculation
		QPixmap prePixBase(":/gfx/logoChip3D.png");
		QPixmap prePix(300, 200);
		prePix.fill(Qt::transparent); // force alpha channel
		{
			QPainter painter(&prePix);
			painter.drawPixmap(0, 0, prePixBase);
			painter.setPen(Qt::white);
			painter.drawText(10, 160, "loading ...");
		}
		preSplashFirstRun.setPixmap(prePix);
		preSplashFirstRun.show();

		QPixmap pix(":/android/android-data/gfx/gui/misc/startwindowbg10_mobile.png");
		pix = pix.scaled(screenWidth, screenHeight, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
		pix.save(customStartWindowBgFileString);
	}

	if(!customWelcomePokerTHFile.exists()) {
		QPixmap base(customStartWindowBgFileString);
		//scale overlay "have a lot of fun" at first
		QPixmap overlay(":/android/android-data/gfx/gui/misc/welcomepokerth10_mobile.png");
		overlay = overlay.scaled(screenWidth, screenHeight, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
		QPixmap result(base.width(), base.height());
		result.fill(Qt::transparent); // force alpha channel
		{
			QPainter painter(&result);
			painter.drawPixmap(0, 0, base);
			painter.drawPixmap(0, 0, overlay);
		}
		result.save(customWelcomePokerTHFileString);
		preSplashFirstRun.hide();
	}

	QPixmap pixmap;
	if(customWelcomePokerTHFile.exists()) {
		pixmap.load(QFileInfo(customWelcomePokerTHFile).absoluteFilePath());
	} else {
		//if custom welcome pic could not be saved locally we need to scale it on the fly
		pixmap.load(":/android/android-data/gfx/gui/misc/welcomepokerth10_mobile.png");
		pixmap = pixmap.scaled(screenWidth, screenHeight, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
	}

#else
	QPixmap pixmap(myAppDataPath + "gfx/gui/misc/welcomepokerth10_desktop.png");
#endif
	StartSplash splash(pixmap);
	if(!myConfig->readConfigInt("DisableSplashScreenOnStartup")) {
		splash.show();
		splash.showMessage(QString("Version %1").arg(POKERTH_BETA_RELEASE_STRING), 0x0042, QColor(255,255,255));
	}

	//Set translations
	QTranslator qtTranslator;
	if (qtTranslator.load(QString(myAppDataPath +"translations/qt_") + QString::fromStdString(myConfig->readConfigString("Language")))) {
		a.installTranslator(&qtTranslator);
	}
	QTranslator translator;
	if (translator.load(QString(myAppDataPath +"translations/pokerth_") + QString::fromStdString(myConfig->readConfigString("Language")))) {
		a.installTranslator(&translator);
	}

	qRegisterMetaType<unsigned>("unsigned");
	qRegisterMetaType<boost::shared_ptr<Game> >("boost::shared_ptr<Game>");
	qRegisterMetaType<ServerStats>("ServerStats");
	qRegisterMetaType<DenyGameInvitationReason>("DenyGameInvitationReason");
	///////////////////////////////////////////////////

	startWindowImpl mainWin(myConfig,myLog);
#ifdef ANDROID
// 	//Do not start if API is smaller than x
// 	int api = -2;
// #ifndef ANDROID_TEST
// 	JavaVM *currVM = (JavaVM *)QApplication::platformNativeInterface()->nativeResourceForIntegration("JavaVM");
// 	JNIEnv* env;
// 	if (currVM->AttachCurrentThread(&env, NULL)<0) {
// 		qCritical()<<"AttachCurrentThread failed";
// 	} else {
// 		jclass jclassApplicationClass = env->FindClass("android/os/Build$VERSION");
// 		if (jclassApplicationClass) {
// 			api = env->GetStaticIntField(jclassApplicationClass, env->GetStaticFieldID(jclassApplicationClass,"SDK_INT", "I"));
// 		}
// 		currVM->DetachCurrentThread();
// 	}
// #endif
// Test api and maybe do not start for further android releases
//	if(api < 14) {
//		QMessageBox box(QMessageBox::Critical, "PokerTH Error", "Sorry, PokerTH needs Android version 4.0 or above to start", QMessageBox::Ok);
//		box.show();
//	}
//	else {
//		mainWin.show();
//	}
	mainWin.show();
#else
	// a.setActivationWindow(&mainWin, true);
#endif
	int retVal = a.exec();
	return retVal;
}


// END OF OLD QT-WIDGETS GUI SECTION
#endif
