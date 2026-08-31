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
                text: "系统设置"
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
                        text: "界面外观"
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
                            text: "界面主色"
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

            // 工程交付说明
            AppCard {
                Layout.fillWidth: true
                padding: 18
                implicitHeight: extColumn.implicitHeight + 36

                ColumnLayout {
                    id: extColumn
                    anchors.fill: parent
                    spacing: 10

                    Text {
                        text: "工程使用流程"
                        color: Theme.textPrimary
                        font.family: Theme.fontFamily
                        font.pixelSize: 15
                        font.weight: Font.DemiBold
                    }

                    Text {
                        Layout.fillWidth: true
                        text: "按以下流程完成工程配置与结果复核。配置工程前应先关闭 Keil："
                        color: Theme.textSecondary
                        font.family: Theme.fontFamily
                        font.pixelSize: 12
                        wrapMode: Text.WordWrap
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        implicitHeight: extCodeText.implicitHeight + 22
                        radius: Theme.radius
                        color: Theme.surfaceRaised
                        border.width: 1
                        border.color: Theme.divider

                        Text {
                            id: extCodeText
                            anchors.fill: parent
                            anchors.margins: 11
                            text: "1. 载入 .uvprojx 或 .uvmpw 工程文件。\n"
                                  + "2. 在「工程功能」中核对构建目标与器件信息。\n"
                                  + "3. 将存储分析功能配置到工程，重新打开 Keil 执行 Build/Rebuild。\n"
                                  + "4. 在「存储占用分析」和「操作记录」中复核结果。"
                            color: Theme.textPrimary
                            font.family: Theme.fontFamily
                            font.pixelSize: 12
                            lineHeight: 1.45
                            wrapMode: Text.WordWrap
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: "工程集成仅修改 uvprojx 的 After Build 用户命令；写回前会校验 XML，"
                              + "从工程移除时会恢复原有命令。Intel HEX 合并在本机独立执行，不修改 Keil 工程。"
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
                        text: "版本与运行环境"
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
                            text: "软件版本"
                            color: Theme.textTertiary
                            font.family: Theme.fontFamily
                            font.pixelSize: 12
                        }

                        Text {
                            text: "KeilPlugin Studio v1.3.0"
                            color: Theme.textPrimary
                            font.family: Theme.fontFamily
                            font.pixelSize: 12
                            font.weight: Font.Medium
                        }

                        Text {
                            text: "运行框架"
                            color: Theme.textTertiary
                            font.family: Theme.fontFamily
                            font.pixelSize: 12
                        }

                        Text {
                            text: "Qt 6.6 / Qt Quick / C++17"
                            color: Theme.textPrimary
                            font.family: Theme.fontFamily
                            font.pixelSize: 12
                            font.weight: Font.Medium
                        }

                        Text {
                            text: "交付方式"
                            color: Theme.textTertiary
                            font.family: Theme.fontFamily
                            font.pixelSize: 12
                        }

                        Text {
                            text: "Windows 自包含目录 · 无 Python 依赖"
                            color: Theme.textPrimary
                            font.family: Theme.fontFamily
                            font.pixelSize: 12
                            font.weight: Font.Medium
                        }

                        Text {
                            text: "命令行接口"
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
                        text: "工程配置完成后，Keil 的 After Build 用户命令会调用本软件的命令行接口，"
                              + "并在 Build Output 窗口输出统一的 FLASH/RAM 存储分析报告。"
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
