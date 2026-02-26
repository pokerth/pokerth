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

#ifndef APPIMAGE_UTILS_H
#define APPIMAGE_UTILS_H

#include <QUrl>
#include <QProcess>
#include <QProcessEnvironment>
#include <QDesktopServices>
#include <QString>
#include <QStringList>
#include <QLabel>
#include <QTextBrowser>
#include <QWidget>
#include <cstdlib>

#ifdef Q_OS_LINUX
#include <QDebug>
#endif

/**
 * Utility functions for AppImage compatibility.
 *
 * Problem: When running inside an AppImage with a bundled glibc/ld-linux,
 * the LD_LIBRARY_PATH and LD_PRELOAD environment variables point to the
 * bundled libraries. Child processes (like xdg-open) inherit these variables
 * but use the system's ld-linux loader, causing a glibc version mismatch
 * and SIGSEGV.
 *
 * Solution: When POKERTH_APPIMAGE=1 is set (by the AppRun script), we
 * sanitize the environment before launching external processes:
 *  - openUrlSafe(): replaces QDesktopServices::openUrl()
 *  - cleanProcessEnvironment(): returns a QProcessEnvironment without
 *    the AppImage-specific LD variables, for use with QProcess.
 */
namespace AppImageUtils {

/**
 * Returns true if running inside an AppImage (POKERTH_APPIMAGE=1).
 */
inline bool isAppImage()
{
#ifdef Q_OS_LINUX
    const char* val = std::getenv("POKERTH_APPIMAGE");
    return val && QString::fromLatin1(val) == QLatin1String("1");
#else
    return false;
#endif
}

/**
 * Returns the original LD_LIBRARY_PATH that was active before the AppImage
 * injected its own paths. The AppRun saves it as POKERTH_ORIG_LD_LIBRARY_PATH.
 * Returns empty string if not set.
 */
inline QString origLdLibraryPath()
{
    const char* val = std::getenv("POKERTH_ORIG_LD_LIBRARY_PATH");
    return val ? QString::fromLocal8Bit(val) : QString();
}

/**
 * Returns a sanitized QProcessEnvironment suitable for launching
 * external host processes from within the AppImage.
 *
 * Restores the original LD_LIBRARY_PATH (before AppImage modification)
 * and removes LD_PRELOAD to prevent the bundled glibc from leaking
 * into child processes like xdg-open, paplay, etc.
 */
inline QProcessEnvironment cleanProcessEnvironment()
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    if (!isAppImage()) {
        return env;
    }

    // Restore original LD_LIBRARY_PATH or remove entirely
    QString origLdPath = origLdLibraryPath();
    if (origLdPath.isEmpty()) {
        env.remove(QStringLiteral("LD_LIBRARY_PATH"));
    } else {
        env.insert(QStringLiteral("LD_LIBRARY_PATH"), origLdPath);
    }

    // Remove LD_PRELOAD — the bundled libs must not be preloaded in host processes
    env.remove(QStringLiteral("LD_PRELOAD"));

    return env;
}

/**
 * Opens a URL safely, even from within an AppImage.
 *
 * On non-AppImage builds, or on non-Linux platforms, this simply
 * delegates to QDesktopServices::openUrl().
 *
 * Inside an AppImage, it launches xdg-open as a detached process
 * with a sanitized environment (no bundled LD_LIBRARY_PATH/LD_PRELOAD).
 */
inline bool openUrlSafe(const QUrl& url)
{
#ifdef Q_OS_LINUX
    if (isAppImage()) {
        QProcess process;
        process.setProcessEnvironment(cleanProcessEnvironment());
        process.setProgram(QStringLiteral("xdg-open"));
        process.setArguments({url.toString()});

        return process.startDetached();
    }
#endif
    return QDesktopServices::openUrl(url);
}

/**
 * Launches a detached external process with a sanitized environment.
 * Use this instead of QProcess::startDetached() for host-native tools
 * like paplay, pw-play, etc.
 *
 * On non-AppImage builds this is equivalent to QProcess::startDetached().
 */
inline bool startDetachedSafe(const QString& program, const QStringList& args)
{
#ifdef Q_OS_LINUX
    if (isAppImage()) {
        QProcess process;
        process.setProcessEnvironment(cleanProcessEnvironment());
        process.setProgram(program);
        process.setArguments(args);
        return process.startDetached();
    }
#endif
    return QProcess::startDetached(program, args);
}

/**
 * Patches all QLabels and QTextBrowsers with openExternalLinks=true
 * inside the given widget tree.
 *
 * When running inside an AppImage, openExternalLinks causes Qt to call
 * QDesktopServices::openUrl() internally, which inherits the bundled
 * LD_LIBRARY_PATH and crashes child processes (xdg-open, /bin/sh, etc.).
 *
 * This function:
 *  1. Finds all QLabel children with openExternalLinks == true
 *     - Disables openExternalLinks
 *     - Connects linkActivated to openUrlSafe()
 *  2. Finds all QTextBrowser children with openExternalLinks == true
 *     (e.g. chat displays in lobby and game table)
 *     - Disables openExternalLinks and openLinks
 *     - Connects anchorClicked to openUrlSafe()
 *
 * Call this once after setupUi() in each dialog constructor.
 * On non-AppImage builds this is a no-op.
 */
inline void patchExternalLinks(QWidget* root)
{
#ifdef Q_OS_LINUX
    if (!isAppImage() || !root) {
        return;
    }

    // Patch QLabels
    const auto labels = root->findChildren<QLabel*>();
    for (QLabel* label : labels) {
        if (label->openExternalLinks()) {
            label->setOpenExternalLinks(false);
            QObject::connect(label, &QLabel::linkActivated, [](const QString& urlString) {
                openUrlSafe(QUrl(urlString));
            });
        }
    }

    // Patch QTextBrowsers (chat displays, about dialogs, etc.)
    const auto browsers = root->findChildren<QTextBrowser*>();
    for (QTextBrowser* browser : browsers) {
        if (browser->openExternalLinks()) {
            // Disable Qt's built-in external link handling
            browser->setOpenExternalLinks(false);
            // Also disable openLinks so QTextBrowser doesn't try to
            // navigate internally — we handle all clicks ourselves.
            browser->setOpenLinks(false);
            QObject::connect(browser, &QTextBrowser::anchorClicked, [](const QUrl& url) {
                openUrlSafe(url);
            });
        }
    }
#else
    Q_UNUSED(root);
#endif
}

} // namespace AppImageUtils

#endif // APPIMAGE_UTILS_H
