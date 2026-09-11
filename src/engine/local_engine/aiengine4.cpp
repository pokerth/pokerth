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

/* Opening ranges, ordered from the earliest to the latest position. The
 * fewer players still sit behind us, the wider we may open
 * -- exactly the consideration that is completely missing in the old engine.
 */
const char* const OPENING_RANGES[] = {
	// very early (five or more players behind us), about 12 %
	"66+, ATs+, KTs+, QTs+, JTs, AJo+, KQo",
	// middle field (three to four behind us), about 18 %
	"44+, A8s+, K9s+, Q9s+, J9s+, T9s, ATo+, KJo+, QJo",
	// late (one to two behind us), about 28 %
	"22+, A2s+, K7s+, Q8s+, J8s+, T8s+, 97s+, 87s, 76s, 65s, A9o+, KTo+, QTo+, JTo",
	// Button, rund 45 %
	"22+, A2s+, K2s+, Q5s+, J7s+, T7s+, 96s+, 86s+, 75s+, 64s+, 54s, "
	"A2o+, K7o+, Q8o+, J8o+, T8o+, 98o, 87o"
};
const int OPENING_RANGE_COUNT = 4;

/* Hands with which we join a cheap pot with several players afterwards.
 * Deliberately NOT the button range: against many opponents the hands that
 * hit rarely but big pay off (small pairs, suited
 * connectors). Offsuit broadway hands such as KTo, by contrast, are
 * constantly dominated there and do not belong in it.
 */
const char* const LIMP_BEHIND_RANGE = "22-TT, A2s+, K9s+, QTs+, J9s+, T9s, 98s, 87s, 76s, 65s, 54s";

// Hands with which we raise ourselves against a raise (for value).
const char* const THREE_BET_RANGE = "JJ+, AKs, AQs, AKo";

// Hands with which we only call a raise.
const char* const CALL_RAISE_IN_POSITION = "22+, A9s+, KTs+, QTs+, JTs, T9s, 98s, AJo+, KQo";
const char* const CALL_RAISE_OUT_OF_POSITION = "55+, ATs+, KJs+, QJs, AQo+";

// The big blind has already bet and gets better pot odds, so it defends
// considerably wider.
const char* const BIG_BLIND_DEFEND = "22+, A2s+, K5s+, Q7s+, J7s+, T7s+, 96s+, 86s+, 75s+, 65s, 54s, "
									 "A7o+, K9o+, Q9o+, J9o+, T9o";

// Push-or-fold with a short stack: from here on only all or nothing pays off.
const char* const SHORT_STACK_PUSH_EARLY = "44+, A7s+, KTs+, QJs, ATo+, KQo";
const char* const SHORT_STACK_PUSH_LATE  = "22+, A2s+, K7s+, Q9s+, J9s+, T9s, A5o+, K9o+, QTo+, JTo";

/* The assumption about an opponent's hands, graded by the observed entering
 * behaviour. Whoever plays only every eighth hand holds something different when
 * calling than somebody who plays every second one -- the old engine assumes
 * the same random hand for both.
 */
const char* const OPPONENT_RANGE_VERY_TIGHT = "77+, ATs+, KJs+, QJs, AJo+, KQo";
const char* const OPPONENT_RANGE_MEDIUM     = "22+, A2s+, K8s+, Q9s+, J9s+, T9s, 98s, "
		"A9o+, KTo+, QTo+, JTo";

// The assumption about the hands an opponent can still hold after the flop.
const char* const OPPONENT_RANGE_RAISED_POT = "44+, A8s+, KTs+, QTs+, JTs, ATo+, KJo+, QJo";
const char* const OPPONENT_RANGE_LIMPED_POT = "22+, A2s+, K5s+, Q7s+, J7s+, T7s+, 96s+, 86s+, 75s+, 64s+, 54s, "
		"A2o+, K8o+, Q9o+, J9o+, T9o, 98o";

