/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 *                                                                           *
 * Overlay for the emoji reaction choreography at the game table – a 1:1 port *
 * of the QML component GameReactionFx (keyframes, effect catalogue and timings *
 * identical, see src/gui/qt6-qml/config/ReactionCatalog.qml).               *
 *****************************************************************************/
#include "reactionfx.h"
#include "emojipicker.h"

#include <QRandomGenerator>
#include <cmath>

// ── Choreographies ──
// Keyframes of the CSS animations rfx* of the web client. Channels: x/y in
// CSS percent (base -50), s = scale, r = rotation, ry = rotation around the
// Y axis (card turner), o = opacity, b = brightening.
struct ReactionKey {
	qreal t;
	qreal v;
};
struct ReactionBezier {
	qreal x1, y1, x2, y2;
};
struct ReactionAnim {
	int dur = 1600;
	ReactionBezier ease = {0.0, 0.0, 1.0, 1.0};
	QVector<ReactionKey> x, y, s, r, ry, o, b;
};

namespace
{

// -150 % (the standard rise) corresponds to 160 px of flight height.
const qreal kPxPerPercent = 1.6;
// Base size of the large emoji at scale 1.0.
const qreal kBaseSize = 34.0;
// The keyframe durations below are those of the web client (1400–1700 ms) and thus
// noticeably tighter than the fixed flight time of 2000 ms in 2.1.7. This factor
// stretches them back to the usual pace (standard choreography "pop":
// 1600 ms * 1.25 = 2000 ms) – it applies to the choreography and to all delays
// aligned to it. Identical in ReactionCatalog.qml
// (durationScale).
const qreal kDurationScale = 1.25;

const ReactionBezier kLinear  = {0.0,  0.0, 1.0,  1.0};
const ReactionBezier kEaseOut = {0.0,  0.0, 0.58, 1.0};
const ReactionBezier kEaseIn  = {0.42, 0.0, 1.0,  1.0};

double rnd(double from, double to)
{
	return from + QRandomGenerator::global()->generateDouble() * (to - from);
}

// Cubic Bézier timing function (like CSS cubic-bezier): Newton iteration
// on x(t) = f, then y(t).
qreal bezier(const ReactionBezier &b, qreal f)
{
	if (f <= 0.0) return 0.0;
	if (f >= 1.0) return 1.0;
	const qreal cx = 3.0 * b.x1, bx = 3.0 * (b.x2 - b.x1) - cx, ax = 1.0 - cx - bx;
	const qreal cy = 3.0 * b.y1, by = 3.0 * (b.y2 - b.y1) - cy, ay = 1.0 - cy - by;
	qreal t = f;
	for (int i = 0; i < 8; ++i) {
		const qreal x = ((ax * t + bx) * t + cx) * t - f;
		if (std::fabs(x) < 1e-5) break;
		const qreal d = (3.0 * ax * t + 2.0 * bx) * t + cx;
		if (std::fabs(d) < 1e-6) break;
		t -= x / d;
	}
	return ((ay * t + by) * t + cy) * t;
}

// Value of a channel at the time share t (0..1). Before the first and after the
// last keyframe the respective edge value applies – that is how CSS holds properties
// that are only set at the beginning. The timing function applies – likewise as
// in CSS – to each keyframe section individually.
qreal sample(const ReactionAnim &a, const QVector<ReactionKey> &kf, qreal t, qreal fallback)
{
	if (kf.isEmpty())
		return fallback;
	if (t <= kf.first().t)
		return kf.first().v;
	for (int i = 1; i < kf.size(); ++i) {
		if (t <= kf.at(i).t) {
			const qreal span = kf.at(i).t - kf.at(i - 1).t;
			const qreal f = span > 0 ? (t - kf.at(i - 1).t) / span : 1.0;
			return kf.at(i - 1).v + (kf.at(i).v - kf.at(i - 1).v) * bezier(a.ease, f);
		}
	}
	return kf.last().v;
}

ReactionAnim mk(int dur, const ReactionBezier &ease)
{
	ReactionAnim a;
	a.dur = qRound(dur * kDurationScale);
	a.ease = ease;
	return a;
}

const QHash<QString, ReactionAnim> &animTable()
{
	static const QHash<QString, ReactionAnim> table = []() {
		QHash<QString, ReactionAnim> t;
		{
			ReactionAnim a = mk(1600, {0.2, 0.8, 0.3, 1.0});
			a.y = {{0, -50}, {0.25, -50}, {0.55, -60}, {1, -150}};
			a.s = {{0, 0.2}, {0.25, 1.45}, {0.55, 1.05}, {1, 0.9}};
			a.o = {{0, 0}, {0.25, 1}, {1, 0}};
			t.insert("pop", a);
		}
		{
			ReactionAnim a = mk(1700, kEaseOut);
			a.y = {{0, -50}, {0.2, -55}, {0.4, -70}, {0.6, -90}, {1, -150}};
			a.s = {{0, 0.3}, {0.2, 1.4}, {0.4, 1.2}, {0.6, 1.35}, {1, 1}};
			a.r = {{0.2, 0}, {0.4, -4}, {0.6, 4}, {1, 0}};
			a.o = {{0, 0}, {0.2, 1}, {1, 0}};
			t.insert("fire", a);
		}
		{
			ReactionAnim a = mk(1700, kEaseOut);
			a.y = {{0, -50}, {0.15, -55}, {0.25, -60}, {0.35, -65}, {0.45, -72},
				{0.55, -80}, {0.7, -95}, {0.85, -115}, {1, -150}
			};
			a.s = {{0, 0.3}, {0.15, 1.35}};
			a.r = {{0.15, 0}, {0.25, -14}, {0.35, 14}, {0.45, -12}, {0.55, 12},
				{0.7, -6}, {0.85, 4}, {1, 0}
			};
			a.o = {{0, 0}, {0.15, 1}, {1, 0}};
			t.insert("shake", a);
		}
		{
			ReactionAnim a = mk(1700, kEaseOut);
			a.y = {{0, -50}, {0.2, -55}, {0.33, -60}, {0.48, -70}, {0.63, -82}, {1, -150}};
			a.s = {{0, 0.2}, {0.2, 1.5}, {0.33, 1.05}, {0.48, 1.4}, {0.63, 1.1}, {1, 1}};
			a.o = {{0, 0}, {0.2, 1}, {1, 0}};
			t.insert("beat", a);
		}
		{
			ReactionAnim a = mk(1600, {0.2, 0.7, 0.3, 1.0});
			a.y = {{0, -50}, {0.25, -60}, {1, -150}};
			a.s = {{0, 0.3}, {0.25, 1.45}, {1, 1}};
			a.r = {{0, 0}, {0.25, 200}, {1, 720}};
			a.o = {{0, 0}, {0.25, 1}, {1, 0}};
			t.insert("spin", a);
		}
		{
			ReactionAnim a = mk(1600, kEaseOut);
			a.y = {{0, -50}, {0.25, -58}, {1, -150}};
			a.s = {{0, 0.3}, {0.25, 1.45}, {1, 1.05}};
			a.o = {{0, 0}, {0.25, 1}, {1, 0}};
			// CSS: brightness(2.2) + glow at half of the animation.
			a.b = {{0, 0}, {0.25, 0}, {0.5, 0.55}, {1, 0}};
			t.insert("shine", a);
		}
		{
			// Note: in the web client flex stays invisible with opacity 0 → 0
			// (a CSS bug in rfxFlex). Here the curve that was obviously meant.
			ReactionAnim a = mk(1600, kEaseOut);
			a.y = {{0, -50}, {0.2, -55}, {0.35, -60}, {0.5, -70}, {0.7, -90}, {1, -150}};
			a.s = {{0, 0.3}, {0.2, 1.5}, {0.35, 1.1}, {0.5, 1.45}, {0.7, 1.15}, {1, 1}};
			a.o = {{0, 0}, {0.2, 1}, {1, 0}};
			t.insert("flex", a);
		}
		{
			ReactionAnim a = mk(1400, {0.3, 0.1, 0.4, 1.0});
			a.y = {{0, -30}, {0.2, -50}, {1, -260}};
			a.s = {{0, 0.5}, {0.2, 1.3}, {1, 1}};
			a.r = {{0, -8}, {1, -8}};
			a.o = {{0, 0}, {0.2, 1}, {1, 0}};
			t.insert("launch", a);
		}
		{
			ReactionAnim a = mk(1500, kEaseOut);
			a.y = {{0, -170}, {0.35, -50}, {0.5, -72}, {0.65, -50}, {0.8, -58}, {1, -70}};
			a.s = {{0, 0.7}, {0.35, 1.25}, {0.5, 1.1}, {0.65, 1.2}, {0.8, 1.15}, {1, 1.1}};
			a.o = {{0, 0}, {0.35, 1}, {1, 0}};
			t.insert("drop", a);
		}
		{
			ReactionAnim a = mk(1700, kEaseOut);
			a.y = {{0, -50}, {0.15, -55}, {0.3, -62}, {0.45, -70}, {0.6, -80},
				{0.75, -95}, {1, -150}
			};
			a.s = {{0, 0.3}, {0.15, 1.35}};
			a.r = {{0, 0}, {0.15, 16}, {0.3, -14}, {0.45, 11}, {0.6, -8}, {0.75, 5}, {1, 0}};
			a.o = {{0, 0}, {0.15, 1}, {1, 0}};
			t.insert("wobble", a);
		}
		{
			ReactionAnim a = mk(1600, {0.2, 0.7, 0.3, 1.0});
			a.y = {{0, -50}, {0.25, -58}, {1, -150}};
			a.s = {{0, 0.4}, {0.25, 1.4}, {1, 1}};
			a.ry = {{0, 0}, {0.25, 360}, {1, 900}};
			a.o = {{0, 0}, {0.25, 1}, {1, 0}};
			t.insert("flip", a);
		}
		{
			ReactionAnim a = mk(1600, kEaseOut);
			a.y = {{0, -50}, {0.2, -50}, {0.45, -55}, {0.6, -58}, {1, -150}};
			a.s = {{0, 3}, {0.2, 2.4}, {0.45, 1.05}, {0.6, 1.18}, {1, 1}};
			a.o = {{0, 0}, {0.2, 1}, {1, 0}};
			t.insert("zoomout", a);
		}
		{
			ReactionAnim a = mk(1700, kEaseOut);
			a.y = {{0, -50}, {0.12, -52}, {0.22, -56}, {0.32, -60}, {0.42, -66},
				{0.58, -76}, {0.72, -90}, {1, -150}
			};
			a.s = {{0, 0.2}, {0.12, 1.45}, {0.22, 1.05}, {0.32, 1.5}, {0.42, 1.08},
				{0.58, 1.42}, {0.72, 1.1}, {1, 1}
			};
			a.o = {{0, 0}, {0.12, 1}, {1, 0}};
			t.insert("heartbeat", a);
		}
		{
			ReactionAnim a = mk(1600, kLinear);
			a.x = {{0, -50}, {0.12, -52}, {0.24, -48}, {0.36, -52}, {0.48, -48},
				{0.6, -52}, {0.72, -48}, {0.84, -52}, {1, -50}
			};
			a.y = {{0, -50}, {0.12, -54}, {0.24, -58}, {0.36, -63}, {0.48, -68},
				{0.6, -74}, {0.72, -82}, {0.84, -95}, {1, -150}
			};
			a.s = {{0, 0.3}, {0.12, 1.3}, {0.6, 1.3}, {0.72, 1.25}, {0.84, 1.2}, {1, 1.1}};
			a.o = {{0, 0}, {0.12, 1}, {1, 0}};
			t.insert("shiver", a);
		}
		{
			ReactionAnim a = mk(1600, kEaseIn);
			a.y = {{0, -50}, {0.2, -55}, {0.45, -65}, {1, -160}};
			a.s = {{0, 0.3}, {0.2, 1.4}, {0.45, 1.2}, {1, 1}};
			a.r = {{0, 0}, {0.2, 12}, {0.45, 28}, {1, 560}};
			a.o = {{0, 0}, {0.2, 1}, {1, 0}};
			t.insert("tilt", a);
		}
		{
			// 🔫: a double recoil to the RIGHT (the glyph points left),
			// plus a slight kick-up of the muzzle. The lateral offset
			// is given in pixels in the web client (14/4/10 px); divided by
			// kPxPerPercent this yields the CSS percentages of this channel.
			ReactionAnim a = mk(1500, kEaseOut);
			a.x = {{0, -50}, {0.12, -50}, {0.2, -41.25}, {0.34, -47.5},
				{0.42, -43.75}, {0.56, -50}, {1, -50}
			};
			a.y = {{0, -50}, {0.12, -50}, {0.2, -52}, {0.34, -52}, {0.42, -54},
				{0.56, -58}, {1, -150}
			};
			a.s = {{0, 0.4}, {0.12, 1.35}, {0.2, 1.3}, {0.34, 1.25}, {0.42, 1.28},
				{0.56, 1.2}, {1, 1}
			};
			a.r = {{0.12, 0}, {0.2, 9}, {0.34, 2}, {0.42, 6}, {0.56, 0}, {1, 0}};
			a.o = {{0, 0}, {0.12, 1}, {1, 0}};
			t.insert("recoil", a);
		}
		return t;
	}
	();
	return table;
}

const ReactionAnim *animFor(const QString &name)
{
	const QHash<QString, ReactionAnim> &t = animTable();
	auto it = t.constFind(name);
	if (it == t.constEnd())
		it = t.constFind(QStringLiteral("pop"));
	return &it.value();
}

// ── Effect catalogue ──
// Particle specification per reaction: either a preset or an explicit
// specification. a0..a1 = angle range (degrees, 0 = right, -90 = up),
// dist = throwing distance, g = additional fall at the end, life = lifetime ms.
// Without characters, coloured dots are thrown.
struct FxSpec {
	QString preset;        // "sparkle" | "shock" | "confetti" | "boom" |
	// "gunshot" | ""
	QStringList chars;     // leer ⇒ farbige Punkte
	int count = 0, size = 14, a0 = 0, a1 = 360, dist = 54, g = 0, life = 1000;
	bool rot = false;
	QString color;
};
struct FxDef {
	QString anim;
	FxSpec p;
};

FxSpec preset(const char *name)
{
	FxSpec s;
	s.preset = QString::fromLatin1(name);
	return s;
}

FxSpec glyphs(const QStringList &chars, int count, int size, int a0, int a1,
			  int dist, int g, int life, bool rot = false, const char *color = "")
{
	FxSpec s;
	s.chars = chars;
	s.count = count;
	s.size = size;
	s.a0 = a0;
	s.a1 = a1;
	s.dist = dist;
	s.g = g;
	s.life = life;
	s.rot = rot;
	s.color = QString::fromLatin1(color);
	return s;
}

FxSpec dots(int count, int size, int a0, int a1,
			int dist, int g, int life, bool rot = false, const char *color = "")
{
	FxSpec s;
	s.count = count;
	s.size = size;
	s.a0 = a0;
	s.a1 = a1;
	s.dist = dist;
	s.g = g;
	s.life = life;
	s.rot = rot;
	s.color = QString::fromLatin1(color);
	return s;
}

// The 90 reactions with their choreography and their particles (order
// as in the reaction picker; the values identical to the QML and the web client).
const QHash<QString, FxDef> &fxTable()
{
	static const QHash<QString, FxDef> table = []() {
		const struct {
			const char *emoji;
			const char *anim;
			FxSpec p;
		} entries[] = {
			// ── Seite 1 (😀 Emotions) ──
			{ "😂", "shake", glyphs({"💧"}, 7, 13, -30, 210, 55, 36, 850) },
			{ "🤣", "shake", glyphs({"💧"}, 7, 13, -30, 210, 55, 36, 850) },
			{ "😅", "shake", glyphs({"💧"}, 5, 12, -40, 220, 48, 40, 800) },
			{ "😭", "shake", glyphs({"💧"}, 11, 14, -20, 200, 60, 70, 1000) },
			{ "🥺", "heartbeat", glyphs({"✨", "💖"}, 8, 13, 0, 360, 56, 0, 850) },
			{ "😢", "wobble", glyphs({"💧"}, 6, 13, -30, 210, 50, 55, 900) },
			{ "😏", "pop", preset("sparkle") },
			{ "🙄", "tilt", glyphs({"✦"}, 5, 11, 0, 360, 44, 0, 650) },
			{ "😳", "zoomout", preset("sparkle") },
			{ "🤪", "wobble", glyphs({"✦", "✧"}, 8, 12, 0, 360, 58, 0, 800, true) },
			{ "😇", "shine", glyphs({"✨"}, 8, 13, -160, -20, 58, -26, 900, false, "#E3C800") },
			{ "😍", "heartbeat", glyphs({"❤️", "💖"}, 8, 16, -160, -20, 64, -30, 1100) },
			{ "🥰", "heartbeat", glyphs({"💕", "💖"}, 9, 15, -170, -10, 62, -28, 1050) },
			{ "😘", "heartbeat", glyphs({"💋", "❤️"}, 7, 15, -150, -30, 60, -32, 1000) },
			{ "😬", "shiver", glyphs({"💦"}, 4, 11, -120, -60, 40, 44, 700) },
			{ "😴", "drop", glyphs({"💤"}, 5, 14, -120, -60, 52, -40, 1100) },
			{ "🤔", "wobble", glyphs({"✦"}, 5, 11, 0, 360, 42, 0, 650) },
			{ "👀", "zoomout", preset("sparkle") },
			{ "😮", "zoomout", glyphs({"✦"}, 6, 12, 0, 360, 50, 0, 700) },
			{ "😱", "shake", glyphs({"💦"}, 6, 12, -120, -60, 48, 50, 780) },
			{ "🤯", "tilt", preset("shock") },
			{ "😡", "shiver", glyphs({"💢", "🔥"}, 7, 13, 0, 360, 54, 0, 800) },
			{ "😤", "flex", glyphs({"💨"}, 6, 14, -190, 10, 52, 0, 750) },
			{ "🤢", "wobble", dots(8, 7, 0, 360, 50, 0, 750, false, "#7ee37e") },
			{ "🥴", "wobble", glyphs({"🌀", "✦"}, 6, 12, 0, 360, 52, 0, 850, true) },
			{ "🙃", "flip", preset("sparkle") },
			{ "🫣", "pop", glyphs({"✦"}, 5, 11, 0, 360, 44, 0, 650) },
			{ "😐", "pop", glyphs({"✦"}, 3, 10, 0, 360, 36, 0, 600) },
			{ "🥱", "wobble", glyphs({"💤"}, 5, 14, -130, -50, 52, -42, 1100) },
			{ "🙈", "shake", glyphs({"✦"}, 6, 11, 0, 360, 48, 0, 700) },
			// ── Seite 2 (👏 Mood & gestures) ──
			{ "😎", "pop", preset("sparkle") },
			{ "🤩", "shine", glyphs({"✨"}, 8, 13, 0, 360, 60, 0, 800, true) },
			{ "🤡", "wobble", preset("confetti") },
			{ "😈", "tilt", glyphs({"🔥", "✦"}, 8, 13, 0, 360, 58, 0, 850, true) },
			{ "🫠", "wobble", glyphs({"💧"}, 6, 12, 40, 140, 44, 70, 950) },
			{ "🥶", "shiver", glyphs({"❄️", "🧊"}, 8, 13, 0, 360, 56, 0, 900, true) },
			{ "🥵", "fire", glyphs({"🔥", "💦"}, 8, 13, -160, -20, 60, -20, 900) },
			{ "🎉", "pop", preset("confetti") },
			{ "🥳", "pop", preset("confetti") },
			{ "🍿", "beat", glyphs({"🍿"}, 9, 13, -160, -20, 60, 70, 1000, true) },
			{ "👏", "beat", glyphs({"✦", "✧"}, 9, 13, 0, 360, 60, 0, 750, false, "#E3C800") },
			{ "🙌", "beat", glyphs({"✦", "✧"}, 9, 13, 0, 360, 62, 0, 780, false, "#E3C800") },
			{ "💪", "flex", glyphs({"✦"}, 6, 13, 0, 360, 50, 0, 700, false, "#E3C800") },
			{ "👍", "beat", preset("sparkle") },
			{ "👎", "drop", dots(7, 6, 20, 160, 48, 60, 800, false, "#9aa0a6") },
			{ "🤝", "pop", preset("sparkle") },
			{ "👊", "flex", preset("shock") },
			{ "🙏", "shine", glyphs({"✨"}, 9, 13, -160, -20, 60, -24, 950, false, "#E3C800") },
			{ "🤞", "beat", glyphs({"🍀", "✨"}, 8, 13, 0, 360, 58, 0, 850, true) },
			{ "🫵", "zoomout", preset("sparkle") },
			{ "🫡", "pop", preset("sparkle") },
			{ "🤫", "pop", glyphs({"✦"}, 4, 10, 0, 360, 38, 0, 600) },
			{ "🤦", "drop", glyphs({"💧"}, 4, 12, -120, -60, 42, 46, 700) },
			{ "🚬", "wobble", glyphs({"💨"}, 7, 14, -130, -50, 64, -46, 1400, true) },
			{ "⏳", "flip", glyphs({"✦"}, 6, 11, 0, 360, 48, 0, 700) },
			{ "🍺", "wobble", glyphs({"🫧"}, 9, 12, -140, -40, 58, -50, 1100) },
			{ "☕", "pop", glyphs({"💨"}, 5, 13, -120, -60, 50, -40, 1000) },
			{ "💣", "drop", preset("boom") },
			{ "🚀", "launch", glyphs({"🔥", "✨"}, 10, 13, 60, 120, 80, 60, 900) },
			{ "⚡", "zoomout", glyphs({"⚡", "✦"}, 8, 14, 0, 360, 66, 0, 750, true) },
			// ── Seite 3 (♠️ Poker & luck) ──
			{ "💰", "pop", glyphs({"🪙", "💵", "✦"}, 12, 16, -170, -10, 72, 90, 1200, true) },
			{ "🤑", "pop", glyphs({"🪙", "💵"}, 10, 16, -170, -10, 70, 90, 1100, true) },
			{ "💵", "drop", glyphs({"💵", "🪙"}, 10, 15, -170, -10, 70, 85, 1150, true) },
			{ "💎", "shine", glyphs({"✨", "✦"}, 9, 13, 0, 360, 64, 0, 850, true) },
			{ "🎰", "spin", glyphs({"✨", "🪙"}, 9, 14, 0, 360, 66, 0, 950, true) },
			{ "🍀", "spin", glyphs({"✨", "🍀"}, 8, 13, 0, 360, 62, 0, 950, true, "#7ee37e") },
			{ "🃏", "flip", preset("sparkle") },
			{ "♠️", "flip", glyphs({"♠️", "♥️", "♦️", "♣️"}, 8, 14, 0, 360, 62, 0, 900, true) },
			{ "🎲", "spin", glyphs({"✦", "✧"}, 8, 12, 0, 360, 58, 0, 800, true) },
			{ "🎯", "zoomout", preset("sparkle") },
			{ "🏆", "shine", glyphs({"⭐", "✨"}, 10, 14, 0, 360, 68, 0, 1000, true, "#E3C800") },
			{ "🥇", "shine", glyphs({"✨"}, 8, 13, 0, 360, 60, 0, 900, false, "#E3C800") },
			{ "💸", "launch", glyphs({"💵", "🪙"}, 10, 14, -150, -30, 75, -40, 1100, true) },
			{ "🪤", "drop", preset("shock") },
			{ "👑", "shine", glyphs({"✨", "⭐"}, 10, 14, 0, 360, 70, 0, 1000, true, "#E3C800") },
			{ "🔥", "fire", glyphs({"🔥", "✦"}, 9, 14, -150, -30, 70, -24, 1000, true) },
			{ "💀", "shiver", dots(8, 6, 0, 360, 52, 0, 800, false, "#9aa0a6") },
			{ "🦈", "pop", glyphs({"💦", "🌊"}, 8, 14, -170, -10, 62, 40, 900) },
			{ "🐟", "wobble", glyphs({"🫧"}, 9, 12, -140, -40, 58, -52, 1150) },
			{ "🐔", "shake", glyphs({"🪶"}, 8, 14, -30, 210, 56, 60, 1200, true) },
			{ "🫏", "wobble", glyphs({"✦"}, 6, 11, 0, 360, 48, 0, 750) },
			{ "🎩", "flip", glyphs({"✨"}, 7, 12, 0, 360, 54, 0, 800) },
			{ "🧊", "shiver", glyphs({"❄️"}, 7, 12, 0, 360, 52, 0, 850) },
			{ "🌪️", "tilt", glyphs({"🍃", "💨"}, 10, 13, 0, 360, 74, 0, 950, true) },
			{ "🔫", "recoil", preset("gunshot") },
			{ "📈", "launch", dots(8, 6, -120, -60, 62, -30, 850, false, "#7ee37e") },
			{ "📉", "drop", dots(8, 6, 60, 120, 58, 70, 850, false, "#e05252") },
			{ "🔮", "shine", glyphs({"✨", "✦"}, 8, 13, 0, 360, 60, 0, 900, true) },
			{ "💯", "zoomout", glyphs({"✦", "💯"}, 6, 13, 0, 360, 56, 0, 800) },
			{ "⭐", "shine", glyphs({"⭐", "✨"}, 9, 13, 0, 360, 62, 0, 900, true) },
		};
		QHash<QString, FxDef> t;
		for (const auto &e : entries)
			t.insert(QString::fromUtf8(e.emoji), FxDef{QString::fromLatin1(e.anim), e.p});
		return t;
	}
	();
	return table;
}

const FxDef &fxFor(const QString &emoji)
{
	// Unknown emojis (e.g. from a newer client version) get the
	// default effect, so that the reaction becomes visible anyway.
	static const FxDef fallback{QStringLiteral("pop"), preset("sparkle")};
	const QHash<QString, FxDef> &t = fxTable();
	auto it = t.constFind(emoji);
	return it == t.constEnd() ? fallback : it.value();
}

} // namespace

