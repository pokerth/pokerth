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

#ifndef AVATARIMPORT_H
#define AVATARIMPORT_H

#include <QBuffer>
#include <QByteArray>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QString>

#include <core/avatarmanager.h>

/**
 * Auswahl und Prüfung eigener Avatar-Bilder – gemeinsam genutzt vom
 * QML-Client (SettingsManager) und vom Qt-Widgets-Client (Avatar-Dialog).
 *
 * Ob ein Bild taugt, entscheidet nicht diese Datei, sondern die Engine:
 * AvatarManager::OpenAvatarFileForChunkRead() ist dieselbe Funktion, die den
 * Avatar später zum Server hochlädt und die der Server beim Ausliefern
 * anwendet. Nur so können Warnung und Server-Prüfung nicht auseinanderlaufen.
 *
 * Wichtig ist dabei, dass die Dateigröße allein nichts aussagt: Der Server
 * begrenzt seit 2.1.8 auch die Bildabmessungen (eine stark komprimierte Datei
 * kann beim Dekodieren im Client sonst beliebig viel Speicher belegen). Ein
 * flächiges 2000x2000-PNG bleibt aber locker unter 30 KB – genau solche
 * Bilder waren jahrelang als Avatar gesetzt und werden seither abgelehnt.
 */
namespace AvatarImport
{

// Kantenlängen, auf die ein zu großes Bild der Reihe nach heruntergerechnet
// wird, bis es die Engine-Prüfung besteht (Avatare werden ohnehin klein
// dargestellt).
inline QList<int> scaleSteps()
{
	return QList<int>{ 192, 128, 96, 64 };
}

// Engine-Prüfung einer Datei: Größe, Format und Bildabmessungen. Ein leerer
// Pfad gilt als in Ordnung – kein Avatar gewählt ist kein Fehler.
inline bool isUsable(const QString &path)
{
	if (path.isEmpty())
		return true;

	unsigned fileSize = 0;
	AvatarFileType fileType = AVATAR_FILE_TYPE_UNKNOWN;
	return AvatarManager::OpenAvatarFileForChunkRead(
			   path.toStdString(), fileSize, fileType).get() != nullptr;
}

// Dieselbe Prüfung auf bereits eingelesenen Daten – für Bilder, die noch gar
// nicht als Datei vorliegen (Android-content://-URIs, frisch skalierte Bilder).
inline bool isDataUsable(const QByteArray &data, const QString &ext)
{
	if (data.size() < MIN_AVATAR_FILE_SIZE || data.size() > MAX_AVATAR_FILE_SIZE)
		return false;
	// GetAvatarFileType() erwartet einen Dateinamen und liest dessen Endung.
	// Ein blosses ".png" wäre für boost::filesystem ein Name ohne Endung,
	// daher der Dummy-Stamm davor.
	const AvatarFileType type =
		AvatarManager::GetAvatarFileType((QStringLiteral("avatar") + ext).toStdString());
	return AvatarManager::IsValidAvatarFileType(
			   type, reinterpret_cast<const unsigned char *>(data.constData()),
			   static_cast<size_t>(data.size()));
}

/**
 * Übernimmt eine Bildauswahl als eigenen Avatar.
 *
 * Engine-taugliche lokale Dateien werden unverändert durchgereicht (der
 * Widget-Client speichert seit jeher den gewählten Pfad). In zwei Fällen
 * entsteht dagegen eine neue Datei unter <userDataDir>gfx/avatars/user/:
 *  - Android-content://-URIs sind nur für die laufende Sitzung lesbar und
 *    weder von der Bildvorschau noch von der Engine (std::ifstream beim
 *    Avatar-Upload) zu öffnen.
 *  - Bilder, welche die Engine ablehnt: zu groß in Kilobyte (typisch für
 *    Fotos aus der Galerie) oder zu groß in Pixeln (typisch für flächige
 *    Grafiken, die trotz großer Abmessungen winzige Dateien ergeben).
 *
 * Der Dateiname ist der MD5 des Inhalts (Namenskonvention des
 * AvatarManagers), so entstehen bei wiederholter Auswahl keine Duplikate.
 * Rückgabe: der zu speichernde Pfad, leer bei Abbruch oder Fehler.
 *
 * userDataDir muss mit einem Verzeichnis-Trennzeichen enden (wie der
 * gleichnamige Config-Wert).
 */
inline QString importImage(const QString &picked, const QString &userDataDir)
{
	if (picked.isEmpty())
		return picked;

	const bool isContentUri = picked.startsWith(QLatin1String("content:"));
	if (!isContentUri && isUsable(picked))
		return picked;

	QFile src(picked);
	if (!src.open(QIODevice::ReadOnly))
		return QString();
	QByteArray data = src.readAll();
	if (data.isEmpty())
		return QString();

	// Dateiendung anhand des Dateikopfs bestimmen (content://-URIs haben
	// keine; die Engine erkennt den Avatar-Typ an der Endung).
	QString ext;
	if (data.startsWith("\x89PNG"))
		ext = QStringLiteral(".png");
	else if (data.startsWith("\xFF\xD8\xFF"))
		ext = QStringLiteral(".jpg");
	else if (data.startsWith("GIF8"))
		ext = QStringLiteral(".gif");
	else
		return QString();

	if (!isDataUsable(data, ext)) {
		// Neu kodieren und stufenweise verkleinern, bis das Ergebnis die
		// Engine-Prüfung besteht. Animierte GIFs werden dabei zum Standbild.
		// QImage::fromData() liefert bei absurd großen Bildern ein Nullbild
		// (Qt begrenzt die Allokation beim Dekodieren), was hier korrekt als
		// Fehlschlag ankommt.
		const QImage img = QImage::fromData(data);
		if (img.isNull())
			return QString();
		const char *format = img.hasAlphaChannel() ? "PNG" : "JPG";
		ext = img.hasAlphaChannel() ? QStringLiteral(".png") : QStringLiteral(".jpg");
		QByteArray scaledData;
		const QList<int> steps = scaleSteps();
		for (int edge : steps) {
			const QImage scaled = (img.width() > edge || img.height() > edge)
								  ? img.scaled(edge, edge, Qt::KeepAspectRatio, Qt::SmoothTransformation)
								  : img;
			QByteArray out;
			QBuffer buffer(&out);
			buffer.open(QIODevice::WriteOnly);
			if (!scaled.save(&buffer, format, 85))
				return QString();
			if (isDataUsable(out, ext)) {
				scaledData = out;
				break;
			}
		}
		if (scaledData.isEmpty())
			return QString();
		data = scaledData;
	}

	const QString dirPath = userDataDir + QStringLiteral("gfx/avatars/user");
	if (!QDir().mkpath(dirPath))
		return QString();

	const QString target = dirPath + QStringLiteral("/")
						   + QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex())
						   + ext;
	if (!QFile::exists(target)) {
		QFile dst(target);
		if (!dst.open(QIODevice::WriteOnly) || dst.write(data) != data.size()) {
			dst.remove();
			return QString();
		}
	}
	return target;
}

} // namespace AvatarImport

#endif // AVATARIMPORT_H
