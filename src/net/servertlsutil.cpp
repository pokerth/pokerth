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
 *****************************************************************************/

#include <net/servertlsutil.h>

#include <QCoreApplication>
#include <QDir>

namespace
{

std::string
FallbackTlsFile(const char *fileName)
{
	// applicationDirPath() resolves the executable's directory (via
	// /proc/self/exe on Linux), so it is absolute and independent of the
	// current working directory. The tls/ folder sits next to bin/ in the
	// build/deploy layout, hence "<exeDir>/../tls".
	const QString exeDir = QCoreApplication::applicationDirPath();
	const QString tlsDir = QDir::cleanPath(exeDir + QLatin1String("/../tls")) + QLatin1Char('/');
	return (tlsDir + QLatin1String(fileName)).toStdString();
}

std::string
ResolveTlsFile(const std::string &configuredPath, const char *fallbackFileName)
{
	if (configuredPath.empty())
		return FallbackTlsFile(fallbackFileName);
	// Configured paths are used verbatim; cleanPath only normalises separators
	// and removes redundant elements, it does not resolve against the working
	// directory, so a relative entry stays the caller's responsibility.
	return QDir::cleanPath(QString::fromStdString(configuredPath)).toStdString();
}

}

std::string
GetServerTlsCertFile(const std::string &configuredPath)
{
	return ResolveTlsFile(configuredPath, "server.crt");
}

std::string
GetServerTlsKeyFile(const std::string &configuredPath)
{
	return ResolveTlsFile(configuredPath, "server.key");
}
