import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects
import QtQuick.Window

import "../config" as Config

// Action-Leiste unten am Tisch: vorbereitbarer Raise-Bereich (Eingabe, Slider,
// Pot-%-Buttons, All-In/Show, Spielmodus) plus die Aktions-Buttons
// Fold / Check-Call / Bet-Raise – inklusive Vorwahl-Logik (pre-selection) und
// Auto-Spielmodus, 1:1 portiert aus dem Qt-Widgets-Client.
Item {
    id: actionBar

    // Querformat? Begrenzt/zentriert das Panel und gibt ihm Abstand zum Rand.
    property bool wide: false
    // Sichtbare Breite der Community-Cards – Referenz für panelWidth im Querformat.
    property real communityVisualWidth: 0

    // Höhe wächst dynamisch mit dem Inhalt (Desktop-Querformat: +8 px, damit das
    // Panel mit 8 px Abstand über dem unteren Bildschirmrand schwebt). Auf dem
    // Phone (compactActions) sitzt das Panel bündig am unteren Bildschirmrand.
    implicitHeight: actionBarCol.implicitHeight
                    + (actionBar.wide && !actionBar.compactActions ? 8 : 0)

    // Eckenradius der Theme-Button-SVGs, in Einheiten ihrer 168x43-Zeichenfläche
    // (<ActionButtonBorderRadius> im Tisch-Stil). Die Zustands-Rahmen (Vorwahl
    // gold, primäre Aktion) liegen als Rechteck ÜBER dem SVG; nur mit dem Radius
    // des Stils schließen sie bündig ab, statt an den Ecken zu klaffen.
    readonly property real themeButtonRadiusUnits:
        (typeof StyleProvider !== "undefined" && StyleProvider)
        ? StyleProvider.actionButtonBorderRadius : 9

    // Der Button wird auf seine tatsächliche Größe gestreckt, der Radius also mit
    // – in x und y unterschiedlich stark. Ein Rectangle kann aber nur einen
    // kreisrunden Radius: wir nehmen den kleineren der beiden, dann bleibt der
    // Rahmen innerhalb der Kontur statt aus ihr herauszulaufen.
    function themeButtonRadius(w, h) {
        return Math.min(w / 2, h / 2,
                        actionBar.themeButtonRadiusUnits * w / 168,
                        actionBar.themeButtonRadiusUnits * h / 43)
    }

    // Spielmodus-Aktion: vom Aufrufer (Shortcuts) und der Modus-ComboBox genutzt.
    // Falls bereits mein Zug: gewählten Auto-Modus ausführen – aber VERZÖGERT
    // (Qt.callLater), niemals synchron. fold()/call() verändert sofort den
    // Spielzustand und löst ein erneutes myTurnChanged + Re-Layout der ActionBar
    // (inkl. dieser ComboBox) aus; synchron mitten im Klick-/Signal-Handler
    // führte das zu Re-Entrancy (lokales Spiel fror ein, Netzwerk-Spiel stürzte ab).
    function applyPlayingMode(index) {
        actionBar.playingMode = index
        if (GameTable && GameTable.myTurn)
            Qt.callLater(actionBar.runAutoAction)
    }

    // Auto-Modus-Aktion im nächsten Event-Loop-Durchlauf ausführen. Der Zustand
    // wird erneut geprüft, da er sich seit der Planung geändert haben kann
    // (z.B. Zug bereits vorbei). Qt.callLater dedupliziert Mehrfachaufrufe.
    function runAutoAction() {
        if (!GameTable || !GameTable.myTurn)
            return
        if (actionBar.playingMode === 2) {            // Auto Check/Fold
            actionBar.preSelectEnabled = false        // eigener (Auto-)Zug erledigt
            if (actionBar.canCheck) GameTable.call()
            else GameTable.fold()
        } else if (actionBar.playingMode === 1) {     // Auto Check/Call
            actionBar.preSelectEnabled = false
            GameTable.call()
        }
    }

    // Querformat: Inhalt auf die (skalierte) Breite des Community-Cards-
    // Bereichs begrenzen und zentrieren – sonst wird u. a. der Slider viel
    // zu breit. Eine Untergrenze stellt sicher, dass die Steuerelemente
    // (Pot-Buttons + All-In + Spielmodus) nicht zu eng werden. Hochformat:
    // volle Breite.
    readonly property real panelWidth: actionBar.wide
        ? Math.min(width, Math.max(actionBar.communityVisualWidth, 380))
        : width

    // Aktuell vorbereiteter Raise-Betrag; kann auch vor dem eigenen Zug gesetzt werden
    property int raiseAmount: 0

    readonly property bool raiseAvailable: GameTable !== null
                                           && GameTable.maxRaiseAmount > 0
                                           && GameTable.minRaiseAmount > 0
    readonly property int raiseMinAmount: raiseAvailable ? GameTable.minRaiseAmount : 0
    readonly property int raiseMaxAmount: raiseAvailable ? GameTable.maxRaiseAmount : 0

    // Selbstheilung der Raise-Vorbelegung: ein auf 0 (oder unter das Minimum)
    // zurückgesetzter Betrag wird sofort auf das gültige Minimum gehoben, sobald
    // ein Raise möglich ist. Nötig, weil die Rundenende-/Phasen-Resets raiseAmount
    // auf 0 setzen können, NACHDEM min/max bereits final stehen – dann liefert kein
    // min/maxRaiseAmountChanged mehr ein syncRaiseAmount(), und der Bet/Raise-Button
    // bliebe bei „$0" hängen, bis irgendein späteres Compute feuert. Dieser Handler
    // ist reihenfolge-unabhängig und schließt das Fenster. Re-Entry ist unkritisch:
    // nach dem Setzen auf raiseMinAmount ist die Bedingung sofort false.
    onRaiseAmountChanged: {
        if (raiseAvailable && raiseAmount < raiseMinAmount)
            raiseAmount = raiseMinAmount
    }

    // Dynamische Button-Beschriftungen – analog zum Qt-Widgets-Client:
    //  • nichts zu callen  → "Check"      sonst → "Call $X"
    //  • Preflop oder schon gesetzt → "Raise $X"; postflop ohne Einsatz → "Bet $X"
    readonly property bool canCheck: GameTable !== null && GameTable.callAmount === 0
    readonly property bool isPreflop: GameTable !== null && GameTable.phaseText === "Preflop"
    readonly property string _amountSep: "\n"

    // Einstellung „Internationale Pokerausdrücke nicht übersetzen" (Config-Key
    // DontTranslateInternationalPokerStringsFromStyle). Ist sie an, werden die
    // Aktions-Begriffe fest auf Englisch gezeigt statt über qsTr() lokalisiert –
    // wie der Qt-Widgets-Client, der dann die internationalen Style-Strings
    // umgeht. Die qsTr()-Literale bleiben für die Übersetzungsextraktion erhalten.
    readonly property bool dontTranslatePokerTerms:
        (typeof SettingsManager !== "undefined" && SettingsManager && SettingsManager.configRevision >= 0)
            ? SettingsManager.readConfigInt("DontTranslateInternationalPokerStringsFromStyle") !== 0 : false
    // Einstellung „Fokus ins Einsatzfeld bei eigenem Zug" (EnableBetInputFocusSwitch).
    readonly property bool focusBetInputOnTurn:
        (typeof SettingsManager !== "undefined" && SettingsManager && SettingsManager.configRevision >= 0)
            ? SettingsManager.readConfigInt("EnableBetInputFocusSwitch") !== 0 : false
    // Einstellung „Versehentliches Call nach großem Raise verhindern"
    // (AccidentallyCallBlocker). Wie der Qt-Widgets-Client: ändert sich die Call-/
    // Check-Beschriftung (z. B. weil ein Gegner erhöht hat), wird der Call-Button
    // kurz gesperrt, damit ein bereits gezielter Klick nicht den neuen (höheren)
    // Betrag callt. callBlocked wird per Timer nach 1 s wieder freigegeben.
    readonly property bool accidentalCallBlockerEnabled:
        (typeof SettingsManager !== "undefined" && SettingsManager && SettingsManager.configRevision >= 0)
            ? SettingsManager.readConfigInt("AccidentallyCallBlocker") !== 0 : true
    // Einstellung „Pot-Prozent-Buttons anzeigen" (ShowPotPercentButtons).
    // configRevision-Referenz nötig, damit das Umschalten ingame sofort wirkt.
    readonly property bool showPotPercentButtons:
        (typeof SettingsManager !== "undefined" && SettingsManager && SettingsManager.configRevision >= 0)
            ? SettingsManager.readConfigInt("ShowPotPercentButtons") !== 0 : true
    property bool callBlocked: false
    // Kurz nach dem Aktivieren der Buttons (Rundenbeginn / eigener Zug) settlen die
    // Call-/Raise-Werte noch (neutraler Zustand → tatsächlicher Betrag, ggf. in
    // mehreren Schritten, s. raiseAmount-Selbstheilung). In diesem Einschwing-
    // Fenster darf der AccidentallyCallBlocker NICHT anspringen – sonst wirkt der
    // Call-Button am Rundenbeginn verzögert, während Fold/Raise schon aktiv sind.
    // Erst wenn die Buttons „scharf" sind (_callBlockerHot, nach kurzer Wartezeit),
    // gilt eine Änderung der Call-/Check-Beschriftung als echte Gegner-(Re-)Erhöhung
    // und sperrt den Call-Button kurz gegen versehentliche Klicks.
    property bool _callBlockerHot: false
    onActionsArmedChanged: {
        callBlocked = false
        _callBlockerHot = false
        if (actionsArmed)
            callBlockerArmTimer.restart()
        else
            callBlockerArmTimer.stop()
    }
    Timer {
        id: callBlockerArmTimer
        interval: 500
        onTriggered: actionBar._callBlockerHot = true
    }
    onCheckCallTextChanged: {
        if (accidentalCallBlockerEnabled && actionsArmed && _callBlockerHot) {
            callBlocked = true
            callBlockTimer.restart()
        }
    }
    Timer {
        id: callBlockTimer
        interval: 1000
        onTriggered: actionBar.callBlocked = false
    }

    // Aktions-Begriffe zentral über StaticData.pokerActionWord (1=Fold … 6=All-In),
    // damit die Boxen und die Action-Bar denselben Switch nicht mehrfach pflegen.
    readonly property string foldWord:  Config.StaticData.pokerActionWord(1, dontTranslatePokerTerms)
    readonly property string checkWord: Config.StaticData.pokerActionWord(2, dontTranslatePokerTerms)
    readonly property string callWord:  Config.StaticData.pokerActionWord(3, dontTranslatePokerTerms)
    readonly property string betWord:   Config.StaticData.pokerActionWord(4, dontTranslatePokerTerms)
    readonly property string raiseWord: Config.StaticData.pokerActionWord(5, dontTranslatePokerTerms)
    readonly property string allInWord: Config.StaticData.pokerActionWord(6, dontTranslatePokerTerms)
    // Beträge nur zeigen, solange die Buttons aktiv sind (eigener Zug oder
    // zulässige Vorauswahl). Im Showdown UND am Rundenende (nach der letzten
    // Spieleraktion) sind die Buttons inaktiv und die letzten Call-/Raise-Werte
    // nicht mehr gültig → neutrale Labels ohne Betrag. Erst zu Rundenbeginn
    // (neue Werte aus computeCallAndRaiseAmounts) erscheinen wieder Beträge.
    readonly property string checkCallText: (GameTable === null || !actionsArmed) ? callWord
        : (canCheck ? checkWord : callWord + _amountSep + "$" + GameTable.callAmount)
    readonly property string betRaiseText: {
        if (GameTable === null || !actionsArmed) return raiseWord
        var word = (!isPreflop && canCheck) ? betWord : raiseWord
        return raiseAvailable ? (word + _amountSep + "$" + raiseAmount) : word
    }

    // ── Vorwahl (pre-selection): vor dem eigenen Zug eine Aktion vormerken ──
    property string preAction: ""        // "", "fold", "call", "raise", "allin"
    // Vorauswahl-Freigabe: false nach eigenem Zug / am Rundenende (postflop:
    // gesperrt bis die Aufdeck-Animation durch ist, s. onBoardDealingChanged),
    // true bei Rundenstart, eigenem Zug oder Gegner-Aktion.
    property bool preSelectEnabled: true
    // Runden-Übergangssperre: true ab der letzten Aktion einer Setzrunde
    // (Signal bettingRoundEnded aus dem GameHandler) bis die nächste Runde mit
    // frischen Werten startet (roundValuesReady), es wieder mein Zug ist oder
    // eine neue Hand beginnt. Solange true sind die Aktions-Buttons inaktiv und
    // tragen keine veralteten Call-/Raise-Beträge mehr (s. actionsArmed). Genau
    // das verlangte Verhalten: letzter Spieler handelt → Buttons sofort
    // zurückgesetzt + deaktiviert → nächste Runde → wieder aktiv mit neuen Werten.
    property bool roundEnded: false
    // Werden gerade neue Gemeinschaftskarten aufgedeckt? Wird von GamePage aus
    // CommunityCards.dealing gespeist. Solange true, ist KEINE Aktion möglich
    // (actionsArmed gatet darauf) – exakt die Vorgabe: während Aufdeck-
    // Animationen sind die Buttons gesperrt. Sobald die Animation durch ist,
    // schaltet onBoardDealingChanged die Vorauswahl frei (= Rundenstart).
    property bool boardDealing: false
    onBoardDealingChanged: {
        if (!actionBar.boardDealing)
            actionBar.preSelectEnabled = true   // Aufdecken fertig → Runde läuft → Vorauswahl frei
    }
    // Reset bei Handwechsel oder Showdown
    property int lastHandNumber: -1
    Connections {
        target: GameTable
        function onHandNumberChanged() {
            if (GameTable && GameTable.handNumber !== actionBar.lastHandNumber) {
                actionBar.preAction = ""
                actionBar.preSelectEnabled = true   // neue Hand → Vorauswahl freischalten
                actionBar.roundEnded = false        // neue Hand → Rundensperre lösen
                actionBar.raiseAmount = 0
                actionBar.lastHandNumber = GameTable.handNumber
                // console.log("[ACTDBG] Reset: Neue Hand " + actionBar.lastHandNumber)
            }
        }
        function onBettingRoundEnded() {
            // Letzte Aktion der Setzrunde ist erfolgt → Buttons SOFORT zurücksetzen
            // und sperren: Vorwahl verwerfen, vorbereiteten Raise löschen und die
            // Übergangssperre setzen. Erst die nächste Runde (roundValuesReady),
            // der nächste eigene Zug oder eine neue Hand hebt sie wieder auf.
            actionBar.preAction = ""
            actionBar.preSelectEnabled = false
            actionBar.raiseAmount = 0
            actionBar.roundEnded = true
            // console.log("[ACTDBG] Reset (Rundenende): Buttons gesperrt bis nächste Runde")
        }
        function onPhaseTextChanged() {
            if (!GameTable) return
            // phaseText ist immer Preflop/Flop/Turn/River (nie "Showdown" – das
            // signalisiert showdownActive, s. onShowdownActiveChanged). Jeder
            // Phasenwechsel = Rundengrenze: Vorwahl/Werte zurücksetzen.
            actionBar.preAction = ""
            actionBar.raiseAmount = 0
            if (GameTable.phaseText === "Preflop") {
                // Preflop hat keine Board-Aufdeck-Animation → Vorauswahl sofort frei.
                actionBar.preSelectEnabled = true
            } else {
                // Flop/Turn/River: bis die Karten-Aufdeck-Animation durch ist
                // gesperrt; onBoardDealingChanged schaltet danach frei.
                actionBar.preSelectEnabled = false
            }
            // console.log("[ACTDBG] Rundenwechsel →", GameTable.phaseText)
        }
        function onShowdownActiveChanged() {
            // Showdown beginnt → alles zurücksetzen, damit keine veralteten
            // Werte/Markierungen in die Ergebnisanzeige hineinragen:
            //  • vorgemerkte Aktion verwerfen,
            //  • Vorauswahl sperren (Buttons bleiben so auch dann inaktiv, falls
            //    inShowdown kurz wackelt – armed = … && preSelectEnabled),
            //  • vorbereiteten Raise-Betrag löschen. Zu Rundenbeginn füllt
            //    syncRaiseAmount() ihn aus den neuen min/max-Werten neu.
            if (GameTable && GameTable.showdownActive) {
                actionBar.preAction = ""
                actionBar.preSelectEnabled = false
                actionBar.raiseAmount = 0
                // console.log("[ACTDBG] Reset (preAction/preSelect/raiseAmount): Showdown")
            }
        }
    }
    property int preCallAmount: -1        // callAmount zum Zeitpunkt der Vorwahl
    // Spielmodus: 0 = manuell, 1 = Auto Check/Call, 2 = Auto Check/Fold.
    property int playingMode: 0

    readonly property bool canAct: GameTable !== null && GameTable.canAct

    // Showdown-/Ergebnisanzeige (Post-River bis zur nächsten Hand): die
    // Aktions-Buttons müssen IMMER deaktiviert sein, sonst klickt man verfrüht
    // für die nächste Runde. Eigenes Gate, weil `armed` über myTurnNow auch an
    // canAct vorbei aktiv werden könnte (stale m_myTurn).
    readonly property bool inShowdown: GameTable !== null && GameTable.showdownActive

    // Zentraler „Buttons aktiv"-Zustand für Fold/Check-Call. Wahr, wenn ich am
    // Zug bin ODER eine Vorauswahl zulässig ist (canAct + Freigabe) – und nie im
    // Showdown oder während neue Gemeinschaftskarten aufgedeckt werden
    // (boardDealing). Die Button-Beschriftungen hängen daran: nur solange aktiv
    // werden Beträge gezeigt, sonst neutrale Labels (zurückgesetzte Werte).
    readonly property bool actionsArmed: !inShowdown && !boardDealing && !roundEnded
        && ((GameTable !== null && GameTable.myTurn)
            || (canAct && preSelectEnabled))

    // Kompakte Action-Bar nur auf echten Mobilgeräten mit knappem
    // vertikalem Platz (Phone-Landscape). Auf dem Desktop bleiben die
    // Buttons groß – auch bei breitem Aspect-Ratio (Ultrawide/HiDPI),
    // wo landscapeCompact geometrisch ebenfalls greift.
    readonly property bool compactActions:
        Config.Responsive.landscapeCompact && Config.Responsive.isMobile
    // Höhen der drei Action-Bar-Reihen.
    readonly property int actionRowHeight: compactActions ? 40 : (Config.Theme.compact ? 56 : 54)
    readonly property int raiseRowHeight:  compactActions ? 22 : 26

    // Während der Vorwahl zeigt der Fold-Button bei freiem Check "Check / Fold"
    // Vorwahl bei gratis Check: zweizeilig, damit auch längere Übersetzungen
    // (z. B. "Check / Se coucher") auf den Button passen.
    readonly property string foldText: (GameTable !== null && actionsArmed && !GameTable.myTurn && canCheck)
        ? (checkWord + " /\n" + foldWord) : foldWord

    function fireAction(which) {
        if (GameTable === null) return
        // Eigener Zug ausgeführt → Vorauswahl SOFORT sperren (Vertrag von
        // preSelectEnabled: „false nach eigenem Zug"). Hier zuverlässig, weil
        // onMeInActionTriggered bei rein timer-getriebenen Netzwerk-Zügen u.U.
        // gar nicht feuert und das Zurücksetzen dort verschluckt würde → die
        // Buttons blieben nach meinem Zug fälschlich aktiv mit aktualisierten
        // Werten. Eine echte (Re-)Erhöhung eines Gegners schaltet die Vorauswahl
        // über onRefreshActionTriggered (callAmount > 0) wieder frei.
        actionBar.preSelectEnabled = false
        if (which === "fold")       GameTable.fold()
        else if (which === "call")  GameTable.call()
        else if (which === "raise") GameTable.raise(raiseAmount)
        else if (which === "allin") GameTable.allIn()
    }

    // Vorgemerkte Aktion beim eigenen Zug ausführen.
    // Vorgemerktes "Fold" wird zu "Check", falls ein Check gratis möglich ist.
    function runPreAction(which) {
        if (which === "fold" && canCheck) {
            actionBar.preSelectEnabled = false   // wie fireAction: eigener Zug erledigt
            GameTable.call()
        } else {
            fireAction(which)
        }
    }

    function clickAction(which) {
        if (GameTable === null) return
        // Eigener Klick auf einen Action-Button hat Vorrang vor dem
        // Auto-Modus → zurück auf "manuell", dann die Aktion ausführen
        // bzw. vormerken (wie im Qt-Widgets-Client).
        if (playingMode !== 0)
            playingMode = 0
        // Es ist mein Zug, sobald der Server meinen Aktions-Timer zählt
        // (timeoutSeatId === 0) – auch wenn das myTurn-Flag noch nicht
        // gesetzt sein sollte. Dann SOFORT ausführen, sonst nur vormerken.
        var myTurnNow = GameTable.myTurn || GameTable.timeoutSeatId === 0
        var p0btnDbg = GameTable.players.length > 0 ? GameTable.players[0]["button"] : -1
        // console.log("[ACTDBG] click", which,
                    // "myTurn=", GameTable.myTurn,
                    // "tSeat=", GameTable.timeoutSeatId,
                    // "canAct=", GameTable.canAct,
                    // "callAmt=", GameTable.callAmount,
                    // "preSel=", preSelectEnabled,
                    // "p0btn=", p0btnDbg,
                    // "(1=D,2=SB,3=BB)",
                    // "phase=", GameTable.phaseText,
                    // "pre=", preAction,
                    // "→ myTurnNow=", myTurnNow)
        if (myTurnNow) {
            preAction = ""
            fireAction(which)
        } else if (canAct) {
            if (preAction === which) {
                preAction = ""
            } else {
                preAction = which
                preCallAmount = (which === "call") ? GameTable.callAmount : -1
            }
        }
    }

    function raiseStepFor(maximum) {
        if (maximum <= 1000)
            return 10
        if (maximum <= 10000)
            return 50
        if (maximum <= 100000)
            return 500
        return 5000
    }

    function roundedRaiseAmount(amount) {
        if (!raiseAvailable)
            return 0
        if (amount >= raiseMaxAmount)
            return raiseMaxAmount
        var step = raiseStepFor(raiseMaxAmount)
        return Math.floor(amount / step) * step
    }

    function clampRaiseAmount(amount) {
        if (!raiseAvailable)
            return 0
        return Math.max(raiseMinAmount, Math.min(raiseMaxAmount, amount))
    }

    function syncRaiseAmount() {
        if (!raiseAvailable) {
            raiseAmount = 0
            return
        }
        if (raiseAmount <= 0)
            raiseAmount = raiseMinAmount
        else
            // NUR auf den gültigen Bereich [min,max] klemmen – NICHT erneut aufs
            // Slider-Raster runden. Sonst würde ein bewusst gesetzter Betrag (z. B.
            // exakt 3200 über den Pot-Button) beim Übernehmen (eigener Zug →
            // syncRaiseAmount) auf das Raster abgerundet (z. B. 3000) und „springt
            // zurück". Das Raster-Runden bleibt dem Slider-Ziehen vorbehalten (onMoved).
            raiseAmount = clampRaiseAmount(raiseAmount)
    }

    // Raise-Wert vorbereiten, Vorwahl ausführen bzw. bei Änderungen verwerfen
    Connections {
        target: GameTable
        function onMyTurnChanged() {
            // Eigener Zug beginnt → Vorauswahl immer freischalten.
            // Ausführung der vorgemerkten/automatischen Aktion in onMeInActionTriggered.
            if (GameTable.myTurn) {
                actionBar.preSelectEnabled = true
                actionBar.roundEnded = false   // mein Zug → Rundensperre lösen
            }
            actionBar.syncRaiseAmount()
            // Einstellung „Fokus ins Einsatzfeld bei eigenem Zug" (Config-Key
            // EnableBetInputFocusSwitch): Eingabefeld fokussieren, sofern ein
            // Raise/Bet überhaupt möglich ist.
            //
            // ERST NACH syncRaiseAmount(): Solange das Feld den activeFocus hat,
            // schreibt die Connections-Bindung unten den neuen raiseAmount NICHT
            // mehr in den Text (damit sie die Tipp-Eingabe nicht überschreibt).
            // Vorher fokussiert stünde also noch der Betrag des letzten Zuges drin.
            if (GameTable.myTurn && actionBar.focusBetInputOnTurn
                && actionBar.raiseAvailable)
                raiseAmountInput.focusAndSelectAll()
        }
        function onMeInActionTriggered() {
            // Mein Zug steht fest → eine evtl. noch aktive Rundensperre lösen.
            actionBar.roundEnded = false
            // Wie meInAction() im Widgets-Client: GENAU HIER die gemerkte
            // bzw. automatische Aktion ausführen. Dieser Callback kommt bei
            // jedem eigenen Zug verlässlich (auch wenn m_myTurn schon true
            // war) → keine verschluckten Aktionen mehr.
            var p0btnDbg2 = GameTable.players.length > 0 ? GameTable.players[0]["button"] : -1
            // console.log("[ACTDBG] meInActionTriggered",
                        // "pre=", actionBar.preAction,
                        // "preCallAmt=", actionBar.preCallAmount,
                        // "mode=", actionBar.playingMode,
                        // "myTurn=", GameTable.myTurn,
                        // "tSeat=", GameTable.timeoutSeatId,
                        // "callAmt=", GameTable.callAmount,
                        // "p0btn=", p0btnDbg2,
                        // "(1=D,2=SB,3=BB)",
                        // "phase=", GameTable.phaseText,
                        // "canAct=", GameTable.canAct,
                        // "preSel=", actionBar.preSelectEnabled)
            actionBar.syncRaiseAmount()

            // Wie meInAction() im Widgets-Client (setFocus + selectAll): auch hier
            // fokussieren, weil myTurnChanged nicht bei jedem eigenen Zug feuert
            // (m_myTurn kann schon true gewesen sein). focusAndSelectAll() ist
            // idempotent – hat das Feld den Fokus bereits, wird nur neu markiert.
            if (actionBar.focusBetInputOnTurn && actionBar.raiseAvailable)
                raiseAmountInput.focusAndSelectAll()

            if (actionBar.playingMode === 2 || actionBar.playingMode === 1) {
                actionBar.runAutoAction()
            } else if (actionBar.preAction !== "") {       // Manuell: Vorwahl ausführen
                var a = actionBar.preAction
                actionBar.preAction = ""
                actionBar.runPreAction(a)
            }
            // Nach eigenem Zug: Vorauswahl sperren bis Gegner-Aktion oder Rundenwechsel
            actionBar.preSelectEnabled = false
        }
        function onRoundValuesReady() {
            // Werte nach Rundenwechsel sind jetzt korrekt (nach computeCallAndRaiseAmounts()).
            // Preflop hat keine Board-Aufdeck-Animation → Vorauswahl sofort frei.
            // Postflop bleibt gesperrt, bis die Aufdeck-Animation durch ist
            // (onBoardDealingChanged), damit während des Aufdeckens keine Aktion
            // möglich ist.
            // Frische Werte der neuen Runde liegen vor → Rundensperre lösen.
            actionBar.roundEnded = false
            if (GameTable && GameTable.phaseText === "Preflop")
                actionBar.preSelectEnabled = true
        }
        function onRefreshActionTriggered() {
            // Vorauswahl nur dann wieder freischalten, wenn ich auch WIRKLICH noch
            // handeln darf (GameTable.canAct). Sonst reaktivierte eine Gegner-Aktion
            // die Buttons mit veralteten Werten, obwohl ich raus bin:
            //   • nach eigenem All-In (canAct=false: kein Cash/Action=ALLIN),
            //   • im Rundenende-Fenster (canAct=false: roundClosed, s. C++-Fix),
            //   • nach Fold.
            // Bei einer echten (Re-)Erhöhung, auf die ich reagieren können soll, ist
            // canAct dagegen true (prevPlayerId != 0, Runde offen) → Vorauswahl frei.
            if (GameTable.callAmount > 0 && !GameTable.myTurn && GameTable.canAct) {
                // Gegner hat gesetzt/erhöht → Vorauswahl freischalten.
                // callAmountChanged allein taugt nicht: feuert auch nach
                // eigener Aktion (onRefreshSet/Pot/Cash) mit veralteten Werten.
                // Erneute Erhöhung über meinen bereits gematchten Betrag hinaus
                // (callAmount > 0) ⇒ ich muss erneut entscheiden → Vorauswahl
                // wieder freischalten, auch wenn ich diese Runde schon gehandelt
                // (und preSelectEnabled in fireAction auf false gesetzt) habe.
                // Reine Calls/Checks der Gegner lassen callAmount bei 0 und heben
                // die Sperre NICHT auf – die Buttons bleiben nach meinem Zug also
                // gesperrt, bis wirklich (re-)erhöht wird oder die Runde startet.
                actionBar.preSelectEnabled = true
                actionBar.roundEnded = false
            }
            // Sicherheit: vorgemerkter Call verfällt nur bei einer ECHTEN
            // Gegner-Aktion (FOLD/CHECK/CALL/BET/RAISE/ALLIN), die den Call-
            // Betrag verändert hat. refreshActionTriggered feuert
            // ausschließlich für solche Aktionen — Blind-Posts (preflop
            // SB→BB) lösen dieses Signal NICHT aus, sodass eine
            // Vorauswahl während des Blindings nicht mehr stillschweigend
            // gelöscht wird (war Auslöser für „UTG-preflop ohne Reaktion,
            // Timeout mit Default-Action").
            if (actionBar.preAction === "call"
                && GameTable.callAmount !== actionBar.preCallAmount)
                actionBar.preAction = ""
        }
        function onCallAmountChanged() {
            // KEIN preSelectEnabled=true hier: callAmountChanged feuert bei
            // jedem computeCallAndRaiseAmounts()-Aufruf (onRefreshSet/Pot/Cash)
            // auch mit veralteten Werten → Freischalten nur in onRefreshActionTriggered.
            // Den Pre-Action-Sicherheits-Check führen wir bewusst NICHT
            // mehr hier aus, sondern in onRefreshActionTriggered (s.o.) —
            // sonst löschten Blind-Posts (callAmount 0→SB→BB) jede
            // UTG-Pre-Action.
            actionBar.syncRaiseAmount()
        }
        function onMinRaiseAmountChanged() {
            // Widget-Parität (provideMyActions): War ein Bet/Raise vorgemerkt und
            // liegt der vorbereitete Betrag jetzt UNTER dem neuen Minimum – etwa
            // weil ein Gegner (re-)erhöht hat – wird die Vorauswahl VERWORFEN,
            // nicht stillschweigend auf das neue (höhere) Minimum angehoben und
            // trotzdem ausgeführt. Im Qt-Widgets-Client:
            //   int lastBetValue = <alter Button-Betrag>;
            //   if (lastBetValue < slider->minimum() && betRaise->isChecked())
            //       uncheckMyButtons();  // Vorwahl löschen
            // raiseAmount entspricht dem zuletzt vorbereiteten Button-Betrag und
            // wird erst danach per syncRaiseAmount() neu gesetzt – die Prüfung
            // sieht hier also noch den ALTEN Wert (= lastBetValue).
            if (actionBar.preAction === "raise"
                && (!actionBar.raiseAvailable
                    || actionBar.raiseAmount < GameTable.minRaiseAmount)) {
                // console.log("[ACTDBG] Raise-Vorwahl verworfen: vorbereitet",
                            // actionBar.raiseAmount, "< neues Minimum",
                            // GameTable.minRaiseAmount, "(Gegner hat (re-)erhöht)")
                actionBar.preAction = ""
            }
            actionBar.syncRaiseAmount()
        }
        function onMaxRaiseAmountChanged() {
            // Wie der Widgets-Client: ändert sich nur das Maximum (z. B. mein Cash
            // nach Sets-Einsammeln), wird die Vorwahl NICHT verworfen – der Betrag
            // wird lediglich auf das gültige Maximum begrenzt (syncRaiseAmount).
            // Verworfen wird nur, wenn gar kein Raise mehr möglich ist.
            if (actionBar.preAction === "raise" && !actionBar.raiseAvailable)
                actionBar.preAction = ""
            actionBar.syncRaiseAmount()
        }
        function onCanActChanged() {
            if (GameTable.canAct)
                return
            // canAct bündelt die Spielberechtigung (gefoldet/all-in/kein
            // Cash) mit der reinen Button-Freigabe (m_myTurn ||
            // prevPlayerId != 0). Eine vorgemerkte Aktion darf NUR
            // verfallen, wenn ich diese Hand wirklich nicht mehr handeln
            // kann – NICHT durch das transiente Gating kurz bevor ich am
            // Zug bin (prevPlayerId == 0, m_myTurn noch false), sonst
            // wird die Vorauswahl (typisch BB-Option) verschluckt und
            // beim eigenen Zug nicht ausgeführt. Der Widgets-Client
            // verwirft die gemerkte Aktion beim Gating ebenfalls nicht.
            var me = GameTable.players.length > 0 ? GameTable.players[0] : null
            if (me && (me["folded"] === true || me["stack"] === 0))
                actionBar.preAction = ""
        }
    }

    Rectangle {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        // Desktop-Querformat: kleiner Abstand zum unteren Bildschirmrand
        // (Tisch zeigt sich darunter durch). Phone (compactActions):
        // Panel bündig am unteren Bildschirmrand.
        anchors.bottomMargin: actionBar.wide && !actionBar.compactActions ? 8 : 0
        anchors.horizontalCenter: parent.horizontalCenter
        width: actionBar.panelWidth
        color: Qt.rgba(0, 0, 0, 0.82)
        // Geschrumpft (Querformat) als leicht abgerundetes Panel.
        radius: actionBar.wide ? 10 : 0
    }

    Column {
        id: actionBarCol
        width: actionBar.panelWidth
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 0

        // ── Raise-Bereich: dauerhaft vorbereitbar, Aktion erst beim eigenen Zug ──
        Column {
            id: raiseSection
            width: parent.width
            spacing: 3
            topPadding: 4
            bottomPadding: 2
            leftPadding: 8
            rightPadding: 8
            visible: GameTable !== null
            height: visible ? implicitHeight : 0
            clip: true

            // Zeile 1: Betrag-Eingabe (links) + Slider
            RowLayout {
                width: parent.width - 16
                spacing: 6

                // Betrag-Eingabe – links neben dem Slider
                Rectangle {
                    Layout.preferredWidth: 78
                    Layout.preferredHeight: actionBar.raiseRowHeight
                    Layout.alignment: Qt.AlignVCenter
                    radius: 5
                    color: actionBar.raiseAvailable ? "#1a2a1a" : "#171717"
                    border.color: actionBar.raiseAvailable ? "#4CAF50" : "#3a3a3a"
                    border.width: 1
                    TextInput {
                        id: raiseAmountInput
                        anchors { fill: parent; leftMargin: 6; rightMargin: 6 }
                        enabled: actionBar.raiseAvailable
                        text: actionBar.raiseAmount.toString()
                        color: enabled ? "#FFFFFF" : "#8a8a8a"
                        font.family: Config.StaticData.loadedFont.font.family
                        font.pixelSize: 13
                        font.bold: true
                        horizontalAlignment: Qt.AlignHCenter
                        verticalAlignment: Qt.AlignVCenter
                        inputMethodHints: Qt.ImhDigitsOnly
                        validator: IntValidator { bottom: 0; top: 9999999 }
                        // Auto-Fokus bei eigenem Zug: Betrag komplett markieren, damit
                        // die erste getippte Ziffer den vorgeschlagenen Betrag ERSETZT
                        // statt ihn zu verlängern (Parität zum Qt-Widgets-Client:
                        // spinBox_betValue->setFocus(); spinBox_betValue->selectAll()).
                        // Den Text vorher explizit nachziehen: Hatte das Feld den
                        // activeFocus schon (z. B. aus dem letzten Zug), hat die
                        // Connections-Bindung unten ihn bewusst nicht aktualisiert.
                        function focusAndSelectAll() {
                            // Widget-Parität: Tippt der Nutzer gerade in einem anderen
                            // Textfeld (Chat-Eingabe), den Fokus NICHT wegreißen – im
                            // Widgets-Client hängen beide Aufrufstellen an
                            // `!lineEdit_ChatInput->hasFocus()` bzw. `text() == ""`.
                            // Statt beide ChatBox-Instanzen hierher durchzureichen,
                            // prüfen wir generisch das aktuell fokussierte Item: Nur
                            // Texteingaben haben selectAll().
                            var af = Window.activeFocusItem
                            if (af && af !== raiseAmountInput && af.selectAll !== undefined)
                                return
                            text = actionBar.raiseAmount.toString()
                            forceActiveFocus()
                            selectAll()
                        }
                        // Live-Aktualisierung des Bet/Raise-Buttons während der Eingabe –
                        // analog zu spinBoxBetValueChanged() im Qt-Widgets-Client.
                        onTextChanged: {
                            var v = parseInt(text)
                            if (!isNaN(v) && actionBar.raiseAvailable)
                                actionBar.raiseAmount = actionBar.clampRaiseAmount(v)
                        }
                        onAccepted: {
                            var v = parseInt(text)
                            if (!isNaN(v) && GameTable) {
                                actionBar.raiseAmount = actionBar.clampRaiseAmount(v)
                            }
                            // Enter im Raise-Feld löst Bet/Raise aus (wie der
                            // Qt-Widgets-Client: Enter bei fokussiertem Betrag).
                            actionBar.clickAction("raise")
                        }
                        // Fokus abgeben, sobald das Feld gesperrt wird (Zug vorbei →
                        // min/maxRaiseAmount werden 0 → raiseAvailable false).
                        //
                        // Nötig, weil enabled=false NUR den activeFocus nimmt, die
                        // focus-Eigenschaft aber true lässt: Beim nächsten eigenen Zug
                        // aktiviert raiseAvailable das Feld wieder, und der Fokus-Scope
                        // gibt ihm den activeFocus von selbst zurück. Auf Touch-Geräten
                        // (iPad/iPhone) fährt dadurch ab dem ersten Tippen ins Feld bei
                        // JEDEM Zug ungefragt die Bildschirmtastatur hoch – auch wenn man
                        // nur checken will. Ohne Zutun des Nutzers wieder fokussiert zu
                        // werden ist auch auf dem Desktop falsch (Tastatureingaben landen
                        // dann im Betragsfeld statt bei den Aktions-Shortcuts).
                        // Der gewollte Auto-Fokus kommt ausschließlich aus der Einstellung
                        // EnableBetInputFocusSwitch (onMyTurnChanged oben) – die ruft
                        // forceActiveFocus() erst NACH dem Reaktivieren und bleibt wirksam.
                        onEnabledChanged: {
                            if (!enabled)
                                focus = false
                        }
                        // Text bleibt synchron mit raiseAmount (von Slider/%-Buttons)
                        onActiveFocusChanged: {
                            if (!activeFocus) {
                                text = actionBar.raiseAmount.toString()
                            }
                        }
                        Connections {
                            target: actionBar
                            function onRaiseAmountChanged() {
                                if (!raiseAmountInput.activeFocus)
                                    raiseAmountInput.text = actionBar.raiseAmount.toString()
                            }
                        }
                    }
                }

                Slider {
                    id: raiseSlider
                    Layout.fillWidth: true
                    Layout.preferredHeight: actionBar.raiseRowHeight
                    Layout.alignment: Qt.AlignVCenter
                    enabled: actionBar.raiseAvailable
                    opacity: enabled ? 1.0 : 0.45
                    from: actionBar.raiseMinAmount
                    to: actionBar.raiseAvailable ? Math.max(actionBar.raiseMinAmount, actionBar.raiseMaxAmount) : 1
                    stepSize: actionBar.raiseStepFor(actionBar.raiseMaxAmount)
                    value: actionBar.raiseAmount
                    onMoved: actionBar.raiseAmount = actionBar.clampRaiseAmount(actionBar.roundedRaiseAmount(value))

                    background: Rectangle {
                        x: raiseSlider.leftPadding
                        y: raiseSlider.topPadding + raiseSlider.availableHeight / 2 - height / 2
                        width: raiseSlider.availableWidth
                        height: 4
                        radius: 2
                        color: "#333333"
                        Rectangle {
                            width: raiseSlider.visualPosition * parent.width
                            height: parent.height
                            radius: 2
                            color: "#4CAF50"
                        }
                    }
                    handle: Rectangle {
                        x: raiseSlider.leftPadding + raiseSlider.visualPosition * (raiseSlider.availableWidth - width)
                        y: raiseSlider.topPadding + raiseSlider.availableHeight / 2 - height / 2
                        width: 18; height: 18; radius: 9
                        color: raiseSlider.pressed ? "#80FF80" : "#4CAF50"
                        border.color: "#2a7a2a"
                        border.width: 1
                    }
                }
            }

            // Zeile 2: Pot-%-Buttons + All-In (bündig) + Spielmodus-Dropdown (rechts)
            RowLayout {
                width: parent.width - 16
                spacing: 4

                // Pot-Prozent-Buttons: 1/3 · 1/2 · Pot
                Repeater {
                    model: [
                        { label: "1/3", frac: 1.0 / 3.0 },
                        { label: "1/2", frac: 0.5 },
                        { label: "Pot", frac: 1.0 }
                    ]
                    delegate: Rectangle {
                        required property var modelData
                        visible: actionBar.showPotPercentButtons
                        Layout.preferredWidth: visible ? 38 : 0
                        Layout.preferredHeight: actionBar.raiseRowHeight
                        radius: 5
                        enabled: actionBar.raiseAvailable
                        color: !enabled ? "#202020" : potBtnArea.containsPress ? "#2e7d32" : potBtnArea.containsMouse ? "#388e3c" : "#1b5e20"
                        border.color: enabled ? "#4CAF50" : "#3a3a3a"
                        border.width: 1
                        AppText {
                            anchors.centerIn: parent
                            text: modelData.label
                            color: parent.enabled ? "#FFFFFF" : "#8a8a8a"
                            font.pixelSize: 11
                            font.bold: true
                        }
                        MouseArea {
                            id: potBtnArea
                            anchors.fill: parent
                            cursorShape: parent.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
                            hoverEnabled: parent.enabled
                            enabled: parent.enabled
                            onClicked: {
                                if (!GameTable || !actionBar.raiseAvailable) return
                                var tp = GameTable.totalPot
                                var tgt = Math.round(tp * modelData.frac)
                                actionBar.raiseAmount = actionBar.clampRaiseAmount(tgt)
                            }
                        }
                    }
                }

                // All-In / Show – bündig an die Pot-Buttons
                // Im Post-River: zeigt "Show"-Button wenn der Spieler seine Karten
                // freiwillig zeigen kann (temporär als Ersatz für All-In).
                Rectangle {
                    id: allInBtn
                    readonly property bool isShowMode: typeof GameTable !== "undefined" && GameTable && GameTable.canShowCards
                    // klickbar: exakt dieselbe Bedingung wie Fold/Check-Call
                    // (actionsArmed). All-In ist immer zulässig, sobald ich
                    // handeln darf – es gibt keinen separaten „All-In-verfügbar"-
                    // Betrag wie beim Raise. Wichtig: actionsArmed lässt den
                    // eigenen Zug (myTurn) allein gelten und verlangt NICHT
                    // zusätzlich canAct. Sonst blieb der All-In-Button in dem
                    // Zeitfenster gesperrt, in dem der Zug nur über den
                    // Action-Timer signalisiert wird (startTimeout setzt myTurn,
                    // canAct ist dann noch stale=false) – während Fold/Call/Raise
                    // bereits klickbar waren. Der „Show"-Modus (isShowMode)
                    // bleibt davon unberührt.
                    readonly property bool armed: actionBar.actionsArmed
                    // Vorwahl-Markierung nur, solange der Button auch klickbar ist.
                    readonly property bool preChecked: armed && actionBar.preAction === "allin"
                    // Theme-Grafik nur im normalen All-In-Modus (nicht im
                    // "Show"-Modus, der seine eigene grüne Optik behält).
                    readonly property bool useTheme: !allInBtn.isShowMode
                                                     && StyleProvider && StyleProvider.allInButton !== ""
                    Layout.preferredWidth: 52
                    Layout.preferredHeight: actionBar.raiseRowHeight
                    // Mit Theme-SVG den Radius des Stils, sonst den Fallback-Wert.
                    radius: allInBtn.useTheme
                            ? actionBar.themeButtonRadius(allInBtn.width, allInBtn.height) : 5
                    opacity: (isShowMode || allInBtn.armed) ? 1.0 : 0.4
                    color: allInBtn.useTheme ? "transparent"
                         : allInArea.containsPress
                         ? Qt.lighter(isShowMode ? "#2d6e2d" : Config.Theme.colorAllInBottom, 1.35)
                         : allInArea.containsMouse
                         ? (isShowMode ? "#3a8f3a" : Config.Theme.colorAllInTop)
                         : (isShowMode ? "#2d6e2d" : Config.Theme.colorAllInBottom)
                    border.color: isShowMode ? "#80FF90"
                                : allInBtn.preChecked ? "#FFD700"
                                : Config.Theme.colorAllInEdge
                    // Mit Theme-SVG entfällt der eigene Rahmen (SVG bringt seinen
                    // mit); nur Show-/Vorwahl-Zustand zeichnet weiterhin einen.
                    border.width: (isShowMode || allInBtn.preChecked) ? 2 : (allInBtn.useTheme ? 0 : 1)
                    scale: (allInArea.pressed && (allInBtn.armed || isShowMode)) ? 0.95 : 1.0
                    Behavior on scale { NumberAnimation { duration: 90; easing.type: Easing.OutQuad } }

                    // Theme-SVG-Hintergrund (ohne Text). Inset um border.width,
                    // damit ein evtl. Zustands-Rahmen (gold) sichtbar bleibt.
                    Image {
                        anchors.fill: parent
                        anchors.margins: allInBtn.border.width
                        visible: allInBtn.useTheme
                        source: allInBtn.useTheme ? StyleProvider.allInButton : ""
                        sourceSize.width: Math.max(1, Math.round(allInBtn.width))
                        sourceSize.height: Math.max(1, Math.round(allInBtn.height))
                        fillMode: Image.Stretch
                        smooth: true
                    }
                    // Hover-/Press-Feedback über dem Theme-SVG.
                    Rectangle {
                        anchors.fill: parent
                        anchors.margins: allInBtn.border.width
                        visible: allInBtn.useTheme
                        radius: actionBar.themeButtonRadius(width, height)
                        color: "#FFFFFF"
                        opacity: allInArea.containsPress ? 0.18
                               : allInArea.containsMouse ? 0.08 : 0.0
                    }

                    AppText {
                        anchors.centerIn: parent
                        text: allInBtn.isShowMode ? qsTr("Show") : actionBar.allInWord
                        color: (allInBtn.useTheme && StyleProvider.allInButtonTextColor !== "")
                               ? StyleProvider.allInButtonTextColor : "#FFFFFF"
                        font.pixelSize: 12
                        font.bold: true
                    }
                    MouseArea {
                        id: allInArea
                        anchors.fill: parent
                        enabled: allInBtn.armed || allInBtn.isShowMode
                        cursorShape: (allInBtn.armed || allInBtn.isShowMode) ? Qt.PointingHandCursor : Qt.ArrowCursor
                        hoverEnabled: true
                        onPressed: function(mouse) {
                            // console.log("[ACTDBG] AllIn MouseArea press",
                                        // "enabled=", allInArea.enabled,
                                        // "myTurn=", GameTable ? GameTable.myTurn : "n/a")
                        }
                        onClicked: {
                            // console.log("[ACTDBG] AllIn MouseArea click isShow=", allInBtn.isShowMode)
                            if (allInBtn.isShowMode)
                                GameTable.showMyCards()
                            else
                                actionBar.clickAction("allin")
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                // Spielmodus-Dropdown (rechts): Manuell / Auto Check/Call / Auto Check/Fold
                ComboBox {
                    id: playingModeCombo
                    Layout.preferredWidth: 132
                    Layout.preferredHeight: actionBar.raiseRowHeight
                    font.family: Config.StaticData.loadedFont.font.family
                    font.pixelSize: 11
                    model: [ qsTr("Manuell"), qsTr("Auto Check/Call"), qsTr("Auto Check/Fold") ]
                    currentIndex: actionBar.playingMode
                    onActivated: (index) => actionBar.applyPlayingMode(index)
                    // Popup nach oben öffnen – verhindert, dass er hinter
                    // der Android-Navigationsleiste verschwindet.
                    popup.y: -popup.implicitHeight

                    contentItem: Text {
                        leftPadding: 8
                        rightPadding: playingModeCombo.indicator.width + 4
                        text: playingModeCombo.displayText
                        font: playingModeCombo.font
                        color: "#FFFFFF"
                        verticalAlignment: Text.AlignVCenter
                        elide: Text.ElideRight
                    }
                    background: Rectangle {
                        radius: 5
                        color: actionBar.playingMode === 0 ? "#222222" : "#3a2e10"
                        border.color: actionBar.playingMode === 0 ? "#3a3a3a" : Config.Theme.colorAccent
                        border.width: 1
                    }
                }
            }
        }

        // ── Aktions-Buttons: Fold / Check-Call / Bet-Raise ────────────────
        // Dynamische Beschriftung + Aktivierung wie im Qt-Widgets-Client.
        Item {
            width: parent.width
            height: actionBar.actionRowHeight

            // Wiederverwendbarer Aktions-Button mit Verlauf, dynamischem Text und
            // Vorwahl-Zustand (goldener Rahmen = vorgemerkt).
            component ActionButton: Item {
                id: ab
                property string actionKey: ""
                property string label: ""
                property color topColor: "#4080d8"
                property color bottomColor: "#1a3d8b"
                property color edgeColor: "#6aa0e8"
                // Schriftfarbe: vom Theme vorgegeben bzw. automatisch
                // kontrastiert (s. StyleProvider). Default für den
                // Fallback-Gradient-Button.
                property color textColor: "#F0F0F0"
                // Aktions-Button-Grafik des aktuellen Tisch-Stils (nur Optik/
                // Rahmen, ohne Text). Leer → Fallback auf den hartcodierten
                // Gradient-Button.
                property url themeSource: ""
                readonly property bool hasTheme: ab.themeSource != ""
                property bool armed: false   // klickbar: eigener Zug ODER Vorwahl möglich
                property bool highlight: false   // primäre Aktion hervorheben (Raise)
                // Call-Blocker: nur der Call-Button wird nach einer Betrags-/
                // Beschriftungsänderung kurz gesperrt (s. actionBar.callBlocked).
                readonly property bool blocked: ab.actionKey === "call" && actionBar.callBlocked
                readonly property bool myTurnNow: GameTable !== null && GameTable.myTurn
                // Vorwahl-Markierung (goldener Rahmen/Punkt) nur, solange der
                // Button auch klickbar ist. Sonst bliebe nach Runden-/Handende
                // eine veraltete Vorauswahl sichtbar, obwohl die Buttons in der
                // Übergangsphase inaktiv sind.
                readonly property bool preChecked: ab.armed && ab.actionKey !== "" && actionBar.preAction === ab.actionKey

                // onArmedChanged: console.log("[ACTDBG] armed", ab.actionKey, "→", ab.armed,
                                            // "(myTurn=", GameTable ? GameTable.myTurn : "n/a",
                                            // "canAct=", actionBar.canAct,
                                            // "preSel=", actionBar.preSelectEnabled, ")")

                opacity: (!ab.armed || ab.blocked) ? 0.4 : ((ab.myTurnNow || ab.preChecked) ? 1.0 : 0.72)

                // Theme-SVG-Hintergrund (nur Optik, ohne eingebackenen Text)
                Image {
                    anchors.fill: parent
                    visible: ab.hasTheme
                    source: ab.themeSource
                    sourceSize.width: Math.max(1, Math.round(ab.width))
                    sourceSize.height: Math.max(1, Math.round(ab.height))
                    fillMode: Image.Stretch
                    smooth: true
                }

                // Fallback-Gradient-Button (kein Theme-SVG vorhanden)
                Rectangle {
                    anchors.fill: parent
                    visible: !ab.hasTheme
                    radius: 9
                    border.width: (ab.preChecked || (ab.highlight && ab.armed)) ? 2 : 1
                    border.color: ab.preChecked ? "#FFD700" : (ab.armed ? ab.edgeColor : "#3a3a3a")
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: ab.armed ? ab.topColor : "#2b2b2b" }
                        GradientStop { position: 1.0; color: ab.armed ? ab.bottomColor : "#1c1c1c" }
                    }
                }

                // Zustands-Rahmen ÜBER dem Theme-SVG: Vorwahl (gold) bzw.
                // primäre Aktion (Raise) hervorheben. Beim Fallback erledigt
                // das der border des Gradient-Rechtecks oben.
                //
                // Der Rahmen wird in der 168x43-Zeichenfläche des Button-SVG
                // gezeichnet und mit derselben (ungleichmäßigen) Skalierung
                // gestreckt wie das Bild. Nur so deckt er sich mit der Kontur:
                // Image.Stretch macht aus der SVG-Ecke eine Ellipse, und ein
                // Rectangle.radius in Pixeln kann immer nur kreisrund sein –
                // im Portrait ist der Button deutlich gedrungener als 168x43,
                // dort liefen die beiden Radien sichtbar auseinander.
                Rectangle {
                    visible: ab.hasTheme && (ab.preChecked || (ab.highlight && ab.armed))
                    width: 168
                    height: 43
                    transform: Scale {
                        xScale: ab.width / 168
                        yScale: ab.height / 43
                    }
                    radius: actionBar.themeButtonRadiusUnits
                    color: "transparent"
                    // In Zeichenflächen-Einheiten; die Skalierung dünnt sie auf
                    // die gewohnten ~2 px aus.
                    border.width: 2.6
                    border.color: ab.preChecked ? "#FFD700" : ab.edgeColor
                }

                // Press-Feedback: kurzes Einsinken beim Tippen.
                scale: (abMouse.pressed && ab.armed) ? 0.96 : 1.0
                Behavior on scale { NumberAnimation { duration: 90; easing.type: Easing.OutQuad } }

                // Raise als primäre Aktion mit weichem Glow hervorheben.
                layer.enabled: Config.Theme.effectsEnabled && ab.highlight && ab.armed
                layer.effect: MultiEffect {
                    shadowEnabled: true
                    shadowColor: ab.edgeColor
                    shadowOpacity: 0.55
                    shadowBlur: 0.8
                    shadowVerticalOffset: 0
                    shadowHorizontalOffset: 0
                }

                AppText {
                    anchors.centerIn: parent
                    horizontalAlignment: Text.AlignHCenter
                    text: ab.label
                    color: ab.textColor
                    font.pixelSize: actionBar.compactActions ? 12 : 15
                    font.bold: true
                    font.letterSpacing: 0.5
                    lineHeight: 0.95
                }

                // kleiner "vorgemerkt"-Punkt oben rechts
                Rectangle {
                    visible: ab.preChecked
                    anchors { top: parent.top; right: parent.right; margins: 4 }
                    width: 8; height: 8; radius: 4
                    color: "#FFD700"
                }

                MouseArea {
                    id: abMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    enabled: ab.armed && !ab.blocked
                    cursorShape: (ab.armed && !ab.blocked) ? Qt.PointingHandCursor : Qt.ArrowCursor
                    onPressed: function(mouse) {
                        // console.log("[ACTDBG] MouseArea press", ab.actionKey,
                                    // "armed=", ab.armed,
                                    // "myTurn=", GameTable ? GameTable.myTurn : "n/a",
                                    // "canAct=", GameTable ? GameTable.canAct : "n/a",
                                    // "preSel=", actionBar.preSelectEnabled,
                                    // "btn=", mouse.button)
                    }
                    onClicked: {
                        // console.log("[ACTDBG] MouseArea click", ab.actionKey)
                        actionBar.clickAction(ab.actionKey)
                    }
                }
            }

            RowLayout {
                anchors {
                    fill: parent; leftMargin: 8; rightMargin: 8
                    topMargin: 5
                    bottomMargin: Config.Theme.compact ? 6 : 5
                }
                spacing: 8

                ActionButton {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    actionKey: "fold"
                    label: actionBar.foldText
                    themeSource: StyleProvider ? StyleProvider.foldButton : ""
                    textColor: (StyleProvider && StyleProvider.foldButtonTextColor !== "")
                               ? StyleProvider.foldButtonTextColor : "#F0F0F0"
                    topColor: Config.Theme.colorFoldTop
                    bottomColor: Config.Theme.colorFoldBottom
                    edgeColor: Config.Theme.colorFoldEdge
                    // myTurnNow gatet nie den echten Zug; preSelectEnabled sperrt
                    // die Vorauswahl nach eigenem Zug/Rundenwechsel. Im Showdown
                    // und am Rundenende (canAct=false) immer aus.
                    armed: actionBar.actionsArmed
                }

                ActionButton {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    actionKey: "call"
                    label: actionBar.checkCallText
                    themeSource: StyleProvider ? StyleProvider.checkCallButton : ""
                    textColor: (StyleProvider && StyleProvider.checkCallButtonTextColor !== "")
                               ? StyleProvider.checkCallButtonTextColor : "#F0F0F0"
                    topColor: Config.Theme.colorCallTop
                    bottomColor: Config.Theme.colorCallBottom
                    edgeColor: Config.Theme.colorCallEdge
                    armed: actionBar.actionsArmed
                }

                ActionButton {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    actionKey: "raise"
                    label: actionBar.betRaiseText
                    themeSource: StyleProvider ? StyleProvider.betRaiseButton : ""
                    textColor: (StyleProvider && StyleProvider.betRaiseButtonTextColor !== "")
                               ? StyleProvider.betRaiseButtonTextColor : "#F0F0F0"
                    topColor: Config.Theme.colorRaiseTop
                    bottomColor: Config.Theme.colorRaiseBottom
                    edgeColor: Config.Theme.colorRaiseEdge
                    highlight: true     // primäre Aktion betonen
                    armed: actionBar.actionsArmed && actionBar.raiseAvailable
                }
            }
        }
    }
}
