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

#include "styleprovider.h"
#include "configfile.h"
#include <QColor>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QUrl>
#include <QXmlStreamReader>

StyleProvider::StyleProvider(boost::shared_ptr<ConfigFile> config, QObject *parent)
	: QObject(parent), m_config(config)
{
	reload();
}

QString StyleProvider::styleDirPath(const QString &category, const QString &name) const
{
	if (!m_config || name.isEmpty())
		return QString();
	// Mitgelieferte Stile liegen unter <AppDataDir>/gfx/qml/, importierte unter
	// <UserDataDir>/gfx/qml/ (siehe SettingsManager::importStyle). Beide Dirs
	// enden bereits mit einem Verzeichnis-Trennzeichen.
	const QString rel = "gfx/qml/" + category + "/" + name;
	const QString appPath =
		QString::fromStdString(m_config->readConfigString("AppDataDir")) + rel;
	if (QDir(appPath).exists())
		return appPath;
	return QString::fromStdString(m_config->readConfigString("UserDataDir")) + rel;
}

void StyleProvider::loadTableStyle()
{
	m_tableBackground.clear();
	m_tableBackgroundAlignment.clear();
	m_tableBackgroundZoom = 1.0;
	m_actionButtonBorderRadius = 9.0;
	m_playerBoxAccent.clear();
	m_dealerPuck.clear();
	m_smallBlindPuck.clear();
	m_bigBlindPuck.clear();
	m_foldButton.clear();
	m_checkCallButton.clear();
	m_betRaiseButton.clear();
	m_allInButton.clear();
	m_foldButtonTextColor.clear();
	m_checkCallButtonTextColor.clear();
	m_betRaiseButtonTextColor.clear();
	m_allInButtonTextColor.clear();

	// Chat-/Log-Box-Farben: erst NACH dem Parsen setzen – welcher Default-Satz
	// gilt (hell/dunkel), hängt vom eingelesenen <ChatLogBackground> ab.
	// applyChatLogColors() macht das am Ende dieser Funktion; für den Fall,
	// dass wir vorher aussteigen (kein Stil-Verzeichnis/keine XML), wird es
	// dort ebenfalls mit lauter leeren Werten aufgerufen.
	ChatLogTags chatLogTags;

	QDir dir(styleDirPath("table", m_tableStyleName));
	if (!dir.exists()) {
		applyChatLogColors(chatLogTags);
		return;
	}
	const QStringList xmlFiles =
		dir.entryList(QStringList() << "*tablestyle.xml", QDir::Files, QDir::Name);
	if (xmlFiles.isEmpty()) {
		applyChatLogColors(chatLogTags);
		return;
	}

	auto urlIfExists = [&dir](const QString &rel) -> QString {
		if (rel.isEmpty())
			return QString();
		const QString abs = dir.absoluteFilePath(rel);
		if (!QFileInfo::exists(abs))
			return QString();
		return QUrl::fromLocalFile(abs).toString();
	};

	// Relative SVG-Pfade + evtl. explizite Schriftfarben sammeln; die effektive
	// Textfarbe wird nach dem Parsen bestimmt (Override > style-weit > aus SVG).
	QString foldRel, callRel, raiseRel, allInRel;
	QString styleWideTextColor;
	QString foldTextColor, callTextColor, raiseTextColor, allInTextColor;

	QFile f(dir.absoluteFilePath(xmlFiles.first()));
	if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
		applyChatLogColors(chatLogTags);
		return;
	}
	QXmlStreamReader xml(&f);
	while (!xml.atEnd()) {
		if (xml.readNext() != QXmlStreamReader::StartElement)
			continue;
		const QString tag = xml.name().toString();
		const QString value = xml.attributes().value("value").toString();
		if (tag == "Table")
			m_tableBackground = urlIfExists(value);
		else if (tag == "TableBackgroundAlign")
			m_tableBackgroundAlignment = value.toLower().trimmed();
		else if (tag == "TableBackgroundZoom") {
			// Optionaler Crop-/Zoom-Faktor (>= 1.0) für den center-Modus: skaliert
			// das Tischbild über die Minimal-Deckung hinaus → mehr Beschnitt des
			// äußeren Randes, Tisch wirkt größer. Per Daten justierbar (kein Build).
			bool ok = false;
			const double z = value.toDouble(&ok);
			if (ok && z > 0.0)
				m_tableBackgroundZoom = z;
		} else if (tag == "ActionButtonBorderRadius") {
			// Eckenradius der Button-SVGs, in Einheiten ihrer 168x43-Zeichen-
			// fläche. QML rechnet ihn auf die tatsächliche Button-Höhe um und
			// zeichnet die Zustands-Rahmen damit deckungsgleich zur Button-Form.
			bool ok = false;
			const double r = value.toDouble(&ok);
			if (ok && r >= 0.0)
				m_actionButtonBorderRadius = r;
		} else if (tag == "PlayerBoxAccent")
			m_playerBoxAccent = value.trimmed();
		else if (tag == "DealerPuck")
			m_dealerPuck = urlIfExists(value);
		else if (tag == "SmallBlindPuck")
			m_smallBlindPuck = urlIfExists(value);
		else if (tag == "BigBlindPuck")
			m_bigBlindPuck = urlIfExists(value);
		else if (tag == "FoldButton") {
			foldRel = value;
			m_foldButton = urlIfExists(value);
		} else if (tag == "CheckCallButton") {
			callRel = value;
			m_checkCallButton = urlIfExists(value);
		} else if (tag == "BetRaiseButton") {
			raiseRel = value;
			m_betRaiseButton = urlIfExists(value);
		} else if (tag == "AllInButton") {
			allInRel = value;
			m_allInButton = urlIfExists(value);
		} else if (tag == "ActionButtonTextColor")
			styleWideTextColor = value.trimmed();
		else if (tag == "FoldButtonTextColor")
			foldTextColor = value.trimmed();
		else if (tag == "CheckCallButtonTextColor")
			callTextColor = value.trimmed();
		else if (tag == "BetRaiseButtonTextColor")
			raiseTextColor = value.trimmed();
		else if (tag == "AllInButtonTextColor")
			allInTextColor = value.trimmed();
		// Chat-/Log-Box-Farben (optional je Stil): nur gültige, nicht-leere
		// Werte übernehmen; was fehlt, füllt applyChatLogColors() auf.
		else if (tag == "ChatLogBackground")
			chatLogTags.background = value.trimmed();
		else if (tag == "ChatLogSurface")
			chatLogTags.surface = value.trimmed();
		else if (tag == "ChatLogBorder")
			chatLogTags.border = value.trimmed();
		else if (tag == "ChatLogText")
			chatLogTags.text = value.trimmed();
		else if (tag == "ChatLogTextSecondary")
			chatLogTags.textSecondary = value.trimmed();
		else if (tag == "ChatLogTextMuted")
			chatLogTags.textMuted = value.trimmed();
		else if (tag == "ChatLogAccent")
			chatLogTags.accent = value.trimmed();
		else if (tag == "ChatLogAccentText")
			chatLogTags.accentText = value.trimmed();
		else if (tag == "ChatLogWinner")
			chatLogTags.winner = value.trimmed();
		else if (tag == "ChatLogWinnerSide")
			chatLogTags.winnerSide = value.trimmed();
		else if (tag == "ChatLogBoard")
			chatLogTags.board = value.trimmed();
		else if (tag == "ChatLogSend")
			chatLogTags.send = value.trimmed();
	}

	applyChatLogColors(chatLogTags);

	// Effektive Schriftfarbe je Button bestimmen: explizite Theme-Angabe
	// (per Button oder style-weit) hat Vorrang, sonst automatisch aus der
	// Button-Helligkeit – so steht die Schrift immer im Kontrast zum Button.
	auto effectiveTextColor = [&](const QString &override, const QString &rel) -> QString {
		if (!override.isEmpty())
			return override;
		if (!styleWideTextColor.isEmpty())
			return styleWideTextColor;
		if (rel.isEmpty())
			return QString();
		const QString abs = dir.absoluteFilePath(rel);
		if (!QFileInfo::exists(abs))
			return QString();
		return contrastTextColor(abs);
	};
	m_foldButtonTextColor = effectiveTextColor(foldTextColor, foldRel);
	m_checkCallButtonTextColor = effectiveTextColor(callTextColor, callRel);
	m_betRaiseButtonTextColor = effectiveTextColor(raiseTextColor, raiseRel);
	m_allInButtonTextColor = effectiveTextColor(allInTextColor, allInRel);
}

