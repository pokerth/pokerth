/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 *                                                                           *
 * Overlay für die Emoji-Reaktions-Choreografie am Spieltisch – 1:1-Port     *
 * der QML-Komponente GameReactionFx: ein großes Emoji erscheint am Sitz,    *
 * spielt eine von 15 Choreografien (Aufstieg, Wackeln, Drehen, Fallen …)   *
 * und verblasst; dazu ein Partikel-Burst (Funken/Konfetti/Tropfen/Münzen …  *
 * je nach Emoji) und – bei 🤯/💣 & Co. – Druckwellen-Ringe.                 *
 *****************************************************************************/
#ifndef REACTIONFX_H
#define REACTIONFX_H

#include <QtWidgets>

// Keyframe-Tabelle einer Choreografie (Definition in reactionfx.cpp).
struct ReactionAnim;

class ReactionFxOverlay : public QWidget
{
public:
	explicit ReactionFxOverlay(QWidget *parent);

	// Choreografie am Ankerpunkt (Parent-Koordinaten, Box-Mitte/Oberkante)
	// abspielen. Mehrere gleichzeitige Reaktionen sind möglich.
	void play(const QString &emoji, QPoint anchor);

protected:
	void paintEvent(QPaintEvent *event) override;

private:
	struct Particle {
		QPixmap pm;            // gerendertes Partikelzeichen (nur kind 0)
		QColor color;
		int kind = 0;          // 0 = Zeichen, 1 = farbiger Punkt, 2 = Konfetti
		qreal w = 0, h = 0;    // Konfetti-Maße
		qreal size = 14;       // Zielgröße (px)
		qreal dx = 0, dy = 0;  // Ziel-Versatz
		qreal g = 0;           // zusätzlicher Fall am Ende
		qreal rot = 0;         // End-Rotation
		int life = 1000;       // Lebensdauer ms
		int delay = 0;         // Startverzögerung ms (Preset "boom")
	};
	struct Ring {
		int delay = 0;         // Startverzögerung ms
		int dur = 800;
		QColor color;
		qreal width = 3;
		qreal to = 4;          // End-Skalierung (Startgröße 30 px, Skalierung 0.3)
	};
	struct Burst {
		QString emoji;
		QPixmap emojiPm;       // großes Emoji, vorgerendert (2× für Schärfe)
		const ReactionAnim *anim = nullptr;
		QPoint anchor;
		qint64 start = 0;
		int life = 2000;       // Gesamtdauer inkl. Partikel und Ringe
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