ReactionFxOverlay::ReactionFxOverlay(QWidget *parent)
	: QWidget(parent)
{
	setAttribute(Qt::WA_TransparentForMouseEvents);
	setAttribute(Qt::WA_NoSystemBackground);
	setAttribute(Qt::WA_TranslucentBackground);
	hide();

	myClock.start();
	myTicker.setInterval(16);
	QObject::connect(&myTicker, &QTimer::timeout, this, [this]() {
		const qint64 now = myClock.elapsed();
		for (int i = myBursts.size() - 1; i >= 0; --i)
			if (now - myBursts.at(i).start > myBursts.at(i).life)
				myBursts.removeAt(i);
		if (myBursts.isEmpty()) {
			myTicker.stop();
			hide();
			return;
		}
		update();
	});
}

void ReactionFxOverlay::play(const QString &emoji, QPoint anchor)
{
	Burst b;
	b.emoji = emoji;
	b.anchor = anchor;
	b.start = myClock.elapsed();
	buildBurst(b);
	myBursts.append(b);

	setGeometry(parentWidget()->rect());
	show();
	raise();
	if (!myTicker.isActive())
		myTicker.start();
}

void ReactionFxOverlay::buildBurst(Burst &burst)
{
	const FxDef &def = fxFor(burst.emoji);
	burst.anim = animFor(def.anim);
	// Pre-render at 2× the logical base size – it stays sharp at the peak scale.
	// emojiPixmap guarantees the target size for bitmap emoji fonts as well
	// (Qt does not scale their glyphs).
	burst.emojiPm = EmojiPicker::emojiPixmap(burst.emoji, int(kBaseSize * 2));

	// Throw the particles of an explicit specification into the angle range
	// a0..a1.
	auto spawn = [&burst](const FxSpec &spec, int delay) {
		for (int i = 0; i < spec.count; ++i) {
			Particle p;
			p.kind = spec.chars.isEmpty() ? 1 : 0;
			if (p.kind == 0)
				p.pm = EmojiPicker::emojiPixmap(
						   spec.chars.at(QRandomGenerator::global()->bounded(spec.chars.size())),
						   spec.size * 2);
			p.color = spec.color.isEmpty() ? QColor("#E3C800") : QColor(spec.color);
			p.size = spec.size;
			const double ang = rnd(spec.a0, spec.a1) * M_PI / 180.0;
			const double d = spec.dist * rnd(0.55, 1.15);
			p.dx = std::cos(ang) * d;
			p.dy = std::sin(ang) * d;
			p.g = spec.g;
			p.rot = spec.rot ? rnd(-360, 360) : 0;
			p.life = spec.life;
			p.delay = delay;
			burst.particles.append(p);
		}
	};

	FxSpec spec = def.p;
	int delay = 0;

	if (spec.preset == QLatin1String("sparkle")) {
		spec = glyphs({"✦", "✧"}, 7, 12, 0, 360, 54, 0, 700, false, "#E3C800");
	} else if (spec.preset == QLatin1String("shock")) {
		Ring r;
		r.dur = 800;
		r.color = QColor("#FFE066");
		r.width = 3;
		r.to = 4;
		burst.rings.append(r);
		spec = glyphs({"💥", "✦"}, 8, 15, 0, 360, 70, 0, 800);
	} else if (spec.preset == QLatin1String("boom")) {
		// 💣: the bomb falls first ("drop"), it only explodes on impact
		// – hence the offset of 420 ms; the second ring follows 120 ms
		// later. Both times are aligned to the choreography and
		// are stretched with it (kDurationScale).
		Ring r;
		r.dur = 900;
		r.color = QColor("#ff9040");
		r.width = 4;
		r.to = 6.5;
		r.delay = qRound(420 * kDurationScale);
		burst.rings.append(r);
		r.delay = qRound(540 * kDurationScale);
		burst.rings.append(r);
		spec = glyphs({"💥", "🔥", "✦"}, 14, 18, 0, 360, 95, 0, 950, true);
		delay = qRound(420 * kDurationScale);
	} else if (spec.preset == QLatin1String("confetti")) {
		static const QColor cols[] = {
			QColor("#9b59b6"), QColor("#e84393"), QColor("#27ae60"),
			QColor("#c0392b"), QColor("#7ec8e3"), QColor("#e67e22"), QColor("#ffffff")
		};
		for (int i = 0; i < 24; ++i) {
			Particle p;
			p.kind = 2;
			p.color = cols[QRandomGenerator::global()->bounded(7)];
			p.w = rnd(5, 9);
			p.h = rnd(7, 11);
			const double ang = rnd(-170, -10) * M_PI / 180.0;
			const double d = rnd(70, 130);
			p.dx = std::cos(ang) * d;
			p.dy = std::sin(ang) * d;
			p.g = 130;
			p.rot = rnd(-360, 360);
			p.life = int(rnd(1300, 1700));
			burst.particles.append(p);
		}
		spec = FxSpec();   // no further particles
	} else if (spec.preset == QLatin1String("gunshot")) {
		// 🔫: muzzle flash, a golden tracer projectile to the LEFT (the
		// glyph points left in all emoji fonts), its spark trail and
		// the cartridge ejected to the upper right. The emoji itself plays
		// the choreography "recoil" along with it.
		Particle flash;                  // stays at the muzzle
		flash.pm = EmojiPicker::emojiPixmap(QStringLiteral("💥"), 40);
		flash.size = 20;
		flash.ox = -20;
		flash.dx = -30;
		flash.pulse = true;
		flash.life = 240;
		burst.particles.append(flash);
		Particle bullet;                 // the projectile
		bullet.kind = 1;
		bullet.color = QColor("#E3C800");
		bullet.size = 7;
		bullet.ox = -24;
		bullet.dx = -195;
		bullet.dy = 10;
		bullet.life = 420;
		burst.particles.append(bullet);
		spawn(glyphs({"✦"}, 5, 9, 172, 188, 120, 0, 420), 0);
		spawn(glyphs({"✨"}, 2, 10, -110, -60, 40, 55, 650), 0);
		spec = FxSpec();   // no further particles
	}

	spawn(spec, delay);

	// Total duration = the longest sub-animation (emoji, particles, rings).
	burst.life = burst.anim->dur;
	for (const Particle &p : burst.particles)
		burst.life = qMax(burst.life, p.delay + p.life);
	for (const Ring &r : burst.rings)
		burst.life = qMax(burst.life, r.delay + r.dur);
	burst.life += 150;
}

