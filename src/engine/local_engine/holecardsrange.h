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

#ifndef HOLECARDSRANGE_H
#define HOLECARDSRANGE_H

#include <string>
#include <vector>

/* Set of starting hands ("range") for AI engine 4.
 *
 * Internally a mask over the 169 starting hand types. It is built from the
 * usual poker notation so that the charts stay readable in the code:
 *
 *   "AA"        exactly one type
 *   "77+"       all pairs from sevens upwards
 *   "22-55"     pairs within a span
 *   "AKs"       ace-king suited, "AKo" offsuit
 *   "A9s+"      all ace-x suited from nine upwards
 *   "KTo-K7o"   span with the same high card
 *
 * Several expressions are separated by commas.
 */
class HoleCardsRange
{
public:
	HoleCardsRange();
	explicit HoleCardsRange(const std::string& notation);

	// Adds further expressions. Returns false on unreadable notation.
	bool add(const std::string& notation);

	void clear();

	// Is the starting hand (cards 0..51) part of the range?
	bool contains(int cardA, int cardB) const;

	bool empty() const;

	// Share of all 1326 card combinations covered by the range (0..1).
	// Takes into account that pairs have 6, suited hands 4 and offsuit
	// hands 12 combinations.
	double combinationShare() const;

	// Index 0..168 of a starting hand type; -1 for invalid cards.
	static int typeIndex(int cardA, int cardB);

private:
	bool myTypes[169];
};

#endif
