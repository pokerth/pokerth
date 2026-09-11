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

#ifndef EQUITYCALCULATOR_H
#define EQUITYCALCULATOR_H

#include <vector>

class HoleCardsRange;

/* Monte Carlo equity for AI engine 4.
 *
 * Answers the question the old engine never asked: how often do I win this
 * hand against ALL remaining opponents at the same time? calcMyOdds()
 * instead computes against exactly one random opponent and uses the result
 * at a full table as well -- the main reason why the bots massively
 * overestimate their hands there.
 *
 * In addition, the opponent hands can be drawn from a range instead of
 * uniformly from the remaining deck. Whoever has paid a raise simply does
 * not hold a random hand.
 */
class EquityCalculator
{
public:
	struct Result {
		// Erwarteter Anteil am Pot, Gleichstaende anteilig gewertet (0..1).
		double equity;
		// Share of the runs with a sole win or with a tie.
		double win;
		double tie;
		// Runs actually evaluated. Can be smaller than requested if the
		// opponent ranges can hardly be filled without collisions.
		int samples;
	};

	/* holeCards: two own cards.
	 * boardCards/boardSize: 0 (preflop), 3, 4 or 5 open cards.
	 * opponentRanges: one entry per opponent; nullptr = any hand.
	 * samples: desired number of simulation runs.
	 */
	static Result equity(const int* holeCards,
						 const int* boardCards, int boardSize,
						 const std::vector<const HoleCardsRange*>& opponentRanges,
						 int samples);

	// Convenient case: all opponents with the same range (or nullptr).
	static Result equity(const int* holeCards,
						 const int* boardCards, int boardSize,
						 int opponents, const HoleCardsRange* range,
						 int samples);
};

#endif
