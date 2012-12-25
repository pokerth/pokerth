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

#include <core/crypthelper.h>

#include <core/openssl_wrapper.h>
#include <cstring>
#include <cstdio>

using namespace std;

// Helper function.
static int
fromHex(int ch)
{
	int retVal = -1;

	if (ch >= '0' && ch <= '9')
		retVal = ch - '0';
	else if (ch >= 'a' && ch <= 'f')
		retVal = ch - 'a' + 10;
	else if (ch >= 'A' && ch <= 'F')
		retVal = ch - 'A' + 10;

	return retVal;
}

HashBuf::~HashBuf()
{
}

std::string
HashBuf::ToString() const
{
	// Create a hex-based string from the MD5 data.
	string retValue;
	char tmpBuf[2 + 1];
	tmpBuf[sizeof(tmpBuf) - 1] = 0;
	const unsigned char *tmpData = GetData();
	for (int i = 0; i < GetDataSize(); i++) {
		sprintf(tmpBuf, "%02x", tmpData[i]);
		retValue += tmpBuf;
	}
	return retValue;
}

bool
HashBuf::FromString(const std::string &text)
{
	// Convert hex-based string to MD5 data.
	bool retVal = false;
	int tmpSize = GetDataSize();
	if (text.size() == 2 * (unsigned)tmpSize) {
		unsigned char *tmpData = GetData();
		const char *t = text.c_str();
		int i = 0;
		for (; i < tmpSize; i++) {
			int part1 = fromHex(*t++);
			if (part1 == -1)
				break;
			int part2 = fromHex(*t++);
			if (part2 == -1)
				break;
			*tmpData++ = (part1<<4) + part2;
		}
		retVal = i == tmpSize;
	}
	return retVal;
}

bool
HashBuf::IsZero() const
{
	int dataSize = GetDataSize();
	const unsigned char *tmpData = GetData();
	int i;
	for (i = 0; i < dataSize; i++) {
		if (tmpData[i] != 0)
			break;
	}
	return i == dataSize;
}

bool
HashBuf::operator==(const HashBuf &other) const
{
	return GetDataSize() == other.GetDataSize() && memcmp(GetData(), other.GetData(), GetDataSize()) == 0;
}

bool
HashBuf::operator<(const HashBuf &other) const
{
	int smallestDataSize = GetDataSize() < other.GetDataSize() ? GetDataSize() : other.GetDataSize();
	return memcmp(GetData(), other.GetData(), smallestDataSize) < 0;
}


MD5Buf::MD5Buf()
{
	memset(m_data, 0, sizeof(m_data));
}

unsigned char *
MD5Buf::GetData()
{
	return m_data;
}

const unsigned char *
MD5Buf::GetData() const
{
	return m_data;
}

int
MD5Buf::GetDataSize() const
{
	return sizeof(m_data);
}

SHA1Buf::SHA1Buf()
{
	memset(m_data, 0, sizeof(m_data));
}

unsigned char *
SHA1Buf::GetData()
{
	return m_data;
}

const unsigned char *
SHA1Buf::GetData() const
{
	return m_data;
}

int
SHA1Buf::GetDataSize() const
{
	return sizeof(m_data);
}

bool
CryptHelper::MD5Sum(const std::string &fileName, MD5Buf &buf)
{
	bool retVal = false;
	FILE *file = fopen(fileName.c_str(), "rb");

	if (file) {
		// Calculate MD5 sum of file.
		unsigned char *readBuf = new unsigned char[8192];
		size_t numBytes;

#ifdef HAVE_OPENSSL
		MD5_CTX context;
		MD5_Init(&context);
		while ((numBytes = fread(readBuf, 1, sizeof(readBuf), file)) > 0) {
			MD5_Update(&context, readBuf, numBytes);
		}
		MD5_Final(buf.GetData(), &context);
#else
		gcry_md_hd_t hash;
		gcry_md_open(&hash, GCRY_MD_MD5, 0);
		while ((numBytes = fread(readBuf, 1, sizeof(readBuf), file)) > 0) {
			gcry_md_write(hash, readBuf, numBytes);
		}
		memcpy(buf.GetData(), gcry_md_read(hash, GCRY_MD_MD5), MD5_DATA_SIZE);
		gcry_md_close(hash);
#endif

		retVal = ferror(file) == 0;

		delete[] readBuf;
		fclose(file);
	}
	return retVal;
}

