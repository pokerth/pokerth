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

#include "equitycalculator.h"

#include "handevaluator.h"
#include "holecardsrange.h"
#include "tools.h"

#include <random>
#include <utility>

namespace
{

// Schneller Generator fuer die Simulation. Die Entropie kommt einmal pro
// Thread aus Tools::GetRand(); dessen boost::random_device direkt zu benutzen
// waere fuer hunderttausend Zufallszahlen je Entscheidung viel zu langsam.
std::mt19937& simulationRng()
{
	static thread_local bool seeded = false;
	static thread_local std::mt19937 rng;
	if(!seeded) {
		int seed[2];
		Tools::GetRand(0, 0x7FFFFFFF, 2, seed);
		std::seed_seq sequence{ seed[0], seed[1] };
		rng.seed(sequence);
		seeded = true;
	}
	return rng;
}

inline unsigned nextBelow(std::mt19937& rng, unsigned bound)
{
	return static_cast<unsigned>(rng()) % bound;
}

typedef std::pair<signed char, signed char> CardPair;

// Alle Startkarten-Kombinationen, die zu einer Range passen und keine bereits
// bekannte Karte benutzen. Vorab aufgebaut, damit enge Ranges nicht durch
// wiederholtes Verwerfen teuer werden.
void buildCombos(const HoleCardsRange* range, unsigned long long knownMask, std::vector<CardPair>& out)
{
	out.clear();
	out.reserve(1326);
	for(int a = 0; a < 51; ++a) {
		if(knownMask & (1ULL << a)) continue;
		for(int b = a + 1; b < 52; ++b) {
			if(knownMask & (1ULL << b)) continue;
			if(range && !range->contains(a, b)) continue;
			out.push_back(CardPair(static_cast<signed char>(a), static_cast<signed char>(b)));
		}
	}
}

}

EquityCalculator::Result EquityCalculator::equity(const int* holeCards,
        const int* boardCards, int boardSize,
        const std::vector<const HoleCardsRange*>& opponentRanges,
        int samples)
{
	Result result = { 0.0, 0.0, 0.0, 0 };

	const int opponents = static_cast<int>(opponentRanges.size());
	if(opponents < 1 || samples < 1 || boardSize < 0 || boardSize > 5) return result;

	HandEvaluator::init();

	// Bekannte Karten sperren.
	unsigned long long knownMask = 0;
	int known[7];
	int knownCount = 0;
	known[knownCount++] = holeCards[0];
	known[knownCount++] = holeCards[1];
	for(int i = 0; i < boardSize; ++i) known[knownCount++] = boardCards[i];
	for(int i = 0; i < knownCount; ++i) {
		if(known[i] < 0 || known[i] > 51) return result;
		if(knownMask & (1ULL << known[i])) return result;  // doppelte Karte
		knownMask |= 1ULL << known[i];
	}

	// Zulaessige Kombinationen je Gegner.
	std::vector< std::vector<CardPair> > combos(opponents);
	for(int o = 0; o < opponents; ++o) {
		buildCombos(opponentRanges[o], knownMask, combos[o]);
		// Eine Range, die nach Abzug der bekannten Karten leer ist, laesst sich
		// nicht simulieren -- dann lieber beliebige Haende als gar kein Ergebnis.
		if(combos[o].empty()) buildCombos(0, knownMask, combos[o]);
		if(combos[o].empty()) return result;
	}

	std::mt19937& rng = simulationRng();

	const int missingBoard = 5 - boardSize;
	int myCards[7];
	myCards[0] = holeCards[0];
	myCards[1] = holeCards[1];

	std::vector<int> opponentCards(2 * opponents);
	double equitySum = 0.0;
	int wins = 0, ties = 0, played = 0;

	for(int s = 0; s < samples; ++s) {
		unsigned long long usedMask = knownMask;
		bool usable = true;

		// Gegnerhaende ziehen.
		for(int o = 0; o < opponents && usable; ++o) {
			const std::vector<CardPair>& list = combos[o];
			bool placed = false;
			// Kollisionen mit schon vergebenen Karten werden neu gezogen. Der
			// Deckel verhindert Endlosschleifen, wenn kaum etwas frei ist.
			for(int attempt = 0; attempt < 200 && !placed; ++attempt) {
				const CardPair& pick = list[nextBelow(rng, static_cast<unsigned>(list.size()))];
				const unsigned long long bits = (1ULL << pick.first) | (1ULL << pick.second);
				if(usedMask & bits) continue;
				usedMask |= bits;
				opponentCards[2 * o] = pick.first;
				opponentCards[2 * o + 1] = pick.second;
				placed = true;
			}
			if(!placed) usable = false;
		}
		if(!usable) continue;

		// Board vervollstaendigen.
		int board[5];
		for(int i = 0; i < boardSize; ++i) board[i] = boardCards[i];
		for(int i = 0; i < missingBoard; ++i) {
			int card;
			do {
				card = static_cast<int>(nextBelow(rng, 52));
			} while(usedMask & (1ULL << card));
			usedMask |= 1ULL << card;
			board[boardSize + i] = card;
		}

		for(int i = 0; i < 5; ++i) myCards[2 + i] = board[i];
		const unsigned myValue = HandEvaluator::value(myCards);

		// Gegner auswerten und zaehlen, wer mit uns gleichauf liegt.
		int equalBest = 0;
		bool beaten = false;
		for(int o = 0; o < opponents; ++o) {
			int hand[7];
			hand[0] = opponentCards[2 * o];
			hand[1] = opponentCards[2 * o + 1];
			for(int i = 0; i < 5; ++i) hand[2 + i] = board[i];
			const unsigned value = HandEvaluator::value(hand);
			if(value > myValue) { beaten = true; break; }
			if(value == myValue) ++equalBest;
		}

		++played;
		if(beaten) continue;
		if(equalBest == 0) {
			++wins;
			equitySum += 1.0;
		} else {
			++ties;
			equitySum += 1.0 / (equalBest + 1);
		}
	}

	if(played > 0) {
		result.equity = equitySum / played;
		result.win = static_cast<double>(wins) / played;
		result.tie = static_cast<double>(ties) / played;
		result.samples = played;
	}

	return result;
}

EquityCalculator::Result EquityCalculator::equity(const int* holeCards,
        const int* boardCards, int boardSize,
        int opponents, const HoleCardsRange* range,
        int samples)
{
	std::vector<const HoleCardsRange*> ranges(opponents > 0 ? opponents : 0, range);
	return equity(holeCards, boardCards, boardSize, ranges, samples);
}
