/*!
 * This file is part of WatchFlower.
 * COPYRIGHT (C) 2020 Emeric Grange - All Rights Reserved
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * \date      2020
 * \author    Emeric Grange <emeric.grange@gmail.com>
 */

import QtQuick 2.9
import QtQuick.Controls 2.2

import ThemeEngine 1.0

Item {
    id: permissionsScreen
    width: 480
    height: 640
    anchors.fill: parent
    anchors.leftMargin: screenLeftPadding
    anchors.rightMargin: screenRightPadding

    Rectangle {
        id: rectangleHeader
        color: Theme.colorForeground
        height: 80
        z: 5

        //visible: (Qt.platform.os === "android" || Qt.platform.os === "ios")

        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right

        Text {
            id: textTitle
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.top: parent.top
            anchors.topMargin: 12

            text: qsTr("Permissions")
            font.bold: true
            font.pixelSize: Theme.fontSizeTitle
            color: Theme.colorText
        }

        Text {
            id: textSubtitle
            anchors.left: parent.left
            anchors.leftMargin: 16
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 14

            text: qsTr("What are we doing")
            color: Theme.colorSubText
            font.pixelSize: 18
        }
    }

    ////////////////////////////////////////////////////////////////////////////

    ScrollView {
        id: scrollView
        contentWidth: -1

        anchors.top: (Qt.platform.os !== "android" && Qt.platform.os !== "ios") ? rectangleHeader.bottom : parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom

        Column {
            anchors.fill: parent
            anchors.leftMargin: 16
            anchors.rightMargin: 16

            topPadding: 8
            bottomPadding: 8
            spacing: 8

            // Android only
            //visible: (Qt.platform.os === "android")

            ////////

            Item {
                id: element_gps
                height: 48
                anchors.right: parent.right
                anchors.left: parent.left
/*
                ImageSvg {
                    id: image_gps
                    width: 24
                    height: 24
                    anchors.left: parent.left
                    anchors.leftMargin: 16
                    anchors.verticalCenter: parent.verticalCenter

                    color: Theme.colorText
                    source: "qrc:/assets/icons_material/baseline-bluetooth_disabled-24px.svg"
                }
*/
                Text {
                    id: text_gps
                    height: 16
                    anchors.left: parent.left
                    anchors.leftMargin: 0
                    anchors.right: switch_gps.left
                    anchors.rightMargin: 16
                    anchors.verticalCenter: parent.verticalCenter

                    text: qsTr("Location")
                    wrapMode: Text.WordWrap
                    font.pixelSize: 16
                    color: Theme.colorText
                    verticalAlignment: Text.AlignVCenter
                }

                SwitchThemedMobile {
                    id: switch_gps
                    anchors.right: parent.right
                    anchors.rightMargin: 0
                    anchors.verticalCenter: parent.verticalCenter
                    z: 1

                    //Component.onCompleted: checked = settingsManager.bluetoothControl
                    //onCheckedChanged: settingsManager.bluetoothControl = checked
                }
            }
            Text {
                id: legend_gps
                anchors.left: parent.left
                anchors.leftMargin: 0
                anchors.right: parent.right
                anchors.rightMargin: 0
                topPadding: -8
                bottomPadding: 0

                text: qsTr("Android operating system required applications to ask for device position in order to scan for Bluetooth devices.<br>You can actually remove this permission as long as you don't need to scan for new devices.")
                wrapMode: Text.WordWrap
                color: Theme.colorSubText
                font.pixelSize: 14
            }

            ////////

            Item {
                height: 8
                anchors.right: parent.right
                anchors.left: parent.left

                Rectangle {
                    height: 1
                    color: Theme.colorSeparator
                    anchors.right: parent.right
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            Item {
                id: element_writesd
                height: 48
                anchors.right: parent.right
                anchors.left: parent.left
/*
                ImageSvg {
                    id: image_writesd
                    width: 24
                    height: 24
                    anchors.left: parent.left
                    anchors.leftMargin: 16
                    anchors.verticalCenter: parent.verticalCenter

                    color: Theme.colorText
                    source: "qrc:/assets/icons_material/baseline-bluetooth_disabled-24px.svg"
                }
*/
                Text {
                    id: text_writesd
                    height: 16
                    anchors.left: parent.left
                    anchors.leftMargin: 0
                    anchors.right: switch_gps.left
                    anchors.rightMargin: 16
                    anchors.verticalCenter: parent.verticalCenter

                    text: qsTr("SD card permission")
                    wrapMode: Text.WordWrap
                    font.pixelSize: 16
                    color: Theme.colorText
                    verticalAlignment: Text.AlignVCenter
                }

                SwitchThemedMobile {
                    id: switch_writesd
                    anchors.right: parent.right
                    anchors.rightMargin: 0
                    anchors.verticalCenter: parent.verticalCenter
                    z: 1

                    //Component.onCompleted: checked = settingsManager.bluetoothControl
                    //onCheckedChanged: settingsManager.bluetoothControl = checked
                }
            }
            Text {
                id: legend_writesd
                anchors.left: parent.left
                anchors.leftMargin: 0
                anchors.right: parent.right
                anchors.rightMargin: 0
                topPadding: -8
                bottomPadding: 0

                text: qsTr("SD card write permission is needed for exporting sensors data.")
                wrapMode: Text.WordWrap
                color: Theme.colorSubText
                font.pixelSize: 14
            }

            ////////

            Item {
                height: 8
                anchors.right: parent.right
                anchors.left: parent.left

                Rectangle {
                    height: 1
                    color: Theme.colorSeparator
                    anchors.right: parent.right
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                }
            }

            Item {
                id: element_bluetooth
                height: 48
                anchors.right: parent.right
                anchors.left: parent.left
/*
                ImageSvg {
                    id: image_bluetooth
                    width: 24
                    height: 24
                    anchors.left: parent.left
                    anchors.leftMargin: 16
                    anchors.verticalCenter: parent.verticalCenter

                    color: Theme.colorText
                    source: "qrc:/assets/icons_material/baseline-bluetooth_disabled-24px.svg"
                }
*/
                Text {
                    id: text_bluetooth
                    height: 16
                    anchors.left: parent.left
                    anchors.leftMargin: 0
                    anchors.right: switch_gps.left
                    anchors.rightMargin: 16
                    anchors.verticalCenter: parent.verticalCenter

                    text: qsTr("Bluetooth control")
                    wrapMode: Text.WordWrap
                    font.pixelSize: 16
                    color: Theme.colorText
                    verticalAlignment: Text.AlignVCenter
                }

                SwitchThemedMobile {
                    id: switch_bluetooth
                    anchors.right: parent.right
                    anchors.rightMargin: 0
                    anchors.verticalCenter: parent.verticalCenter
                    z: 1

                    enabled: false
                    checked: true
                    //Component.onCompleted: checked = settingsManager.bluetoothControl
                    //onCheckedChanged: settingsManager.bluetoothControl = checked
                }
            }
            Text {
                id: legend_bluetooth
                anchors.left: parent.left
                anchors.leftMargin: 0
                anchors.right: parent.right
                anchors.rightMargin: 0
                topPadding: -8
                bottomPadding: 0

                text: qsTr("WatchFlower can activate your device's Bluetooth in order to operate.")
                wrapMode: Text.WordWrap
                color: Theme.colorSubText
                font.pixelSize: 14
            }

            ////////
        }
    }
}