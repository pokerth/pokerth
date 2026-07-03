/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 *                                                                           *
 * Overlay für die Emoji-Reaktions-Choreografie am Spieltisch – 1:1-Port     *
 * der QML-Komponente GameReactionFx (Keyframes, Partikel-Katalog und        *
 * Timings identisch).                                                       *
 *****************************************************************************/
#include "reactionfx.h"
#include "emojipicker.h"

#include <QRandomGenerator>
#include <cmath>

namespace
{

const int kBurstLifeMs = 2400;

qreal eased(QEasingCurve::Type type, qreal progress)
{
	return QEasingCurve(type).valueForProgress(qBound<qreal>(0.0, progress, 1.0));
}

// Stückweise Interpolation: value zwischen from..to im Zeitfenster t0..t1.
qreal seg(qint64 t, qint64 t0, qint64 t1, qreal from, qreal to, QEasingCurve::Type type)
{
	if (t <= t0) return from;
	if (t >= t1) return to;
	return from + (to - from) * eased(type, qreal(t - t0) / qreal(t1 - t0));
}

double rnd(double from, double to)
{
	return from + QRandomGenerator::global()->generateDouble() * (to - from);
}

// Partikel-Spezifikation (Port des fxCatalog aus GameReactionFx.qml).
struct FxDef {
	int anim = 0;                  // 0 = pop, 1 = shake, 2 = spin
	QStringList chars;             // Partikelzeichen
	int count = 7;
	int size = 12;
	int a0 = 0, a1 = 360;          // Winkelbereich (Grad, 0 = rechts, -90 = oben)
	int dist = 54;                 // Wurfweite
	int g = 0;                     // zusätzlicher Fall am Ende
	int life = 700;                // Lebensdauer ms
	bool rot = false;
	bool confetti = false;
	bool ring = false;
};

FxDef fxFor(const QString &e)
{
	if (e == "🎉" || e == "🥳" || e == "🎊") { FxDef d; d.anim = 0; d.confetti = true; return d; }
	if (e == "🔥") return {1, {"🔥", "✦"}, 9, 14, -150, -30, 70, -24, 1000, true, false, false};
	if (e == "💰") return {0, {"🪙", "💵", "✦"}, 12, 16, -170, -10, 72, 90, 1200, true, false, false};
	if (e == "🤑") return {0, {"🪙", "💵"}, 10, 16, -170, -10, 70, 90, 1100, true, false, false};
	if (e == "💎") return {0, {"✨", "✦"}, 9, 13, 0, 360, 64, 0, 850, true, false, false};
	if (e == "🤩") return {0, {"✨"}, 8, 13, 0, 360, 60, 0, 800, true, false, false};
	if (e == "😂" || e == "🤣") return {1, {"💧"}, 7, 13, -30, 210, 55, 36, 850, false, false, false};
	if (e == "😱") return {1, {"💦"}, 6, 12, -120, -60, 48, 50, 780, false, false, false};
	if (e == "🤯") return {0, {"💥", "✦"}, 8, 15, 0, 360, 70, 0, 800, false, false, true};
	if (e == "🍀") return {2, {"✨", "🍀"}, 8, 13, 0, 360, 62, 0, 950, true, false, false};
	if (e == "🎰") return {2, {"✨", "🪙"}, 9, 14, 0, 360, 66, 0, 950, true, false, false};
	if (e == "👑") return {0, {"✨", "⭐"}, 10, 14, 0, 360, 70, 0, 1000, true, false, false};
	if (e == "😍") return {0, {"❤️", "💖"}, 8, 16, -160, -20, 64, -30, 1100, false, false, false};
	// Default: Pop + Goldfunken ("sparkle")
	return {0, {"✦", "✧"}, 7, 12, 0, 360, 54, 0, 700, false, false, false};
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
			if (now - myBursts.at(i).start > kBurstLifeMs)
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
	const FxDef def = fxFor(burst.emoji);
	burst.anim = def.anim;
	burst.ring = def.ring;
	// 2× der logischen Basisgröße (34 px) vorrendern – bleibt beim
	// Peak-Scale 1.45 scharf. emojiPixmap garantiert die Zielgröße auch
	// für Bitmap-Emoji-Fonts (Qt skaliert deren Glyphen nicht).
	burst.emojiPm = EmojiPicker::emojiPixmap(burst.emoji, 68);

	if (def.confetti) {
		static const QColor cols[] = {
			QColor("#9b59b6"), QColor("#e84393"), QColor("#27ae60"),
			QColor("#c0392b"), QColor("#7ec8e3"), QColor("#e67e22"), QColor("#ffffff")
		};
		for (int i = 0; i < 24; ++i) {
			Particle p;
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
		return;
	}

	for (int i = 0; i < def.count; ++i) {
		Particle p;
		p.pm = EmojiPicker::emojiPixmap(
			def.chars.at(QRandomGenerator::global()->bounded(def.chars.size())),
			def.size * 2);
		p.color = QColor("#E3C800");
		p.size = def.size;
		const double ang = rnd(def.a0, def.a1) * M_PI / 180.0;
		const double d = def.dist * rnd(0.55, 1.15);
		p.dx = std::cos(ang) * d;
		p.dy = std::sin(ang) * d;
		p.g = def.g;
		p.rot = def.rot ? rnd(-360, 360) : 0;
		p.life = def.life;
		burst.particles.append(p);
	}
}

void ReactionFxOverlay::paintEvent(QPaintEvent * /*event*/)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing);
	painter.setRenderHint(QPainter::TextAntialiasing);

