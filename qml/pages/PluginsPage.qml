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
    property string pendingOperation: ""
    readonly property var pluginList: PluginManager.plugins
    readonly property var currentPlugin: pluginList.length > 0
                                         ? pluginList[Math.min(selectedIndex, pluginList.length - 1)]
                                         : null
    readonly property bool hookPlugin: currentPlugin
                                       && currentPlugin.capabilities.indexOf("install") >= 0
    readonly property bool runPlugin: currentPlugin
                                      && currentPlugin.capabilities.indexOf("run") >= 0
    // The main window keeps the navigation visible. At its minimum supported
    // size the content page is therefore much narrower than the window itself.
    // Switch the dense controls before they begin to force the detail card out
    // of the viewport.
    readonly property bool narrowLayout: width < 1100
    readonly property int pluginListWidth: narrowLayout ? 230 : 316
    readonly property int targetColumnWidth: narrowLayout ? 86 : 150

    component InfoField: ColumnLayout {
        property string label: ""
        property string value: ""
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        spacing: 2

        Text {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
            text: parent.label
            color: Theme.textTertiary
            font.family: Theme.fontFamily
            font.pixelSize: 10
        }

        Text {
            Layout.fillWidth: true
            Layout.minimumWidth: 0
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
            return I18n.t("Project integration", "工程集成")
        if (capability === "uninstall")
            return I18n.t("Reversible", "支持移除")
        if (capability === "report")
            return I18n.t("Report output", "报告输出")
        if (capability === "run")
            return I18n.t("Local tool", "本地执行")
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
        const updating = currentPlugin.needsMigrationCount > 0
        pendingOperation = "install"
        showConfirmDialog().openFor({
            titleText: updating ? I18n.t("Update Project Integration", "更新工程集成")
                                : I18n.t("Configure Project Integration", "配置工程集成"),
            messageText: updating
                         ? I18n.t("Consolidate legacy memory reports into one " + I18n.pluginName(currentPlugin.id, currentPlugin.name) + " report.",
                                  "将旧内存报告合并为「" + currentPlugin.name + "」统一输出")
                         : I18n.t("Add " + I18n.pluginName(currentPlugin.id, currentPlugin.name) + " to the selected project's post-build workflow.",
                                  "将「" + currentPlugin.name + "」接入所选工程的构建后处理流程"),
            noteText: (updating
                      ? I18n.t("Duplicate keil-build-viewer output will be disabled, and the original command will be kept for restoration.",
                               "将停用 keil-build-viewer 的重复输出，并保留移除集成时可恢复的原命令。")
                      : I18n.t("This operation updates post-build commands for all configurations in the project file.",
                               "本操作会更新工程文件中所有构建配置的构建后命令。"))
                      + I18n.t(" Close Keil / NovoStudio first so cached settings cannot overwrite this change.",
                                 "执行前请关闭 Keil / NovoStudio，避免其退出时以旧配置覆盖本次变更。"),
            accent: Theme.primary,
            riskText: I18n.t("Moderate", "中风险"),
            confirmText: updating ? I18n.t("Update", "确认更新") : I18n.t("Apply", "写入工程配置"),
            infoRows: [
                { label: I18n.t("Project scope", "工程范围"), value: project.sourcePath },
                { label: I18n.t("Component", "功能组件"), value: I18n.pluginName(currentPlugin.id, currentPlugin.name) + " v" + currentPlugin.version },
                { label: I18n.t("Configuration", "配置位置"), value: I18n.t("After Build user command", "After Build 用户命令") },
                { label: I18n.t("Execution", "执行时机"), value: I18n.t("After every Build/Rebuild", "每次 Build/Rebuild 完成后") }
            ]
        })
    }

    function requestUninstall() {
        if (!PluginManager.projectLoaded || !currentPlugin)
            return
        const project = PluginManager.project
        pendingOperation = "uninstall"
        showConfirmDialog().openFor({
            titleText: I18n.t("Remove Project Integration", "移除工程集成"),
            messageText: I18n.t("Remove " + I18n.pluginName(currentPlugin.id, currentPlugin.name) + " from the selected project's post-build workflow.",
                                "将从所选工程移除「" + currentPlugin.name + "」的构建后处理配置"),
            noteText: I18n.t("Existing post-build commands will be preserved. Close Keil / NovoStudio first so cached settings cannot overwrite this change.",
                              "移除时保留原有构建后命令。执行前请关闭 Keil / NovoStudio，避免其退出时覆盖本次变更。"),
            accent: Theme.warning,
            riskText: I18n.t("Moderate", "中风险"),
            confirmText: I18n.t("Remove", "确认移除"),
            infoRows: [
                { label: I18n.t("Project scope", "工程范围"), value: project.sourcePath },
                { label: I18n.t("Component", "功能组件"), value: I18n.pluginName(currentPlugin.id, currentPlugin.name) + " v" + currentPlugin.version },
                { label: I18n.t("Result", "变更结果"), value: I18n.t("Reports will no longer be printed after Build/Rebuild.", "Build/Rebuild 后不再自动输出报告") }
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
            Layout.preferredWidth: root.pluginListWidth
            // ScrollView 子项会在 polish 阶段抬升父布局的隐式宽度提示，
            // 显式钳制最大宽度保证左列固定宽、右侧详情占满剩余空间。
            Layout.minimumWidth: root.pluginListWidth
            Layout.maximumWidth: root.pluginListWidth
            Layout.fillHeight: true
            spacing: 10

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Text {
                    text: I18n.t("Project Tools", "工程功能")
                    color: Theme.textPrimary
                    font.family: Theme.fontFamily
                    font.pixelSize: 16
                    font.weight: Font.DemiBold
                }

                Item { Layout.fillWidth: true }

                StatusPill {
                    text: I18n.t(pluginList.length + " item(s)", pluginList.length + " 个")
                    accent: Theme.primary
                    dot: false
                    scaleFactor: 0.9
                }
            }

            ScrollView {
                id: pluginListScroll
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

                Column {
                    width: pluginListScroll.availableWidth
                    spacing: 8

                    Repeater {
                        model: pluginList

                        delegate: AppCard {
                            id: pluginCard
                            required property var modelData
                            required property int index
                            width: parent.width
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
                                        text: I18n.pluginName(pluginCard.modelData.id, pluginCard.modelData.name)
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
                                                  ? I18n.t("Build integration", "构建集成")
                                                  : I18n.t("Image tool", "镜像工具")
                                            accent: root.pluginAccent(pluginCard.modelData)
                                            dot: false
                                            scaleFactor: 0.82
                                        }

                                        StatusPill {
                                            visible: !root.narrowLayout
                                                     && pluginCard.modelData.installedCount >= 0
                                            text: pluginCard.modelData.needsMigrationCount > 0
                                                  ? I18n.t("Update needed", "待更新")
                                                  : pluginCard.modelData.targetCount > 0
                                                  && pluginCard.modelData.installedCount
                                                     === pluginCard.modelData.targetCount
                                                  ? I18n.t("Configured", "已配置")
                                                  : I18n.t("Not configured", "未配置")
                                            accent: pluginCard.modelData.needsMigrationCount > 0
                                                    ? Theme.warning
                                                    : pluginCard.modelData.targetCount > 0
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
            padding: root.narrowLayout ? 16 : 22

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

                            Text {
                                Layout.fillWidth: true
                                Layout.minimumWidth: 0
                                text: root.currentPlugin ? I18n.pluginName(root.currentPlugin.id, root.currentPlugin.name) : ""
                                color: Theme.textPrimary
                                font.family: Theme.fontFamily
                                font.pixelSize: 19
                                font.weight: Font.DemiBold
                                elide: Text.ElideRight
                            }

                            Flow {
                                Layout.fillWidth: true
                                Layout.preferredHeight: implicitHeight
                                spacing: 6

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
                                text: root.currentPlugin ? I18n.pluginSummary(root.currentPlugin.id, root.currentPlugin.summary) : ""
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
                                text: root.currentPlugin ? I18n.pluginDescription(root.currentPlugin.id, root.currentPlugin.description) : ""
                                color: Theme.textSecondary
                                font.family: Theme.fontFamily
                                font.pixelSize: 12
                                lineHeight: 1.35
                                wrapMode: Text.WordWrap
                            }

                            GridLayout {
                                Layout.fillWidth: true
                                columns: root.narrowLayout ? 2 : 4
                                columnSpacing: 18
                                rowSpacing: 8

                                InfoField { label: I18n.t("Component ID", "功能标识"); value: root.currentPlugin ? root.currentPlugin.id : "" }
                                InfoField { label: I18n.t("Version", "组件版本"); value: root.currentPlugin ? "v" + root.currentPlugin.version : "" }
                                InfoField { label: I18n.t("Maintainer", "维护者"); value: root.currentPlugin ? root.currentPlugin.author : "" }
                                InfoField {
                                    label: I18n.t("Integration", "集成方式")
                                    value: root.currentPlugin
                                           ? (root.currentPlugin.category === "hook"
                                              ? I18n.t("After Build (local integration)", "构建后处理（本地集成）")
                                              : I18n.t("Image processing (local tool)", "镜像处理（本地工具）"))
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
                                        text: I18n.t("Requirements", "实施要求")
                                        color: Theme.warning
                                        font.family: Theme.fontFamily
                                        font.pixelSize: 12
                                        font.weight: Font.DemiBold
                                    }

                                    Repeater {
                                        model: root.currentPlugin ? I18n.pluginNotes(root.currentPlugin.id, root.currentPlugin.notes) : []

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
                                        text: I18n.t("Target configuration status", "目标配置状态")
                                        color: Theme.textPrimary
                                        font.family: Theme.fontFamily
                                        font.pixelSize: 14
                                        font.weight: Font.DemiBold
                                    }

                                    StatusPill {
                                        visible: root.hookPlugin && root.currentPlugin
                                                 && root.currentPlugin.targetCount > 0
                                        text: root.currentPlugin
                                              ? (root.currentPlugin.needsMigrationCount > 0
                                                 ? I18n.t(root.currentPlugin.needsMigrationCount + " update(s) needed", root.currentPlugin.needsMigrationCount + " 个待更新")
                                                 : root.currentPlugin.installedCount + "/"
                                                   + root.currentPlugin.targetCount + I18n.t(" configured", " 已配置"))
                                              : ""
                                        accent: root.currentPlugin
                                                && root.currentPlugin.needsMigrationCount > 0
                                                ? Theme.warning
                                                : root.currentPlugin
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

                                        Text { Layout.preferredWidth: root.targetColumnWidth; Layout.minimumWidth: 0; text: I18n.t("Project file", "工程文件"); color: Theme.textTertiary; font.family: Theme.fontFamily; font.pixelSize: 11; font.weight: Font.DemiBold; elide: Text.ElideRight }
                                        Text { Layout.preferredWidth: root.targetColumnWidth; Layout.minimumWidth: 0; text: I18n.t("Build target", "构建目标"); color: Theme.textTertiary; font.family: Theme.fontFamily; font.pixelSize: 11; font.weight: Font.DemiBold; elide: Text.ElideRight }
                                        Text { Layout.preferredWidth: root.targetColumnWidth; Layout.minimumWidth: 0; text: I18n.t("Device", "目标器件"); color: Theme.textTertiary; font.family: Theme.fontFamily; font.pixelSize: 11; font.weight: Font.DemiBold; elide: Text.ElideRight }
                                        Item { Layout.fillWidth: true; Layout.minimumWidth: 0 }
                                        Text { Layout.preferredWidth: 74; text: I18n.t("Status", "配置状态"); color: Theme.textTertiary; font.family: Theme.fontFamily; font.pixelSize: 11; font.weight: Font.DemiBold; horizontalAlignment: Text.AlignHCenter }
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
                                                Layout.preferredWidth: root.targetColumnWidth
                                                Layout.minimumWidth: 0
                                                text: targetRow.modelData.project
                                                color: Theme.textPrimary
                                                font.family: Theme.fontFamily
                                                font.pixelSize: 12
                                                elide: Text.ElideMiddle
                                            }

                                            Text {
                                                Layout.preferredWidth: root.targetColumnWidth
                                                Layout.minimumWidth: 0
                                                text: targetRow.modelData.name
                                                color: Theme.textPrimary
                                                font.family: Theme.fontFamily
                                                font.pixelSize: 12
                                                font.weight: Font.Medium
                                                elide: Text.ElideRight
                                            }

                                            Text {
                                                Layout.preferredWidth: root.targetColumnWidth
                                                Layout.minimumWidth: 0
                                                text: targetRow.modelData.device
                                                color: Theme.textSecondary
                                                font.family: "Consolas"
                                                font.pixelSize: 11
                                                elide: Text.ElideRight
                                            }

                                            Item { Layout.fillWidth: true; Layout.minimumWidth: 0 }

                                            StatusPill {
                                                Layout.preferredWidth: 74
                                            text: {
                                                    const migrations = root.currentPlugin
                                                                       ? root.currentPlugin.perTargetNeedsMigration : []
                                                    if (migrations[targetRow.index])
                                                        return I18n.t("Update", "待更新")
                                                    const perTarget = root.currentPlugin
                                                                      ? root.currentPlugin.perTarget : []
                                                    return perTarget[targetRow.index]
                                                           ? I18n.t("Configured", "已配置")
                                                           : I18n.t("Not configured", "未配置")
                                                }
                                                accent: {
                                                    const migrations = root.currentPlugin
                                                                       ? root.currentPlugin.perTargetNeedsMigration : []
                                                    if (migrations[targetRow.index])
                                                        return Theme.warning
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
                                    text: I18n.t("Image merge parameters", "镜像合并参数")
                                    color: Theme.textPrimary
                                    font.family: Theme.fontFamily
                                    font.pixelSize: 14
                                    font.weight: Font.DemiBold
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8

                                    Text {
                                        Layout.preferredWidth: 96
                                        text: I18n.t("Bootloader image", "Bootloader 镜像")
                                        color: Theme.textSecondary
                                        font.family: Theme.fontFamily
                                        font.pixelSize: 12
                                    }

                                    PathField {
                                        id: bootHexField
                                        Layout.fillWidth: true
                                        placeholder: I18n.t("Select a Bootloader Intel HEX file", "选择 Bootloader Intel HEX 文件")
                                    }

                                    AppButton {
                                        compact: true
                                        textPixelSize: 12
                                        text: I18n.t("Browse…", "浏览…")
                                        onClicked: bootHexDialog.open()
                                    }
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8

                                    Text {
                                        Layout.preferredWidth: 96
                                        text: I18n.t("Application image", "应用镜像")
                                        color: Theme.textSecondary
                                        font.family: Theme.fontFamily
                                        font.pixelSize: 12
                                    }

                                    PathField {
                                        id: appHexField
                                        Layout.fillWidth: true
                                        placeholder: I18n.t("Select an application Intel HEX file", "选择应用程序 Intel HEX 文件")
                                    }

                                    AppButton {
                                        compact: true
                                        textPixelSize: 12
                                        text: I18n.t("Browse…", "浏览…")
                                        onClicked: appHexDialog.open()
                                    }
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 8

                                    Text {
                                        Layout.preferredWidth: 96
                                        text: I18n.t("Output image", "输出镜像")
                                        color: Theme.textSecondary
                                        font.family: Theme.fontFamily
                                        font.pixelSize: 12
                                    }

                                    PathField {
                                        id: outputHexField
                                        Layout.fillWidth: true
                                        placeholder: I18n.t("Choose where to save the merged image", "选择合并镜像的保存路径")
                                    }

                                    AppButton {
                                        compact: true
                                        textPixelSize: 12
                                        text: I18n.t("Save as…", "保存到…")
                                        onClicked: outputHexDialog.open()
                                    }
                                }

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: 10

                                    AppButton {
                                        leadingSymbol: "⊕"
                                        text: I18n.t("Generate merged image", "生成合并镜像")
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
                                        text: I18n.t("Address overlap stops the operation; no output file is created.", "检测到地址重叠时立即中止，不生成输出文件")
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

                    Flow {
                        Layout.fillWidth: true
                        Layout.preferredHeight: implicitHeight
                        spacing: 10

                        AppButton {
                            visible: root.hookPlugin
                            leadingSymbol: "＋"
                            text: root.currentPlugin && root.currentPlugin.needsMigrationCount > 0
                                  ? I18n.t("Update Project Integration", "更新工程集成")
                                  : I18n.t("Configure Project", "配置到工程")
                            variant: "primary"
                            enabled: PluginManager.projectLoaded && root.currentPlugin
                                     && (root.currentPlugin.installedCount
                                         < root.currentPlugin.targetCount
                                         || root.currentPlugin.needsMigrationCount > 0)
                            onClicked: root.requestInstall()
                        }

                        AppButton {
                            visible: root.hookPlugin
                            leadingSymbol: "−"
                            text: I18n.t("Remove from Project", "从工程移除")
                            variant: "danger"
                            enabled: PluginManager.projectLoaded && root.currentPlugin
                                     && root.currentPlugin.installedCount > 0
                            onClicked: root.requestUninstall()
                        }

                        AppButton {
                            visible: root.hookPlugin
                            text: I18n.t("Recheck", "重新检测")
                            onClicked: PluginManager.reloadProject()
                        }

                        AppButton {
                            visible: root.hookPlugin
                            leadingSymbol: "◫"
                            text: I18n.t("View Memory Analysis", "查看存储分析")
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
                if (root.pendingOperation === "install")
                    PluginManager.installPlugin(root.currentPlugin.id)
                else if (root.pendingOperation === "uninstall")
                    PluginManager.uninstallPlugin(root.currentPlugin.id)
                root.pendingOperation = ""
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
        title: I18n.t("Select Bootloader Image", "选择 Bootloader 镜像")
        nameFilters: ["Intel HEX (*.hex)", I18n.t("All files (*)", "所有文件 (*)")]
        fileMode: FileDialog.OpenFile
        onAccepted: bootHexField.text = selectedFile
    }

    FileDialog {
        id: appHexDialog
        title: I18n.t("Select Application Image", "选择应用程序镜像")
        nameFilters: ["Intel HEX (*.hex)", I18n.t("All files (*)", "所有文件 (*)")]
        fileMode: FileDialog.OpenFile
        onAccepted: appHexField.text = selectedFile
    }

    FileDialog {
        id: outputHexDialog
        title: I18n.t("Save Merged Image", "保存合并镜像")
        nameFilters: ["Intel HEX (*.hex)", I18n.t("All files (*)", "所有文件 (*)")]
        fileMode: FileDialog.SaveFile
        onAccepted: outputHexField.text = selectedFile
    }
}
