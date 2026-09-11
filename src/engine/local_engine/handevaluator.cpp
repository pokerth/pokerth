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

#include "handevaluator.h"

#include <bit>

namespace
{

// Hand classes, ascending. The value is packed as (class << 20) | kickers,
// the kickers as four bits each in descending significance.
enum HandClass {
	HC_HIGHCARD      = 0,
	HC_PAIR          = 1,
	HC_TWOPAIR       = 2,
	HC_TRIPS         = 3,
	HC_STRAIGHT      = 4,
	HC_FLUSH         = 5,
	HC_FULLHOUSE     = 6,
	HC_QUADS         = 7,
	HC_STRAIGHTFLUSH = 8
};

// For each of the 8192 possible value bit masks the highest straight head
// (4 = five-high up to 12 = ace-high), otherwise -1.
struct StraightTable {
	signed char top[8192];

	StraightTable()
	{
		for(int mask = 0; mask < 8192; ++mask) {
			top[mask] = -1;
			// Ass-tiefe Strasse (Wheel): A-2-3-4-5
			if((mask & (1 << 12)) && (mask & 1) && (mask & 2) && (mask & 4) && (mask & 8)) {
				top[mask] = 3;
			}
			for(int high = 4; high <= 12; ++high) {
				const int need = 0x1F << (high - 4);
				if((mask & need) == need) top[mask] = static_cast<signed char>(high);
			}
		}
	}
};

// Magic static: the build-up runs thread-safely exactly once. Important
// because the engine computes from several game threads inside the server.
const StraightTable& straightTable()
{
	static const StraightTable table;
	return table;
}

// The n highest set bits as 4 bit groups, packed in descending order.
inline unsigned topRanks(unsigned mask, int n)
{
	unsigned packed = 0;
	for(int rank = 12; rank >= 0 && n > 0; --rank) {
		if(mask & (1u << rank)) {
			packed = (packed << 4) | static_cast<unsigned>(rank);
			--n;
		}
	}
	// If cards are missing (does not happen with seven cards), it stays padded to the right.
	return packed << (4 * n);
}

inline unsigned highestRank(unsigned mask)
{
	return 31u - static_cast<unsigned>(std::countl_zero(mask));
}

}

void HandEvaluator::init()
{
	straightTable();
}

unsigned HandEvaluator::value(const int* cards)
{
	const StraightTable& straights = straightTable();

	unsigned rankMask = 0;
	unsigned suitMask[4] = { 0, 0, 0, 0 };
	int rankCount[13] = { 0 };

	for(int i = 0; i < 7; ++i) {
		const int suit = cards[i] / 13;
		const int rank = cards[i] % 13;
		rankMask |= 1u << rank;
		suitMask[suit] |= 1u << rank;
		++rankCount[rank];
	}

	// Flush and straight flush. With seven cards at most one suit can occur
	// five times, and next to a flush neither quads nor a full house is
	// possible -- which is why we may return directly here.
	for(int suit = 0; suit < 4; ++suit) {
		if(std::popcount(suitMask[suit]) >= 5) {
			const int sf = straights.top[suitMask[suit]];
			if(sf >= 0) return (HC_STRAIGHTFLUSH << 20) | (static_cast<unsigned>(sf) << 16);
			return (HC_FLUSH << 20) | topRanks(suitMask[suit], 5);
		}
	}

	unsigned quads = 0, trips = 0, pairs = 0;
	for(int rank = 0; rank < 13; ++rank) {
		if(rankCount[rank] == 4) quads |= 1u << rank;
		else if(rankCount[rank] == 3) trips |= 1u << rank;
		else if(rankCount[rank] == 2) pairs |= 1u << rank;
	}

	if(quads) {
		const unsigned q = highestRank(quads);
		return (HC_QUADS << 20) | (q << 16) | (topRanks(rankMask & ~(1u << q), 1) << 12);
	}

	if(trips) {
		const unsigned t = highestRank(trips);
		// A second set of trips counts as a pair for the full house.
		const unsigned rest = (trips & ~(1u << t)) | pairs;
		if(rest) {
			return (HC_FULLHOUSE << 20) | (t << 16) | (highestRank(rest) << 12);
		}
	}

	const int straight = straights.top[rankMask];
	if(straight >= 0) return (HC_STRAIGHT << 20) | (static_cast<unsigned>(straight) << 16);

	if(trips) {
		const unsigned t = highestRank(trips);
		return (HC_TRIPS << 20) | (t << 16) | (topRanks(rankMask & ~(1u << t), 2) << 8);
	}

	if(std::popcount(pairs) >= 2) {
		const unsigned high = highestRank(pairs);
		const unsigned low = highestRank(pairs & ~(1u << high));
		const unsigned kicker = topRanks(rankMask & ~(1u << high) & ~(1u << low), 1);
		return (HC_TWOPAIR << 20) | (high << 16) | (low << 12) | (kicker << 8);
	}

	if(pairs) {
		const unsigned p = highestRank(pairs);
		return (HC_PAIR << 20) | (p << 16) | (topRanks(rankMask & ~(1u << p), 3) << 4);
	}

	return (HC_HIGHCARD << 20) | topRanks(rankMask, 5);
}
