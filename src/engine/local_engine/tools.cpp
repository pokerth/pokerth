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

// Zufallsquelle fuer Karten und KI-Entscheidungen.
//
// Die Karten muessen kryptografisch stark gezogen werden: mt19937 & Co. legen
// ihren kompletten Zustand nach wenigen hundert Ausgaben offen, und ein Deck
// besteht nun einmal aus veroeffentlichten Ausgaben. Frueher wurde deshalb pro
// Hand ein ganzer mt19937 (624 Woerter = 2496 Byte) aus dem Betriebssystem
// geseedet und jede einzelne Zahl aus Tools::GetRand direkt vom Zufallsgeraet
// gelesen - ein Syscall je Zufallszahl, und die KI zieht pro Entscheidung
// etliche.
//
// Statt dessen liefert der DRBG von OpenSSL den Zufall (ebenfalls
// kryptografisch stark, aber ohne Geraetezugriff je Zahl), blockweise
// gepuffert. Der Puffer haelt nur DRBG-Ausgabe, nie dessen Zustand - aus
// gelesenen Werten laesst sich also weiterhin nichts ueber kommende
// vorhersagen.
class RandomSource
{
public:
	RandomSource() : m_pos(kWords) {}

	uint32_t next32() {
		if(m_pos == kWords) Refill();
		return m_words[m_pos++];
	}

	// Gleichverteilt in [0, bound) ohne Modulo-Verzerrung (Methode von Lemire:
	// das Produkt liefert den Wert in der oberen Haelfte, die untere entscheidet,
	// ob der seltene Rest-Bereich verworfen werden muss).
	uint32_t Below(uint32_t bound) {
		if(bound < 2) return 0;
		uint64_t product = static_cast<uint64_t>(next32()) * bound;
		uint32_t low = static_cast<uint32_t>(product);
		if(low < bound) {
			// 2^32 mod bound, in vorzeichenloser Arithmetik gerechnet (bound kann
			// bis 2^31 gross sein, ein Umweg ueber int32_t waere dort undefiniert).
			const uint32_t threshold = (0u - bound) % bound;
			while(low < threshold) {
				product = static_cast<uint64_t>(next32()) * bound;
				low = static_cast<uint32_t>(product);
			}
		}
		return static_cast<uint32_t>(product >> 32);
	}

private:
	void Refill() {
		bool filled = false;
#ifdef HAVE_OPENSSL
		filled = RAND_bytes(reinterpret_cast<unsigned char *>(m_words), static_cast<int>(sizeof(m_words))) == 1;
		if(!filled) {
			LOG_ERROR(__FILE__ << " (" << __LINE__ << "): RAND_bytes failed, falling back to std::random_device");
		}
#endif
		if(!filled) {
			// Nur Notnagel (OpenSSL nicht verfuegbar oder Entropie erschoepft):
			// std::random_device ist die einzige verbleibende nichtdeterministische
			// Quelle, dafuer mit Geraetezugriff je Wort.
			std::random_device device;
			for(unsigned i = 0; i < kWords; i++) {
				m_words[i] = static_cast<uint32_t>(device());
			}
		}
		m_pos = 0;
	}

	static const unsigned kWords = 256; // 1 KiB, also ein DRBG-Aufruf je 256 Zahlen
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

	// Fisher-Yates: jede der count! Reihenfolgen ist exakt gleich wahrscheinlich.
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

	// Spanne als 64 Bit rechnen, damit auch [INT_MIN, INT_MAX] nicht ueberlaeuft.
	const uint64_t span = static_cast<uint64_t>(static_cast<int64_t>(maxValue) - static_cast<int64_t>(minValue)) + 1;
	RandomSource &source = Source();
	for(unsigned i = 0; i < count; i++) {
		const uint64_t value = span > 0xFFFFFFFFull ? source.next32() : source.Below(static_cast<uint32_t>(span));
		out[i] = static_cast<int>(static_cast<int64_t>(minValue) + static_cast<int64_t>(value));
	}
}