// Helligkeit (0..1) einer Farbe – gewichtet wie in contrastTextColor().
static qreal colorBrightness(const QColor &c)
{
	if (!c.isValid())
		return 0.0;
	return (0.299 * c.red() + 0.587 * c.green() + 0.114 * c.blue()) / 255.0;
}

QString StyleProvider::contrastTextOn(const QString &color)
{
	return colorBrightness(QColor(color)) > 0.6 ? QStringLiteral("#101010")
		   : QStringLiteral("#FFFFFF");
}

void StyleProvider::applyChatLogColors(const ChatLogTags &tags)
{
	// Welcher Default-Satz gilt, entscheidet der Panel-Hintergrund: ein Theme
	// mit hellem <ChatLogBackground> (z. B. "Ivoire - Chene") bekäme sonst die
	// Dunkel-Defaults – heller Text auf hellem Grund. Die Dunkel-Werte sind die
	// bisherigen (App-Dunkelpalette + Gold/Orange des Widgets-Clients), die
	// Hell-Werte deren abgedunkelte Gegenstücke (Kontrast >= 4.3:1 auf #f5eee1).
	struct Defaults {
		const char *background, *surface, *border, *text, *textSecondary, *textMuted;
		const char *accent, *winner, *winnerSide, *board, *send;
	};
	static const Defaults kDark = {
		"#1d222b", "#394150", "#576378", "#eff1f5", "#cdd3e0", "#7787a3",
		"#E3C800", "#FFFF00", "#FFFFCC", "#FF6633", "#4ade80"
	};
	static const Defaults kLight = {
		"#e3e8f0", "#dce2ec", "#a0acc4", "#1d222b", "#394150", "#7787a3",
		"#7a6000", "#6b5400", "#8a6a2a", "#a8431a", "#0e7a37"
	};

	const QColor bg(tags.background);
	const Defaults &def = (bg.isValid() && colorBrightness(bg) > 0.5) ? kLight : kDark;

	auto pick = [](const QString &tag, const char *fallback) {
		return tag.isEmpty() ? QString::fromLatin1(fallback) : tag;
	};
	m_chatLogBackground    = pick(tags.background,    def.background);
	m_chatLogSurface       = pick(tags.surface,       def.surface);
	m_chatLogBorder        = pick(tags.border,        def.border);
	m_chatLogText          = pick(tags.text,          def.text);
	m_chatLogTextSecondary = pick(tags.textSecondary, def.textSecondary);
	m_chatLogTextMuted     = pick(tags.textMuted,     def.textMuted);
	m_chatLogAccent        = pick(tags.accent,        def.accent);
	m_chatLogWinner        = pick(tags.winner,        def.winner);
	m_chatLogWinnerSide    = pick(tags.winnerSide,    def.winnerSide);
	m_chatLogBoard         = pick(tags.board,         def.board);
	m_chatLogSend          = pick(tags.send,          def.send);
	// Schrift auf dem Akzent (Selektion): folgt dem – ggf. vom Theme gesetzten –
	// Akzent, nicht dem Default-Satz.
	m_chatLogAccentText    = tags.accentText.isEmpty() ? contrastTextOn(m_chatLogAccent)
							 : tags.accentText;
}