// Build the ranges once and reuse them -- parsing them per decision would be
// unnecessary work. Thread-safe via magic statics.
const HoleCardsRange& cachedRange(const char* notation)
{
	// A small cache is enough, there are only a handful of fixed charts.
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
	// Cache full (does not happen with fixed charts): build it without the cache.
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
// Key figures of the situation
// ---------------------------------------------------------------------------

struct Metrics {
	int bigBlind;
	int toCall;         // what a call costs (already limited to the stack)
	int pot;            // everything that currently lies in the pot
	double potOdds;     // the share needed for a call to pay off
	double stackInBB;   // our own stack in big blinds
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

// Position: we are late if few players sit behind us.
bool inPosition(const AiSituation& s)
{
	return s.opponentsBehind == 0;
}

// Index of the opening range from the number of players behind us.
int openingRangeIndex(const AiSituation& s, const AiPersonality& p)
{
	int index;
	if(s.opponentsBehind >= 5) index = 0;
	else if(s.opponentsBehind >= 3) index = 1;
	else if(s.opponentsBehind >= 1) index = 2;
	else index = 3;

	// Loose natures open one step wider, cautious ones one step tighter.
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

	/* Which hands do we credit the opponents with? A large pot before the flop
	 * means that somebody has raised -- then the selection is tighter. If, on the
	 * other hand, many players are in the hand, not everybody can possibly hold a
	 * premium hand, so we compute wider again.
	 */
	const char* rangeNotation;
	if(s.opponentVpipPercent >= 0) {
		// Beobachtung schlaegt Annahme.
		if(s.opponentVpipPercent < 18) rangeNotation = OPPONENT_RANGE_VERY_TIGHT;
		else if(s.opponentVpipPercent < 30) rangeNotation = OPPONENT_RANGE_RAISED_POT;
		else if(s.opponentVpipPercent < 45) rangeNotation = OPPONENT_RANGE_MEDIUM;
		else rangeNotation = OPPONENT_RANGE_LIMPED_POT;
	} else {
		// Without data the pot size stays the best indication.
		const double potInBigBlinds = static_cast<double>(m.pot) / m.bigBlind;
		const bool raisedPot = potInBigBlinds > 6.0 && s.opponents <= 3;
		rangeNotation = raisedPot ? OPPONENT_RANGE_RAISED_POT : OPPONENT_RANGE_LIMPED_POT;
	}
	const HoleCardsRange& opponentRange = cachedRange(rangeNotation);

	/* Simulation effort by round. Before the flop five board cards are still
	 * to come, which scatters the most; at the river a few runs are enough.
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
// Before the flop
// ---------------------------------------------------------------------------

AiDecision fold(const Metrics& m)
{
	AiDecision d;
	// If calling costs nothing, we never fold.
	d.action = m.canCheck ? PLAYER_ACTION_CHECK : PLAYER_ACTION_FOLD;
	return d;
}

AiDecision call(const Metrics& m)
{
	AiDecision d;
	d.action = m.canCheck ? PLAYER_ACTION_CHECK : PLAYER_ACTION_CALL;
	return d;
}

// A raise to a target bet. evaluation() truncates to the stack itself and
// enforces the minimum raise.
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

/* Short stack: raising means the whole stack anyway, so the only decision left
 * is between "everything" and "nothing". The old engine lacks this
 * mode entirely -- it blinds itself to death in the tournament endgame.
 */
AiDecision decideShortStack(const AiSituation& s, const AiPersonality& p, const Metrics& m)
{
	const bool late = s.opponentsBehind <= 2;
	const HoleCardsRange& pushRange =
		cachedRange(late ? SHORT_STACK_PUSH_LATE : SHORT_STACK_PUSH_EARLY);

	if(pushRange.contains(s.holeCards[0], s.holeCards[1])) return allIn(s, m);

	// Outside the push range: only play on if it costs nothing.
	if(m.canCheck) return call(m);

	/* In the big blind against a small raise we often get such good pot
	 * odds that folding would be more expensive than calling.
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
			// A base size of three big blinds, one more per limper.
			const int limpers = std::max(0, (s.setsThisRound - m.bigBlind - s.smallBlind) / m.bigBlind);
			int target = static_cast<int>((3 + limpers) * m.bigBlind * p.aggression);
			target = std::max(target, s.highestSet + m.bigBlind);
			return raiseTo(s, m, target);
		}

		if(m.canCheck) return call(m);

		/* Outside the opening range we only call if somebody has already limped
		 * in front of us and the pot has a correspondingly large number of players. Limping
		 * into a pot that is still untouched, by contrast, would only be weak.
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
		// Not at any price: a very large raise blows the odds.
		const double priceInBB = static_cast<double>(m.toCall) / m.bigBlind;
		if(priceInBB <= 12.0 * p.looseness) return call(m);
	}

	// An occasional bluff against few opponents, so that we do not become readable.
	if(s.opponents == 1 && inPosition(s) && randomPercent() <= static_cast<int>(6 * p.bluffRate)) {
		return raiseTo(s, m, 3 * s.highestSet);
	}

	return fold(m);
}

}

namespace
{

// ---------------------------------------------------------------------------
// After the flop
// ---------------------------------------------------------------------------

AiDecision decidePostflop(const AiSituation& s, const AiPersonality& p, const Metrics& m)
{
	const double equity = AiEngine4::winningChance(s);

	/* The reference size is the share that would be due to us if everybody were
	 * equally strong. At a full table that is little, heads-up it is half -- which is why
	 * the same hand must not be played like a monster multiway.
	 */
	const double fairShare = 1.0 / (s.opponents + 1);

	if(m.canCheck) {
		if(equity > fairShare * 1.5 / p.looseness) {
			// The clearer the lead, the larger the bet.
			double fraction = (equity > fairShare * 2.2) ? 0.75 : 0.55;
			fraction *= p.aggression;
			const int amount = std::max(m.bigBlind, static_cast<int>(m.pot * fraction));
			return raiseTo(s, m, s.mySet + amount);
		}

		// Bluff only against few opponents and preferably as the last one.
		if(s.opponents <= 2 && inPosition(s) && randomPercent() <= static_cast<int>(18 * p.bluffRate)) {
			const int amount = std::max(m.bigBlind, static_cast<int>(m.pot * 0.5));
			return raiseTo(s, m, s.mySet + amount);
		}

		return call(m);
	}

	/* A call pays off if the winning chance exceeds the pot odds. Before
	 * the river a surcharge is added, because later rounds can still cost
	 * money.
	 */
	double margin = (s.round == GAME_STATE_RIVER) ? 0.0 : 0.03;

	/* Whoever bets and raises frequently inevitably does so with weak
	 * hands as well -- against such opponents one may call more cheaply. Against notably
	 * passive opponents the opposite applies: their bets mean more.
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

	/* Semi-bluff: too little for a call, but enough substance to win with a
	 * raise if the opponent gives up.
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

	// Without opponents there is nothing to decide.
	if(situation.opponents < 1) return call(m);

	if(situation.round == GAME_STATE_PREFLOP) return decidePreflop(situation, personality, m);

	return decidePostflop(situation, personality, m);
}
