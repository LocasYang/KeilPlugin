import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import KeilPluginStudio 1.0
import KeilPluginStudio.Backend 1.0

Item {
    id: root

    ScrollView {
        id: settingsScroll
        anchors.fill: parent
        clip: true
        contentWidth: availableWidth
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            width: settingsScroll.contentWidth
            spacing: 12

            Text {
                text: "设置"
                color: Theme.textPrimary
                font.family: Theme.fontFamily
                font.pixelSize: 22
                font.weight: Font.DemiBold
            }

            // 主题外观
            AppCard {
                Layout.fillWidth: true
                padding: 18
                implicitHeight: appearanceColumn.implicitHeight + 36

                ColumnLayout {
                    id: appearanceColumn
                    anchors.fill: parent
                    spacing: 12

                    Text {
                        text: "外观"
                        color: Theme.textPrimary
                        font.family: Theme.fontFamily
                        font.pixelSize: 15
                        font.weight: Font.DemiBold
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        AppButton {
                            text: "浅色主题"
                            variant: !Theme.dark ? "primary" : "default"
                            onClicked: Theme.dark = false
                        }

                        AppButton {
                            text: "深色主题"
                            variant: Theme.dark ? "primary" : "default"
                            onClicked: Theme.dark = true
                        }

                        Item { Layout.fillWidth: true }

                        Text {
                            text: "主色令牌"
                            color: Theme.textSecondary
                            font.family: Theme.fontFamily
                            font.pixelSize: 12
                        }

                        Row {
                            spacing: 8
                            Repeater {
                                model: ["#1677FF", "#722ED1", "#13C2C2", "#52C41A", "#FAAD14", "#FF4D4F"]

                                Rectangle {
                                    required property string modelData
                                    width: 26
                                    height: 26
                                    radius: 13
                                    color: modelData
                                    border.width: Theme.primary.toString().toUpperCase()
                                                  === modelData ? 3 : 0
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
            }

            // 插件扩展说明
            AppCard {
                Layout.fillWidth: true
                padding: 18
                implicitHeight: extColumn.implicitHeight + 36

                ColumnLayout {
                    id: extColumn
                    anchors.fill: parent
                    spacing: 10

                    Text {
                        text: "扩展新插件"
                        color: Theme.textPrimary
                        font.family: Theme.fontFamily
                        font.pixelSize: 15
                        font.weight: Font.DemiBold
                    }

                    Text {
                        Layout.fillWidth: true
                        text: "插件以 C++ 原生方式内置于应用，新增插件无需改动任何界面代码："
                        color: Theme.textSecondary
                        font.family: Theme.fontFamily
                        font.pixelSize: 12
                        wrapMode: Text.WordWrap
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: extCodeText.implicitHeight + 22
                        radius: Theme.radius
                        color: Theme.dark ? "#0B0F1B" : "#0B0F1B"
                        border.width: 1
                        border.color: Theme.dark ? "#242B3E" : "#242B3E"

                        Text {
                            id: extCodeText
                            anchors.fill: parent
                            anchors.margins: 11
                            text: "class MyPlugin : public KeilPlugin {\n"
                                  + "    PluginDescriptor descriptor() const override;\n"
                                  + "    // installProject / uninstallProject / targetInstalled / runTool\n"
                                  + "};\n"
                                  + "// 在 PluginRegistry 构造中注册:\n"
                                  + "PluginRegistry::instance().registerPlugin(new MyPlugin);"
                            color: "#CDD6E7"
                            font.family: "Consolas"
                            font.pixelSize: 11
                            lineHeight: 1.3
                            wrapMode: Text.NoWrap
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: "插件通过能力声明（install / uninstall / report / run）描述自己，"
                              + "插件中心会自动渲染详情、安装状态表与操作按钮；"
                              + "「run」类插件可在详情页提供独立工具面板。"
                        color: Theme.textSecondary
                        font.family: Theme.fontFamily
                        font.pixelSize: 12
                        wrapMode: Text.WordWrap
                        lineHeight: 1.3
                    }
                }
            }

            // 关于
            AppCard {
                Layout.fillWidth: true
                padding: 18
                implicitHeight: aboutColumn.implicitHeight + 36

                ColumnLayout {
                    id: aboutColumn
                    anchors.fill: parent
                    spacing: 10

                    Text {
                        text: "关于"
                        color: Theme.textPrimary
                        font.family: Theme.fontFamily
                        font.pixelSize: 15
                        font.weight: Font.DemiBold
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        columns: 2
                        columnSpacing: 24
                        rowSpacing: 8

                        Text {
                            text: "应用"
                            color: Theme.textTertiary
                            font.family: Theme.fontFamily
                            font.pixelSize: 12
                        }

                        Text {
                            text: "KeilPlugin Studio v0.1.0"
                            color: Theme.textPrimary
                            font.family: Theme.fontFamily
                            font.pixelSize: 12
                            font.weight: Font.Medium
                        }

                        Text {
                            text: "运行环境"
                            color: Theme.textTertiary
                            font.family: Theme.fontFamily
                            font.pixelSize: 12
                        }

                        Text {
                            text: "Qt 6.6 / Qt Quick · 原生 C++（无 Python 依赖）"
                            color: Theme.textPrimary
                            font.family: Theme.fontFamily
                            font.pixelSize: 12
                            font.weight: Font.Medium
                        }

                        Text {
                            text: "界面语言"
                            color: Theme.textTertiary
                            font.family: Theme.fontFamily
                            font.pixelSize: 12
                        }

                        Text {
                            text: "设计令牌与 PowerConfig Studio 保持一致"
                            color: Theme.textPrimary
                            font.family: Theme.fontFamily
                            font.pixelSize: 12
                            font.weight: Font.Medium
                        }

                        Text {
                            text: "命令行"
                            color: Theme.textTertiary
                            font.family: Theme.fontFamily
                            font.pixelSize: 12
                        }

                        Text {
                            text: "--size-report <工程> [--top N] [--flash 230K] [--ram 64K] [--quiet]"
                            color: Theme.textPrimary
                            font.family: "Consolas"
                            font.pixelSize: 11
                            font.weight: Font.Medium
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: "安装插件后，Keil 的 After Build 用户命令会调用本应用命令行模式，"
                              + "在 Build Output 窗口自动打印 Flash/RAM 占用报告。"
                        color: Theme.textSecondary
                        font.family: Theme.fontFamily
                        font.pixelSize: 12
                        wrapMode: Text.WordWrap
                        lineHeight: 1.3
                    }
                }
            }

            Item { Layout.preferredHeight: 4 }
        }
    }
}
