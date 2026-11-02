import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.Window
import KeilPluginStudio 1.0
import KeilPluginStudio.Backend 1.0

ApplicationWindow {
    id: root

    width: 1280
    height: 800
    // 1100 × 700 is the smallest desktop canvas supported by the responsive
    // pages. Below this size the navigation and build actions no longer have
    // enough room to remain readable.
    minimumWidth: 1100
    minimumHeight: 700
    visible: true
    title: "KeilPlugin Studio"
    color: Theme.background
    font.family: Theme.fontFamily

    readonly property real resolutionRatio: Math.min(width / 1280.0, height / 800.0)
    readonly property real interfaceScale: Math.max(
                                               0.92,
                                               Math.min(
                                                   1.35,
                                                   1.0 + (resolutionRatio - 1.0) * 0.45))

    property int currentPage: 0
    readonly property bool projectLoaded: PluginManager.projectLoaded
    readonly property var projectTargets: PluginManager.project ? PluginManager.project.targets : []

    function syncNativeTitleBar() {
        WindowChrome.apply(
            root,
            Theme.dark,
            Theme.chrome,
            Theme.textPrimary,
            Theme.chrome)
    }

    property color toastAccent: Theme.primary

    function showToast(text, accent) {
        if (!text || text.length === 0)
            return
        root.toastAccent = accent || Theme.primary
        toastText.text = text
        toast.open()
        toastTimer.restart()
    }

    function openProjectDialog() {
        projectFileDialog.open()
    }

    // 功能组件 id -> StackLayout 页索引。组件若自带报告/输出视图：
    // 在 descriptor 声明 viewLabel，并在这里登记页索引，
    // 侧边栏“分析报告”子菜单会自动出现该项。
    property var pluginViewPages: ({ "keil-size-report": 1 })

    function pageForPlugin(pluginId) {
        return pluginViewPages[pluginId] !== undefined ? pluginViewPages[pluginId] : -1
    }

    // 有独立报告视图的功能组件（按注册表自动生成子菜单项）。
    readonly property var pluginViewItems: {
        const items = []
        const plugins = PluginManager.plugins
        for (let i = 0; i < plugins.length; ++i) {
            const plugin = plugins[i]
            const page = pageForPlugin(plugin.id)
            if (plugin.viewLabel && plugin.viewLabel.length > 0 && page >= 0)
                items.push({ id: plugin.id, label: plugin.viewLabel, page: page })
        }
        return items
    }

    palette.window: Theme.background
    palette.windowText: Theme.textPrimary
    palette.text: Theme.textPrimary
    palette.buttonText: Theme.textPrimary
    palette.button: Theme.surfaceRaised
    palette.base: Theme.inputBackground
    palette.highlight: Theme.primary
    palette.highlightedText: "#FFFFFF"

    FileDialog {
        id: projectFileDialog
        title: I18n.t("Select Keil / NovoStudio Project", "选择 Keil / NovoStudio 工程文件")
        nameFilters: [I18n.t("ARM projects (*.uvprojx *.uvmpw .cproject .project .novoproject)", "ARM 工程 (*.uvprojx *.uvmpw .cproject .project .novoproject)"),
                      I18n.t("All files (*)", "所有文件 (*)")]
        onAccepted: PluginManager.loadProject(selectedFile)
    }

    Connections {
        target: PluginManager

        function onLastResultChanged() {
            const result = PluginManager.lastResult
            if (!result || Object.keys(result).length === 0)
                return
            root.showToast(result.text, result.ok ? Theme.success : Theme.error)
        }
    }

    Connections {
        target: Theme
        function onDarkChanged() {
            Qt.callLater(root.syncNativeTitleBar)
        }
    }

    Item {
        anchors.fill: parent
        clip: true

        Rectangle {
            width: parent.width / root.interfaceScale
            height: parent.height / root.interfaceScale
            scale: root.interfaceScale
            transformOrigin: Item.TopLeft
            color: Theme.background

            Behavior on color { ColorAnimation { duration: Theme.durationNormal } }

            Rectangle {
                width: 520
                height: 520
                radius: 260
                x: parent.width - 390
                y: -280
                color: Theme.alpha(Theme.primary, Theme.dark ? 0.07 : 0.055)
            }

            Rectangle {
                width: 340
                height: 340
                radius: 170
                x: 185
                y: parent.height - 160
                color: Theme.alpha(Theme.cyan, Theme.dark ? 0.05 : 0.035)
            }

            ColumnLayout {
                anchors.fill: parent
                spacing: 0

                Rectangle {
                    id: globalHeader
                    z: 4
                    Layout.fillWidth: true
                    Layout.preferredHeight: 60
                    color: "transparent"
                    border.width: 0

                    gradient: Gradient {
                        orientation: Gradient.Vertical
                        GradientStop { position: 0.0; color: Theme.chrome }
                        GradientStop { position: 1.0; color: Theme.chromeBody }
                    }

                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.bottom
                        height: 9
                        gradient: Gradient {
                            orientation: Gradient.Vertical
                            GradientStop { position: 0.0; color: Theme.alpha(Theme.shadow, Theme.dark ? 0.16 : 0.075) }
                            GradientStop { position: 1.0; color: "transparent" }
                        }
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 18
                        anchors.rightMargin: 14
                        spacing: 12

                        Rectangle {
                            Layout.preferredWidth: 34
                            Layout.preferredHeight: 34
                            radius: 10
                            color: Theme.alpha(Theme.primary, Theme.dark ? 0.22 : 0.12)
                            border.width: 1
                            border.color: Theme.alpha(Theme.primary, 0.32)

                            Text {
                                anchors.centerIn: parent
                                text: "⌁"
                                color: Theme.primary
                                font.pixelSize: 19
                                font.weight: Font.DemiBold
                            }
                        }

                        ColumnLayout {
                            spacing: 0

                            Text {
                                text: "KeilPlugin Studio"
                                color: Theme.textPrimary
                                font.family: Theme.fontFamily
                                font.pixelSize: 16
                                font.weight: Font.DemiBold
                            }

                            Text {
                                text: I18n.t("Keil / NovoStudio build and delivery tool", "Keil / NovoStudio 构建与交付工具")
                                color: Theme.textTertiary
                                font.family: Theme.fontFamily
                                font.pixelSize: 10
                            }
                        }

                        Rectangle {
                            Layout.leftMargin: 10
                            Layout.preferredWidth: 1
                            Layout.preferredHeight: 24
                            color: Theme.divider
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 1

                            Text {
                                Layout.fillWidth: true
                                text: root.projectLoaded
                                      ? PluginManager.project.sourcePath
                                      : I18n.t("No ARM project loaded", "尚未载入 ARM 工程")
                                color: Theme.textSecondary
                                font.family: Theme.fontFamily
                                font.pixelSize: 12
                                font.weight: Font.Medium
                                elide: Text.ElideMiddle
                            }

                            Text {
                                Layout.fillWidth: true
                                visible: root.projectLoaded
                                text: root.projectTargets.length > 0
                                      ? I18n.t(root.projectTargets.length + " target(s) · " +
                                                 (PluginManager.project.kind === "uvmpw" ? "multi-project workspace"
                                                  : PluginManager.project.kind === "dir" ? "project directory"
                                                  : "single project"),
                                                 root.projectTargets.length + " 个目标 · " +
                                                 (PluginManager.project.kind === "uvmpw" ? "多工程工作区"
                                                  : PluginManager.project.kind === "dir" ? "工程目录"
                                                  : "单工程"))
                                      : I18n.t("Load a Keil project or NovoStudio .cproject file.",
                                                 "载入 Keil 工程或 NovoStudio 的 .cproject 文件")
                                color: Theme.textTertiary
                                font.family: Theme.fontFamily
                                font.pixelSize: 10
                                elide: Text.ElideRight
                            }
                        }

                        StatusPill {
                            text: root.projectLoaded
                                  ? I18n.t("Project loaded", "工程已载入")
                                  : I18n.t("No project", "未载入工程")
                            accent: root.projectLoaded ? Theme.success : Theme.textTertiary
                        }

                        AppButton {
                            compact: true
                            textPixelSize: 12
                            leadingSymbol: "📂"
                            text: I18n.t("Load Project", "载入工程")
                            variant: "primary"
                            onClicked: root.openProjectDialog()
                        }

                        AppButton {
                            compact: true
                            textPixelSize: 12
                            text: Theme.dark ? I18n.t("Light", "浅色") : I18n.t("Dark", "深色")
                            variant: "text"
                            onClicked: Theme.dark = !Theme.dark
                        }

                        AppButton {
                            compact: true
                            textPixelSize: 12
                            text: I18n.t("Accent", "主题色")
                            variant: "text"
                            onClicked: themePopup.open()
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 0

                    Rectangle {
                        Layout.preferredWidth: 236
                        Layout.fillHeight: true
                        color: Theme.alpha(Theme.sidebar, Theme.dark ? 0.96 : 0.92)
                        border.width: 1
                        border.color: Theme.divider

                        Behavior on color { ColorAnimation { duration: Theme.durationNormal } }

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 8

                            AppCard {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 96
                                padding: 14
                                tint: Theme.alpha(Theme.primary, Theme.dark ? 0.11 : 0.07)
                                outline: Theme.alpha(Theme.primary, 0.16)

                                ColumnLayout {
                                    anchors.fill: parent
                                    spacing: 6

                                    RowLayout {
                                        Layout.fillWidth: true

                                        StatusPill {
                                            text: I18n.t("Components", "交付组件")
                                            accent: Theme.primary
                                            dot: false
                                            scaleFactor: 0.9
                                        }

                                        Item { Layout.fillWidth: true }

                                        Text {
                                            text: I18n.t(PluginManager.plugins.length + " item(s)", PluginManager.plugins.length + " 个")
                                            color: Theme.textTertiary
                                            font.family: Theme.fontFamily
                                            font.pixelSize: 11
                                        }
                                    }

                                    Text {
                                        Layout.fillWidth: true
                                        text: I18n.t("Project Tools", "工程功能")
                                        color: Theme.textPrimary
                                        font.family: Theme.fontFamily
                                        font.pixelSize: 15
                                        font.weight: Font.DemiBold
                                    }

                                    Text {
                                        Layout.fillWidth: true
                                        text: I18n.t("Configure / Analyze / Build image", "配置 / 分析 / 镜像生成")
                                        color: Theme.textSecondary
                                        font.family: Theme.fontFamily
                                        font.pixelSize: 11
                                        elide: Text.ElideRight
                                    }
                                }

                                MouseArea {
                                    anchors.fill: parent
                                    cursorShape: Qt.PointingHandCursor
                                    onClicked: root.currentPage = 0
                                }
                            }

                            Text {
                                Layout.leftMargin: 8
                                Layout.topMargin: 2
                                text: I18n.t("NAVIGATION", "功能导航")
                                color: Theme.textTertiary
                                font.family: Theme.fontFamily
                                font.pixelSize: 11
                                font.weight: Font.DemiBold
                            }

                            Column {
                                width: 212
                                spacing: 3

                                NavItem {
                                    width: 212
                                    text: I18n.t("Project Tools", "工程功能")
                                    symbol: "⧉"
                                    current: root.currentPage === 0
                                    onClicked: root.currentPage = 0
                                }

                                Text {
                                    width: 212
                                    height: visible ? 26 : 0
                                    visible: root.pluginViewItems.length > 0
                                    text: "  " + I18n.t("Analysis", "分析报告")
                                    verticalAlignment: Text.AlignVCenter
                                    color: Theme.textTertiary
                                    font.family: Theme.fontFamily
                                    font.pixelSize: 11
                                    font.weight: Font.DemiBold
                                }

                                Repeater {
                                    model: root.pluginViewItems

                                    delegate: NavItem {
                                        required property var modelData
                                        width: 212
                                        subLevel: true
                                        symbol: "◫"
                                        text: I18n.pluginViewLabel(modelData.id, modelData.label)
                                        current: root.currentPage === modelData.page
                                        onClicked: root.currentPage = modelData.page
                                    }
                                }

                                Rectangle {
                                    width: 212
                                    height: 1
                                    color: Theme.divider
                                }

                                Repeater {
                                    model: [
                                        { label: I18n.t("Activity Log", "操作记录"), symbol: "▤", page: 2 },
                                        { label: I18n.t("Settings", "系统设置"), symbol: "⚙", page: 3 }
                                    ]

                                    delegate: NavItem {
                                        required property var modelData
                                        width: 212
                                        text: modelData.label
                                        symbol: modelData.symbol
                                        current: root.currentPage === modelData.page
                                        onClicked: root.currentPage = modelData.page
                                    }
                                }
                            }

                            Item { Layout.fillHeight: true }

                            Text {
                                Layout.fillWidth: true
                                text: I18n.t("v1.4.0 · Qt 6.6 · Local", "v1.4.0 · Qt 6.6 · 本地运行")
                                horizontalAlignment: Text.AlignHCenter
                                color: Theme.textTertiary
                                font.family: Theme.fontFamily
                                font.pixelSize: 10
                            }
                        }
                    }

                    Item {
                        Layout.fillWidth: true
                        Layout.fillHeight: true

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 18
                            spacing: 10

                            StackLayout {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                currentIndex: root.currentPage

                                PluginsPage {
                                    id: pluginsPage
                                    pickProject: root.openProjectDialog
                                    onRequestPluginView: function(pluginId) {
                                        root.currentPage = root.pageForPlugin(pluginId)
                                    }
                                    onToastRequested: function(text, ok) {
                                        root.showToast(text, ok ? Theme.success : Theme.error)
                                    }
                                }
                                ReportPage { pickProject: root.openProjectDialog }
                                LogPage { }
                                SettingsPage { }
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 28
                    color: Theme.surface
                    border.width: 1
                    border.color: Theme.divider

                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 14
                        anchors.rightMargin: 14

                        Rectangle {
                            width: 7
                            height: 7
                            radius: 4
                            color: root.projectLoaded ? Theme.success : Theme.textTertiary
                        }

                        Text {
                            text: root.projectLoaded
                                  ? I18n.t("Project loaded · " + root.projectTargets.length + " target(s)",
                                           PluginManager.statusMessage)
                                  : I18n.t("Ready · " + PluginManager.plugins.length + " component(s) loaded",
                                           PluginManager.statusMessage)
                            color: Theme.textSecondary
                            font.family: Theme.fontFamily
                            font.pixelSize: 11
                            elide: Text.ElideRight
                        }

                        Item { Layout.fillWidth: true }

                        Text {
                            text: I18n.t("Targets " + root.projectTargets.length + " · Components "
                                         + PluginManager.plugins.length,
                                         "Target " + root.projectTargets.length + " · 功能组件 "
                                         + PluginManager.plugins.length)
                            color: Theme.textTertiary
                            font.family: Theme.fontFamily
                            font.pixelSize: 11
                        }
                    }
                }
            }
        }
    }

    Popup {
        id: themePopup
        parent: Overlay.overlay
        x: root.width - width - 18
        y: 52
        width: 270
        height: 190
        padding: 18
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

        background: Rectangle {
            color: Theme.surface
            radius: Theme.radius
            border.width: 1
            border.color: Theme.border
        }

        contentItem: ColumnLayout {
            spacing: 12

            Text {
                text: I18n.t("Appearance", "界面主题")
                color: Theme.textPrimary
                font.family: Theme.fontFamily
                font.pixelSize: 15
                font.weight: Font.DemiBold
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                AppButton {
                    Layout.fillWidth: true
                    compact: true
                    text: I18n.t("Light", "浅色")
                    variant: !Theme.dark ? "primary" : "default"
                    onClicked: Theme.dark = false
                }

                AppButton {
                    Layout.fillWidth: true
                    compact: true
                    text: I18n.t("Dark", "深色")
                    variant: Theme.dark ? "primary" : "default"
                    onClicked: Theme.dark = true
                }
            }

            Text {
                text: I18n.t("Accent color", "界面主色")
                color: Theme.textSecondary
                font.family: Theme.fontFamily
                font.pixelSize: 12
            }

            Row {
                spacing: 10
                Repeater {
                    model: ["#1677FF", "#722ED1", "#13C2C2", "#52C41A", "#FAAD14", "#FF4D4F"]

                    Rectangle {
                        required property string modelData
                        width: 28
                        height: 28
                        radius: 14
                        color: modelData
                        border.width: Theme.primary.toString().toUpperCase() === modelData ? 3 : 0
                        border.color: Theme.textPrimary

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: Theme.setAccent(parent.color)
                        }
                    }
                }
            }
        }
    }

    // Result toast: center it in the window so it never covers the bottom
    // action row or status bar.
    Popup {
        id: toast
        parent: Overlay.overlay
        x: Math.round((parent.width - width) / 2)
        y: Math.round((parent.height - height) / 2)
        width: Math.min(350, root.width - 40)
        height: 72
        padding: 0
        modal: false
        closePolicy: Popup.NoAutoClose

        background: Rectangle {
            color: Theme.surface
            radius: Theme.radius
            border.width: 1
            border.color: Theme.border
        }

        contentItem: Item {
            Text {
                id: toastText
                anchors.fill: parent
                anchors.margins: 14
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text: ""
                color: root.toastAccent
                font.family: Theme.fontFamily
                font.pixelSize: 13
                font.weight: Font.Medium
                wrapMode: Text.WordWrap
                elide: Text.ElideRight
            }
        }
    }

    Timer {
        id: toastTimer
        interval: 2600
        onTriggered: toast.close()
    }

    Component.onCompleted: {
        const args = Qt.application.arguments
        for (let i = 0; i < args.length; ++i) {
            if (args[i].indexOf("--project=") === 0)
                PluginManager.loadProject(args[i].substring(10))
            else if (args[i].indexOf("--page=") === 0)
                root.currentPage = Math.max(0, Math.min(3, Number(args[i].substring(7))))
        }
        if (Qt.application.arguments.indexOf("--dialog=confirm-install") >= 0)
            Qt.callLater(function() { pluginsPage.openInstallConfirm() })
        if (Qt.application.arguments.indexOf("--dark") >= 0)
            Theme.dark = true
        Qt.callLater(root.syncNativeTitleBar)
    }

    onVisibilityChanged: {
        if (visible)
            Qt.callLater(root.syncNativeTitleBar)
    }
}
