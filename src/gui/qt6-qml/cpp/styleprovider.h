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

// Delivers the asset URLs of the currently selected table or card deck style
// from <AppDataDir>/gfx/qml/ to the QML client. An empty string means
// "no style asset present" → the QML page falls back to the bundled
// qrc default. Via NOTIFY changed() all bindings rebind as soon as
// a different style is chosen via setTableStyle()/setCardDeckStyle().
class StyleProvider : public QObject
{
	Q_OBJECT

	// Tisch-Stil
	Q_PROPERTY(QString tableStyleName READ tableStyleName NOTIFY changed)
	Q_PROPERTY(QString tableBackground READ tableBackground NOTIFY changed)
	Q_PROPERTY(QString tableBackgroundAlignment READ tableBackgroundAlignment NOTIFY changed)
	Q_PROPERTY(qreal tableBackgroundZoom READ tableBackgroundZoom NOTIFY changed)
	// Corner radius of the action buttons, in units of the 168x43 drawing area
	// of the button SVGs. The client draws state frames (preselection gold,
	// primary action) as a rectangle above the SVG – without this value its
	// corners did not match the button shape. Default 9 = bundled style.
	Q_PROPERTY(qreal actionButtonBorderRadius READ actionButtonBorderRadius NOTIFY changed)
	// Accent colour of the player boxes (<PlayerBoxAccent>). Tints the gradient and
	// the frame of the box. Empty = neutral, bundled default.
	Q_PROPERTY(QString playerBoxAccent READ playerBoxAccent NOTIFY changed)
	Q_PROPERTY(QString dealerPuck READ dealerPuck NOTIFY changed)
	Q_PROPERTY(QString smallBlindPuck READ smallBlindPuck NOTIFY changed)
	Q_PROPERTY(QString bigBlindPuck READ bigBlindPuck NOTIFY changed)
	// Action button graphics (look/frame only – the dynamic text with the
	// amounts is laid over it in QML). Empty = fallback to the
	// hardcoded gradient button in GameActionBar.qml.
	Q_PROPERTY(QString foldButton READ foldButton NOTIFY changed)
	Q_PROPERTY(QString checkCallButton READ checkCallButton NOTIFY changed)
	Q_PROPERTY(QString betRaiseButton READ betRaiseButton NOTIFY changed)
	Q_PROPERTY(QString allInButton READ allInButton NOTIFY changed)
	// Text colour of the action buttons: explicitly via the theme XML
	// (<FoldButtonTextColor> … or style-wide <ActionButtonTextColor>) or
	// otherwise derived automatically from the button brightness (light background
	// → dark text). Empty = no theme button → QML uses its default.
	Q_PROPERTY(QString foldButtonTextColor READ foldButtonTextColor NOTIFY changed)
	Q_PROPERTY(QString checkCallButtonTextColor READ checkCallButtonTextColor NOTIFY changed)
	Q_PROPERTY(QString betRaiseButtonTextColor READ betRaiseButtonTextColor NOTIFY changed)
	Q_PROPERTY(QString allInButtonTextColor READ allInButtonTextColor NOTIFY changed)
	// Colours of the chat and log box (floating side panels at the table). Deliberately
	// independent of the light/dark mode of the rest of the app: only the table theme
	// is authoritative. Overridable via the table theme XML, otherwise the
	// bundled defaults (dark) apply. Solid hex colours – the translucency
	// (withAlpha) is done by the QML client.
	Q_PROPERTY(QString chatLogBackground READ chatLogBackground NOTIFY changed)
	Q_PROPERTY(QString chatLogSurface READ chatLogSurface NOTIFY changed)
	Q_PROPERTY(QString chatLogBorder READ chatLogBorder NOTIFY changed)
	Q_PROPERTY(QString chatLogText READ chatLogText NOTIFY changed)
	Q_PROPERTY(QString chatLogTextSecondary READ chatLogTextSecondary NOTIFY changed)
	Q_PROPERTY(QString chatLogTextMuted READ chatLogTextMuted NOTIFY changed)
	// Content colours of the same boxes: accent (panel title, active tab,
	// selection, mention in the chat, odds bar), text ON the accent,
	// the three roles of the game history and the send symbol. They can be set
	// via XML as well (<ChatLogAccent> …); if a tag is missing, the default
	// matching the brightness of <ChatLogBackground> applies – otherwise e.g. a
	// light table theme would show white/gold on a light background.
	Q_PROPERTY(QString chatLogAccent READ chatLogAccent NOTIFY changed)
	Q_PROPERTY(QString chatLogAccentText READ chatLogAccentText NOTIFY changed)
	Q_PROPERTY(QString chatLogWinner READ chatLogWinner NOTIFY changed)
	Q_PROPERTY(QString chatLogWinnerSide READ chatLogWinnerSide NOTIFY changed)
	Q_PROPERTY(QString chatLogBoard READ chatLogBoard NOTIFY changed)
	Q_PROPERTY(QString chatLogSend READ chatLogSend NOTIFY changed)
	// Kartenstapel-Stil (52 Vorderseiten)
	Q_PROPERTY(QString cardDeckName READ cardDeckName NOTIFY changed)
	Q_PROPERTY(QString cardDeckDir READ cardDeckDir NOTIFY changed)
	// Card back – a style category of its own (gfx/qml/backside/<name>/)
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

	// Set the style: writes the config key, reloads the assets and reports changed().
	Q_INVOKABLE void setTableStyle(const QString &name);
	Q_INVOKABLE void setCardDeckStyle(const QString &name);
	Q_INVOKABLE void setCardBackStyle(const QString &name);
	// Re-read the config keys (e.g. after resetToDefaults).
	Q_INVOKABLE void reload();

signals:
	void changed();

private:
	void loadTableStyle();
	void loadCardDeckStyle();
	void loadCardBackStyle();
	QString styleDirPath(const QString &category, const QString &name) const;
	// Delivers a well readable text colour (#1A1A1A or #FFFFFF) based on the
	// averaged brightness of the gradient colours of an action button SVG.
	QString contrastTextColor(const QString &svgAbsPath) const;
	// Well readable text colour (#101010 or #FFFFFF) on a given
	// surface colour – for the text on the accent (selection).
	static QString contrastTextOn(const QString &color);

	// Raw <ChatLog*> values as they stand in the theme XML (empty = not set).
	struct ChatLogTags {
		QString background, surface, border, text, textSecondary, textMuted;
		QString accent, accentText, winner, winnerSide, board, send;
	};
	// Sets the m_chatLog* members: tags that are set win, for the rest the
	// default set matching the brightness of the panel background applies.
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

	// Chat/log box colours (dark defaults; overridable via the table XML).
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
