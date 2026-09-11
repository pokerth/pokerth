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
 *****************************************************************************/

#ifndef STYLEPROVIDER_H
#define STYLEPROVIDER_H

#include <QObject>
#include <QString>
#include <boost/shared_ptr.hpp>

class ConfigFile;

// Liefert dem QML-Client die Asset-URLs des aktuell gewählten Tisch- bzw.
// Kartenstapel-Stils aus <AppDataDir>/gfx/qml/. Ein leerer String bedeutet
// "kein Stil-Asset vorhanden" → die QML-Seite fällt auf das gebündelte
// qrc-Default zurück. Über NOTIFY changed() rebinden alle Bindungen, sobald
// per setTableStyle()/setCardDeckStyle() ein anderer Stil gewählt wird.
class StyleProvider : public QObject
{
	Q_OBJECT

	// Tisch-Stil
	Q_PROPERTY(QString tableStyleName READ tableStyleName NOTIFY changed)
	Q_PROPERTY(QString tableBackground READ tableBackground NOTIFY changed)
	Q_PROPERTY(QString tableBackgroundAlignment READ tableBackgroundAlignment NOTIFY changed)
	Q_PROPERTY(qreal tableBackgroundZoom READ tableBackgroundZoom NOTIFY changed)
	// Eckenradius der Aktions-Buttons, in Einheiten der 168x43-Zeichenfläche
	// der Button-SVGs. Der Client zeichnet Zustands-Rahmen (Vorwahl gold,
	// primäre Aktion) als Rechteck über das SVG – ohne diesen Wert passten
	// dessen Ecken nicht zur Button-Form. Default 9 = gebündelter Stil.
	Q_PROPERTY(qreal actionButtonBorderRadius READ actionButtonBorderRadius NOTIFY changed)
	// Akzentfarbe der Spielerboxen (<PlayerBoxAccent>). Tönt Verlauf und Rahmen
	// der Box. Leer = neutraler, gebündelter Default.
	Q_PROPERTY(QString playerBoxAccent READ playerBoxAccent NOTIFY changed)
	Q_PROPERTY(QString dealerPuck READ dealerPuck NOTIFY changed)
	Q_PROPERTY(QString smallBlindPuck READ smallBlindPuck NOTIFY changed)
	Q_PROPERTY(QString bigBlindPuck READ bigBlindPuck NOTIFY changed)
	// Aktions-Button-Grafiken (nur Optik/Rahmen – der dynamische Text mit
	// Beträgen wird im QML darüber gelegt). Leer = Fallback auf den
	// hartcodierten Gradient-Button in GameActionBar.qml.
	Q_PROPERTY(QString foldButton READ foldButton NOTIFY changed)
	Q_PROPERTY(QString checkCallButton READ checkCallButton NOTIFY changed)
	Q_PROPERTY(QString betRaiseButton READ betRaiseButton NOTIFY changed)
	Q_PROPERTY(QString allInButton READ allInButton NOTIFY changed)
	// Schriftfarbe der Aktions-Buttons: explizit per Theme-XML
	// (<FoldButtonTextColor> … bzw. style-weit <ActionButtonTextColor>) oder
	// sonst automatisch aus der Button-Helligkeit abgeleitet (heller Hintergrund
	// → dunkle Schrift). Leer = kein Theme-Button → QML nimmt seinen Default.
	Q_PROPERTY(QString foldButtonTextColor READ foldButtonTextColor NOTIFY changed)
	Q_PROPERTY(QString checkCallButtonTextColor READ checkCallButtonTextColor NOTIFY changed)
	Q_PROPERTY(QString betRaiseButtonTextColor READ betRaiseButtonTextColor NOTIFY changed)
	Q_PROPERTY(QString allInButtonTextColor READ allInButtonTextColor NOTIFY changed)
	// Farben der Chat- und Log-Box (schwebende Seiten-Panels am Tisch). Bewusst
	// unabhängig vom Hell/Dunkel-Modus der übrigen App: maßgeblich ist allein
	// das Tisch-Theme. Per Tisch-Theme-XML überschreibbar, sonst gelten die
	// gebündelten Defaults (dunkel). Solide Hex-Farben – die Transluzenz
	// (withAlpha) macht der QML-Client.
	Q_PROPERTY(QString chatLogBackground READ chatLogBackground NOTIFY changed)
	Q_PROPERTY(QString chatLogSurface READ chatLogSurface NOTIFY changed)
	Q_PROPERTY(QString chatLogBorder READ chatLogBorder NOTIFY changed)
	Q_PROPERTY(QString chatLogText READ chatLogText NOTIFY changed)
	Q_PROPERTY(QString chatLogTextSecondary READ chatLogTextSecondary NOTIFY changed)
	Q_PROPERTY(QString chatLogTextMuted READ chatLogTextMuted NOTIFY changed)
	// Inhaltsfarben derselben Boxen: Akzent (Panel-Titel, aktiver Tab,
	// Selektion, Erwähnung im Chat, Chancen-Balken), Schrift AUF dem Akzent,
	// die drei Rollen des Spielverlaufs und das Senden-Symbol. Auch sie sind
	// per XML setzbar (<ChatLogAccent> …); fehlt ein Tag, greift der zur
	// Helligkeit von <ChatLogBackground> passende Default – sonst stünde z. B.
	// bei einem hellen Tisch-Theme Weiß/Gold auf hellem Grund.
	Q_PROPERTY(QString chatLogAccent READ chatLogAccent NOTIFY changed)
	Q_PROPERTY(QString chatLogAccentText READ chatLogAccentText NOTIFY changed)
	Q_PROPERTY(QString chatLogWinner READ chatLogWinner NOTIFY changed)
	Q_PROPERTY(QString chatLogWinnerSide READ chatLogWinnerSide NOTIFY changed)
	Q_PROPERTY(QString chatLogBoard READ chatLogBoard NOTIFY changed)
	Q_PROPERTY(QString chatLogSend READ chatLogSend NOTIFY changed)
	// Kartenstapel-Stil (52 Vorderseiten)
	Q_PROPERTY(QString cardDeckName READ cardDeckName NOTIFY changed)
	Q_PROPERTY(QString cardDeckDir READ cardDeckDir NOTIFY changed)
	// Kartenrückseite – eigene Stil-Kategorie (gfx/qml/backside/<name>/)
	Q_PROPERTY(QString cardBackName READ cardBackName NOTIFY changed)
	Q_PROPERTY(QString cardBack READ cardBack NOTIFY changed)

public:
	explicit StyleProvider(boost::shared_ptr<ConfigFile> config, QObject *parent = nullptr);

