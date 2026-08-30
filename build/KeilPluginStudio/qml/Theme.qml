pragma Singleton

import QtQuick

QtObject {
    id: theme

    property bool dark: false
    property color primary: "#1677FF"
    readonly property color success: "#52C41A"
    readonly property color warning: "#FAAD14"
    readonly property color error: "#FF4D4F"
    readonly property color cyan: "#13C2C2"
    readonly property color purple: "#722ED1"

    readonly property color background: dark ? "#090A2D" : "#F3F5FB"
    readonly property color backgroundSecondary: dark ? "#0E102F" : "#EAEEF8"
    readonly property color chrome: dark ? "#0E1031" : "#EEF3FC"
    readonly property color chromeBody: dark ? "#15162F" : "#F8FAFF"
    readonly property color sidebar: dark ? "#09072A" : "#EFF1FA"
    readonly property color surface: dark ? "#17182F" : "#FFFFFF"
    readonly property color surfaceRaised: dark ? "#1D1E39" : "#F4F6FB"
    readonly property color textPrimary: dark ? "#F3F3F3" : "#171A25"
    readonly property color textSecondary: dark ? "#A5A7BE" : "#50586A"
    readonly property color textTertiary: dark ? "#74778F" : "#727B8E"
    readonly property color border: dark ? alpha("#FFFFFF", 0.10) : alpha("#172033", 0.16)
    readonly property color divider: dark ? alpha("#FFFFFF", 0.07) : alpha("#172033", 0.10)
    readonly property color inputBackground: dark ? alpha("#FFFFFF", 0.045) : "#F1F3F8"
    readonly property color hoverFill: alpha(primary, dark ? 0.18 : 0.10)
    readonly property color selectedFill: alpha(primary, dark ? 0.28 : 0.15)
    readonly property color shadow: dark ? alpha("#000000", 0.34) : alpha("#47536B", 0.16)

    readonly property int radiusSmall: 6
    readonly property int radius: 10
    readonly property int radiusLarge: 16
    readonly property int durationFast: 100
    readonly property int durationNormal: 200
    readonly property string fontFamily: "Microsoft YaHei UI"

    function alpha(value: color, opacity: real): color {
        return Qt.rgba(value.r, value.g, value.b, opacity)
    }

    function setAccent(value) {
        primary = value
    }
}
