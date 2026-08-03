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
/* Locate the server TLS certificate and key files. */

#ifndef _SERVERTLSUTIL_H_
#define _SERVERTLSUTIL_H_

#include <string>

// Resolve the certificate and the private key the server presents to its
// clients. The configured paths come from ServerTlsCertFile / ServerTlsKeyFile
// in the server config and are absolute, pointing outside the source tree: the
// private key must not live in the repository, and in the docker setup the
// directory holding it is mounted into the container. Hence the default
// /etc/tls/server.crt and /etc/tls/server.key.
//
// Certificate and key are configured individually instead of being derived from
// one directory, because a CA issued certificate does not follow the
// server.crt / server.key naming - with certbot the pair is called
// fullchain.pem / privkey.pem.
//
// Clearing an entry selects the historic location, the tls/ folder next to
// bin/. That fallback is derived from the running executable's location
// (<exeDir>/../tls), not from the current working directory, so it survives
// both an arbitrary launch directory and the daemon() chdir("/") that the
// dedicated server performs in release builds.
std::string GetServerTlsCertFile(const std::string &configuredPath);
std::string GetServerTlsKeyFile(const std::string &configuredPath);

#endif
