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

#ifndef HANDEVALUATOR_H
#define HANDEVALUATOR_H

/* Fast 7 card evaluator for AI engine 4.
 *
 * Delivers the same ranking as CardsValue::cardsValue(), but is about
 * seven times faster because it works with bit masks instead of three
 * bubble sorts. The absolute values are NOT comparable to CardsValue --
 * only the ordering matches. CardsValue remains responsible for the
 * showdown display and the pot distribution; this evaluator serves the
 * Monte Carlo simulation alone, which needs tens of thousands of
 * evaluations per decision.
 *
 * Card encoding as in the rest of the engine code: 0..51, suit = c/13,
 * value = c%13 with 0 = two ... 12 = ace.
 */
class HandEvaluator
{
public:
	// Optional warm-up of the lookup table. Without this call it builds itself
	// on the first value() call -- thread-safely, but then inside the first
	// decision.
	static void init();

	// Evaluates exactly seven cards. A larger return value = a stronger hand.
	static unsigned value(const int* cards);
};

#endif
