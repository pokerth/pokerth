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

/* Schneller 7-Karten-Evaluator fuer die AI-Engine 4.
 *
 * Liefert dieselbe Rangfolge wie CardsValue::cardsValue(), ist aber rund
 * siebenmal schneller, weil er ueber Bitmasken statt ueber drei Bubble-Sorts
 * arbeitet. Die absoluten Werte sind NICHT mit CardsValue vergleichbar --
 * nur die Ordnung stimmt ueberein. Fuer Showdown-Anzeige und Pot-Verteilung
 * bleibt weiterhin CardsValue zustaendig; dieser Evaluator dient allein der
 * Monte-Carlo-Simulation, die pro Entscheidung zehntausende Auswertungen
 * braucht.
 *
 * Kartenkodierung wie im uebrigen Engine-Code: 0..51, Farbe = c/13,
 * Wert = c%13 mit 0 = Zwei ... 12 = Ass.
 */
class HandEvaluator
{
public:
	// Optionales Vorwaermen der Lookup-Tabelle. Ohne diesen Aufruf baut sie
	// sich beim ersten value()-Aufruf selbst auf -- threadsicher, aber dann
	// eben innerhalb der ersten Entscheidung.
	static void init();

	// Bewertet genau sieben Karten. Groesserer Rueckgabewert = staerkere Hand.
	static unsigned value(const int* cards);
};

#endif
