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
            return "Unified Flash/RAM reports for Keil armlink and NovoStudio GNU Arm builds."
        if (pluginId === "hex-merge")
            return "Validate and merge Bootloader and application Intel HEX files into a deliverable image."
        return fallback
    }

    function pluginDescription(pluginId, fallback) {
        if (!english)
            return fallback
        if (pluginId === "keil-size-report")
            return "Supports Keil .uvprojx and NovoStudio .cproject projects. NovoStudio reports use matching MAP + ELF files and linker MEMORY capacities, including code executed in RAM and heap/stack reservations. The report is appended to Post-build steps, preserving existing BIN/disassembly commands. Keil integration uses After Build and IROM/IRAM capacities as before. Both display file usage, build deltas, image totals, memory regions and Top-N hotspots."
        if (pluginId === "hex-merge")
            return "Reads two Intel HEX files locally (Type 00/01/02/04/05), checks every byte for address overlap, then emits a merged HEX file using linear address records. Any address conflict stops the operation before an incomplete output is created."
        return fallback
    }

    function pluginNotes(pluginId, fallback) {
        if (!english)
            return fallback
        if (pluginId === "keil-size-report")
            return ["Close Keil / NovoStudio before configuring or removing integration so cached settings cannot overwrite the change.",
                    "NovoStudio requires matching MAP and ELF files; Debug/Release keep separate delta histories.",
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