void ReactionFxOverlay::paintEvent(QPaintEvent * /*event*/)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::TextAntialiasing);

	const qint64 now = myClock.elapsed();
	for (const Burst &b : myBursts) {
		const qint64 t = now - b.start;
		if (t >= 0 && t <= b.life)
			drawBurst(painter, b, t);
	}
}

void ReactionFxOverlay::drawBurst(QPainter &painter, const Burst &burst, qint64 t) const
{
	// ── Druckwellen-Ringe ──
	for (const Ring &r : burst.rings) {
		const qint64 rt = t - r.delay;
		if (rt < 0 || rt > r.dur)
			continue;
		const qreal frac = qreal(rt) / r.dur;
		const qreal scale = 0.3 + (r.to - 0.3) * bezier(kEaseOut, frac);
		painter.save();
		painter.setOpacity(0.9 * (1.0 - frac));
		QPen pen(r.color);
		pen.setWidthF(r.width);
		painter.setPen(pen);
		painter.setBrush(Qt::NoBrush);
		painter.drawEllipse(QPointF(burst.anchor), 15.0 * scale, 15.0 * scale);
		painter.restore();
	}

	// ── Partikel-Burst ──
	for (const Particle &p : burst.particles) {
		const qint64 pt = t - p.delay;
		if (pt < 0 || pt > p.life)
			continue;
		const qreal frac = qreal(pt) / p.life;
		// Path: 65 % of the time from the start to the target point (OutCubic), then a fall
		// by g (InQuad).
		qreal px, py;
		if (frac <= 0.65) {
			const qreal e = QEasingCurve(QEasingCurve::OutCubic).valueForProgress(frac / 0.65);
			px = p.ox + (p.dx - p.ox) * e;
			py = p.oy + (p.dy - p.oy) * e;
		} else {
			const qreal e = QEasingCurve(QEasingCurve::InQuad).valueForProgress((frac - 0.65) / 0.35);
			px = p.dx;
			py = p.dy + p.g * e;
		}
		qreal opacity = frac <= 0.65 ? 1.0 : 1.0 - (frac - 0.65) / 0.35;
		// Muzzle flash: instead of the full opacity the character flashes up
		// (size 0.4 → 1.25 → 0.7) and fades again.
		qreal pulse = 1.0;
		if (p.pulse) {
			if (frac <= 0.35) {
				pulse = 0.4 + (1.25 - 0.4) * (frac / 0.35);
				opacity = frac / 0.35;
			} else {
				const qreal f = (frac - 0.35) / 0.65;
				pulse = 1.25 + (0.7 - 1.25) * f;
				opacity = 1.0 - f;
			}
		}
		const qreal sz = p.size * pulse;

		painter.save();
		painter.translate(burst.anchor.x() + px, burst.anchor.y() + py);
		painter.rotate(p.rot * frac);
		painter.setOpacity(opacity);
		if (p.kind == 2) {              // Konfetti-Rechteck
			painter.setPen(Qt::NoPen);
			painter.setBrush(p.color);
			painter.drawRoundedRect(QRectF(-p.w / 2, -p.h / 2, p.w, p.h), 1, 1);
		} else if (p.kind == 1) {       // farbiger Punkt
			painter.setPen(Qt::NoPen);
			painter.setBrush(p.color);
			painter.drawEllipse(QPointF(0, 0), sz / 2, sz / 2);
		} else {                        // Emoji-/Zeichen-Partikel
			painter.setRenderHint(QPainter::SmoothPixmapTransform);
			painter.drawPixmap(QRectF(-sz / 2, -sz / 2, sz, sz),
							   p.pm, p.pm.rect());
		}
		painter.restore();
	}

	// ── Large emoji: keyframes of the choreography ──
	const ReactionAnim &a = *burst.anim;
	if (t > a.dur || burst.emojiPm.isNull())
		return;
	const qreal f = qBound<qreal>(0.0, qreal(t) / a.dur, 1.0);
	const qreal xOff = (sample(a, a.x, f, -50) + 50) * kPxPerPercent;
	const qreal yOff = (sample(a, a.y, f, -50) + 50) * kPxPerPercent;
	const qreal scale = sample(a, a.s, f, 1);
	const qreal opacity = sample(a, a.o, f, 1);
	if (opacity <= 0.0 || scale <= 0.0)
		return;

	painter.save();
	painter.translate(burst.anchor.x() + xOff, burst.anchor.y() + yOff);
	painter.scale(scale, scale);
	painter.rotate(sample(a, a.r, f, 0));
	// Card turner ("flip"): rotation around the Y axis as a horizontal squeeze
	// (the glyph mirrors itself in the process as in the web client).
	if (!a.ry.isEmpty())
		painter.scale(std::cos(sample(a, a.ry, f, 0) * M_PI / 180.0), 1.0);
	painter.setOpacity(opacity);
	painter.setRenderHint(QPainter::SmoothPixmapTransform);
	const QRectF target(-kBaseSize / 2, -kBaseSize / 2, kBaseSize, kBaseSize);
	painter.drawPixmap(target, burst.emojiPm, burst.emojiPm.rect());
	// Flash of the "shine" choreography: brighten additively.
	const qreal bright = sample(a, a.b, f, 0);
	if (bright > 0.0) {
		painter.setCompositionMode(QPainter::CompositionMode_Plus);
		painter.setOpacity(opacity * bright);
		painter.drawPixmap(target, burst.emojiPm, burst.emojiPm.rect());
	}
	painter.restore();
}
