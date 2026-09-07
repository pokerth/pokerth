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

#ifndef AIENGINE4_H
#define AIENGINE4_H

#include "game_defs.h"

/* AI-Engine 4 -- die Entscheidungslogik der Computerspieler.
 *
 * Bewusst frei von Spielzustand: die Engine bekommt eine Momentaufnahme und
 * liefert eine Absicht zurueck. Das Verbuchen von Chips, die Full-Bet-Rule und
 * die Mindesterhoehung bleiben in LocalPlayer::evaluation(), wo sie seit jeher
 * korrekt behandelt werden. Dadurch ist die Engine ohne Tisch, Board und
 * Spielerliste testbar.
 *
 * Was sie anders macht als die alten Engines:
 *  - Sie rechnet die Gewinnchance gegen ALLE verbleibenden Gegner, nicht gegen
 *    einen einzelnen Zufallsgegner.
 *  - Sie kennt den Pot und vergleicht die Chance mit den Pot Odds, statt feste
 *    Schwellenwerte abzufragen.
 *  - Sie kennt ihre Position und spielt vor dem Flop nach Positions-Charts.
 *  - Sie setzt in Pot-Anteilen statt in Vielfachen des Small Blind.
 *  - Sie kennt Push-or-Fold bei kurzem Stack.
 */

struct AiSituation {
	// --- Karten ---
	int holeCards[2];
	int boardCards[5];
	int boardSize;              // 0 (praeflop), 3, 4 oder 5
	int round;                  // GAME_STATE_PREFLOP .. GAME_STATE_RIVER

	// --- Geld, alles in Chips ---
	int myCash;                 // noch vor mir liegende Chips
	int mySet;                  // in dieser Setzrunde bereits gesetzt
	int highestSet;             // hoechster Satz dieser Runde
	int minimumRaise;           // kleinste zulaessige Erhoehung
	int smallBlind;
	int potBefore;              // eingesammelter Pot der frueheren Runden
	int setsThisRound;          // Summe der Saetze dieser Runde

	// --- Tisch ---
	int opponents;              // Gegner, die noch in der Hand sind
	int opponentsBehind;        // davon noch nicht am Zug -- unsere Position
	int effectiveOpponentCash;  // groesster Stack unter den Gegnern
	int raisesThisRound;        // Anzahl Erhoehungen ueber den Grundsatz hinaus
	bool amBigBlind;            // wir sind der Big Blind (praeflop wichtig)

	/* --- Gegnermodell ---
	 * Beobachtungswerte der Gegner, die noch in der Hand sind. Jeweils -1,
	 * solange zu wenige Haende vorliegen, um daraus etwas abzuleiten; dann
	 * faellt die Engine auf ihre Standardannahmen zurueck.
	 */
	int opponentVpipPercent;        // wie oft sie vor dem Flop freiwillig einsteigen
	int opponentAggressionPercent;  // Anteil von Einsatz/Erhoehung an ihren Aktionen nach dem Flop
};

/* Spielernaturen. Alle Werte sind Multiplikatoren um 1.0 herum; sie
 * verschieben das Verhalten, ohne die EV-Grundlage auszuhebeln. So bleiben die
 * Bots unterscheidbar, ohne wieder in Zufallsspiel zu verfallen.
 */
struct AiPersonality {
	double looseness;   // > 1 spielt mehr Haende
	double aggression;  // > 1 setzt und erhoeht haeufiger und groesser
	double bluffRate;   // Anteil reiner Bluffs

	AiPersonality() : looseness(1.0), aggression(1.0), bluffRate(1.0) {}
};

struct AiDecision {
	PlayerAction action;
	// Fuer PLAYER_ACTION_BET der angestrebte Satz, fuer PLAYER_ACTION_RAISE der
	// Betrag oberhalb des hoechsten Satzes -- genau das, was
	// LocalPlayer::evaluation(bet, raise) erwartet.
	int bet;
	int raise;

	AiDecision() : action(PLAYER_ACTION_NONE), bet(0), raise(0) {}
};

class AiEngine4
{
public:
	static AiDecision decide(const AiSituation& situation, const AiPersonality& personality);

	// Gewinnchance in der aktuellen Lage (0..1). Oeffentlich, damit sich das
	// Verhalten der Engine nachvollziehen und protokollieren laesst.
	static double winningChance(const AiSituation& situation);
};

#endif
