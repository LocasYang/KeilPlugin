import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import KeilPluginStudio 1.0
import KeilPluginStudio.Backend 1.0

Item {
    id: root

    property var pickProject: null
    signal requestPluginView(string pluginId)
    signal toastRequested(string text, bool ok)

    property int selectedIndex: 0
    readonly property var pluginList: PluginManager.plugins
    readonly property var currentPlugin: pluginList.length > 0
                                         ? pluginList[Math.min(selectedIndex, pluginList.length - 1)]
                                         : null
    readonly property bool hookPlugin: currentPlugin
                                       && currentPlugin.capabilities.indexOf("install") >= 0
    readonly property bool runPlugin: currentPlugin
                                      && currentPlugin.capabilities.indexOf("run") >= 0

    component InfoField: ColumnLayout {
        property string label: ""
        property string value: ""
        spacing: 2

        Text {
            text: parent.label
            color: Theme.textTertiary
            font.family: Theme.fontFamily
            font.pixelSize: 10
        }

        Text {
            Layout.fillWidth: true
            text: parent.value
            color: Theme.textPrimary
            font.family: Theme.fontFamily
            font.pixelSize: 12
            font.weight: Font.Medium
            elide: Text.ElideRight
        }
    }

    component PathField: TextField {
        property string placeholder: ""
        height: 36
        color: Theme.textPrimary
        placeholderText: placeholder
        placeholderTextColor: Theme.textTertiary
        font.family: "Consolas"
        font.pixelSize: 11
        selectByMouse: true

        background: Rectangle {
            color: Theme.inputBackground
            radius: Theme.radiusSmall
            border.width: parent.activeFocus ? 2 : 1
            border.color: parent.activeFocus ? Theme.primary : Theme.border

            Behavior on color { ColorAnimation { duration: Theme.durationFast } }
            Behavior on border.color { ColorAnimation { duration: Theme.durationFast } }
        }
    }

    function capabilityAccent(capability) {
        if (capability === "install")
            return Theme.primary
        if (capability === "uninstall")
            return Theme.warning
        if (capability === "report")
            return Theme.cyan
        if (capability === "run")
            return Theme.purple
        return Theme.textTertiary
    }

    function capabilityLabel(capability) {
        if (capability === "install")
            return "可安装"
        if (capability === "uninstall")
            return "可卸载"
        if (capability === "report")
            return "可报告"
        if (capability === "run")
            return "独立工具"
        return capability
    }

    function pluginAccent(plugin) {
        if (!plugin)
            return Theme.textTertiary
        if (plugin.id === "keil-size-report")
            return Theme.primary
        if (plugin.id === "hex-merge")
            return Theme.cyan
        return Theme.primary
    }

    function openInstallConfirm() {
        requestInstall()
    }

    function requestInstall() {
        if (!PluginManager.projectLoaded || !currentPlugin)
            return
        const project = PluginManager.project
        showConfirmDialog().openFor({
            titleText: "安装插件",
            messageText: "即将把「" + currentPlugin.name + "」的构建钩子写入所选 Keil 工程",
            noteText: "安装会修改 uvprojx 文件中的 After Build 用户命令。"
                      + "请先关闭 Keil，否则 Keil 退出时会用内存中的旧配置覆盖本次修改。",
            accent: Theme.primary,
            riskText: "中风险",
            confirmText: "安装插件",
            infoRows: [
                { label: "目标工程", value: project.sourcePath },
                { label: "插件", value: currentPlugin.name + " v" + currentPlugin.version },
                { label: "写入位置", value: "<AfterMake> 用户命令槽位" },
                { label: "影响", value: "每次 Build/Rebuild 后自动执行" }
            ]
        })
    }

    function requestUninstall() {
        if (!PluginManager.projectLoaded || !currentPlugin)
            return
        const project = PluginManager.project
        showConfirmDialog().openFor({
            titleText: "卸载插件",
            messageText: "即将从所选 Keil 工程移除「" + currentPlugin.name + "」的构建钩子",
            noteText: "卸载会还原 After Build 用户命令（包括解除被包装的原命令）。"
                      + "请先关闭 Keil，否则修改会被覆盖。同时会清理旧版 Python 脚本钩子。",
            accent: Theme.warning,
            riskText: "中风险",
            confirmText: "卸载插件",
            infoRows: [
                { label: "目标工程", value: project.sourcePath },
                { label: "插件", value: currentPlugin.name + " v" + currentPlugin.version },
                { label: "影响", value: "编译后不再自动打印报告" }
            ]
        })
    }

    RowLayout {
        id: pageRow
        anchors.fill: parent
        spacing: 14

        // -------- Left: plugin list.
        ColumnLayout {
            id: leftColumn
            Layout.preferredWidth: 316
            // ScrollView 子项会在 polish 阶段抬升父布局的隐式宽度提示，
            // 显式钳制最大宽度保证左列固定宽、右侧详情占满剩余空间。
            Layout.maximumWidth: 316
            Layout.fillHeight: true
            spacing: 10

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Text {
                    text: "可用插件"
                    color: Theme.textPrimary
                    font.family: Theme.fontFamily
                    font.pixelSize: 16
                    font.weight: Font.DemiBold
                }

                Item { Layout.fillWidth: true }

                StatusPill {
                    text: pluginList.length + " 个"
                    accent: Theme.primary
                    dot: false
                    scaleFactor: 0.9
                }
            }

            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                Column {
                    width: 292
                    spacing: 8

                    Repeater {
                        model: pluginList

                        delegate: AppCard {
                            id: pluginCard
                            required property var modelData
                            required property int index
                            width: 292
                            height: 88
                            padding: 12
                            tint: root.selectedIndex === index
                                  ? Theme.alpha(root.pluginAccent(modelData), Theme.dark ? 0.16 : 0.08)
                                  : Theme.surface
                            outline: root.selectedIndex === index
                                     ? Theme.alpha(root.pluginAccent(modelData), 0.55)
                                     : Theme.border

                            RowLayout {
                                anchors.fill: parent
                                spacing: 10

                                Rectangle {
                                    Layout.preferredWidth: 40
                                    Layout.preferredHeight: 40
                                    radius: 10
                                    color: Theme.alpha(root.pluginAccent(pluginCard.modelData),
                                                       Theme.dark ? 0.20 : 0.10)
                                    border.width: 1
                                    border.color: Theme.alpha(root.pluginAccent(pluginCard.modelData), 0.28)

                                    Text {
                                        anchors.centerIn: parent
                                        text: pluginCard.modelData.category === "hook" ? "⌁" : "⊕"
                                        color: root.pluginAccent(pluginCard.modelData)
                                        font.family: Theme.fontFamily
                                        font.pixelSize: 19
                                        font.weight: Font.DemiBold
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 3

                                    Text {
                                        Layout.fillWidth: true
                                        text: pluginCard.modelData.name
                                        color: Theme.textPrimary
                                        font.family: Theme.fontFamily
                                        font.pixelSize: 14
                                        font.weight: Font.DemiBold
                                        elide: Text.ElideRight
                                    }

                                    Text {
                                        Layout.fillWidth: true
                                        text: "v" + pluginCard.modelData.version + " · "
                                              + pluginCard.modelData.author
                                        color: Theme.textTertiary
                                        font.family: Theme.fontFamily
                                        font.pixelSize: 11
                                        elide: Text.ElideRight
                                    }

                                    RowLayout {
                                        Layout.fillWidth: true
                                        spacing: 6

                                        StatusPill {
                                            text: pluginCard.modelData.category === "hook"
                                                  ? "工程钩子"
                                                  : "独立工具"
                                            accent: root.pluginAccent(pluginCard.modelData)
                                            dot: false
                                            scaleFactor: 0.82
                                        }

                                        StatusPill {
                                            visible: pluginCard.modelData.installedCount >= 0
                                            text: pluginCard.modelData.targetCount > 0
                                                  && pluginCard.modelData.installedCount
                                                     === pluginCard.modelData.targetCount
                                                  ? "已安装"
                                                  : "未安装"
                                            accent: pluginCard.modelData.targetCount > 0
                                                    && pluginCard.modelData.installedCount
                                                       === pluginCard.modelData.targetCount
                                                    ? Theme.success : Theme.textTertiary
                                            dot: false
                                            scaleFactor: 0.82
                                        }
                                    }
                                }
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: root.selectedIndex = pluginCard.index
                            }
                        }
                    }
                }
            }
        }

        // -------- Right: plugin detail.
        AppCard {
            Layout.fillWidth: true
            Layout.fillHeight: true
            padding: 22

            ColumnLayout {
                anchors.fill: parent
                spacing: 12

                Item { Layout.fillHeight: true; visible: !root.currentPlugin }

                ColumnLayout {
                    visible: root.currentPlugin
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 12

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        Rectangle {
                            Layout.preferredWidth: 46
                            Layout.preferredHeight: 46
                            radius: 12
                            color: Theme.alpha(root.pluginAccent(root.currentPlugin || null),
                                               Theme.dark ? 0.20 : 0.10)
                            border.width: 1
                            border.color: Theme.alpha(root.pluginAccent(root.currentPlugin || null), 0.28)

                            Text {
                                anchors.centerIn: parent
                                text: root.currentPlugin && root.currentPlugin.category === "hook"
                                      ? "⌁" : "⊕"
                                color: root.pluginAccent(root.currentPlugin || null)
                                font.family: Theme.fontFamily
                                font.pixelSize: 21
                                font.weight: Font.DemiBold
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 2

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: 8

                                Text {
                                    Layout.fillWidth: true
                                    text: root.currentPlugin ? root.currentPlugin.name : ""
                                    color: Theme.textPrimary
                                    font.family: Theme.fontFamily
                                    font.pixelSize: 19
                                    font.weight: Font.DemiBold
                                    elide: Text.ElideRight
                                }

                                Repeater {
                                    model: root.currentPlugin ? root.currentPlugin.capabilities : []

                                    delegate: StatusPill {
                                        required property var modelData
                                        text: root.capabilityLabel(modelData)
                                        accent: root.capabilityAccent(modelData)
                                        dot: false
                                        scaleFactor: 0.85
                                    }
                                }
                            }

                            Text {
                                Layout.fillWidth: true
                                text: root.currentPlugin ? root.currentPlugin.summary : ""
                                color: Theme.textSecondary
                                font.family: Theme.fontFamily
                                font.pixelSize: 12
                                wrapMode: Text.WordWrap
                            }
                        }
                    }

                    ScrollView {
                        id: detailScroll
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        clip: true
                        contentWidth: availableWidth
                        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                        ColumnLayout {
                            width: detailScroll.contentWidth
                            spacing: 12

                            Text {
                                Layout.fillWidth: true
                                text: root.currentPlugin ? root.currentPlugin.description : ""
                                color: Theme.textSecondary
                                font.family: Theme.fontFamily
                                font.pixelSize: 12
                                lineHeight: 1.35
                                wrapMode: Text.WordWrap
                            }

                            GridLayout {
                                Layout.fillWidth: true
                                columns: 4
                                columnSpacing: 18
                                rowSpacing: 8

                                InfoField { label: "插件标识"; value: root.currentPlugin ? root.currentPlugin.id : "" }
                                InfoField { label: "版本"; value: root.currentPlugin ? "v" + root.currentPlugin.version : "" }
                                InfoField { label: "作者"; value: root.currentPlugin ? root.currentPlugin.author : "" }
                                InfoField {
                                    label: "类型"
                                    value: root.currentPlugin
                                           ? (root.currentPlugin.category === "hook" ? "工程钩子（原生内置）" : "独立工具（原生内置）")
                                           : ""
                                }
                            }

                            Rectangle {
                                Layout.fillWidth: true
                                visible: root.currentPlugin && root.currentPlugin.notes.length > 0
                                implicitHeight: notesColumn.implicitHeight + 22
                                radius: Theme.radius
                                color: Theme.alpha(Theme.warning, Theme.dark ? 0.08 : 0.055)
                                border.width: 1
                                border.color: Theme.alpha(Theme.warning, 0.24)

                                ColumnLayout {
                                    id: notesColumn
                                    anchors.fill: parent
                                    anchors.margins: 11
                                    spacing: 5

                                    Text {
                                        text: "使用注意"
                                        color: Theme.warning
                                        font.family: Theme.fontFamily
                                        font.pixelSize: 12
                                        font.weight: Font.DemiBold
                                    }

                                    Repeater {
                                        model: root.currentPlugin ? root.currentPlugin.notes : []

                                        delegate: Text {
                                            required property var modelData
                                            Layout.fillWidth: true
                                            text: "·  " + modelData
                                            color: Theme.textSecondary
                                            font.family: Theme.fontFamily
                                            font.pixelSize: 11
                                            wrapMode: Text.WordWrap
                                            lineHeight: 1.25
                                        }
                                    }
                                }
                            }

                            // Per-target install status (hook plugins only).
                            ColumnLayout {
                                Layout.fillWidth: true
                                visible: root.hookPlugin
                                spacing: 6

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8

                                    Text {
                                        text: "工程内安装状态"
                                        color: Theme.textPrimary
                                        font.family: Theme.fontFamily
                                        font.pixelSize: 14
                                        font.weight: Font.DemiBold
                                    }

                                    StatusPill {
                                        visible: root.hookPlugin && root.currentPlugin
                                                 && root.currentPlugin.targetCount > 0
                                        text: root.currentPlugin
                                              ? root.currentPlugin.installedCount + "/"
                                                + root.currentPlugin.targetCount + " 已安装"
                                              : ""
                                        accent: root.currentPlugin
                                                && root.currentPlugin.installedCount
                                                   === root.currentPlugin.targetCount
                                                ? Theme.success : Theme.textTertiary
                                        dot: false
                                        scaleFactor: 0.85
                                    }

                                    Item { Layout.fillWidth: true }
                                }

                                Rectangle {
                                    Layout.fillWidth: true
                                    implicitHeight: statusHeaderRow.implicitHeight + 16
                                    color: Theme.surfaceRaised
                                    radius: Theme.radiusSmall

                                    RowLayout {
                                        id: statusHeaderRow
                                        anchors.fill: parent
                                        anchors.leftMargin: 12
                                        anchors.rightMargin: 12
                                        spacing: 12

                                        Text { Layout.preferredWidth: 150; text: "目标工程"; color: Theme.textTertiary; font.family: Theme.fontFamily; font.pixelSize: 11; font.weight: Font.DemiBold }
                                        Text { Layout.preferredWidth: 150; text: "Target"; color: Theme.textTertiary; font.family: Theme.fontFamily; font.pixelSize: 11; font.weight: Font.DemiBold }
                                        Text { Layout.preferredWidth: 150; text: "器件"; color: Theme.textTertiary; font.family: Theme.fontFamily; font.pixelSize: 11; font.weight: Font.DemiBold }
                                        Item { Layout.fillWidth: true }
                                        Text { Layout.preferredWidth: 74; text: "状态"; color: Theme.textTertiary; font.family: Theme.fontFamily; font.pixelSize: 11; font.weight: Font.DemiBold; horizontalAlignment: Text.AlignHCenter }
                                    }
                                }

                                Repeater {
                                    model: PluginManager.project ? PluginManager.project.targets : []

                                    delegate: Rectangle {
                                        id: targetRow
                                        required property var modelData
                                        required property int index
                                        Layout.fillWidth: true
                                        implicitHeight: 42
                                        color: index % 2 === 0 ? "transparent"
                                                               : Theme.alpha(Theme.textPrimary, Theme.dark ? 0.03 : 0.018)
                                        radius: Theme.radiusSmall

                                        RowLayout {
                                            anchors.fill: parent
                                            anchors.leftMargin: 12
                                            anchors.rightMargin: 12
                                            spacing: 12

                                            Text {
                                                Layout.preferredWidth: 150
                                                text: targetRow.modelData.project
                                                color: Theme.textPrimary
                                                font.family: Theme.fontFamily
                                                font.pixelSize: 12
                                                elide: Text.ElideMiddle
                                            }

                                            Text {
                                                Layout.preferredWidth: 150
                                                text: targetRow.modelData.name
                                                color: Theme.textPrimary
                                                font.family: Theme.fontFamily
                                                font.pixelSize: 12
                                                font.weight: Font.Medium
                                                elide: Text.ElideRight
                                            }

                                            Text {
                                                Layout.preferredWidth: 150
                                                text: targetRow.modelData.device
                                                color: Theme.textSecondary
                                                font.family: "Consolas"
                                                font.pixelSize: 11
                                                elide: Text.ElideRight
                                            }

                                            Item { Layout.fillWidth: true }

                                            StatusPill {
                                                Layout.preferredWidth: 74
                                                text: {
                                                    const perTarget = root.currentPlugin
                                                                      ? root.currentPlugin.perTarget : []
                                                    return perTarget[targetRow.index] ? "已安装" : "未安装"
                                                }
                                                accent: {
                                                    const perTarget = root.currentPlugin
                                                                      ? root.currentPlugin.perTarget : []
                                                    return perTarget[targetRow.index] ? Theme.success
                                                                                      : Theme.textTertiary
                                                }
                                                dot: false
                                                scaleFactor: 0.85
                                            }
                                        }
                                    }
                                }
                            }

                            // Standalone tool panel (hex merge).
                            ColumnLayout {
                                Layout.fillWidth: true
                                visible: root.runPlugin
                                spacing: 8

                                Text {
                                    text: "工具操作"
                                    color: Theme.textPrimary
                                    font.family: Theme.fontFamily
                                    font.pixelSize: 14
                                    font.weight: Font.DemiBold
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8

                                    Text {
                                        Layout.preferredWidth: 72
                                        text: "Boot HEX"
                                        color: Theme.textSecondary
                                        font.family: Theme.fontFamily
                                        font.pixelSize: 12
                                    }

                                    PathField {
                                        id: bootHexField
                                        Layout.fillWidth: true
                                        placeholder: "选择 Bootloader HEX 文件"
                                    }

                                    AppButton {
                                        compact: true
                                        textPixelSize: 12
                                        text: "浏览…"
                                        onClicked: bootHexDialog.open()
                                    }
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8

                                    Text {
                                        Layout.preferredWidth: 72
                                        text: "App HEX"
                                        color: Theme.textSecondary
                                        font.family: Theme.fontFamily
                                        font.pixelSize: 12
                                    }

                                    PathField {
                                        id: appHexField
                                        Layout.fillWidth: true
                                        placeholder: "选择应用程序 HEX 文件"
                                    }

                                    AppButton {
                                        compact: true
                                        textPixelSize: 12
                                        text: "浏览…"
                                        onClicked: appHexDialog.open()
                                    }
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8

                                    Text {
                                        Layout.preferredWidth: 72
                                        text: "输出 HEX"
                                        color: Theme.textSecondary
                                        font.family: Theme.fontFamily
                                        font.pixelSize: 12
                                    }

                                    PathField {
                                        id: outputHexField
                                        Layout.fillWidth: true
                                        placeholder: "合并结果保存路径"
                                    }

                                    AppButton {
                                        compact: true
                                        textPixelSize: 12
                                        text: "保存到…"
                                        onClicked: outputHexDialog.open()
                                    }
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 10

                                    AppButton {
                                        leadingSymbol: "⊕"
                                        text: "合并 HEX"
                                        variant: "primary"
                                        accent: Theme.cyan
                                        onClicked: PluginManager.runTool("hex-merge", {
                                            boot: bootHexField.text,
                                            app: appHexField.text,
                                            output: outputHexField.text
                                        })
                                    }

                                    Text {
                                        Layout.fillWidth: true
                                        text: "两段镜像地址重叠时会报错中止，不会写出半成品"
                                        color: Theme.textTertiary
                                        font.family: Theme.fontFamily
                                        font.pixelSize: 11
                                        elide: Text.ElideRight
                                    }
                                }
                            }
                        }
                    }

                    // -------- Action bar.
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: Theme.divider
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 10

                        AppButton {
                            visible: root.hookPlugin
                            leadingSymbol: "＋"
                            text: "安装插件"
                            variant: "primary"
                            enabled: PluginManager.projectLoaded && root.currentPlugin
                                     && root.currentPlugin.installedCount
                                        < root.currentPlugin.targetCount
                            onClicked: root.requestInstall()
                        }

                        AppButton {
                            visible: root.hookPlugin
                            leadingSymbol: "−"
                            text: "卸载插件"
                            variant: "danger"
                            enabled: PluginManager.projectLoaded && root.currentPlugin
                                     && root.currentPlugin.installedCount > 0
                            onClicked: root.requestUninstall()
                        }

                        AppButton {
                            visible: root.hookPlugin
                            text: "刷新状态"
                            onClicked: PluginManager.reloadProject()
                        }

                        AppButton {
                            visible: root.hookPlugin
                            leadingSymbol: "◫"
                            text: "查看占用报告"
                            variant: "outlined"
                            accent: Theme.cyan
                            enabled: PluginManager.projectLoaded
                            onClicked: root.requestPluginView(root.currentPlugin.id)
                        }

                        Item { Layout.fillWidth: true }
                    }
                }

                // Empty state when no project is loaded: still show plugin info,
                // but hint to open one first.
                Item { Layout.fillHeight: true }
            }
        }
    }

    // 确认弹窗懒加载：首次打开时才实例化，避免启动阶段布局告警。
    Loader {
        id: confirmDialogLoader
        active: false

        sourceComponent: ConfirmDialog {
            onOperationConfirmed: {
                if (titleText === "安装插件")
                    PluginManager.installPlugin(root.currentPlugin.id)
                else
                    PluginManager.uninstallPlugin(root.currentPlugin.id)
            }
        }
    }

    function showConfirmDialog() {
        if (!confirmDialogLoader.active)
            confirmDialogLoader.active = true
        return confirmDialogLoader.item
    }

    FileDialog {
        id: bootHexDialog
        title: "选择 Bootloader HEX"
        nameFilters: ["Intel HEX (*.hex)", "所有文件 (*)"]
        fileMode: FileDialog.OpenFile
        onAccepted: bootHexField.text = selectedFile
    }

    FileDialog {
        id: appHexDialog
        title: "选择应用程序 HEX"
        nameFilters: ["Intel HEX (*.hex)", "所有文件 (*)"]
        fileMode: FileDialog.OpenFile
        onAccepted: appHexField.text = selectedFile
    }

    FileDialog {
        id: outputHexDialog
        title: "保存合并结果"
        nameFilters: ["Intel HEX (*.hex)", "所有文件 (*)"]
        fileMode: FileDialog.SaveFile
        onAccepted: outputHexField.text = selectedFile
    }
}
