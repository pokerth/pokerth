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

#include <net/tlspinning.h>
#include <core/loghelper.h>

#include <openssl/evp.h>
#include <openssl/sha.h>

#include <algorithm>
#include <cctype>

using namespace std;

namespace
{

struct BuiltinPin {
	const char *serverAddr;
	const char *spkiPin;
};

// Lobby servers whose public key is known at build time. Several pins may be
// listed for one address to roll a key over without locking anybody out: ship a
// release which trusts the old and the new key, switch the key on the server,
// then drop the old pin in a later release. A rollover which cannot wait for a
// client release can be done through the <TLSPin> element of the server list,
// which is fetched over a CA verified HTTPS connection.
const BuiltinPin BUILTIN_PINS[] = {
	// pokerth.net official server (CN=pokerth.net, self-signed, RSA 4096,
	// valid until 2036-07-31). Key and certificate live in /etc/tls on the
	// server host and are deliberately not part of this repository.
	{ "pthsrv.pokerth.net", "hnyHDGXvmDBFU7MN5xXuiq4OaWWrnHNzqhKlEoSuAV4=" }
};

bool
EqualsIgnoreCase(const string &lhs, const char *rhs)
{
	const string rhsStr(rhs);
	return lhs.size() == rhsStr.size()
		   && equal(lhs.begin(), lhs.end(), rhsStr.begin(),
					[](unsigned char l, unsigned char r) {
		return tolower(l) == tolower(r);
	});
}

bool
VerifyPinnedCert(bool /*preverified*/, boost::asio::ssl::verify_context &verifyCtx, const vector<string> &pins)
{
	// preverified is deliberately ignored: the certificate is self-signed, so
	// the chain check always fails. Trust comes from the pinned key alone.
	X509_STORE_CTX *storeCtx = verifyCtx.native_handle();
	if (!storeCtx)
		return false;

	// OpenSSL invokes the callback for every certificate of the chain, starting
	// at the top. Only the server certificate itself (depth 0) carries the
	// pinned key, everything above it has to be waved through - returning false
	// there would abort the handshake before depth 0 is reached at all.
	if (X509_STORE_CTX_get_error_depth(storeCtx) > 0)
		return true;

	const string pin(TlsPinning::ComputeSpkiPin(X509_STORE_CTX_get_current_cert(storeCtx)));
	if (!pin.empty() && find(pins.begin(), pins.end(), pin) != pins.end())
		return true;

	LOG_ERROR("TLS pinning: server public key " << (pin.empty() ? string("<unreadable>") : pin)
			  << " does not match any of the " << pins.size()
			  << " pinned key(s) - aborting handshake.");
	return false;
}

}

vector<string>
TlsPinning::GetBuiltinPins(const string &serverAddr)
{
	vector<string> pins;
	for (const BuiltinPin &entry : BUILTIN_PINS) {
		if (EqualsIgnoreCase(serverAddr, entry.serverAddr))
			pins.push_back(entry.spkiPin);
	}
	return pins;
}

string
TlsPinning::ComputeSpkiPin(X509 *cert)
{
	string pin;
	if (!cert)
		return pin;

	X509_PUBKEY *pubKey = X509_get_X509_PUBKEY(cert);
	if (!pubKey)
		return pin;

	unsigned char *der = NULL;
	const int derSize = i2d_X509_PUBKEY(pubKey, &der);
	if (der) {
		if (derSize > 0) {
			unsigned char digest[SHA256_DIGEST_LENGTH];
			SHA256(der, (size_t)derSize, digest);

			// 32 byte digest, i.e. 44 base64 characters plus terminator.
			unsigned char encoded[EVP_ENCODE_LENGTH(SHA256_DIGEST_LENGTH)];
			const int encodedSize = EVP_EncodeBlock(encoded, digest, sizeof(digest));
			if (encodedSize > 0)
				pin.assign(reinterpret_cast<const char *>(encoded), (size_t)encodedSize);
		}
		OPENSSL_free(der);
	}
	return pin;
}

bool
TlsPinning::ApplyPins(boost::asio::ssl::context &sslCtx, const vector<string> &pins)
{
	if (pins.empty())
		return false;

	sslCtx.set_verify_mode(boost::asio::ssl::verify_peer);
	sslCtx.set_verify_callback(
		[pins](bool preverified, boost::asio::ssl::verify_context & verifyCtx) {
		return VerifyPinnedCert(preverified, verifyCtx, pins);
	});
	return true;
}

bool
TlsPinning::ApplyPins(SslStream &sslStream, const vector<string> &pins)
{
	if (pins.empty())
		return false;

	sslStream.set_verify_mode(boost::asio::ssl::verify_peer);
	sslStream.set_verify_callback(
		[pins](bool preverified, boost::asio::ssl::verify_context & verifyCtx) {
		return VerifyPinnedCert(preverified, verifyCtx, pins);
	});
	return true;
}
