/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 *                                                                           *
 * Overlay for the emoji reaction choreography at the game table – a 1:1     *
 * port of the QML component GameReactionFx: a large emoji appears at the    *
 * seat, plays one of 16 choreographies (rise, wobble, spin, fall …)        *
 * and fades; plus a particle burst (sparks/confetti/drops/coins …          *
 * depending on the emoji) and – for 🤯/💣 & co. – shock wave rings.        *
 *****************************************************************************/
#ifndef REACTIONFX_H
#define REACTIONFX_H

#include <QtWidgets>

// Keyframe table of a choreography (definition in reactionfx.cpp).
struct ReactionAnim;

class ReactionFxOverlay : public QWidget
{
public:
	explicit ReactionFxOverlay(QWidget *parent);

	// Play the choreography at the anchor point (parent coordinates, box
	// centre/top edge). Several simultaneous reactions are possible.
	void play(const QString &emoji, QPoint anchor);

protected:
	void paintEvent(QPaintEvent *event) override;

private:
	struct Particle {
		QPixmap pm;            // rendered particle character (kind 0 only)
		QColor color;
		int kind = 0;          // 0 = character, 1 = coloured dot, 2 = confetti
		qreal w = 0, h = 0;    // confetti dimensions
		qreal size = 14;       // target size (px)
		qreal ox = 0, oy = 0;  // start offset (preset "gunshot")
		qreal dx = 0, dy = 0;  // target offset
		qreal g = 0;           // additional fall at the end
		qreal rot = 0;         // end rotation
		bool pulse = false;    // flash briefly instead of full opacity
		int life = 1000;       // lifetime ms
		int delay = 0;         // start delay ms (preset "boom")
	};
	struct Ring {
		int delay = 0;         // start delay ms
		int dur = 800;
		QColor color;
		qreal width = 3;
		qreal to = 4;          // end scale (start size 30 px, scale 0.3)
	};
	struct Burst {
		QString emoji;
		QPixmap emojiPm;       // large emoji, pre-rendered (2× for sharpness)
		const ReactionAnim *anim = nullptr;
		QPoint anchor;
		qint64 start = 0;
		int life = 2000;       // total duration incl. particles and rings
		QVector<Particle> particles;
		QVector<Ring> rings;
	};

	void buildBurst(Burst &burst);
	void drawBurst(QPainter &painter, const Burst &burst, qint64 t) const;

	QVector<Burst> myBursts;
	QElapsedTimer myClock;
	QTimer myTicker;
};

#endif // REACTIONFX_H
