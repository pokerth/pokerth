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

/* Menge von Starthaenden ("Range") fuer die AI-Engine 4.
 *
 * Intern eine Maske ueber die 169 Starthand-Typen. Gebaut wird sie aus der
 * ueblichen Pokernotation, damit die Charts im Code lesbar bleiben:
 *
 *   "AA"        genau ein Typ
 *   "77+"       alle Paare ab Sieben aufwaerts
 *   "22-55"     Paare in einer Spanne
 *   "AKs"       Ass-Koenig gleichfarbig, "AKo" ungleichfarbig
 *   "A9s+"      alle Ass-x gleichfarbig ab Neun aufwaerts
 *   "KTo-K7o"   Spanne bei gleicher hoher Karte
 *
 * Mehrere Ausdruecke werden per Komma getrennt.
 */
class HoleCardsRange
{
public:
	HoleCardsRange();
	explicit HoleCardsRange(const std::string& notation);

	// Fuegt weitere Ausdruecke hinzu. Liefert false bei unlesbarer Notation.
	bool add(const std::string& notation);

	void clear();

	// Ist die Starthand (Karten 0..51) Teil der Range?
	bool contains(int cardA, int cardB) const;

	bool empty() const;

	// Anteil aller 1326 Kartenkombinationen, den die Range abdeckt (0..1).
	// Beruecksichtigt, dass Paare 6, gleichfarbige 4 und ungleichfarbige
	// Haende 12 Kombinationen haben.
	double combinationShare() const;

	// Index 0..168 eines Starthand-Typs; -1 bei ungueltigen Karten.
	static int typeIndex(int cardA, int cardB);

private:
	bool myTypes[169];
};

#endif
