/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 *                                                                           *
 * Slim ZIP helpers (miniz) for the style import/export of the QML client.   *
 *****************************************************************************/

#ifndef ZIPUTILS_H
#define ZIPUTILS_H

#include <QString>
#include <QByteArray>

// Thin wrappers around miniz for the style import/export (.zip). Input and
// output deliberately go through QByteArray instead of FILE*, so that Flatpak
// portal and Android content:// paths are served as well (they do not let a
// regular FILE* be opened, but can be read/written via QFile).
namespace ZipUtils
{
// Extracts a ZIP passed as a memory block into destDir. Secured against
// zip slip (path traversal via "../" or absolute entries) and against zip
// bombs (total extracted size and entry count limited). On errors, error is
// set and false is returned; a possible partial result in destDir has to be
// cleaned up by the caller.
bool extractArchive(const QByteArray &zipData, const QString &destDir, QString &error);

// Packs the content of srcDir recursively into a ZIP and puts all entries
// under rootName/…, so that the archive contains a dedicated style folder
// (which the import recognises as the folder name = style name). The return
// value is the archive memory block; on errors, error is set and an empty
// QByteArray is returned.
QByteArray createArchive(const QString &srcDir, const QString &rootName, QString &error);
}

#endif // ZIPUTILS_H
