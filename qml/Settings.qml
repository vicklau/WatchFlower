/*!
 * This file is part of WatchFlower.
 * COPYRIGHT (C) 2018 Emeric Grange - All Rights Reserved
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
 * \date      2018
 * \author    Emeric Grange <emeric.grange@gmail.com>
 */

import QtQuick 2.7
import QtQuick.Controls 2.0

Rectangle {
    id: rectangleSettings
    width: 480
    height: 640
    color: "#00000000"
    anchors.fill: parent

    Rectangle {
        id: rectangleSettingsTitle
        height: 80
        color: "#e8e9e8"
        border.width: 0

        anchors.right: parent.right
        anchors.rightMargin: 0
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.top: parent.top
        anchors.topMargin: 0

        Text {
            id: textTitle
            height: 32
            color: "#454b54"
            text: qsTr("Settings")
            anchors.right: parent.right
            anchors.rightMargin: 12
            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.top: parent.top
            anchors.topMargin: 12
            font.bold: true
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: 26
        }

        Text {
            id: textSubtitle
            y: 46
            text: qsTr("Change persistent settings here")
            font.pixelSize: 16
            anchors.left: parent.left
            anchors.leftMargin: 13
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 14
        }
    }

    Rectangle {
        id: rectangleSettingsContent
        height: 240
        color: "#ffffff"

        anchors.top:rectangleSettingsTitle.bottom
        anchors.topMargin: 0
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.right: parent.right
        anchors.rightMargin: 0

        Component.onCompleted: {
            if (Qt.platform.os === "android" || Qt.platform.os === "ios") {
                checkBox_systray.enabled = false
            }
        }

        Image {
            id: image_systray
            x: 12
            width: 40
            height: 40
            fillMode: Image.PreserveAspectCrop
            anchors.verticalCenter: checkBox_systray.verticalCenter
            anchors.right: parent.right
            anchors.rightMargin: 8
            source: "../assets/desktop/watchflower_tray_dark.svg"
        }

        CheckBox {
            id: checkBox_systray
            height: 40
            anchors.top: parent.top
            anchors.topMargin: 8
            anchors.left: parent.left
            anchors.leftMargin: 6
            text: qsTr("Enable system tray and notifications")
            font.pixelSize: 16

            checked: settingsManager.systray
            onCheckStateChanged: { settingsManager.systray = checked }
        }

        SpinBox {
            id: spinBox_update
            width: 128
            height: 40
            anchors.top: checkBox_systray.bottom
            anchors.topMargin: 16
            from: 30
            stepSize: 30
            to: 120
            anchors.left: parent.left
            anchors.leftMargin: 12
            value: settingsManager.interval

            onValueChanged: { settingsManager.interval = value }
        }

        Text {
            id: text2
            height: 40
            anchors.left: spinBox_update.right
            anchors.leftMargin: 8

            verticalAlignment: Text.AlignVCenter
            text: qsTr("Update interval in minutes")
            anchors.verticalCenter: spinBox_update.verticalCenter
            font.pixelSize: 16
        }

        Text {
            id: text3
            x: 3
            y: 128
            height: 40
            anchors.top: spinBox_update.bottom
            anchors.topMargin: 16
            anchors.left: parent.left
            anchors.leftMargin: 12

            text: qsTr("Temperature unit:")
            font.pixelSize: 16
            verticalAlignment: Text.AlignVCenter
        }

        RadioDelegate {
            id: radioDelegateCelsius
            height: 40
            text: qsTr("°C")
            font.pixelSize: 16
            anchors.left: text3.right
            anchors.leftMargin: 0
            anchors.top: text3.top
            anchors.topMargin: 0

            checked: {
                if (settingsManager.tempunit === 'C') {
                    radioDelegateCelsius.checked = true
                    radioDelegateFahrenheit.checked = false
                } else {
                    radioDelegateCelsius.checked = false
                    radioDelegateFahrenheit.checked = true
                }
            }

            onCheckedChanged: {
                if (checked == true)
                    settingsManager.tempunit = 'C'
            }
        }

        RadioDelegate {
            id: radioDelegateFahrenheit
            height: 40
            text: qsTr("°F")
            font.pixelSize: 16
            anchors.left: radioDelegateCelsius.right
            anchors.leftMargin: 0
            anchors.top: text3.top
            anchors.topMargin: 0

            checked: {
                if (settingsManager.tempunit === 'F') {
                    radioDelegateCelsius.checked = false
                    radioDelegateFahrenheit.checked = true
                } else {
                    radioDelegateFahrenheit.checked = false
                    radioDelegateCelsius.checked = true
                }
            }

            onCheckedChanged: {
                if (checked === true)
                    settingsManager.tempunit = 'F'
            }
        }

        Text {
            id: text4
            height: 40
            text: qsTr("Default graph:")
            font.pixelSize: 16
            verticalAlignment: Text.AlignVCenter
            anchors.top: text3.bottom
            anchors.topMargin: 16
            anchors.left: parent.left
            anchors.leftMargin: 12
        }

        ComboBox {
            id: comboBox_view
            anchors.left: text4.right
            anchors.leftMargin: 8
            anchors.top: text4.top
            anchors.topMargin: 0
            model: ListModel {
                id: cbItemsView
                ListElement { text: qsTr("daily"); }
                ListElement { text: qsTr("hourly"); }
            }
            Component.onCompleted: {
                currentIndex = find(settingsManager.graphview)
                if (currentIndex === -1) { currentIndex = 0 }
            }
            property bool cbinit: false
            width: 100
            onCurrentIndexChanged: {
                if (cbinit)
                    settingsManager.graphview = cbItemsView.get(currentIndex).text
                else
                    cbinit = true
            }
        }

        ComboBox {
            id: comboBox_data
            anchors.left: comboBox_view.right
            anchors.leftMargin: 8
            anchors.top: comboBox_view.top
            anchors.topMargin: 0
            model: ListModel {
                id: cbItemsData
                ListElement { text: qsTr("hygro"); }
                ListElement { text: qsTr("temp"); }
                ListElement { text: qsTr("luminosity"); }
                ListElement { text: qsTr("conductivity"); }
            }
            Component.onCompleted: {
                currentIndex = find(settingsManager.graphdata)
                if (currentIndex === -1) { currentIndex = 0 }
            }
            property bool cbinit: false
            onCurrentIndexChanged: {
                if (cbinit)
                    settingsManager.graphdata = cbItemsData.get(currentIndex).text
                else
                    cbinit = true
            }
        }
    }

    Rectangle {
        id: rectangleInfos
        y: 365
        width: 270
        height: 80
        color: "#00000000"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: rectangleSettingsContent.bottom
        anchors.topMargin: 16

        Image {
            id: imageLogo
            width: 80
            height: 80
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 0
            source: "qrc:/assets/desktop/watchflower.png"
        }

        Text {
            id: textVersion
            width: 184
            anchors.verticalCenterOffset: -10
            anchors.left: imageLogo.right
            anchors.leftMargin: 4
            anchors.verticalCenter: parent.verticalCenter

            color: "#343434"
            text: qsTr("WatchFlower") + " / " + settingsManager.getAppVersion()
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 16
        }
        Text {
            id: textUrl
            width: 184
            anchors.verticalCenterOffset: 16
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: imageLogo.right
            anchors.leftMargin: 4

            color: "#343434"
            text: "Visit our <html><style type=\"text/css\"></style><a href=\"https://github.com/emericg/WatchFlower\">github</a></html> page!"
            horizontalAlignment: Text.AlignHCenter
            font.pixelSize: 16
            onLinkActivated: Qt.openUrlExternally("https://github.com/emericg/WatchFlower")
        }
    }

    Rectangle {
        id: rectangleReset
        height: 48
        color: "#f75a5a"

        anchors.bottom: parent.bottom
        anchors.bottomMargin: 16
        anchors.left: parent.left
        anchors.leftMargin: 16
        anchors.right: parent.right
        anchors.rightMargin: 16

        property bool weAreBlinking: false

        function startTheBlink() {
            if (weAreBlinking === true) {
                settingsManager.resetSettings()
                stopTheBlink()
            } else {
                weAreBlinking = true
                timerReset.start()
                blinkReset.start()
                textReset.text = qsTr("!!! Click again to confirm !!!")
            }
        }
        function stopTheBlink() {
            weAreBlinking = false
            timerReset.stop()
            blinkReset.stop()
            textReset.text = qsTr("Reset everything!")
            rectangleReset.color = "#f75a5a"
        }

        SequentialAnimation on color {
            id: blinkReset
            running: false
            loops: Animation.Infinite
            ColorAnimation { from: "#f75a5a"; to: "#ff0000"; duration: 750 }
            ColorAnimation { from: "#ff0000"; to: "#f75a5a"; duration: 750 }
        }

        Timer {
            id: timerReset
            interval: 4000
            running: false
            repeat: false
            onTriggered: {
                rectangleReset.stopTheBlink()
            }
        }

        Text {
            id: textReset
            anchors.fill: parent
            color: "#ffffff"
            text: qsTr("Reset everything!")
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            font.bold: true
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            font.pixelSize: 20
        }
        MouseArea {
            anchors.fill: parent

            onPressed: {
                rectangleReset.anchors.bottomMargin = rectangleReset.anchors.bottomMargin + 4
                rectangleReset.anchors.leftMargin = rectangleReset.anchors.leftMargin + 4
                rectangleReset.anchors.rightMargin = rectangleReset.anchors.rightMargin + 4
                rectangleReset.width = rectangleReset.width - 8
                rectangleReset.height = rectangleReset.height - 8
            }
            onReleased: {
                rectangleReset.anchors.bottomMargin = rectangleReset.anchors.bottomMargin - 4
                rectangleReset.anchors.leftMargin = rectangleReset.anchors.leftMargin - 4
                rectangleReset.anchors.rightMargin = rectangleReset.anchors.rightMargin - 4
                rectangleReset.width = rectangleReset.width + 8
                rectangleReset.height = rectangleReset.height + 8
            }
            onClicked: {
                rectangleReset.startTheBlink()
            }
        }
    }
}
