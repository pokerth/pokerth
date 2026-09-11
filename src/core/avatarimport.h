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
 * Selection and validation of your own avatar images – shared by the
 * QML client (SettingsManager) and the Qt widgets client (avatar dialog).
 *
 * Whether an image is suitable is not decided by this file but by the engine:
 * AvatarManager::OpenAvatarFileForChunkRead() is the same function that later
 * uploads the avatar to the server and that the server applies when serving
 * it. Only that way can the warning and the server check not drift apart.
 *
 * What matters here is that the file size alone says nothing: since 2.1.8 the
 * server also limits the image dimensions (a heavily compressed file
 * could otherwise take arbitrarily much memory when decoded in the client). A
 * flat 2000x2000 PNG stays easily below 30 KB though – exactly such
 * images were set as avatars for years and have been rejected ever since.
 */
namespace AvatarImport
{

// Edge lengths to which a too large image is scaled down one after another
// until it passes the engine check (avatars are displayed small
// anyway).
inline QList<int> scaleSteps()
{
	return QList<int> { 192, 128, 96, 64 };
}

// Engine check of a file: size, format and image dimensions. An empty
// path counts as fine – no avatar chosen is not an error.
inline bool isUsable(const QString &path)
{
	if (path.isEmpty())
		return true;

	unsigned fileSize = 0;
	AvatarFileType fileType = AVATAR_FILE_TYPE_UNKNOWN;
	return AvatarManager::OpenAvatarFileForChunkRead(
			   path.toStdString(), fileSize, fileType).get() != nullptr;
}

// The same check on data already read in – for images that do not exist
// as a file at all yet (Android content:// URIs, freshly scaled images).
inline bool isDataUsable(const QByteArray &data, const QString &ext)
{
	if (data.size() < MIN_AVATAR_FILE_SIZE || data.size() > MAX_AVATAR_FILE_SIZE)
		return false;
	// GetAvatarFileType() expects a file name and reads its extension.
	// A bare ".png" would be a name without an extension for boost::filesystem,
	// hence the dummy stem in front of it.
	const AvatarFileType type =
		AvatarManager::GetAvatarFileType((QStringLiteral("avatar") + ext).toStdString());
	return AvatarManager::IsValidAvatarFileType(
			   type, reinterpret_cast<const unsigned char *>(data.constData()),
			   static_cast<size_t>(data.size()));
}

/**
 * Adopts an image selection as your own avatar.
 *
 * Local files suitable for the engine are passed through unchanged (the
 * widget client has always stored the chosen path). In two cases, by contrast,
 * a new file is created under <userDataDir>gfx/avatars/user/:
 *  - Android content:// URIs are only readable for the running session and
 *    can be opened neither by the image preview nor by the engine (std::ifstream
 *    during the avatar upload).
 *  - Images that the engine rejects: too large in kilobytes (typical for
 *    photos from the gallery) or too large in pixels (typical for flat
 *    graphics that result in tiny files despite large dimensions).
 *
 * The file name is the MD5 of the content (the naming convention of the
 * AvatarManager), so repeated selection creates no duplicates.
 * Return value: the path to be stored, empty on cancel or error.
 *
 * userDataDir has to end with a directory separator (like the
 * config value of the same name).
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

	// Determine the file extension from the file header (content:// URIs have
	// none; the engine recognises the avatar type by the extension).
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
		// Re-encode and scale down step by step until the result passes the
		// engine check. Animated GIFs become a still image in the process.
		// QImage::fromData() returns a null image for absurdly large images
		// (Qt limits the allocation when decoding), which arrives here correctly
		// as a failure.
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
