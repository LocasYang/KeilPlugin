import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import KeilPluginStudio 1.0
import KeilPluginStudio.Backend 1.0

Item {
    id: root

    property var pickProject: null
    property int targetIndex: 0
    property var report: null

    readonly property var targets: PluginManager.project ? PluginManager.project.targets : []
    readonly property var targetOptions: {
        const options = []
        for (let i = 0; i < targets.length; ++i)
            options.push({ label: targets[i].project + " · " + targets[i].name, value: i })
        return options
    }

    function refresh() {
        if (targetIndex >= targets.length)
            targetIndex = 0
        report = PluginManager.targetReport(targetIndex)
    }

    onTargetsChanged: Qt.callLater(root.refresh)

    Component.onCompleted: refresh()

    function ratioAccent(ratio) {
        if (ratio >= 0.97)
            return Theme.error
        if (ratio >= 0.90)
            return Theme.warning
        return Theme.primary
    }

    function fmtKB(bytes) {
        return (bytes / 1024.0).toFixed(1) + " KB"
    }

    component UsageCard: AppCard {
        id: usageCard

        property string label: ""
        property real used: 0
        property real cap: 0

        implicitHeight: 128

        ColumnLayout {
            anchors.fill: parent
            spacing: 6

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Text {
                    text: usageCard.label
                    color: Theme.textSecondary
                    font.family: Theme.fontFamily
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                }

                Item { Layout.fillWidth: true }

                StatusPill {
                    visible: usageCard.cap > 0
                    text: (usageCard.used / usageCard.cap * 100).toFixed(1) + "%"
                    accent: root.ratioAccent(usageCard.cap > 0 ? usageCard.used / usageCard.cap : 0)
                    dot: false
                    scaleFactor: 0.85
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 6

                Text {
                    text: root.fmtKB(usageCard.used)
                    color: Theme.textPrimary
                    font.family: "Consolas"
                    font.pixelSize: 25
                    font.weight: Font.DemiBold
                }

                Text {
                    text: "/ " + (usageCard.cap > 0 ? root.fmtKB(usageCard.cap) : "n/a")
                    color: Theme.textTertiary
                    font.family: "Consolas"
                    font.pixelSize: 12
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 6
                radius: 3
                color: Theme.alpha(Theme.primary, Theme.dark ? 0.16 : 0.12)

                Rectangle {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: parent.width
                           * (usageCard.cap > 0
                              ? Math.max(0.0, Math.min(1.0, usageCard.used / usageCard.cap)) : 0)
                    radius: 3
                    color: root.ratioAccent(usageCard.cap > 0 ? usageCard.used / usageCard.cap : 0)

                    Behavior on width { NumberAnimation { duration: Theme.durationNormal } }
                }
            }

            Text {
                Layout.fillWidth: true
                text: usageCard.cap > 0
                      ? I18n.t("Remaining ", "剩余 ") + root.fmtKB(Math.max(0, usageCard.cap - usageCard.used))
                      : I18n.t("IROM/IRAM capacity was not found in the project configuration", "工程配置中未识别到 IROM/IRAM 容量")
                color: Theme.textTertiary
                font.family: Theme.fontFamily
                font.pixelSize: 11
            }
        }
    }

    component TopList: ColumnLayout {
        id: topList

        property string title: ""
        property bool byRom: true

        readonly property var rows: {
            if (!root.report || !root.report.components)
                return []
            const list = root.report.components.slice()
            list.sort(function(a, b) {
                const key = topList.byRom ? "rom" : "ram"
                return b[key] - a[key]
            })
            return list.slice(0, 6)
        }

        readonly property real maxValue: {
            let maxVal = 0
            for (let i = 0; i < topList.rows.length; ++i) {
                const value = topList.rows[i][topList.byRom ? "rom" : "ram"]
                if (value > maxVal)
                    maxVal = value
            }
            return maxVal > 0 ? maxVal : 1
        }

        spacing: 6

        Text {
            text: topList.title
            color: Theme.textSecondary
            font.family: Theme.fontFamily
            font.pixelSize: 12
            font.weight: Font.DemiBold
        }

        Repeater {
            model: topList.rows

            delegate: RowLayout {
                id: topRow
                required property var modelData
                Layout.fillWidth: true
                spacing: 8

                Text {
                    Layout.preferredWidth: 190
                    text: topRow.modelData.name
                    color: Theme.textPrimary
                    font.family: "Consolas"
                    font.pixelSize: 11
                    elide: Text.ElideMiddle
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 5
                    radius: 2.5
                    color: Theme.alpha(Theme.primary, Theme.dark ? 0.16 : 0.12)

                    Rectangle {
                        anchors.left: parent.left
                        anchors.top: parent.top
                        anchors.bottom: parent.bottom
                        width: parent.width
                               * Math.max(0.02, topRow.modelData[topList.byRom ? "rom" : "ram"]
                                          / topList.maxValue)
                        radius: 2.5
                        color: Theme.alpha(topList.byRom ? Theme.primary : Theme.cyan, 0.75)
                    }
                }

                Text {
                    Layout.preferredWidth: 64
                    text: root.fmtKB(topRow.modelData[topList.byRom ? "rom" : "ram"])
                    color: Theme.textSecondary
                    font.family: "Consolas"
                    font.pixelSize: 11
                    horizontalAlignment: Text.AlignRight
                }
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Text {
                text: I18n.t("Memory Analysis", "存储占用分析")
                color: Theme.textPrimary
                font.family: Theme.fontFamily
                font.pixelSize: 22
                font.weight: Font.DemiBold
            }

            Item { Layout.fillWidth: true }

            AppComboBox {
                id: targetSelector
                model: root.targetOptions
                textRole: "label"
                implicitWidth: 300
                enabled: root.targetOptions.length > 0
                onActivated: function(index) {
                    root.targetIndex = root.targetOptions[index].value
                    root.refresh()
                }
            }

            AppButton {
                compact: true
                textPixelSize: 12
                leadingSymbol: "↻"
                text: I18n.t("Refresh", "刷新分析")
                enabled: PluginManager.projectLoaded
                onClicked: {
                    PluginManager.reloadProject()
                    root.refresh()
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            DataUnavailableState {
                anchors.fill: parent
                visible: !PluginManager.projectLoaded
                symbol: "📂"
                title: I18n.t("No Keil project loaded", "尚未载入 Keil 工程")
                hint: I18n.t("Load a .uvprojx or .uvmpw project to analyze its latest build.", "载入 .uvprojx / .uvmpw 后可分析最近一次编译结果")
            }

            DataUnavailableState {
                anchors.fill: parent
                visible: PluginManager.projectLoaded
                         && (!root.report || !root.report.ok)
                symbol: "◫"
                accent: Theme.warning
                title: root.report && root.report.reason
                       ? I18n.t("Analysis unavailable", "分析结果不可用")
                       : I18n.t("No analysis result", "暂无分析结果")
                hint: root.report && root.report.reason ? root.report.reason : ""
            }

            ScrollView {
                id: reportScroll
                anchors.fill: parent
                visible: PluginManager.projectLoaded
                         && root.report && root.report.ok
                clip: true
                contentWidth: availableWidth
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                ColumnLayout {
                    width: reportScroll.contentWidth
                    spacing: 12

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        UsageCard {
                            Layout.fillWidth: true
                            label: I18n.t("FLASH image usage", "FLASH 镜像占用")
                            used: root.report && root.report.ok ? root.report.romUsed : 0
                            cap: root.report && root.report.ok ? root.report.flashCap : 0
                        }

                        UsageCard {
                            Layout.fillWidth: true
                            label: I18n.t("RAM runtime usage", "RAM 运行占用")
                            used: root.report && root.report.ok ? root.report.ramUsed : 0
                            cap: root.report && root.report.ok ? root.report.ramCap : 0
                        }
                    }

                    AppCard {
                        Layout.fillWidth: true
                        padding: 16
                        implicitHeight: detailColumn.implicitHeight + 32

                        ColumnLayout {
                            id: detailColumn
                            anchors.fill: parent
                            spacing: 8

                            Text {
                                text: I18n.t("Image composition (linker statistics)", "镜像组成（链接器统计）")
                                color: Theme.textPrimary
                                font.family: Theme.fontFamily
                                font.pixelSize: 14
                                font.weight: Font.DemiBold
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                Repeater {
                                    model: [
                                        { key: "Code", value: root.report && root.report.ok ? root.report.code : 0 },
                                        { key: "RO-data", value: root.report && root.report.ok ? root.report.roData : 0 },
                                        { key: "RW-data", value: root.report && root.report.ok ? root.report.rwData : 0 },
                                        { key: "ZI-data", value: root.report && root.report.ok ? root.report.ziData : 0 }
                                    ]

                                    delegate: Rectangle {
                                        id: detailCell
                                        required property var modelData
                                        Layout.fillWidth: true
                                        implicitHeight: 54
                                        radius: Theme.radiusSmall
                                        color: Theme.surfaceRaised
                                        border.width: 1
                                        border.color: Theme.divider

                                        ColumnLayout {
                                            anchors.centerIn: parent
                                            spacing: 2

                                            Text {
                                                Layout.alignment: Qt.AlignHCenter
                                                text: detailCell.modelData.key
                                                color: Theme.textTertiary
                                                font.family: Theme.fontFamily
                                                font.pixelSize: 10
                                            }

                                            Text {
                                                Layout.alignment: Qt.AlignHCenter
                                                text: detailCell.modelData.value.toLocaleString() + " B"
                                                color: Theme.textPrimary
                                                font.family: "Consolas"
                                                font.pixelSize: 13
                                                font.weight: Font.DemiBold
                                            }
                                        }
                                    }
                                }
                            }

                            Text {
                                Layout.fillWidth: true
                                visible: root.report && root.report.ok
                                         && (root.report.code + root.report.roData
                                             + root.report.rwData > root.report.romUsed)
                                text: I18n.t("RW-data is stored in the FLASH image; use the FLASH total for final image usage.", "RW-data 在 FLASH 中按镜像形式存放，最终占用以 FLASH 汇总值为准")
                                color: Theme.textTertiary
                                font.family: Theme.fontFamily
                                font.pixelSize: 10
                                elide: Text.ElideRight
                            }
                        }
                    }

                    AppCard {
                        Layout.fillWidth: true
                        padding: 16
                        implicitHeight: topColumn.implicitHeight + 32

                        ColumnLayout {
                            id: topColumn
                            anchors.fill: parent
                            spacing: 10

                            Text {
                                text: I18n.t("Largest object files and libraries", "高占用目标文件与库")
                                color: Theme.textPrimary
                                font.family: Theme.fontFamily
                                font.pixelSize: 14
                                font.weight: Font.DemiBold
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 24

                                TopList {
                                    Layout.fillWidth: true
                                    title: I18n.t("FLASH ranking", "FLASH 排名")
                                    byRom: true
                                }

                                TopList {
                                    Layout.fillWidth: true
                                    title: I18n.t("RAM ranking", "RAM 排名")
                                    byRom: false
                                }
                            }
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: root.report && root.report.ok
                              ? I18n.t("MAP file: ", "MAP 文件：") + root.report.mapPath : ""
                        color: Theme.textTertiary
                        font.family: "Consolas"
                        font.pixelSize: 10
                        elide: Text.ElideMiddle
                    }

                    Item { Layout.preferredHeight: 2 }
                }
            }
        }
    }
}