bool
CryptHelper::SHA1Hash(const unsigned char *data, unsigned dataSize, SHA1Buf &buf)
{
	bool retVal;
#ifdef HAVE_OPENSSL
	retVal = SHA1(data, dataSize, buf.GetData()) != NULL;
#else
	// We use the shortcut since we assume that the system supports SHA1.
	// This call has no error return value.
	gcry_md_hash_buffer(GCRY_MD_SHA1, buf.GetData(), data, dataSize);
	retVal = true;
#endif
	return retVal;
}

bool
CryptHelper::HMACSha1(const unsigned char *keyData, unsigned keySize, const unsigned char *plainData, unsigned plainSize, SHA1Buf &buf)
{
	bool retVal;
#ifdef HAVE_OPENSSL
	unsigned hashLen = 0;
	HMAC(EVP_sha1(), keyData, keySize, plainData, plainSize, buf.GetData(), &hashLen);
	retVal = hashLen == (unsigned)buf.GetDataSize();
#else
	retVal = false;
	gcry_md_hd_t hd;
	gcry_error_t err = gcry_md_open(&hd, GCRY_MD_SHA1, GCRY_MD_FLAG_HMAC);
	if (!err) {
		err = gcry_md_setkey(hd, keyData, keySize);
		if (!err) {
			gcry_md_write(hd, plainData, plainSize);
			unsigned char *hash = gcry_md_read(hd, 0);
			if (hash) {
				memcpy(buf.GetData(), hash, buf.GetDataSize());
				retVal = true;
			}
		}
		gcry_md_close(hd);
	}
#endif
	return retVal;
}

void
CryptHelper::BytesToKey(const unsigned char *keyData, unsigned keySize, unsigned char *key, unsigned char *iv)
{
	// The key/iv derivation is kind of like EVP_BytesToKey of OpenSSL with count 2 and no salt.
	// EVP_BytesToKey is not used because there is nothing like it in GnuTLS or gcrypt.
	SHA1Buf tmpBuf1, tmpBuf2, keyBuf1, keyBuf2;
	// First 20 bytes
	CryptHelper::SHA1Hash(keyData, keySize, tmpBuf1);
	CryptHelper::SHA1Hash(tmpBuf1.GetData(), tmpBuf1.GetDataSize(), keyBuf1);
	// Second 20 bytes (we only need a total of 32 bytes, but anyway).
	unsigned tmpKeySize = keySize + keyBuf1.GetDataSize();
	unsigned char *tmpKeyData = new unsigned char[tmpKeySize];
	// Concatenate our first hash and the key data.
	memcpy(tmpKeyData, keyBuf1.GetData(), keyBuf1.GetDataSize());
	memcpy(tmpKeyData + keyBuf1.GetDataSize(), keyData, keySize);
	CryptHelper::SHA1Hash(tmpKeyData, tmpKeySize, tmpBuf2);
	CryptHelper::SHA1Hash(tmpBuf2.GetData(), tmpBuf2.GetDataSize(), keyBuf2);
	delete[] tmpKeyData;
	// Copy the hashes to key/iv.
	memcpy(key, keyBuf1.GetData(), AES_BLOCK_SIZE);
	unsigned tmpivBytes = keyBuf1.GetDataSize() - AES_BLOCK_SIZE;
	memcpy(iv, keyBuf1.GetData() + AES_BLOCK_SIZE, tmpivBytes);
	memcpy(iv + tmpivBytes, keyBuf2.GetData(), AES_BLOCK_SIZE - tmpivBytes);
}

