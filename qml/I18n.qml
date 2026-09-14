pragma Singleton

import QtQuick
import KeilPluginStudio.Backend 1.0

// Centralized UI language state. The default is English. PluginManager restores
// the saved preference before the main page is created.
QtObject {
    id: root

    readonly property string englishCode: "en"
    readonly property string chineseCode: "zh-CN"
    property string language: PluginManager.uiLanguage
    readonly property bool english: language === englishCode

    function setLanguage(code) {
        if (code !== englishCode && code !== chineseCode)
            return
        PluginManager.setUiLanguage(code)
    }

    function t(englishText, chineseText) {
        return english ? englishText : chineseText
    }

    function pluginName(pluginId, fallback) {
        if (!english)
            return fallback
        if (pluginId === "keil-size-report")
            return "Flash/RAM Memory Analysis"
        if (pluginId === "hex-merge")
            return "Intel HEX Image Merge"
        return fallback
    }

    function pluginViewLabel(pluginId, fallback) {
        if (!english)
            return fallback
        if (pluginId === "keil-size-report")
            return "Memory Analysis"
        return fallback
    }

    function pluginSummary(pluginId, fallback) {
        if (!english)
            return fallback
        if (pluginId === "keil-size-report")
            return "Parse the armlink .map file after a build and print one unified memory report in Keil Build Output."
        if (pluginId === "hex-merge")
            return "Validate and merge Bootloader and application Intel HEX files into a deliverable image."
        return fallback
    }

    function pluginDescription(pluginId, fallback) {
        if (!english)
            return fallback
        if (pluginId === "keil-size-report")
            return "Project integration writes an After Build command that invokes this application's command-line mode. After every Build/Rebuild it parses the generated .map file and prints a unified report with full source-file usage, image totals, Execution Region bars, and Top-N hotspots. Capacity limits are read from IROM()/IRAM() in the uvprojx <Cpu> section. When both After Build slots are occupied, the original command is wrapped and restored on removal. Existing keil-build-viewer hooks can be consolidated into one reversible report hook."
        if (pluginId === "hex-merge")
            return "Reads two Intel HEX files locally (Type 00/01/02/04/05), checks every byte for address overlap, then emits a merged HEX file using linear address records. Any address conflict stops the operation before an incomplete output is created."
        return fallback
    }

    function pluginNotes(pluginId, fallback) {
        if (!english)
            return fallback
        if (pluginId === "keil-size-report")
            return ["Close Keil before configuring or removing integration so its cached project settings cannot overwrite the change.",
                    "Confirm Target Options → Listing → Memory Map is enabled (enabled by default in Keil).",
                    "No Python environment is required; the After Build hook calls this application directly.",
                    "Use --flash/--ram when IROM/IRAM capacity is not declared by the project.",
                    "If keil-build-viewer is detected, choose Update Project Integration to avoid duplicate reports."]
        if (pluginId === "hex-merge")
            return ["Address ranges are checked before generation; overlap stops the operation without creating an output file.",
                    "The merged image uses Type 04 linear-address and Type 05 start-address records."]
        return fallback
    }
}
