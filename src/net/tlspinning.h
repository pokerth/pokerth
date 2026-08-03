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
/* Certificate pinning for the TLS connection to a lobby server. */

#ifndef _TLSPINNING_H_
#define _TLSPINNING_H_

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>

#include <openssl/x509.h>

#include <string>
#include <vector>

// The lobby servers use self-signed certificates, so the usual CA based
// verification is not applicable: it would reject exactly the certificate we
// want to talk to. Instead the SHA-256 hash of the server's SubjectPublicKeyInfo
// is compared against a list of known good pins.
//
// The public key is hashed rather than the whole certificate for two reasons: a
// certificate renewed with the same key pair keeps its pin, and no host name has
// to match anything, so servers addressed by IP address work as well.
namespace TlsPinning
{
	typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> SslStream;

	// Pins compiled into the client for a known lobby address. Empty for every
	// other address, which leaves those connections unpinned.
	std::vector<std::string> GetBuiltinPins(const std::string &serverAddr);

	// base64(sha256(DER encoded SubjectPublicKeyInfo)), empty on error. Same
	// value as:
	//   openssl x509 -in cert.pem -pubkey -noout \
	//     | openssl pkey -pubin -outform der \
	//     | openssl dgst -sha256 -binary | openssl enc -base64
	std::string ComputeSpkiPin(X509 *cert);

	// Enforce the given pins, i.e. abort the handshake unless the server proves
	// possession of one of the pinned keys. Returns false and changes nothing if
	// pins is empty; the caller's settings then stay in effect.
	//
	// Both variants have to be called before the handshake. The context variant
	// additionally has to be called before the stream using that context is
	// constructed, because the verification settings are copied into the SSL
	// object at that point.
	bool ApplyPins(boost::asio::ssl::context &sslCtx, const std::vector<std::string> &pins);
	bool ApplyPins(SslStream &sslStream, const std::vector<std::string> &pins);
}

#endif
