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

#include "aiengine4.h"

#include "equitycalculator.h"
#include "holecardsrange.h"
#include "tools.h"

#include <algorithm>
#include <cmath>

namespace
{

// ---------------------------------------------------------------------------
// Praeflop-Charts
// ---------------------------------------------------------------------------

/* Eroeffnungsranges, geordnet von fruehester zu spaetester Position. Je
 * weniger Spieler noch hinter uns sitzen, desto breiter darf eroeffnet werden
 * -- genau die Ueberlegung, die in der alten Engine vollstaendig fehlt.
 */
const char* const OPENING_RANGES[] = {
	// sehr frueh (fuenf oder mehr Spieler hinter uns), rund 12 %
	"66+, ATs+, KTs+, QTs+, JTs, AJo+, KQo",
	// Mittelfeld (drei bis vier hinter uns), rund 18 %
	"44+, A8s+, K9s+, Q9s+, J9s+, T9s, ATo+, KJo+, QJo",
	// spaet (ein bis zwei hinter uns), rund 28 %
	"22+, A2s+, K7s+, Q8s+, J8s+, T8s+, 97s+, 87s, 76s, 65s, A9o+, KTo+, QTo+, JTo",
	// Button, rund 45 %
	"22+, A2s+, K2s+, Q5s+, J7s+, T7s+, 96s+, 86s+, 75s+, 64s+, 54s, "
	"A2o+, K7o+, Q8o+, J8o+, T8o+, 98o, 87o"
};
const int OPENING_RANGE_COUNT = 4;

/* Haende, mit denen wir einem billigen Pot mit mehreren Mitspielern nachtraeglich
 * beitreten. Bewusst NICHT die Button-Range: gegen viele Gegner zahlen sich
 * Haende aus, die selten aber gross treffen (kleine Paare, gleichfarbige
 * Verbinder). Ungleichfarbige Broadway-Haende wie KTo sind dort umgekehrt
 * staendig dominiert und gehoeren nicht dazu.
 */
const char* const LIMP_BEHIND_RANGE = "22-TT, A2s+, K9s+, QTs+, J9s+, T9s, 98s, 87s, 76s, 65s, 54s";

// Haende, mit denen wir gegen eine Erhoehung selbst erhoehen (auf Wert).
const char* const THREE_BET_RANGE = "JJ+, AKs, AQs, AKo";

// Haende, mit denen wir eine Erhoehung nur mitgehen.
const char* const CALL_RAISE_IN_POSITION = "22+, A9s+, KTs+, QTs+, JTs, T9s, 98s, AJo+, KQo";
const char* const CALL_RAISE_OUT_OF_POSITION = "55+, ATs+, KJs+, QJs, AQo+";

// Big Blind hat bereits gesetzt und bekommt bessere Pot Odds, verteidigt also
// deutlich breiter.
const char* const BIG_BLIND_DEFEND = "22+, A2s+, K5s+, Q7s+, J7s+, T7s+, 96s+, 86s+, 75s+, 65s, 54s, "
									 "A7o+, K9o+, Q9o+, J9o+, T9o";

// Push-or-Fold bei kurzem Stack: ab hier lohnt sich nur noch alles oder nichts.
const char* const SHORT_STACK_PUSH_EARLY = "44+, A7s+, KTs+, QJs, ATo+, KQo";
const char* const SHORT_STACK_PUSH_LATE  = "22+, A2s+, K7s+, Q9s+, J9s+, T9s, A5o+, K9o+, QTo+, JTo";

/* Annahme ueber die Haende eines Gegners, gestuft nach beobachtetem Einstiegs-
 * verhalten. Wer nur jede achte Hand spielt, hat beim Mitgehen etwas anderes
 * als jemand, der jede zweite spielt -- die alte Engine unterstellt beiden
 * dieselbe Zufallshand.
 */
const char* const OPPONENT_RANGE_VERY_TIGHT = "77+, ATs+, KJs+, QJs, AJo+, KQo";
const char* const OPPONENT_RANGE_MEDIUM     = "22+, A2s+, K8s+, Q9s+, J9s+, T9s, 98s, "
		"A9o+, KTo+, QTo+, JTo";

// Annahme ueber die Haende, die ein Gegner nach dem Flop noch halten kann.
const char* const OPPONENT_RANGE_RAISED_POT = "44+, A8s+, KTs+, QTs+, JTs, ATo+, KJo+, QJo";
const char* const OPPONENT_RANGE_LIMPED_POT = "22+, A2s+, K5s+, Q7s+, J7s+, T7s+, 96s+, 86s+, 75s+, 64s+, 54s, "
		"A2o+, K8o+, Q9o+, J9o+, T9o, 98o";

// Ranges einmal bauen und wiederverwenden -- das Parsen je Entscheidung waere
// unnoetige Arbeit. Threadsicher ueber magic statics.
const HoleCardsRange& cachedRange(const char* notation)
{
	// Ein kleiner Cache genuegt, es sind nur eine Handvoll fester Charts.
	static thread_local const char* keys[16] = { 0 };
	static thread_local HoleCardsRange values[16];
	for(int i = 0; i < 16; ++i) {
		if(keys[i] == notation) return values[i];
		if(keys[i] == 0) {
			values[i] = HoleCardsRange(notation);
			keys[i] = notation;
			return values[i];
		}
	}
	// Cache voll (kommt bei festen Charts nicht vor): ohne Cache bauen.
	static thread_local HoleCardsRange fallback;
	fallback = HoleCardsRange(notation);
	return fallback;
}

int randomPercent()
{
	int value;
	Tools::GetRand(1, 100, 1, &value);
	return value;
}

// ---------------------------------------------------------------------------
// Kennzahlen der Lage
// ---------------------------------------------------------------------------

struct Metrics {
	int bigBlind;
	int toCall;         // was ein Mitgehen kostet (bereits auf den Stack begrenzt)
	int pot;            // alles, was aktuell im Pot liegt
	double potOdds;     // benoetigter Anteil, damit ein Call sich rechnet
	double stackInBB;   // eigener Stack in Big Blinds
	bool canCheck;
};

Metrics measure(const AiSituation& s)
{
	Metrics m;
	m.bigBlind = std::max(1, 2 * s.smallBlind);
	m.toCall = std::max(0, std::min(s.highestSet - s.mySet, s.myCash));
	m.pot = std::max(0, s.potBefore + s.setsThisRound);
	m.potOdds = (m.pot + m.toCall) > 0 ? static_cast<double>(m.toCall) / (m.pot + m.toCall) : 0.0;
	m.stackInBB = static_cast<double>(s.myCash + s.mySet) / m.bigBlind;
	m.canCheck = (m.toCall == 0);
	return m;
}

// Position: wir sind spaet dran, wenn wenige Spieler hinter uns sitzen.
bool inPosition(const AiSituation& s)
{
	return s.opponentsBehind == 0;
}

// Index der Eroeffnungsrange aus der Anzahl der Spieler hinter uns.
int openingRangeIndex(const AiSituation& s, const AiPersonality& p)
{
	int index;
	if(s.opponentsBehind >= 5) index = 0;
	else if(s.opponentsBehind >= 3) index = 1;
	else if(s.opponentsBehind >= 1) index = 2;
	else index = 3;

	// Lockere Naturen eroeffnen eine Stufe breiter, vorsichtige eine enger.
	if(p.looseness > 1.10) ++index;
	else if(p.looseness < 0.90) --index;

	return std::max(0, std::min(OPENING_RANGE_COUNT - 1, index));
}

}

