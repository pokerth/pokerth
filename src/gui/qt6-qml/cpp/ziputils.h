/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 *                                                                           *
 * Schlanke ZIP-Helfer (miniz) für den Stil-Import/-Export des QML-Clients.   *
 *****************************************************************************/

#ifndef ZIPUTILS_H
#define ZIPUTILS_H

#include <QString>
#include <QByteArray>

// Dünne Wrapper um miniz für den Stil-Import/-Export (.zip). Ein- und Ausgabe
// laufen bewusst über QByteArray statt über FILE*, damit auch Flatpak-Portal-
// und Android-content://-Pfade bedient werden (die kein reguläres FILE* öffnen
// lassen, wohl aber über QFile lesbar/schreibbar sind).
namespace ZipUtils
{
// Entpackt ein als Speicherblock übergebenes ZIP nach destDir. Abgesichert
// gegen Zip-Slip (Pfad-Traversal via "../" oder absolute Einträge) und gegen
// Zip-Bomben (Gesamt-Entpackgröße und Eintragszahl begrenzt). Bei Fehlern wird
// error gesetzt und false zurückgegeben; ein evtl. Teil-Ergebnis in destDir
// muss der Aufrufer aufräumen.
bool extractArchive(const QByteArray &zipData, const QString &destDir, QString &error);

// Packt den Inhalt von srcDir rekursiv in ein ZIP und legt alle Einträge unter
// rootName/… ab, sodass das Archiv einen dedizierten Stil-Ordner enthält (den
// der Import als Ordnernamen = Stilname erkennt). Rückgabe ist der Archiv-
// Speicherblock; bei Fehlern wird error gesetzt und ein leeres QByteArray
// zurückgegeben.
QByteArray createArchive(const QString &srcDir, const QString &rootName, QString &error);
}

#endif // ZIPUTILS_H