QString StyleProvider::contrastTextColor(const QString &svgAbsPath) const
{
	QFile f(svgAbsPath);
	if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
		return QStringLiteral("#FFFFFF");
	const QString svg = QString::fromUtf8(f.readAll());

	// Repräsentative Hintergrundfarbe aus den Gradient-Stops mitteln. Der helle
	// Gloss-Streifen (rgba(255,255,255,…)) und der stroke bleiben außen vor.
	static const QRegularExpression reStop(
		QStringLiteral("stop-color\\s*=\\s*\"(#[0-9a-fA-F]{3,8})\""));
	int r = 0, g = 0, b = 0, n = 0;
	auto it = reStop.globalMatch(svg);
	while (it.hasNext()) {
		const QColor c(it.next().captured(1));
		if (c.isValid()) {
			r += c.red();
			g += c.green();
			b += c.blue();
			++n;
		}
	}
	if (n == 0) {
		// Kein Gradient → erstes solides fill="#…" als Notnagel.
		static const QRegularExpression reFill(
			QStringLiteral("fill\\s*=\\s*\"(#[0-9a-fA-F]{3,8})\""));
		const auto m = reFill.match(svg);
		if (m.hasMatch()) {
			const QColor c(m.captured(1));
			if (c.isValid()) {
				r = c.red();
				g = c.green();
				b = c.blue();
				n = 1;
			}
		}
	}
	if (n == 0)
		return QStringLiteral("#FFFFFF");

	// Wahrgenommene Helligkeit (sRGB-gewichtet, 0..1). Hell → dunkle Schrift.
	const double rr = r / (255.0 * n);
	const double gg = g / (255.0 * n);
	const double bb = b / (255.0 * n);
	const double luminance = 0.2126 * rr + 0.7152 * gg + 0.0722 * bb;
	return luminance > 0.6 ? QStringLiteral("#1A1A1A") : QStringLiteral("#FFFFFF");
}