// ---------------------------------------------------------------------------
// Gewinnchance
// ---------------------------------------------------------------------------

double AiEngine4::winningChance(const AiSituation& s)
{
	if(s.opponents < 1) return 1.0;

	const Metrics m = measure(s);

	/* Welche Haende trauen wir den Gegnern zu? Ein grosser Pot vor dem Flop
	 * bedeutet, dass jemand erhoeht hat -- dann ist die Auswahl enger. Sitzen
	 * dagegen viele Spieler in der Hand, kann unmoeglich jeder eine Spitzenhand
	 * halten, also rechnen wir wieder breiter.
	 */
	const char* rangeNotation;
	if(s.opponentVpipPercent >= 0) {
		// Beobachtung schlaegt Annahme.
		if(s.opponentVpipPercent < 18) rangeNotation = OPPONENT_RANGE_VERY_TIGHT;
		else if(s.opponentVpipPercent < 30) rangeNotation = OPPONENT_RANGE_RAISED_POT;
		else if(s.opponentVpipPercent < 45) rangeNotation = OPPONENT_RANGE_MEDIUM;
		else rangeNotation = OPPONENT_RANGE_LIMPED_POT;
	} else {
		// Ohne Daten bleibt die Potgroesse der beste Anhaltspunkt.
		const double potInBigBlinds = static_cast<double>(m.pot) / m.bigBlind;
		const bool raisedPot = potInBigBlinds > 6.0 && s.opponents <= 3;
		rangeNotation = raisedPot ? OPPONENT_RANGE_RAISED_POT : OPPONENT_RANGE_LIMPED_POT;
	}
	const HoleCardsRange& opponentRange = cachedRange(rangeNotation);

	/* Simulationsaufwand nach Runde. Vor dem Flop sind noch fuenf Board-Karten
	 * offen, das streut am staerksten; am River genuegen wenige Laeufe.
	 */
	int samples;
	switch(s.round) {
	case GAME_STATE_PREFLOP:
		samples = 6000;
		break;
	case GAME_STATE_FLOP:
		samples = 8000;
		break;
	case GAME_STATE_TURN:
		samples = 6000;
		break;
	default:
		samples = 4000;
		break;
	}

	const EquityCalculator::Result result =
		EquityCalculator::equity(s.holeCards, s.boardCards, s.boardSize,
								 s.opponents, &opponentRange, samples);

	return result.samples > 0 ? result.equity : 0.0;
}

