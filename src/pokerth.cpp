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
#include <QQuickStyle>
#include <QIcon>
#include <boost/shared_ptr.hpp>
#include "configfile.h"
#include "session.h"
#include "log.h"
#include <QTranslator>
#include <QQmlContext>
#include <QDebug>
#include <retranslate.h>
#include <settingsxmlhandler.h>
#include <settingsmanager.h>
#include "gui/qt6-qml/cpp/serverconnectionhandler.h"
#include "gui/qt6-qml/cpp/lobbyhandler.h"
#include "gui/qt6-qml/cpp/qmlguiinterface.h"

int main(int argc, char *argv[])
{
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

    boost::shared_ptr<ConfigFile> myConfig;
    myConfig.reset(new ConfigFile(argv[0], false));

    // make QSettings use the default PokerTH config.xml :
	const QSettings::Format XmlFormat = QSettings::registerFormat("xml", &SettingsXmlHandler::readXmlFile, &SettingsXmlHandler::writeXmlFile);
    QFileInfo fi(QString::fromStdString(myConfig->configFileName));
    QSettings::setPath(XmlFormat, QSettings::UserScope, fi.absolutePath().remove("/.pokerth"));
    QSettings settings(XmlFormat, QSettings::UserScope, ".pokerth", "config");

    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE"))
        QQuickStyle::setStyle(settings.value("style").toString());
    const QString styleInSettings = settings.value("style").toString();
    if (styleInSettings.isEmpty())
        settings.setValue(QLatin1String("style"), QQuickStyle::name());

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
    
    // Create GUI Interface with handlers
    QmlGuiInterface *guiInterface = new QmlGuiInterface(myConfig.get(), connectionHandler, lobbyHandler);
    
    // Create and initialize Session
    boost::shared_ptr<Session> session;
    try {
        session.reset(new Session(guiInterface, myConfig.get(), log));
        int initResult = session->init();
        if (initResult != 0) {
            qWarning() << "Session initialization failed with code:" << initResult;
        }
        log->init();
        
        guiInterface->setSession(session);
        connectionHandler->setSession(session);
        lobbyHandler->setSession(session);
        
        // qDebug() << "Session initialized successfully";
    } catch (const std::exception &e) {
        qWarning() << "Exception during session init:" << e.what();
    }

    QQmlApplicationEngine engine;

    SettingsManager settingsMgr(myConfig);
    LanguageManager langMgr(&engine);
    engine.rootContext()->setContextProperty("SettingsManager", &settingsMgr);
    engine.rootContext()->setContextProperty("LanguageManager", &langMgr);
    engine.rootContext()->setContextProperty("ServerConnection", connectionHandler);
    engine.rootContext()->setContextProperty("Lobby", lobbyHandler);
	engine.load(QUrl(QStringLiteral("qrc:/pokerth.qml")));

	if (engine.rootObjects().isEmpty()) {
        delete lobbyHandler;
        delete connectionHandler;
        delete guiInterface;
        delete log;
        return -1;
    }

    int result = app.exec();
    
    // Cleanup
    if (session) {
        session->terminateNetworkClient();
    }
    delete lobbyHandler;
    delete connectionHandler;
    delete guiInterface;
    delete log;
    
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
	//set QApplication default font

	QFontDatabase::addApplicationFont (myAppDataPath +"fonts/n019003l.pfb");
	QFontDatabase::addApplicationFont (myAppDataPath +"fonts/DejaVuSans-Bold.ttf");

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

	// bbcbot code - check for password in argv[1] for auto-login
	std::string bbcbotPassword;
	if (argc > 1) {
		bbcbotPassword = argv[1];
	}

	startWindowImpl mainWin(myConfig, myLog, bbcbotPassword);
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
