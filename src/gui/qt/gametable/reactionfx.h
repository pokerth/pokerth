/*****************************************************************************
 * PokerTH - The open source texas holdem engine                             *
 *                                                                           *
 * Overlay für die Emoji-Reaktions-Choreografie am Spieltisch – 1:1-Port     *
 * der QML-Komponente GameReactionFx: ein großes Emoji poppt am Sitz auf,    *
 * steigt mit Pendeln/Wackeln/Drehung auf und verblasst; dazu ein            *
 * Partikel-Burst (Funken/Konfetti/Tropfen/Münzen … je nach Emoji).          *
 *****************************************************************************/
#ifndef REACTIONFX_H
#define REACTIONFX_H

#include <QtWidgets>

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
		QPixmap pm;            // gerendertes Partikelzeichen; leer = Konfetti
		QColor color;
		qreal w = 0, h = 0;    // Konfetti-Maße
		int size = 14;         // Zielgröße (px)
		qreal dx = 0, dy = 0;  // Ziel-Versatz
		qreal g = 0;           // zusätzlicher Fall am Ende
		qreal rot = 0;         // End-Rotation
		int life = 1000;       // Lebensdauer ms
	};
	struct Burst {
		QString emoji;
		QPixmap emojiPm;       // großes Emoji, vorgerendert (2× für Schärfe)
		int anim = 0;          // 0 = pop, 1 = shake, 2 = spin
		bool ring = false;     // Druckwellen-Ring (🤯)
		QPoint anchor;
		qint64 start = 0;
		QVector<Particle> particles;
	};

	void buildBurst(Burst &burst);
	void drawBurst(QPainter &painter, const Burst &burst, qint64 t) const;

	QVector<Burst> myBursts;
	QElapsedTimer myClock;
	QTimer myTicker;
};

#endif // REACTIONFX_H
