/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 *                                                                           *
 * Schlanke ZIP-Helfer (miniz) für den Stil-Import/-Export des QML-Clients.   *
 *****************************************************************************/

#include "ziputils.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QObject>

#include <cstring>

#include "miniz.h"

namespace
{
// Obergrenzen fürs Entpacken. Ein regulärer Stil (Kartenstapel: 52 SVGs + XML +
// Preview) bleibt weit darunter; die Grenzen schützen vor manipulierten
// Archiven (Zip-Bombe).
constexpr quint64 kMaxTotalUncompressed = 100ull * 1024 * 1024; // 100 MiB
constexpr mz_uint kMaxEntries = 1000;

// Prüft, ob ein Archiv-Eintragsname sicher unterhalb des Zielordners bleibt:
// keine absoluten Pfade, keine Windows-Laufwerke und kein ".."-Segment.
bool isSafeEntryName(const QString &name)
{
    if (name.isEmpty())
        return false;
    if (name.startsWith(QLatin1Char('/')) || name.startsWith(QLatin1Char('\\')))
        return false;
    if (name.size() >= 2 && name.at(1) == QLatin1Char(':'))
        return false;
    QString normalized = name;
    normalized.replace(QLatin1Char('\\'), QLatin1Char('/'));
    const auto parts = normalized.split(QLatin1Char('/'));
    for (const QString &part : parts) {
        if (part == QLatin1String(".."))
            return false;
    }
    return true;
}
} // namespace

bool ZipUtils::extractArchive(const QByteArray &zipData, const QString &destDir, QString &error)
{
    mz_zip_archive zip;
    std::memset(&zip, 0, sizeof(zip));
    if (!mz_zip_reader_init_mem(&zip, zipData.constData(),
                                static_cast<size_t>(zipData.size()), 0)) {
        error = QObject::tr("Das Archiv konnte nicht gelesen werden.");
        return false;
    }

    const mz_uint count = mz_zip_reader_get_num_files(&zip);
    if (count == 0 || count > kMaxEntries) {
        mz_zip_reader_end(&zip);
        error = QObject::tr("Das Archiv ist leer oder enthält zu viele Dateien.");
        return false;
    }

    QDir dest(destDir);
    if (!QDir().mkpath(dest.absolutePath())) {
        mz_zip_reader_end(&zip);
        error = QObject::tr("Das Zielverzeichnis konnte nicht angelegt werden.");
        return false;
    }
    const QString destRoot = dest.absolutePath() + QLatin1Char('/');

    quint64 totalUncompressed = 0;
    bool ok = true;
    for (mz_uint i = 0; i < count && ok; ++i) {
        mz_zip_archive_file_stat st;
        if (!mz_zip_reader_file_stat(&zip, i, &st)) {
            error = QObject::tr("Ein Archiv-Eintrag konnte nicht gelesen werden.");
            ok = false;
            break;
        }

        const QString entryName = QString::fromUtf8(st.m_filename);
        if (!isSafeEntryName(entryName)) {
            error = QObject::tr("Das Archiv enthält einen ungültigen Pfad: %1").arg(entryName);
            ok = false;
            break;
        }

        const QString outPath = QFileInfo(dest.absoluteFilePath(entryName)).absoluteFilePath();
        // Zweite Absicherung gegen Zip-Slip: Zielpfad muss unter destRoot liegen.
        if (!outPath.startsWith(destRoot)) {
            error = QObject::tr("Das Archiv enthält einen ungültigen Pfad: %1").arg(entryName);
            ok = false;
            break;
        }

        if (mz_zip_reader_is_file_a_directory(&zip, i)) {
            if (!QDir().mkpath(outPath)) {
                error = QObject::tr("Verzeichnis konnte nicht angelegt werden: %1").arg(entryName);
                ok = false;
            }
            continue;
        }

        totalUncompressed += st.m_uncomp_size;
        if (totalUncompressed > kMaxTotalUncompressed) {
            error = QObject::tr("Das Archiv ist ungewöhnlich groß.");
            ok = false;
            break;
        }

        if (!QDir().mkpath(QFileInfo(outPath).absolutePath())) {
            error = QObject::tr("Verzeichnis konnte nicht angelegt werden: %1").arg(entryName);
            ok = false;
            break;
        }

        size_t size = 0;
        void *data = mz_zip_reader_extract_to_heap(&zip, i, &size, 0);
        if (!data) {
            error = QObject::tr("Ein Archiv-Eintrag konnte nicht entpackt werden: %1").arg(entryName);
            ok = false;
            break;
        }

        QFile out(outPath);
        if (!out.open(QIODevice::WriteOnly)
            || out.write(reinterpret_cast<const char *>(data), static_cast<qint64>(size))
                   != static_cast<qint64>(size)) {
            error = QObject::tr("Datei konnte nicht geschrieben werden: %1").arg(entryName);
            ok = false;
        }
        out.close();
        mz_free(data);
    }

    mz_zip_reader_end(&zip);
    return ok;
}

QByteArray ZipUtils::createArchive(const QString &srcDir, const QString &rootName, QString &error)
{
    QDir base(srcDir);
    if (!base.exists()) {
        error = QObject::tr("Der Stil-Ordner wurde nicht gefunden.");
        return QByteArray();
    }

    mz_zip_archive zip;
    std::memset(&zip, 0, sizeof(zip));
    if (!mz_zip_writer_init_heap(&zip, 0, 0)) {
        error = QObject::tr("Das Archiv konnte nicht erstellt werden.");
        return QByteArray();
    }

    bool ok = true;
    bool anyFile = false;
    QDirIterator it(base.absolutePath(), QDir::Files | QDir::NoDotAndDotDot,
                    QDirIterator::Subdirectories);
    while (it.hasNext() && ok) {
        const QString filePath = it.next();
        const QString rel = base.relativeFilePath(filePath);
        // Alle Einträge unter rootName/… ablegen: das Archiv enthält damit einen
        // eigenen Stil-Ordner, den der Import als Stilnamen erkennt.
        const QString archiveName = rootName + QLatin1Char('/') + rel;

        QFile f(filePath);
        if (!f.open(QIODevice::ReadOnly)) {
            error = QObject::tr("Datei konnte nicht gelesen werden: %1").arg(rel);
            ok = false;
            break;
        }
        const QByteArray content = f.readAll();
        f.close();

        if (!mz_zip_writer_add_mem(&zip, archiveName.toUtf8().constData(),
                                   content.constData(), static_cast<size_t>(content.size()),
                                   MZ_DEFAULT_LEVEL)) {
            error = QObject::tr("Datei konnte nicht ins Archiv geschrieben werden: %1").arg(rel);
            ok = false;
            break;
        }
        anyFile = true;
    }

    if (ok && !anyFile) {
        error = QObject::tr("Der Stil-Ordner enthält keine Dateien.");
        ok = false;
    }

    QByteArray result;
    if (ok) {
        void *buf = nullptr;
        size_t size = 0;
        if (mz_zip_writer_finalize_heap_archive(&zip, &buf, &size)) {
            result = QByteArray(reinterpret_cast<const char *>(buf), static_cast<qsizetype>(size));
            mz_free(buf);
        } else {
            error = QObject::tr("Das Archiv konnte nicht abgeschlossen werden.");
        }
    }
    mz_zip_writer_end(&zip);
    return result;
}
