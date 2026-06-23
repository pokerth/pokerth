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
    // unabhängig vom Hell/Dunkel-Modus der übrigen App: der Tisch behält immer
    // dieses (dunkle) Aussehen. Per Tisch-Theme-XML überschreibbar, sonst gelten
    // die hier gebündelten Dunkel-Defaults. Solide Hex-Farben – die Transluzenz
    // (withAlpha) macht der QML-Client.
    Q_PROPERTY(QString chatLogBackground READ chatLogBackground NOTIFY changed)
    Q_PROPERTY(QString chatLogSurface READ chatLogSurface NOTIFY changed)
    Q_PROPERTY(QString chatLogBorder READ chatLogBorder NOTIFY changed)
    Q_PROPERTY(QString chatLogText READ chatLogText NOTIFY changed)
    Q_PROPERTY(QString chatLogTextSecondary READ chatLogTextSecondary NOTIFY changed)
    Q_PROPERTY(QString chatLogTextMuted READ chatLogTextMuted NOTIFY changed)
    // Kartenstapel-Stil (52 Vorderseiten)
    Q_PROPERTY(QString cardDeckName READ cardDeckName NOTIFY changed)
    Q_PROPERTY(QString cardDeckDir READ cardDeckDir NOTIFY changed)
    // Kartenrückseite – eigene Stil-Kategorie (gfx/qml/backside/<name>/)
    Q_PROPERTY(QString cardBackName READ cardBackName NOTIFY changed)
    Q_PROPERTY(QString cardBack READ cardBack NOTIFY changed)

public:
    explicit StyleProvider(boost::shared_ptr<ConfigFile> config, QObject *parent = nullptr);

    QString tableStyleName() const { return m_tableStyleName; }
    QString tableBackground() const { return m_tableBackground; }
    QString tableBackgroundAlignment() const { return m_tableBackgroundAlignment; }
    qreal tableBackgroundZoom() const { return m_tableBackgroundZoom; }
    QString dealerPuck() const { return m_dealerPuck; }
    QString smallBlindPuck() const { return m_smallBlindPuck; }
    QString bigBlindPuck() const { return m_bigBlindPuck; }
    QString foldButton() const { return m_foldButton; }
    QString checkCallButton() const { return m_checkCallButton; }
    QString betRaiseButton() const { return m_betRaiseButton; }
    QString allInButton() const { return m_allInButton; }
    QString foldButtonTextColor() const { return m_foldButtonTextColor; }
    QString checkCallButtonTextColor() const { return m_checkCallButtonTextColor; }
    QString betRaiseButtonTextColor() const { return m_betRaiseButtonTextColor; }
    QString allInButtonTextColor() const { return m_allInButtonTextColor; }
    QString chatLogBackground() const { return m_chatLogBackground; }
    QString chatLogSurface() const { return m_chatLogSurface; }
    QString chatLogBorder() const { return m_chatLogBorder; }
    QString chatLogText() const { return m_chatLogText; }
    QString chatLogTextSecondary() const { return m_chatLogTextSecondary; }
    QString chatLogTextMuted() const { return m_chatLogTextMuted; }
    QString cardDeckName() const { return m_cardDeckName; }
    QString cardDeckDir() const { return m_cardDeckDir; }
    QString cardBackName() const { return m_cardBackName; }
    QString cardBack() const { return m_cardBack; }

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

    boost::shared_ptr<ConfigFile> m_config;

    QString m_tableStyleName;
    QString m_tableBackground;
    QString m_tableBackgroundAlignment;
    qreal m_tableBackgroundZoom = 1.0;
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

    QString m_cardDeckName;
    QString m_cardDeckDir;

    QString m_cardBackName;
    QString m_cardBack;
};

#endif // STYLEPROVIDER_H