void StyleProvider::loadCardDeckStyle()
{
	m_cardDeckDir.clear();

	QDir dir(styleDirPath("cards", m_cardDeckName));
	if (!dir.exists())
		return;
	const QStringList xmlFiles =
		dir.entryList(QStringList() << "*deckstyle.xml", QDir::Files, QDir::Name);
	if (xmlFiles.isEmpty())
		return;

	// Karten-Vorderseiten folgen der festen Namenskonvention 0.svg..51.svg
	// (Engine-Index). Erst wenn mindestens "0.svg" existiert, gilt der
	// Stil als nutzbar und QML baut die Pfade aus cardDeckDir.
	if (QFileInfo::exists(dir.absoluteFilePath("0.svg")))
		m_cardDeckDir = QUrl::fromLocalFile(dir.absolutePath()).toString();
}

void StyleProvider::loadCardBackStyle()
{
	m_cardBack.clear();

	QDir dir(styleDirPath("backside", m_cardBackName));
	if (!dir.exists())
		return;
	const QStringList xmlFiles =
		dir.entryList(QStringList() << "*backsidestyle.xml", QDir::Files, QDir::Name);
	if (xmlFiles.isEmpty())
		return;

	// Genau eine Rückseiten-Grafik je Stil, referenziert über <Backside value=...>.
	QFile f(dir.absoluteFilePath(xmlFiles.first()));
	if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
		return;
	QXmlStreamReader xml(&f);
	while (!xml.atEnd()) {
		if (xml.readNext() != QXmlStreamReader::StartElement)
			continue;
		if (xml.name().toString() == "Backside") {
			const QString rel = xml.attributes().value("value").toString();
			const QString abs = dir.absoluteFilePath(rel);
			if (!rel.isEmpty() && QFileInfo::exists(abs))
				m_cardBack = QUrl::fromLocalFile(abs).toString();
			break;
		}
	}
}

void StyleProvider::reload()
{
	if (m_config) {
		m_tableStyleName = QString::fromStdString(m_config->readConfigString("QmlGameTableStyle"));
		m_cardDeckName = QString::fromStdString(m_config->readConfigString("QmlCardDeckStyle"));
		m_cardBackName = QString::fromStdString(m_config->readConfigString("QmlCardBackStyle"));
	}
	loadTableStyle();
	loadCardDeckStyle();
	loadCardBackStyle();
	emit changed();
}

void StyleProvider::setTableStyle(const QString &name)
{
	if (name == m_tableStyleName)
		return;
	m_tableStyleName = name;
	if (m_config) {
		m_config->writeConfigString("QmlGameTableStyle", name.toStdString());
		m_config->writeBuffer();
	}
	loadTableStyle();
	emit changed();
}

void StyleProvider::setCardDeckStyle(const QString &name)
{
	if (name == m_cardDeckName)
		return;
	m_cardDeckName = name;
	if (m_config) {
		m_config->writeConfigString("QmlCardDeckStyle", name.toStdString());
		m_config->writeBuffer();
	}
	loadCardDeckStyle();
	emit changed();
}

void StyleProvider::setCardBackStyle(const QString &name)
{
	if (name == m_cardBackName)
		return;
	m_cardBackName = name;
	if (m_config) {
		m_config->writeConfigString("QmlCardBackStyle", name.toStdString());
		m_config->writeBuffer();
	}
	loadCardBackStyle();
	emit changed();
}