	QString tableStyleName() const
	{
		return m_tableStyleName;
	}
	QString tableBackground() const
	{
		return m_tableBackground;
	}
	QString tableBackgroundAlignment() const
	{
		return m_tableBackgroundAlignment;
	}
	qreal tableBackgroundZoom() const
	{
		return m_tableBackgroundZoom;
	}
	qreal actionButtonBorderRadius() const
	{
		return m_actionButtonBorderRadius;
	}
	QString playerBoxAccent() const
	{
		return m_playerBoxAccent;
	}
	QString dealerPuck() const
	{
		return m_dealerPuck;
	}
	QString smallBlindPuck() const
	{
		return m_smallBlindPuck;
	}
	QString bigBlindPuck() const
	{
		return m_bigBlindPuck;
	}
	QString foldButton() const
	{
		return m_foldButton;
	}
	QString checkCallButton() const
	{
		return m_checkCallButton;
	}
	QString betRaiseButton() const
	{
		return m_betRaiseButton;
	}
	QString allInButton() const
	{
		return m_allInButton;
	}
	QString foldButtonTextColor() const
	{
		return m_foldButtonTextColor;
	}
	QString checkCallButtonTextColor() const
	{
		return m_checkCallButtonTextColor;
	}
	QString betRaiseButtonTextColor() const
	{
		return m_betRaiseButtonTextColor;
	}
	QString allInButtonTextColor() const
	{
		return m_allInButtonTextColor;
	}
	QString chatLogBackground() const
	{
		return m_chatLogBackground;
	}
	QString chatLogSurface() const
	{
		return m_chatLogSurface;
	}
	QString chatLogBorder() const
	{
		return m_chatLogBorder;
	}
	QString chatLogText() const
	{
		return m_chatLogText;
	}
	QString chatLogTextSecondary() const
	{
		return m_chatLogTextSecondary;
	}
	QString chatLogTextMuted() const
	{
		return m_chatLogTextMuted;
	}
	QString chatLogAccent() const
	{
		return m_chatLogAccent;
	}
	QString chatLogAccentText() const
	{
		return m_chatLogAccentText;
	}
	QString chatLogWinner() const
	{
		return m_chatLogWinner;
	}
	QString chatLogWinnerSide() const
	{
		return m_chatLogWinnerSide;
	}
	QString chatLogBoard() const
	{
		return m_chatLogBoard;
	}
	QString chatLogSend() const
	{
		return m_chatLogSend;
	}
	QString cardDeckName() const
	{
		return m_cardDeckName;
	}
	QString cardDeckDir() const
	{
		return m_cardDeckDir;
	}
	QString cardBackName() const
	{
		return m_cardBackName;
	}
	QString cardBack() const
	{
		return m_cardBack;
	}