namespace
{

// ---------------------------------------------------------------------------
// Vor dem Flop
// ---------------------------------------------------------------------------

AiDecision fold(const Metrics& m)
{
	AiDecision d;
	// Kostet das Mitgehen nichts, wird nie geworfen.
	d.action = m.canCheck ? PLAYER_ACTION_CHECK : PLAYER_ACTION_FOLD;
	return d;
}

AiDecision call(const Metrics& m)
{
	AiDecision d;
	d.action = m.canCheck ? PLAYER_ACTION_CHECK : PLAYER_ACTION_CALL;
	return d;
}

// Erhoehung auf einen Zielsatz. evaluation() kuerzt selbst auf den Stack und
// erzwingt die Mindesterhoehung.
AiDecision raiseTo(const AiSituation& s, const Metrics& m, int targetSet)
{
	AiDecision d;
	targetSet = std::min(targetSet, s.myCash + s.mySet);

	if(s.highestSet == 0) {
		d.action = PLAYER_ACTION_BET;
		d.bet = std::max(m.bigBlind, targetSet);
		return d;
	}

	d.action = PLAYER_ACTION_RAISE;
	d.raise = std::max(s.minimumRaise, targetSet - s.highestSet);
	return d;
}

AiDecision allIn(const AiSituation& s, const Metrics& m)
{
	return raiseTo(s, m, s.myCash + s.mySet);
}

/* Kurzer Stack: Erhoehen bedeutet ohnehin den ganzen Stack, also wird nur noch
 * zwischen "alles" und "nichts" entschieden. Der alten Engine fehlt dieser
 * Modus voellig -- sie zahlt sich im Turnierendspiel blind zu Tode.
 */
AiDecision decideShortStack(const AiSituation& s, const AiPersonality& p, const Metrics& m)
{
	const bool late = s.opponentsBehind <= 2;
	const HoleCardsRange& pushRange =
		cachedRange(late ? SHORT_STACK_PUSH_LATE : SHORT_STACK_PUSH_EARLY);

	if(pushRange.contains(s.holeCards[0], s.holeCards[1])) return allIn(s, m);

	// Ausserhalb der Push-Range: nur weiterspielen, wenn es nichts kostet.
	if(m.canCheck) return call(m);

	/* Im Big Blind gegen eine kleine Erhoehung bekommen wir oft so gute Pot
	 * Odds, dass Wegwerfen teurer waere als Mitgehen.
	 */
	if(s.amBigBlind && m.potOdds < 0.25 * p.looseness) return call(m);

	return fold(m);
}

AiDecision decidePreflop(const AiSituation& s, const AiPersonality& p, const Metrics& m)
{
	if(m.stackInBB <= 12.0) return decideShortStack(s, p, m);

	const bool facingRaise = s.highestSet > m.bigBlind;

	if(!facingRaise) {
		const HoleCardsRange& opening = cachedRange(OPENING_RANGES[openingRangeIndex(s, p)]);

		if(opening.contains(s.holeCards[0], s.holeCards[1])) {
			// Grundgroesse drei Big Blinds, je Limper einer mehr.
			const int limpers = std::max(0, (s.setsThisRound - m.bigBlind - s.smallBlind) / m.bigBlind);
			int target = static_cast<int>((3 + limpers) * m.bigBlind * p.aggression);
			target = std::max(target, s.highestSet + m.bigBlind);
			return raiseTo(s, m, target);
		}

		if(m.canCheck) return call(m);

		/* Ausserhalb der Eroeffnungsrange gehen wir nur mit, wenn vor uns bereits
		 * jemand gelimpt hat und der Pot entsprechend viele Mitspieler hat. In
		 * einen noch unberuehrten Pot zu limpen waere dagegen nur schwach.
		 */
		const int limpersBefore = (s.setsThisRound - m.bigBlind - s.smallBlind) / m.bigBlind;
		if(m.toCall <= m.bigBlind && s.opponents >= 3 && limpersBefore >= 1) {
			const HoleCardsRange& speculative = cachedRange(LIMP_BEHIND_RANGE);
			if(speculative.contains(s.holeCards[0], s.holeCards[1])) return call(m);
		}

		return fold(m);
	}

	// Es wurde erhoeht.
	const HoleCardsRange& threeBet = cachedRange(THREE_BET_RANGE);
	if(threeBet.contains(s.holeCards[0], s.holeCards[1])) {
		const int factor = inPosition(s) ? 3 : 4;
		return raiseTo(s, m, static_cast<int>(factor * s.highestSet * p.aggression));
	}

	const HoleCardsRange& calling =
		cachedRange(s.amBigBlind ? BIG_BLIND_DEFEND
					: (inPosition(s) ? CALL_RAISE_IN_POSITION : CALL_RAISE_OUT_OF_POSITION));

	if(calling.contains(s.holeCards[0], s.holeCards[1])) {
		// Nicht um jeden Preis: eine sehr grosse Erhoehung sprengt die Odds.
		const double priceInBB = static_cast<double>(m.toCall) / m.bigBlind;
		if(priceInBB <= 12.0 * p.looseness) return call(m);
	}

	// Gelegentlicher Bluff gegen wenige Gegner, damit wir nicht lesbar werden.
	if(s.opponents == 1 && inPosition(s) && randomPercent() <= static_cast<int>(6 * p.bluffRate)) {
		return raiseTo(s, m, 3 * s.highestSet);
	}

	return fold(m);
}

}

