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

#define NOMINMAX // for Windows

#include "tools.h"
#include <core/loghelper.h>
#include <core/openssl_wrapper.h>

#include <algorithm>
#include <cstdint>
#include <random>

namespace
{

// Source of randomness for cards and AI decisions.
//
// The cards have to be drawn cryptographically strong: mt19937 & co. expose
// their complete state after a few hundred outputs, and a deck simply consists
// of published outputs. That is why a whole mt19937 (624 words = 2496 bytes)
// used to be seeded from the operating system per hand, and every single number
// from Tools::GetRand was read directly from the random device - one syscall per
// random number, and the AI draws quite a few per decision.
//
// Instead the DRBG of OpenSSL delivers the randomness (cryptographically
// strong as well, but without device access per number), buffered in blocks.
// The buffer only holds DRBG output, never its state - so it is still
// impossible to predict coming values from values that have been read.
class RandomSource
{
public:
	RandomSource() : m_pos(kWords) {}

	uint32_t next32()
	{
		if(m_pos == kWords) Refill();
		return m_words[m_pos++];
	}

	// Uniformly distributed in [0, bound) without modulo bias (Lemire's method:
	// the product delivers the value in the upper half, the lower one decides
	// whether the rare remainder range has to be discarded).
	uint32_t Below(uint32_t bound)
	{
		if(bound < 2) return 0;
		uint64_t product = static_cast<uint64_t>(next32()) * bound;
		uint32_t low = static_cast<uint32_t>(product);
		if(low < bound) {
			// 2^32 mod bound, computed in unsigned arithmetic (bound can be as large
			// as 2^31, a detour via int32_t would be undefined there).
			const uint32_t threshold = (0u - bound) % bound;
			while(low < threshold) {
				product = static_cast<uint64_t>(next32()) * bound;
				low = static_cast<uint32_t>(product);
			}
		}
		return static_cast<uint32_t>(product >> 32);
	}

private:
	void Refill()
	{
		bool filled = false;
#ifdef HAVE_OPENSSL
		filled = RAND_bytes(reinterpret_cast<unsigned char *>(m_words), static_cast<int>(sizeof(m_words))) == 1;
		if(!filled) {
			LOG_ERROR(__FILE__ << " (" << __LINE__ << "): RAND_bytes failed, falling back to std::random_device");
		}
#endif
		if(!filled) {
			// Only a last resort (OpenSSL unavailable or entropy exhausted):
			// std::random_device is the only remaining non-deterministic source,
			// but with device access per word.
			std::random_device device;
			for(unsigned i = 0; i < kWords; i++) {
				m_words[i] = static_cast<uint32_t>(device());
			}
		}
		m_pos = 0;
	}

	static const unsigned kWords = 256; // 1 KiB, i.e. one DRBG call per 256 numbers
	uint32_t m_words[kWords];
	unsigned m_pos;
};

RandomSource &Source()
{
	static thread_local RandomSource source;
	return source;
}

}

void Tools::ShuffleArrayNonDeterministic(int *inout, unsigned count)
{
	if(!inout || count < 2) return;

	// Fisher-Yates: each of the count! orderings is exactly equally likely.
	RandomSource &source = Source();
	for(unsigned i = count - 1; i > 0; i--) {
		const unsigned j = source.Below(i + 1);
		std::swap(inout[i], inout[j]);
	}
}

void Tools::GetRand(int minValue, int maxValue, unsigned count, int *out)
{
	if(!out || !count) return;
	if(maxValue < minValue) std::swap(minValue, maxValue);

	// Compute the span as 64 bit so that even [INT_MIN, INT_MAX] does not overflow.
	const uint64_t span = static_cast<uint64_t>(static_cast<int64_t>(maxValue) - static_cast<int64_t>(minValue)) + 1;
	RandomSource &source = Source();
	for(unsigned i = 0; i < count; i++) {
		const uint64_t value = span > 0xFFFFFFFFull ? source.next32() : source.Below(static_cast<uint32_t>(span));
		out[i] = static_cast<int>(static_cast<int64_t>(minValue) + static_cast<int64_t>(value));
	}
}