	// Stil setzen: schreibt den Config-Key, lädt die Assets neu und meldet changed().
	Q_INVOKABLE void setTableStyle(const QString &name);
	Q_INVOKABLE void setCardDeckStyle(const QString &name);
	Q_INVOKABLE void setCardBackStyle(const QString &name);
	// Config-Keys erneut einlesen (z. B. nach resetToDefaults).
	Q_INVOKABLE void reload();

signals:
	void changed();

private:
	void loadTableStyle();
	void loadCardDeckStyle();
	void loadCardBackStyle();
	QString styleDirPath(const QString &category, const QString &name) const;
	// Liefert eine gut lesbare Schriftfarbe (#1A1A1A oder #FFFFFF) anhand der
	// gemittelten Helligkeit der Gradient-Farben einer Action-Button-SVG.
	QString contrastTextColor(const QString &svgAbsPath) const;
	// Gut lesbare Schriftfarbe (#101010 oder #FFFFFF) auf einer gegebenen
	// Flächenfarbe – für die Schrift auf dem Akzent (Selektion).
	static QString contrastTextOn(const QString &color);

	// Rohe <ChatLog*>-Werte, wie sie im Theme-XML stehen (leer = nicht gesetzt).
	struct ChatLogTags {
		QString background, surface, border, text, textSecondary, textMuted;
		QString accent, accentText, winner, winnerSide, board, send;
	};
	// Setzt die m_chatLog*-Member: gesetzte Tags gewinnen, für den Rest gilt
	// der zur Helligkeit des Panel-Hintergrunds passende Default-Satz.
	void applyChatLogColors(const ChatLogTags &tags);

	boost::shared_ptr<ConfigFile> m_config;

	QString m_tableStyleName;
	QString m_tableBackground;
	QString m_tableBackgroundAlignment;
	qreal m_tableBackgroundZoom = 1.0;
	qreal m_actionButtonBorderRadius = 9.0;
	QString m_playerBoxAccent;
	QString m_dealerPuck;
	QString m_smallBlindPuck;
	QString m_bigBlindPuck;
	QString m_foldButton;
	QString m_checkCallButton;
	QString m_betRaiseButton;
	QString m_allInButton;
	QString m_foldButtonTextColor;
	QString m_checkCallButtonTextColor;
	QString m_betRaiseButtonTextColor;
	QString m_allInButtonTextColor;

	// Chat-/Log-Box-Farben (Dunkel-Defaults; per Tisch-XML überschreibbar).
	QString m_chatLogBackground;
	QString m_chatLogSurface;
	QString m_chatLogBorder;
	QString m_chatLogText;
	QString m_chatLogTextSecondary;
	QString m_chatLogTextMuted;
	QString m_chatLogAccent;
	QString m_chatLogAccentText;
	QString m_chatLogWinner;
	QString m_chatLogWinnerSide;
	QString m_chatLogBoard;
	QString m_chatLogSend;

	QString m_cardDeckName;
	QString m_cardDeckDir;

	QString m_cardBackName;
	QString m_cardBack;
};

#endif // STYLEPROVIDER_H