bool
CryptHelper::AES128Encrypt(const unsigned char *keyData, unsigned keySize, const string &plainStr, std::vector<unsigned char> &outCipher)
{
	bool retVal = false;
	unsigned plainSize = static_cast<unsigned>(plainStr.size());
	if (keySize && plainSize) {
		unsigned char key[AES_BLOCK_SIZE];
		unsigned char iv[AES_BLOCK_SIZE];
		BytesToKey(keyData, keySize, key, iv);
		// Add padding to plain data.
		unsigned paddedPlainSize = ADD_PADDING(plainSize);
		unsigned char *paddedPlainStr = (unsigned char *)calloc(paddedPlainSize, 1);
		memcpy(paddedPlainStr, plainStr.c_str(), plainSize);
		// Perform the encryption.
		int cipherSize = paddedPlainSize;
		outCipher.resize(cipherSize);

#ifdef HAVE_OPENSSL
		EVP_CIPHER_CTX encryptCtx;
		EVP_CIPHER_CTX_init(&encryptCtx);
		int outCipherSize = cipherSize;

		int success = EVP_EncryptInit(&encryptCtx, EVP_aes_128_cbc(), key, iv);
		EVP_CIPHER_CTX_set_padding(&encryptCtx, 0);
		if (success) {
			success = EVP_EncryptUpdate(&encryptCtx, &outCipher[0], &outCipherSize, paddedPlainStr, paddedPlainSize);

			if (success && outCipherSize) {
				// Since padding is off, this will not modify the cipher. However, parameters need to be set.
				EVP_EncryptFinal(&encryptCtx, &outCipher[0], &outCipherSize);
				retVal = true;
			}
		} else
			outCipher.clear();
#else
		gcry_cipher_hd_t hd;
		gcry_error_t err = gcry_cipher_open(&hd, GCRY_CIPHER_AES128, GCRY_CIPHER_MODE_CBC, 0);
		if (!err) {
			gcry_cipher_setkey(hd, key, sizeof(key));
			gcry_cipher_setiv(hd, iv, sizeof(iv));
			err = gcry_cipher_encrypt(hd, &outCipher[0], cipherSize, paddedPlainStr, paddedPlainSize);
			if (!err)
				retVal = true;
			else
				outCipher.clear();
		} else
			outCipher.clear();

		gcry_cipher_close(hd);
#endif
		free(paddedPlainStr);
	}
	return retVal;
}

bool
CryptHelper::AES128Decrypt(const unsigned char *keyData, unsigned keySize, const unsigned char *cipher, unsigned cipherSize, string &outPlain)
{
	bool retVal = false;
	if (keySize && cipherSize) {
		unsigned char key[AES_BLOCK_SIZE];
		unsigned char iv[AES_BLOCK_SIZE];
		BytesToKey(keyData, keySize, key, iv);
		outPlain.resize(cipherSize);
#ifdef HAVE_OPENSSL
		EVP_CIPHER_CTX decryptCtx;
		EVP_CIPHER_CTX_init(&decryptCtx);
		int outPlainSize = cipherSize;

		int success = EVP_DecryptInit(&decryptCtx, EVP_aes_128_cbc(), key, iv);
		EVP_CIPHER_CTX_set_padding(&decryptCtx, 0);
		if (success) {
			success = EVP_DecryptUpdate(&decryptCtx, (unsigned char *)&outPlain[0], &outPlainSize, cipher, cipherSize);

			if (success && outPlainSize) {
				// Since padding is off, this will not modify the plain text. However, parameters need to be set.
				EVP_DecryptFinal(&decryptCtx, (unsigned char *)outPlain.c_str(), &outPlainSize);
				retVal = true;
			}
		} else
			outPlain.clear();
#else
		gcry_cipher_hd_t hd;
		gcry_error_t err = gcry_cipher_open(&hd, GCRY_CIPHER_AES128, GCRY_CIPHER_MODE_CBC, 0);
		if (!err) {
			gcry_cipher_setkey(hd, key, sizeof(key));
			gcry_cipher_setiv(hd, iv, sizeof(iv));
			err = gcry_cipher_decrypt(hd, &outPlain[0], outPlain.size(), cipher, cipherSize);
			if (!err)
				retVal = true;
			else
				outPlain.clear();
		} else
			outPlain.clear();

		gcry_cipher_close(hd);
#endif
		// Remove trailing zeroes (padding).
		if (!outPlain.empty()) {
			size_t pos = outPlain.find_first_of('\0');
			if (pos != string::npos)
				outPlain = outPlain.substr(0, pos);
		}
	}
	return retVal;
}

