import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Effects
import QtQuick.Window

import "../config" as Config

// The action bar at the bottom of the table: a raise area that can be prepared (input, slider,
// pot % buttons, all-in/show, the game mode) plus the action buttons
// fold / check-call / bet-raise – including the preselection logic and the
// auto game mode, ported 1:1 from the Qt widgets client.
Item {
    id: actionBar

    // Landscape? It limits/centres the panel and gives it a distance to the edge.
    property bool wide: false
    // The visible width of the community cards – the reference for panelWidth in landscape.
    property real communityVisualWidth: 0

    // The height grows dynamically with the content (desktop landscape: +8 px, so that the
    // panel floats 8 px above the lower screen edge). On the
    // phone (compactActions) the panel sits flush at the lower screen edge.
    implicitHeight: actionBarCol.implicitHeight
                    + (actionBar.wide && !actionBar.compactActions ? 8 : 0)

    // The corner radius of the theme button SVGs, in units of their 168x43 drawing area
    // (<ActionButtonBorderRadius> in the table style). The state frames (preselection
    // gold, the primary action) lie as a rectangle ABOVE the SVG; only with the radius
    // of the style do they end flush instead of gaping at the corners.
    readonly property real themeButtonRadiusUnits:
        (typeof StyleProvider !== "undefined" && StyleProvider)
        ? StyleProvider.actionButtonBorderRadius : 9

    // The button is stretched to its actual size, and so is the radius
    // – to a different degree in x and y. A Rectangle, however, can only have a
    // circular radius: we take the smaller of the two, then the
    // frame stays inside the contour instead of running out of it.
    function themeButtonRadius(w, h) {
        return Math.min(w / 2, h / 2,
                        actionBar.themeButtonRadiusUnits * w / 168,
                        actionBar.themeButtonRadiusUnits * h / 43)
    }

    // The game mode action: used by the caller (shortcuts) and the mode ComboBox.
    // If it is already my turn: execute the chosen auto mode – but DEFERRED
    // (Qt.callLater), never synchronously. fold()/call() changes the
    // game state immediately and triggers another myTurnChanged + a re-layout of the action bar
    // (incl. this ComboBox); synchronously in the middle of the click/signal handler
    // that led to re-entrancy (the local game froze, the network game crashed).
    function applyPlayingMode(index) {
        actionBar.playingMode = index
        if (GameTable && GameTable.myTurn)
            Qt.callLater(actionBar.runAutoAction)
    }

    // Execute the auto mode action in the next event loop pass. The state
    // is checked again, since it may have changed since the planning
    // (e.g. the turn is already over). Qt.callLater deduplicates multiple calls.
    function runAutoAction() {
        if (!GameTable || !GameTable.myTurn)
            return
        if (actionBar.playingMode === 2) {            // Auto check/fold
            // An expected value of 0: ONLY a really free check. If the engine rejects
            // it (an opponent has bet/raised – possibly entered by the network thread
            // only at this moment), it folds. actionBar.canCheck cannot be
            // relied on here: it hangs off the QML value callAmount, which lags behind the
            // engine state by the queued signals – exactly that way
            // "auto check/fold" could put chips into a pot.
            if (!fireAction("call", 0))
                fireAction("fold")
        } else if (actionBar.playingMode === 1) {     // Auto check/call
            // Deliberately without an expected value (−1): this mode calls any amount.
            fireAction("call", -1)
        }
    }

    // Landscape: limit the content to the (scaled) width of the community cards
    // area and centre it – otherwise the slider among other things gets far
    // too wide. A lower bound makes sure that the controls
    // (pot buttons + all-in + the game mode) do not get too tight. Portrait:
    // the full width.
    readonly property real panelWidth: actionBar.wide
        ? Math.min(width, Math.max(actionBar.communityVisualWidth, 380))
        : width

    // The raise amount currently prepared; it can be set before your own turn as well
    property int raiseAmount: 0

    readonly property bool raiseAvailable: GameTable !== null
                                           && GameTable.maxRaiseAmount > 0
                                           && GameTable.minRaiseAmount > 0
    readonly property int raiseMinAmount: raiseAvailable ? GameTable.minRaiseAmount : 0
    readonly property int raiseMaxAmount: raiseAvailable ? GameTable.maxRaiseAmount : 0

    // Self-healing of the raise preset: an amount that has been reset to 0 (or below
    // the minimum) is raised to the valid minimum immediately as soon as
    // a raise is possible. Necessary because the round end/phase resets can set raiseAmount
    // to 0 AFTER min/max are already final – then no
    // min/maxRaiseAmountChanged delivers a syncRaiseAmount() any more, and the bet/raise button
    // would stay stuck at "$0" until some later compute fires. This handler
    // is independent of the order and closes the window. Re-entry is harmless:
    // after setting it to raiseMinAmount the condition is false immediately.
    onRaiseAmountChanged: {
        if (raiseAvailable && raiseAmount < raiseMinAmount)
            raiseAmount = raiseMinAmount
    }

    // Dynamic button captions – analogous to the Qt widgets client:
    //  • nothing to call  → "Check"      otherwise → "Call $X"
    //  • preflop or already bet → "Raise $X"; postflop without a bet → "Bet $X"
    readonly property bool canCheck: GameTable !== null && GameTable.callAmount === 0
    readonly property bool isPreflop: GameTable !== null && GameTable.phaseText === "Preflop"
    readonly property string _amountSep: "\n"

    // The setting "do not translate international poker terms" (the config key
    // DontTranslateInternationalPokerStringsFromStyle). If it is on, the
    // action terms are shown fixed in English instead of localized via qsTr() –
    // like the Qt widgets client, which then bypasses the international style
    // strings. The qsTr() literals are kept for the translation extraction.
    readonly property bool dontTranslatePokerTerms:
        (typeof SettingsManager !== "undefined" && SettingsManager && SettingsManager.configRevision >= 0)
            ? SettingsManager.readConfigInt("DontTranslateInternationalPokerStringsFromStyle") !== 0 : false
    // The setting "focus into the bet field on your own turn" (EnableBetInputFocusSwitch).
    readonly property bool focusBetInputOnTurn:
        (typeof SettingsManager !== "undefined" && SettingsManager && SettingsManager.configRevision >= 0)
            ? SettingsManager.readConfigInt("EnableBetInputFocusSwitch") !== 0 : false
    // The setting "prevent an accidental call after a large raise"
    // (AccidentallyCallBlocker). As in the Qt widgets client: if the call/
    // check caption changes (e.g. because an opponent has raised), the call button is
    // locked briefly, so that a click that is already aimed does not call the new (higher)
    // amount. callBlocked is released again by a timer after 1 s.
    readonly property bool accidentalCallBlockerEnabled:
        (typeof SettingsManager !== "undefined" && SettingsManager && SettingsManager.configRevision >= 0)
            ? SettingsManager.readConfigInt("AccidentallyCallBlocker") !== 0 : true
    // The setting "show pot percentage buttons" (ShowPotPercentButtons).
    // A configRevision reference is necessary so that toggling it in-game takes effect immediately.
    readonly property bool showPotPercentButtons:
        (typeof SettingsManager !== "undefined" && SettingsManager && SettingsManager.configRevision >= 0)
            ? SettingsManager.readConfigInt("ShowPotPercentButtons") !== 0 : true
    property bool callBlocked: false
    // Shortly after the buttons are activated (the start of a round / your own turn) the
    // call/raise values are still settling (a neutral state → the actual amount, possibly in
    // several steps, see the raiseAmount self-healing). In this settling
    // window the AccidentallyCallBlocker must NOT kick in – otherwise the
    // call button seems delayed at the start of a round while fold/raise are already active.
    // Only when the buttons are "armed" (_callBlockerHot, after a short wait) does
    // a change of the call/check caption count as a real (re-)raise of an opponent
    // and lock the call button briefly against accidental clicks.
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

    // The action terms centrally via StaticData.pokerActionWord (1=fold … 6=all-in),
    // so that the boxes and the action bar do not maintain the same switch several times.
    readonly property string foldWord:  Config.StaticData.pokerActionWord(1, dontTranslatePokerTerms)
    readonly property string checkWord: Config.StaticData.pokerActionWord(2, dontTranslatePokerTerms)
    readonly property string callWord:  Config.StaticData.pokerActionWord(3, dontTranslatePokerTerms)
    readonly property string betWord:   Config.StaticData.pokerActionWord(4, dontTranslatePokerTerms)
    readonly property string raiseWord: Config.StaticData.pokerActionWord(5, dontTranslatePokerTerms)
    readonly property string allInWord: Config.StaticData.pokerActionWord(6, dontTranslatePokerTerms)
    // Only show the amounts while the buttons are active (your own turn or
    // a permitted preselection). In the showdown AND at the end of a round (after the last
    // player action) the buttons are inactive and the last call/raise values are
    // no longer valid → neutral labels without an amount. Only at the start of a round
    // (new values from computeCallAndRaiseAmounts) do amounts appear again.
    readonly property string checkCallText: (GameTable === null || !actionsArmed) ? callWord
        : (canCheck ? checkWord : callWord + _amountSep + "$" + GameTable.callAmount)
    readonly property string betRaiseText: {
        if (GameTable === null || !actionsArmed) return raiseWord
        var word = (!isPreflop && canCheck) ? betWord : raiseWord
        return raiseAvailable ? (word + _amountSep + "$" + raiseAmount) : word
    }

    // ── The preselection: note an action before your own turn ──
    property string preAction: ""        // "", "fold", "call", "raise", "allin"
    // The preselection release: false after your own turn / at the end of a round (postflop:
    // locked until the reveal animation is through, see onBoardDealingChanged),
    // true at the start of a round, on your own turn or on an opponent action.
    property bool preSelectEnabled: true
    // The round transition lock: true from the last action of a betting round on
    // (the signal bettingRoundEnded from the GameHandler) until the next round starts with
    // fresh values (roundValuesReady), it is my turn again or
    // a new hand begins. While it is true the action buttons are inactive and
    // no longer carry stale call/raise amounts (see actionsArmed). Exactly
    // the behaviour that was asked for: the last player acts → the buttons are reset
    // + deactivated immediately → the next round → active again with new values.
    property bool roundEnded: false
    // Are new community cards currently being revealed? It is fed from GamePage by
    // CommunityCards.dealing. While it is true, NO action is possible
    // (actionsArmed gates on it) – exactly the requirement: during reveal
    // animations the buttons are locked. As soon as the animation is through,
    // onBoardDealingChanged releases the preselection (= the start of a round).
    property bool boardDealing: false
    onBoardDealingChanged: {
        if (!actionBar.boardDealing)
            actionBar.preSelectEnabled = true   // The revealing is finished → the round is running → the preselection is free
    }
    // A reset on a hand change or the showdown
    property int lastHandNumber: -1
    Connections {
        target: GameTable
        function onHandNumberChanged() {
            if (GameTable && GameTable.handNumber !== actionBar.lastHandNumber) {
                actionBar.preAction = ""
                actionBar.preSelectEnabled = true   // a new hand → release the preselection
                actionBar.roundEnded = false        // a new hand → release the round lock
                actionBar.raiseAmount = 0
                actionBar.lastHandNumber = GameTable.handNumber
                // console.log("[ACTDBG] Reset: Neue Hand " + actionBar.lastHandNumber)
            }
        }
        function onBettingRoundEnded() {
            // The last action of the betting round has happened → reset the buttons IMMEDIATELY
            // and lock them: discard the preselection, clear the prepared raise and set the
            // transition lock. Only the next round (roundValuesReady),
            // the next turn of your own or a new hand lifts it again.
            actionBar.preAction = ""
            actionBar.preSelectEnabled = false
            actionBar.raiseAmount = 0
            actionBar.roundEnded = true
            // console.log("[ACTDBG] reset (end of round): buttons locked until the next round")
        }
        function onPhaseTextChanged() {
            if (!GameTable) return
            // phaseText is always preflop/flop/turn/river (never "showdown" – that is
            // signalled by showdownActive, see onShowdownActiveChanged). Every
            // phase change = a round boundary: reset the preselection/values.
            actionBar.preAction = ""
            actionBar.raiseAmount = 0
            if (GameTable.phaseText === "Preflop") {
                // Preflop has no board reveal animation → the preselection is free immediately.
                actionBar.preSelectEnabled = true
            } else {
                // Flop/turn/river: locked until the card reveal animation is
                // through; onBoardDealingChanged releases it afterwards.
                actionBar.preSelectEnabled = false
            }
            // console.log("[ACTDBG] Rundenwechsel →", GameTable.phaseText)
        }
        function onShowdownActiveChanged() {
            // The showdown begins → reset everything, so that no stale
            // values/marks stick into the result display:
            //  • discard the noted action,
            //  • lock the preselection (that way the buttons stay inactive even if
            //    inShowdown wobbles briefly – armed = … && preSelectEnabled),
            //  • clear the prepared raise amount. At the start of a round
            //    syncRaiseAmount() fills it anew from the new min/max values.
            if (GameTable && GameTable.showdownActive) {
                actionBar.preAction = ""
                actionBar.preSelectEnabled = false
                actionBar.raiseAmount = 0
                // console.log("[ACTDBG] Reset (preAction/preSelect/raiseAmount): Showdown")
            }
        }
    }
    property int preCallAmount: -1        // the callAmount at the time of the preselection
    // The game mode: 0 = manual, 1 = auto check/call, 2 = auto check/fold.
    property int playingMode: 0

    readonly property bool canAct: GameTable !== null && GameTable.canAct

    // The authoritative "the server is waiting for my action NOW" (a network game,
    // from the engine: currentPlayersTurnId == our own unique ID, see
    // GameHandler::engineAwaitsMyAction). Unlike myTurn/timeoutSeatId,
    // this state can be cleared by no trailing refresh callback.
    // It is thereby the reliable source for whether a click has to be executed
    // IMMEDIATELY instead of silently becoming a preselection (which would never be executed
    // inside your own turn window → a server timeout with the default action).
    readonly property bool awaitingMyAction: GameTable !== null && GameTable.awaitingMyAction

    // The showdown/result display (post-river until the next hand): the
    // action buttons have to be deactivated ALWAYS, otherwise you click prematurely
    // for the next round. A gate of its own, because `armed` could become active via myTurnNow
    // past canAct as well (a stale m_myTurn).
    readonly property bool inShowdown: GameTable !== null && GameTable.showdownActive

    // The central "buttons active" state for fold/check-call. True when I am to
    // act OR a preselection is permitted (canAct + the release) – and never in the
    // showdown or while new community cards are being revealed
    // (boardDealing). The button captions hang off it: only while it is active
    // are amounts shown, otherwise neutral labels (reset values).
    readonly property bool actionsArmed: !inShowdown && !boardDealing && !roundEnded
        && ((GameTable !== null && GameTable.myTurn)
            || awaitingMyAction
            || (canAct && preSelectEnabled))

    // A compact action bar only on real mobile devices with little
    // vertical room (phone landscape). On the desktop the buttons stay
    // large – even with a wide aspect ratio (ultrawide/HiDPI),
    // where landscapeCompact applies geometrically as well.
    readonly property bool compactActions:
        Config.Responsive.landscapeCompact && Config.Responsive.isMobile
    // The heights of the three action bar rows.
    readonly property int actionRowHeight: compactActions ? 40 : (Config.Theme.compact ? 56 : 54)
    readonly property int raiseRowHeight:  compactActions ? 22 : 26

    // During the preselection the fold button shows "Check / Fold" with a free check
    // A preselection with a free check: two lines, so that longer translations
    // (e.g. "Check / Se coucher") fit on the button as well.
    readonly property string foldText: (GameTable !== null && actionsArmed && !GameTable.myTurn
                                        && !awaitingMyAction && canCheck)
        ? (checkWord + " /\n" + foldWord) : foldWord

    // Execute the action. With a check/call expectedCall is the amount the
    // player SAW on the button (0 = check), −1 = any. C++ only executes
    // the action if the engine does not demand more (see GameHandler::call)
    // – the return value says whether it really acted.
    function fireAction(which, expectedCall) {
        if (GameTable === null) return false
        // Your own turn has been executed → lock the preselection IMMEDIATELY (the contract of
        // preSelectEnabled: "false after your own turn"). Reliably here, because
        // onMeInActionTriggered may not fire at all with purely timer driven network turns
        // and the reset would be swallowed there → the
        // buttons would wrongly stay active after my turn with updated
        // values. A real (re-)raise of an opponent releases the preselection
        // again via onRefreshActionTriggered (callAmount > 0). If a
        // check/call is discarded, onActionRejected lifts the lock immediately
        // again – it is still my turn after all.
        actionBar.preSelectEnabled = false
        if (which === "fold")       { GameTable.fold();  return true }
        if (which === "call")       return GameTable.call(expectedCall === undefined
                                                          ? GameTable.callAmount : expectedCall)
        if (which === "raise")      { GameTable.raise(raiseAmount); return true }
        if (which === "allin")      { GameTable.allIn(); return true }
        return false
    }

    // Execute the noted action on your own turn. What always counts is what
    // stood on the button at the time of the preselection:
    //  • "fold" with a free check = the caption "Check / Fold" → first try to check
    //    for free, otherwise fold (like the widgets client, which in that
    //    case clicks the check button),
    //  • "call" only up to the amount that was shown then (preCallAmount).
    function runPreAction(which) {
        if (which === "fold") {
            if (canCheck && fireAction("call", 0))
                return
            fireAction("fold")
            return
        }
        if (which === "call") {
            // preCallAmount is the amount shown at the preselection; if it is missing
            // (−1), the safe interpretation "only check for free" applies.
            fireAction("call", actionBar.preCallAmount >= 0 ? actionBar.preCallAmount : 0)
            return
        }
        fireAction(which)
    }

    function clickAction(which) {
        if (GameTable === null) return
        // Your own click on an action button takes precedence over the
        // auto mode → back to "manual", then execute the action
        // or note it (as in the Qt widgets client).
        if (playingMode !== 0)
            playingMode = 0
        // It is my turn as soon as the server counts my action timer
        // (timeoutSeatId === 0) – even if the myTurn flag should not be
        // set yet. Then execute IMMEDIATELY, otherwise only note it.
        //
        // awaitingMyAction first: it is the only source that does not lose a turn window
        // once it has been opened. Without it the click degraded to a
        // preselection as soon as myTurn/timeoutSeatId had been cleared by a trailing
        // callback (disableMyButtons, stopTimeoutAnimation, a phase change …)
        // – and a preselection that is set DURING your own
        // turn window is never executed by anyone (onMeInActionTriggered
        // is long through) → a timeout with the server default.
        var myTurnNow = GameTable.awaitingMyAction
                        || GameTable.myTurn || GameTable.timeoutSeatId === 0
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
            // Without an expectedCall, the amount currently shown (GameTable.callAmount
            // = the caption of the check/call button) counts as the upper limit.
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
            // ONLY clamp to the valid range [min,max] – do NOT round to the
            // slider raster again. Otherwise an amount that was set deliberately (e.g.
            // exactly 3200 via the pot button) would be rounded down to the raster when adopting it (your own turn →
            // syncRaiseAmount) (e.g. 3000) and would "jump
            // back". The raster rounding stays reserved for dragging the slider (onMoved).
            raiseAmount = clampRaiseAmount(raiseAmount)
    }

    // Prepare the raise value, execute the preselection or discard it on changes
    Connections {
        target: GameTable
        function onAwaitingMyActionChanged() {
            // The authoritative turn window edge from the engine. It unlocks exactly
            // like onMyTurnChanged – but also when myTurn was not set in the
            // first place or was cleared again right away. That way no
            // leftover roundEnded/preSelectEnabled=false can block a running
            // turn window any more.
            if (!GameTable.awaitingMyAction) return
            actionBar.preSelectEnabled = true
            actionBar.roundEnded = false
            actionBar.syncRaiseAmount()
            // A safety net: execute a preselection set before the turn in case
            // onMeInActionTriggered fails to appear. A double execution is harmless –
            // preAction is cleared beforehand and the second action bounces off
            // the latch in C++ (m_actionSentForTurn).
            if (actionBar.playingMode === 0 && actionBar.preAction !== "") {
                var pending = actionBar.preAction
                actionBar.preAction = ""
                actionBar.runPreAction(pending)
            }
        }
        function onMyTurnChanged() {
            // Your own turn begins → always release the preselection.
            // The execution of the noted/automatic action is in onMeInActionTriggered.
            if (GameTable.myTurn) {
                actionBar.preSelectEnabled = true
                actionBar.roundEnded = false   // my turn → release the round lock
            }
            actionBar.syncRaiseAmount()
            // The setting "focus into the bet field on your own turn" (the config key
            // EnableBetInputFocusSwitch): focus the input field, provided a
            // raise/bet is possible at all.
            //
            // ONLY AFTER syncRaiseAmount(): while the field has the activeFocus,
            // the Connections binding below does NOT write the new raiseAmount
            // into the text any more (so that it does not overwrite the typing).
            // Focused earlier, the amount of the last turn would still be in it.
            if (GameTable.myTurn && actionBar.focusBetInputOnTurn
                && actionBar.raiseAvailable)
                raiseAmountInput.focusAndSelectAll()
        }
        function onMeInActionTriggered() {
            // My turn is settled → lift a round lock that may still be active.
            actionBar.roundEnded = false
            // As meInAction() in the widgets client: execute the remembered
            // or automatic action EXACTLY HERE. This callback arrives reliably on
            // every turn of your own (even if m_myTurn was already true)
            // → no swallowed actions any more.
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

            // As meInAction() in the widgets client (setFocus + selectAll): focus here
            // as well, because myTurnChanged does not fire on every turn of your own
            // (m_myTurn may already have been true). focusAndSelectAll() is
            // idempotent – if the field already has the focus, it only selects anew.
            if (actionBar.focusBetInputOnTurn && actionBar.raiseAvailable)
                raiseAmountInput.focusAndSelectAll()

            if (actionBar.playingMode === 2 || actionBar.playingMode === 1) {
                actionBar.runAutoAction()
            } else if (actionBar.preAction !== "") {       // Manual: execute the preselection
                var a = actionBar.preAction
                actionBar.preAction = ""
                actionBar.runPreAction(a)
            }
            // After your own turn: lock the preselection until an opponent action or a round change
            actionBar.preSelectEnabled = false
        }
        function onRoundValuesReady() {
            // The values after a round change are correct now (after computeCallAndRaiseAmounts()).
            // Preflop has no board reveal animation → the preselection is free immediately.
            // Postflop it stays locked until the reveal animation is through
            // (onBoardDealingChanged), so that no action is possible during
            // the revealing.
            // Fresh values of the new round are available → release the round lock.
            actionBar.roundEnded = false
            if (GameTable && GameTable.phaseText === "Preflop")
                actionBar.preSelectEnabled = true
        }
        function onRefreshActionTriggered() {
            // Only release the preselection again if I REALLY am still allowed to
            // act (GameTable.canAct). Otherwise an opponent action would reactivate
            // the buttons with stale values although I am out:
            //   • after my own all-in (canAct=false: no cash/action=ALLIN),
            //   • in the round end window (canAct=false: roundClosed, see the C++ fix),
            //   • after a fold.
            // With a real (re-)raise that I should be able to react to, canAct is
            // true by contrast (prevPlayerId != 0, the round is open) → the preselection is free.
            if (GameTable.callAmount > 0 && !GameTable.myTurn && GameTable.canAct) {
                // An opponent has bet/raised → release the preselection.
                // callAmountChanged alone is not suitable: it fires after
                // your own action as well (onRefreshSet/Pot/Cash) with stale values.
                // Another raise beyond the amount I have already matched
                // (callAmount > 0) ⇒ I have to decide again → release the
                // preselection again, even if I have already acted in this round
                // (and preSelectEnabled was set to false in fireAction).
                // Pure calls/checks of the opponents leave callAmount at 0 and do NOT
                // lift the lock – so the buttons stay locked after my turn
                // until there really is a (re-)raise or the round starts.
                actionBar.preSelectEnabled = true
                actionBar.roundEnded = false
            }
            // Safety: a noted call only lapses on a REAL
            // opponent action (FOLD/CHECK/CALL/BET/RAISE/ALLIN) that has changed the call
            // amount. refreshActionTriggered fires
            // exclusively for such actions — blind posts (preflop
            // SB→BB) do NOT trigger this signal, so that a
            // preselection during the blinding is no longer silently
            // deleted (it was the trigger for "UTG preflop without a reaction,
            // a timeout with the default action").
            if (actionBar.preAction === "call"
                && GameTable.callAmount !== actionBar.preCallAmount)
                actionBar.preAction = ""
        }
        function onCallAmountChanged() {
            // NO preSelectEnabled=true here: callAmountChanged fires on
            // every computeCallAndRaiseAmounts() call (onRefreshSet/Pot/Cash)
            // with stale values as well → only release it in onRefreshActionTriggered.
            // We deliberately do NOT run the pre-action safety check
            // here any more but in onRefreshActionTriggered (see above) —
            // otherwise blind posts (callAmount 0→SB→BB) deleted every
            // UTG pre-action.
            actionBar.syncRaiseAmount()
        }
        function onMinRaiseAmountChanged() {
            // Widget parity (provideMyActions): if a bet/raise was noted and
            // the prepared amount now lies BELOW the new minimum – for instance
            // because an opponent has (re-)raised – the preselection is DISCARDED,
            // not silently raised to the new (higher) minimum and
            // executed anyway. In the Qt widgets client:
            //   int lastBetValue = <old button amount>;
            //   if (lastBetValue < slider->minimum() && betRaise->isChecked())
            //       uncheckMyButtons();  // clear the preselection
            // raiseAmount corresponds to the button amount that was prepared last and
            // is only set anew afterwards via syncRaiseAmount() – so the check
            // still sees the OLD value here (= lastBetValue).
            if (actionBar.preAction === "raise"
                && (!actionBar.raiseAvailable
                    || actionBar.raiseAmount < GameTable.minRaiseAmount)) {
                // console.log("[ACTDBG] raise preselection discarded: prepared",
                            // actionBar.raiseAmount, "< new minimum",
                            // GameTable.minRaiseAmount, "(an opponent has (re-)raised)")
                actionBar.preAction = ""
            }
            actionBar.syncRaiseAmount()
        }
        function onMaxRaiseAmountChanged() {
            // As in the widgets client: if only the maximum changes (e.g. my cash
            // after the sets are collected), the preselection is NOT discarded – the amount
            // is merely limited to the valid maximum (syncRaiseAmount).
            // It is only discarded when no raise is possible any more.
            if (actionBar.preAction === "raise" && !actionBar.raiseAvailable)
                actionBar.preAction = ""
            actionBar.syncRaiseAmount()
        }
        function onActionRejected(requiredAmount, expectedAmount) {
            // The check/call was discarded in C++ because the engine meanwhile
            // demands more than stood on the button (an opponent has bet/raised/gone
            // all-in). NOTHING was sent – it is still my turn:
            //  • discard the preselection (it meant a different amount),
            //  • release the buttons again (fireAction had locked them),
            //  • lock the call button briefly (AccidentallyCallBlocker), so that
            //    a second click that is already on its way does not call the
            //    new, higher amount after all.
            // Auto check/fold folds on its own after the rejection (runAutoAction),
            // auto check/call deliberately calls any amount (−1) and is never rejected.
            actionBar.preAction = ""
            actionBar.preSelectEnabled = true
            if (actionBar.accidentalCallBlockerEnabled) {
                actionBar.callBlocked = true
                callBlockTimer.restart()
            }
        }
        function onCanActChanged() {
            if (GameTable.canAct)
                return
            // canAct bundles the permission to play (folded/all-in/no
            // cash) with the pure button release (m_myTurn ||
            // prevPlayerId != 0). A noted action may ONLY
            // lapse when I really cannot act in this hand
            // any more – NOT through the transient gating shortly before I am to
            // act (prevPlayerId == 0, m_myTurn still false), otherwise
            // the preselection (typically the BB option) is swallowed and
            // not executed on your own turn. The widgets client
            // does not discard the remembered action on the gating either.
            var me = GameTable.players.length > 0 ? GameTable.players[0] : null
            if (me && (me["folded"] === true || me["stack"] === 0))
                actionBar.preAction = ""
        }
    }

    Rectangle {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        // Desktop landscape: a small distance to the lower screen edge
        // (the table shows through below it). Phone (compactActions):
        // the panel flush at the lower screen edge.
        anchors.bottomMargin: actionBar.wide && !actionBar.compactActions ? 8 : 0
        anchors.horizontalCenter: parent.horizontalCenter
        width: actionBar.panelWidth
        color: Qt.rgba(0, 0, 0, 0.82)
        // Shrunk (landscape) as a slightly rounded panel.
        radius: actionBar.wide ? 10 : 0
    }

    Column {
        id: actionBarCol
        width: actionBar.panelWidth
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: 0

        // ── The raise area: it can be prepared permanently, the action only on your own turn ──
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

            // Row 1: the amount input (on the left) + the slider
            RowLayout {
                width: parent.width - 16
                spacing: 6

                // The amount input – to the left of the slider
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
                        // NO binding on raiseAmount: onTextChanged below writes
                        // raiseAmount back (a live update of the bet/raise button), so a
                        // binding would be a binding loop. Besides, the
                        // text must NOT be updated while typing –
                        // exactly that is what the Connections block further below does
                        // (only when the field does not have the activeFocus). Here only set the
                        // initial value.
                        Component.onCompleted: text = actionBar.raiseAmount.toString()
                        color: enabled ? "#FFFFFF" : "#8a8a8a"
                        font.family: Config.StaticData.loadedFont.font.family
                        font.pixelSize: 13
                        font.bold: true
                        horizontalAlignment: Qt.AlignHCenter
                        verticalAlignment: Qt.AlignVCenter
                        inputMethodHints: Qt.ImhDigitsOnly
                        validator: IntValidator { bottom: 0; top: 9999999 }
                        // Auto focus on your own turn: select the amount completely, so that
                        // the first digit that is typed REPLACES the suggested amount
                        // instead of extending it (parity with the Qt widgets client:
                        // spinBox_betValue->setFocus(); spinBox_betValue->selectAll()).
                        // Update the text explicitly beforehand: if the field already had the
                        // activeFocus (e.g. from the last turn), the
                        // Connections binding below deliberately did not update it.
                        function focusAndSelectAll() {
                            // Widget parity: if the user is currently typing in another
                            // text field (the chat input), do NOT tear the focus away – in the
                            // widgets client both call sites hang off
                            // `!lineEdit_ChatInput->hasFocus()` or `text() == ""`.
                            // Instead of passing both ChatBox instances in here,
                            // we generically check the item that currently has the focus: only
                            // text inputs have selectAll().
                            var af = Window.activeFocusItem
                            if (af && af !== raiseAmountInput && af.selectAll !== undefined)
                                return
                            text = actionBar.raiseAmount.toString()
                            forceActiveFocus()
                            selectAll()
                        }
                        // A live update of the bet/raise button while typing –
                        // analogous to spinBoxBetValueChanged() in the Qt widgets client.
                        onTextChanged: {
                            var v = parseInt(text)
                            if (!isNaN(v) && actionBar.raiseAvailable)
                                actionBar.raiseAmount = actionBar.clampRaiseAmount(v)
                        }
                        onAccepted: {
                            if (!actionBar.raiseAvailable)
                                return
                            var v = parseInt(text)
                            if (isNaN(v))
                                return
                            // Enter in the raise field triggers bet/raise (as in the
                            // Qt widgets client: Enter with the amount focused) –
                            // but ONLY with a sensible amount. A value that is too high
                            // would otherwise be clamped silently to the remaining stack (= all-in)
                            // and fired surprisingly as an all-in on Enter
                            // (a player report: "typed 300 → all-in"). If the
                            // value entered lies outside [min,max], correct the field visibly
                            // to the real, clamped amount and do NOT
                            // trigger – the user deliberately confirms the amount that is now
                            // visible with a second Enter.
                            if (v < actionBar.raiseMinAmount || v > actionBar.raiseMaxAmount) {
                                actionBar.raiseAmount = actionBar.clampRaiseAmount(v)
                                text = actionBar.raiseAmount.toString()
                                selectAll()
                                return
                            }
                            actionBar.raiseAmount = v
                            actionBar.clickAction("raise")
                        }
                        // Give up the focus as soon as the field is locked (the turn is over →
                        // min/maxRaiseAmount become 0 → raiseAvailable false).
                        //
                        // Necessary because enabled=false only takes the activeFocus but
                        // leaves the focus property true: on the next turn of your own
                        // raiseAvailable activates the field again, and the focus scope
                        // gives it the activeFocus back by itself. On touch devices
                        // (iPad/iPhone) that pulls the on-screen keyboard up unasked on
                        // EVERY turn from the first typing into the field on – even when you
                        // only want to check. Being focused again without any action of the user
                        // is wrong on the desktop as well (keyboard input then lands
                        // in the amount field instead of at the action shortcuts).
                        // The intended auto focus comes exclusively from the setting
                        // EnableBetInputFocusSwitch (onMyTurnChanged above) – it calls
                        // forceActiveFocus() only AFTER the reactivation and stays effective.
                        onEnabledChanged: {
                            if (!enabled)
                                focus = false
                        }
                        // The text stays in sync with raiseAmount (from the slider/% buttons)
                        onActiveFocusChanged: {
                            if (activeFocus) {
                                // Focus received (a click, Tab OR the auto focus): select the
                                // suggested default amount completely,
                                // so that the first digit that is typed REPLACES it instead of being
                                // appended to it. Without that, on a mouse click the
                                // cursor hangs in the existing value and "300" is appended
                                // (e.g. 40 → 40300) → clamped to the remaining stack at the next
                                // compute = a surprising all-in. That is
                                // the actual cause of the player report (see the
                                // widget hotfix 2.1.4: "the amount is added instead of
                                // overwritten"). So far focusAndSelectAll() only covered
                                // the auto focus path (EnableBetInputFocusSwitch) –
                                // the click case was missing. Qt.callLater: select only AFTER the
                                // click cursor placement, otherwise the mouse release lifts the
                                // selection again right away.
                                Qt.callLater(raiseAmountInput.selectAll)
                            } else {
                                // Do NOT keep the scope focus: if the field loses the
                                // activeFocus (e.g. because the user switches into the chat
                                // field), a `focus == true` would stay stuck in the focus scope.
                                // At the next re-layout or when a
                                // chat overlay (a focus scope of its own) closes, the
                                // scope would give the field the activeFocus back by itself – and
                                // it would intercept an Enter that the user meant for the
                                // chat (a player report: "typing in the chat, my
                                // turn comes → Enter triggers BET"). Release the focus here,
                                // then the field can never fetch the activeFocus back by itself;
                                // the (deliberate) auto focus still comes
                                // exclusively from focusAndSelectAll() on your own turn.
                                focus = false
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

            // Row 2: pot % buttons + all-in (flush) + the game mode dropdown (on the right)
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

                // All-in / show – flush with the pot buttons
                // Post-river: it shows a "show" button when the player can show their cards
                // voluntarily (temporarily as a replacement for all-in).
                Rectangle {
                    id: allInBtn
                    readonly property bool isShowMode: typeof GameTable !== "undefined" && GameTable && GameTable.canShowCards
                    // clickable: exactly the same condition as fold/check-call
                    // (actionsArmed). All-in is always permitted as soon as I am allowed
                    // to act – there is no separate "all-in available"
                    // amount as with the raise. Important: actionsArmed lets
                    // your own turn (myTurn) count on its own and does NOT additionally
                    // demand canAct. Otherwise the all-in button stayed locked in the
                    // time window in which the turn is only signalled via the
                    // action timer (startTimeout sets myTurn,
                    // canAct is then still stale=false) – while fold/call/raise
                    // were already clickable. The "show" mode (isShowMode)
                    // stays untouched by it.
                    readonly property bool armed: actionBar.actionsArmed
                    // The preselection mark only while the button is clickable as well.
                    readonly property bool preChecked: armed && actionBar.preAction === "allin"
                    // The theme graphic only in the normal all-in mode (not in the
                    // "show" mode, which keeps its own green look).
                    readonly property bool useTheme: !allInBtn.isShowMode
                                                     && StyleProvider && StyleProvider.allInButton !== ""
                    Layout.preferredWidth: 52
                    Layout.preferredHeight: actionBar.raiseRowHeight
                    // With a theme SVG the radius of the style, otherwise the fallback value.
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
                    // With a theme SVG the border of its own is dropped (the SVG brings its
                    // own); only the show/preselection state still draws one.
                    border.width: (isShowMode || allInBtn.preChecked) ? 2 : (allInBtn.useTheme ? 0 : 1)
                    scale: (allInArea.pressed && (allInBtn.armed || isShowMode)) ? 0.95 : 1.0
                    Behavior on scale { NumberAnimation { duration: 90; easing.type: Easing.OutQuad } }

                    // The theme SVG background (without text). Inset by border.width,
                    // so that a possible state frame (gold) stays visible.
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
                    // Hover/press feedback above the theme SVG.
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
                    // Open the popup upwards – it prevents it from disappearing behind
                    // the Android navigation bar.
                    // IMPORTANT: popup.height, NOT popup.implicitHeight – the
                    // implicit height of the universal popup stays 0 (the height
                    // comes from the style: min(contentItem.implicitHeight,
                    // the window height − the margins)). With implicitHeight y was thus
                    // always 0, the popup opened DOWNWARDS and was only pushed into the
                    // WINDOW bounds by Qt itself – on Android, however,
                    // the window reaches behind the navigation bar,
                    // so that the lowest entry (auto check/fold) could no longer be
                    // tapped in portrait.
                    popup.y: -popup.height

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

        // ── The action buttons: fold / check-call / bet-raise ─────────────
        // A dynamic caption + activation as in the Qt widgets client.
        Item {
            width: parent.width
            height: actionBar.actionRowHeight

            // A reusable action button with a gradient, dynamic text and a
            // preselection state (a golden frame = noted).
            component ActionButton: Item {
                id: ab
                property string actionKey: ""
                property string label: ""
                property color topColor: "#4080d8"
                property color bottomColor: "#1a3d8b"
                property color edgeColor: "#6aa0e8"
                // The text colour: given by the theme or contrasted
                // automatically (see StyleProvider). The default for the
                // fallback gradient button.
                property color textColor: "#F0F0F0"
                // The action button graphic of the current table style (only the look/
                // frame, without text). Empty → a fallback to the hardcoded
                // gradient button.
                property url themeSource: ""
                readonly property bool hasTheme: ab.themeSource != ""
                property bool armed: false   // clickable: your own turn OR a preselection is possible
                property bool highlight: false   // highlight the primary action (raise)
                // The call blocker: only the call button is locked briefly after an amount/
                // caption change (see actionBar.callBlocked).
                readonly property bool blocked: ab.actionKey === "call" && actionBar.callBlocked
                readonly property bool myTurnNow: GameTable !== null
                                                  && (GameTable.myTurn || GameTable.awaitingMyAction)
                // The preselection mark (a golden frame/dot) only while the
                // button is clickable as well. Otherwise a stale preselection would stay
                // visible after the end of a round/hand, although the buttons are inactive in
                // the transition phase.
                readonly property bool preChecked: ab.armed && ab.actionKey !== "" && actionBar.preAction === ab.actionKey

                // onArmedChanged: console.log("[ACTDBG] armed", ab.actionKey, "→", ab.armed,
                                            // "(myTurn=", GameTable ? GameTable.myTurn : "n/a",
                                            // "canAct=", actionBar.canAct,
                                            // "preSel=", actionBar.preSelectEnabled, ")")

                opacity: (!ab.armed || ab.blocked) ? 0.4 : ((ab.myTurnNow || ab.preChecked) ? 1.0 : 0.72)

                // The theme SVG background (only the look, without baked-in text)
                Image {
                    anchors.fill: parent
                    visible: ab.hasTheme
                    source: ab.themeSource
                    sourceSize.width: Math.max(1, Math.round(ab.width))
                    sourceSize.height: Math.max(1, Math.round(ab.height))
                    fillMode: Image.Stretch
                    smooth: true
                }

                // The fallback gradient button (no theme SVG present)
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

                // The state frame ABOVE the theme SVG: highlight the preselection (gold) or
                // the primary action (raise). With the fallback that
                // is done by the border of the gradient rectangle above.
                //
                // The frame is drawn in the 168x43 drawing area of the button SVG
                // and stretched with the same (uneven) scaling
                // as the image. Only that way does it match the contour:
                // Image.Stretch turns the SVG corner into an ellipse, and a
                // Rectangle.radius in pixels can only ever be circular –
                // in portrait the button is considerably more squat than 168x43,
                // and there the two radii visibly diverged.
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
                    // In drawing area units; the scaling thins them down to
                    // the usual ~2 px.
                    border.width: 2.6
                    border.color: ab.preChecked ? "#FFD700" : ab.edgeColor
                }

                // Press feedback: a brief sinking in when tapping.
                scale: (abMouse.pressed && ab.armed) ? 0.96 : 1.0
                Behavior on scale { NumberAnimation { duration: 90; easing.type: Easing.OutQuad } }

                // Highlight raise as the primary action with a soft glow.
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

                // a small "noted" dot at the top right
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
                    // myTurnNow never gates the real turn; preSelectEnabled locks
                    // the preselection after your own turn/a round change. In the showdown
                    // and at the end of a round (canAct=false) always off.
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
                    highlight: true     // emphasize the primary action
                    armed: actionBar.actionsArmed && actionBar.raiseAvailable
                }
            }
        }
    }
}