namespace
{

// ---------------------------------------------------------------------------
// Nach dem Flop
// ---------------------------------------------------------------------------

AiDecision decidePostflop(const AiSituation& s, const AiPersonality& p, const Metrics& m)
{
	const double equity = AiEngine4::winningChance(s);

	/* Bezugsgroesse ist der Anteil, der uns zustuende, wenn alle gleich stark
	 * waeren. Am vollen Tisch ist das wenig, heads-up die Haelfte -- deshalb
	 * darf dieselbe Hand multiway nicht mehr wie ein Monster gespielt werden.
	 */
	const double fairShare = 1.0 / (s.opponents + 1);

	if(m.canCheck) {
		if(equity > fairShare * 1.5 / p.looseness) {
			// Je klarer die Fuehrung, desto groesser der Einsatz.
			double fraction = (equity > fairShare * 2.2) ? 0.75 : 0.55;
			fraction *= p.aggression;
			const int amount = std::max(m.bigBlind, static_cast<int>(m.pot * fraction));
			return raiseTo(s, m, s.mySet + amount);
		}

		// Bluff nur gegen wenige Gegner und moeglichst als Letzter.
		if(s.opponents <= 2 && inPosition(s) && randomPercent() <= static_cast<int>(18 * p.bluffRate)) {
			const int amount = std::max(m.bigBlind, static_cast<int>(m.pot * 0.5));
			return raiseTo(s, m, s.mySet + amount);
		}

		return call(m);
	}

	/* Ein Mitgehen lohnt, wenn die Gewinnchance die Pot Odds uebersteigt. Vor
	 * dem River kommt ein Aufschlag dazu, weil spaetere Runden noch Geld kosten
	 * koennen.
	 */
	double margin = (s.round == GAME_STATE_RIVER) ? 0.0 : 0.03;

	/* Wer haeufig setzt und erhoeht, tut das zwangslaeufig auch mit schwachen
	 * Haenden -- gegen solche Gegner darf man billiger mitgehen. Gegen auffaellig
	 * passive Gegner gilt das Umgekehrte: deren Einsaetze bedeuten mehr.
	 */
	if(s.opponentAggressionPercent >= 0) {
		double shift = (s.opponentAggressionPercent - 40) / 1000.0;
		shift = std::max(-0.03, std::min(0.03, shift));
		margin -= shift;
		margin = std::max(-0.02, margin);
	}

	if(equity >= m.potOdds + margin) {
		const bool clearlyAhead = equity > fairShare * 1.9;
		// Endlose Erhoehungsketten vermeiden.
		if(clearlyAhead && s.raisesThisRound < 3) {
			const int amount = std::max(s.minimumRaise, static_cast<int>(m.pot * 0.7 * p.aggression));
			return raiseTo(s, m, s.highestSet + amount);
		}
		return call(m);
	}

	/* Halbbluff: zu wenig fuer ein Mitgehen, aber genug Substanz, um mit einer
	 * Erhoehung zu gewinnen, wenn der Gegner aufgibt.
	 */
	if(s.opponents == 1 && inPosition(s) && equity > 0.25 &&
			randomPercent() <= static_cast<int>(12 * p.bluffRate)) {
		const int amount = std::max(s.minimumRaise, static_cast<int>(m.pot * 0.6));
		return raiseTo(s, m, s.highestSet + amount);
	}

	return fold(m);
}

}

AiDecision AiEngine4::decide(const AiSituation& situation, const AiPersonality& personality)
{
	const Metrics m = measure(situation);

	// Ohne Gegner gibt es nichts zu entscheiden.
	if(situation.opponents < 1) return call(m);

	if(situation.round == GAME_STATE_PREFLOP) return decidePreflop(situation, personality, m);

	return decidePostflop(situation, personality, m);
}