	const qint64 now = myClock.elapsed();
	for (const Burst &b : myBursts) {
		const qint64 t = now - b.start;
		if (t >= 0 && t <= kBurstLifeMs)
			drawBurst(painter, b, t);
	}
}

void ReactionFxOverlay::drawBurst(QPainter &painter, const Burst &burst, qint64 t) const
{
	// ── Druckwellen-Ring (nur 🤯) ──
	if (burst.ring && t < 800) {
		const qreal prog = eased(QEasingCurve::OutQuad, t / 800.0);
		const qreal scale = 0.3 + (4.0 - 0.3) * prog;
		const qreal radius = 15.0 * scale;
		painter.save();
		painter.setOpacity(0.9 * (1.0 - t / 800.0));
		QPen pen(QColor("#FFE066"));
		pen.setWidthF(3.0);
		painter.setPen(pen);
		painter.setBrush(Qt::NoBrush);
		painter.drawEllipse(QPointF(burst.anchor), radius, radius);
		painter.restore();
	}

	// ── Partikel-Burst ──
	for (const Particle &p : burst.particles) {
		if (t > p.life)
			continue;
		const qreal frac = qreal(t) / p.life;
		// Bahn: 65 % der Zeit zum Ziel (OutCubic), danach Fall um g (InQuad).
		qreal px, py;
		if (frac <= 0.65) {
			const qreal e = eased(QEasingCurve::OutCubic, frac / 0.65);
			px = p.dx * e;
			py = p.dy * e;
		} else {
			const qreal e = eased(QEasingCurve::InQuad, (frac - 0.65) / 0.35);
			px = p.dx;
			py = p.dy + p.g * e;
		}
		const qreal opacity = frac <= 0.65 ? 1.0 : 1.0 - (frac - 0.65) / 0.35;

		painter.save();
		painter.translate(burst.anchor.x() + px, burst.anchor.y() + py);
		painter.rotate(p.rot * frac);
		painter.setOpacity(opacity);
		if (p.pm.isNull()) {
			painter.setPen(Qt::NoPen);
			painter.setBrush(p.color);
			painter.drawRoundedRect(QRectF(-p.w / 2, -p.h / 2, p.w, p.h), 1, 1);
		} else {
			const qreal s = p.size;
			painter.setRenderHint(QPainter::SmoothPixmapTransform);
			painter.drawPixmap(QRectF(-s / 2, -s / 2, s, s), p.pm, p.pm.rect());
		}
		painter.restore();
	}

	// ── Großes Emoji: Pop-in, Aufstieg, Wobble/Spin, Fade-out ──
	// Keyframes identisch zur QML-Komponente (Zeiten in ms).
	qreal yOff;
	if (t <= 330)        yOff = seg(t, 0, 330, 0, -22, QEasingCurve::OutQuad);
	else if (t <= 1430)  yOff = seg(t, 330, 1430, -22, -110, QEasingCurve::InOutQuad);
	else                 yOff = seg(t, 1430, 2000, -110, -160, QEasingCurve::InQuad);

	qreal scale;
	if (t <= 330)        scale = seg(t, 0, 330, 0.2, 1.45, QEasingCurve::OutBack);
	else if (t <= 780)   scale = seg(t, 330, 780, 1.45, 1.05, QEasingCurve::OutQuad);
	else                 scale = seg(t, 780, 2000, 1.05, 0.9, QEasingCurve::Linear);

	qreal opacity;
	if (t <= 250)        opacity = seg(t, 0, 250, 0.0, 1.0, QEasingCurve::Linear);
	else if (t <= 1400)  opacity = 1.0;
	else                 opacity = seg(t, 1400, 2000, 1.0, 0.0, QEasingCurve::Linear);

	qreal rotation = 0;
	switch (burst.anim) {
	case 1: { // shake: 3 Schleifen ±14°
		const qint64 cycle = t % 220;
		const qreal swing = (t < 660)
			? (cycle < 110 ? seg(cycle, 0, 110, 14, -14, QEasingCurve::InOutSine)
			               : seg(cycle - 110, 0, 110, -14, 14, QEasingCurve::InOutSine))
			: 0;
		rotation = swing;
		break;
	}
	case 2: // spin: volle Drehung
		rotation = seg(t, 0, 1800, 0, 720, QEasingCurve::OutQuad);
		break;
	default: // pop: leichtes Pendeln
		if (t <= 500)        rotation = seg(t, 0, 500, -8, 5, QEasingCurve::InOutSine);
		else if (t <= 1300)  rotation = seg(t, 500, 1300, 5, -3, QEasingCurve::InOutSine);
		else                 rotation = seg(t, 1300, 2000, -3, 8, QEasingCurve::InOutSine);
		break;
	}

	painter.save();
	painter.translate(burst.anchor.x(), burst.anchor.y() + yOff);
	painter.scale(scale, scale);
	painter.rotate(rotation);
	painter.setOpacity(opacity);
	painter.setRenderHint(QPainter::SmoothPixmapTransform);
	// Logische Basisgröße 34 px (wie QML); das Pixmap ist 2× vorgerendert.
	if (!burst.emojiPm.isNull())
		painter.drawPixmap(QRectF(-17, -17, 34, 34), burst.emojiPm, burst.emojiPm.rect());
	painter.restore();
}
