// CheckBox bound directly to an integer config key:
//   checked  = readConfigInt(configKey) != 0
//   onToggled→ writeConfigInt(configKey, checked ? 1 : 0)
// Replaces the read/write pair repeated all over the settings. `text` (and
// everything else) is set by the caller as on a normal CheckBox.
// defaultChecked applies as long as the SettingsManager is not (yet) available.
// The base is AppCheckBox – so the caption wraps instead of being cut off on
// the right in narrow windows.
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
