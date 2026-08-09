// CheckBox, die direkt an einen Integer-Config-Key gebunden ist:
//   checked  = readConfigInt(configKey) != 0
//   onToggled→ writeConfigInt(configKey, checked ? 1 : 0)
// Ersetzt das überall wiederholte read/write-Paar in den Settings. `text` (und
// alles Übrige) setzt der Aufrufer wie bei einer normalen CheckBox.
// defaultChecked greift, solange der SettingsManager (noch) nicht verfügbar ist.
// Basis ist AppCheckBox – die Beschriftung bricht also um, statt in schmalen
// Fenstern rechts abgeschnitten zu werden.
AppCheckBox {
    id: control
    property string configKey: ""
    property bool defaultChecked: true

    checked: (typeof SettingsManager !== "undefined" && SettingsManager)
             ? SettingsManager.readConfigInt(configKey) !== 0 : defaultChecked
    onToggled: {
        if (typeof SettingsManager !== "undefined" && SettingsManager)
            SettingsManager.writeConfigInt(configKey, checked ? 1 : 0)
    }
}
