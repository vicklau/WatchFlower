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

import QtGraphicalEffects 1.0
import app.watchflower.theme 1.0

Rectangle {
    id: deviceScreenRectangle
    width: 450
    height: 700

    property var myDevice: curentlySelectedDevice

    property string badColor: "#ffbf66"
    property string neutralColor: "#e4e4e4"
    property string goodColor: "#87d241"

    Component.onCompleted: loadDevice()

    Timer {
        interval: 30000; running: true; repeat: true;
        onTriggered: updateLastUpdateText()
    }

    Connections {
        target: header
        onRightMenuClicked: {
            //
        }
    }

    function updateLastUpdateText() {
        if (typeof myDevice === "undefined") return
        var shortVersion = false
        if (rectangleSubHeader.width < 480)
            shortVersion = true

        //console.log("DeviceScreenDatas // updateLastUpdateText() >> " + myDevice)

        if (myDevice) {
            if (!myDevice.available && myDevice.updating) {
                if (!shortVersion)
                    textLastUpdate.text = qsTr("Device is updating...")
                else
                    textLastUpdate.text = qsTr("Updating...")
                textLastUpdate.color = "#454B54"
                textLastUpdate.font.bold = false
            } else if (!myDevice.available && !myDevice.updating) {
                if (!shortVersion)
                    textLastUpdate.text = qsTr("Device is offline!")
                else
                    textLastUpdate.text = qsTr("Offline!")
                textLastUpdate.font.bold = true
                textLastUpdate.color = Theme.colorOrange
                textRefresh.text = qsTr("Retry")
                //textRefresh.width = 80
            } else {
                if (!shortVersion)
                    textLastUpdate.text = qsTr("Last update:") + " "
                else
                    textLastUpdate.text = ""

                if (myDevice.lastUpdate <= 1)
                    textLastUpdate.text += qsTr("Just now!")
                else
                    textLastUpdate.text += myDevice.lastUpdate + " " + qsTr("min. ago")
                textLastUpdate.color = "#454B54"
                textLastUpdate.font.bold = false
                textRefresh.text = qsTr("Refresh")
                //textRefresh.width = 112
            }
        }
    }

    function loadDevice() {
        if (typeof myDevice === "undefined") return

        //console.log("DeviceScreen // loadDevice() >> " + myDevice)

        rectangleContent.state = "datas"

        updateStatus()

        buttonLimits.initButton()
        rectangleDeviceDatas.loadDatas()
        rectangleDeviceLimits.updateLimits()
        rectangleDeviceLimits.updateLimitsVisibility()
    }

    function updateStatus() {
        if (typeof myDevice === "undefined" || !myDevice) return

        //console.log("DeviceScreen // updateStatus() >> " + myDevice)

        // Update header
        if ((myDevice.deviceCapabilities & 1) == 1) {
            imageBattery.visible = true
            if (Qt.platform.os === "android" || Qt.platform.os === "ios") {
                textBattery.visible = false
            } else {
                textBattery.visible = true
            }

            if (myDevice.deviceBattery < 10) {
                imageBattery.source = "qrc:/assets/icons_material/baseline-battery_alert-24px.svg";
            } else if (myDevice.deviceBattery < 40) {
                imageBattery.source = "qrc:/assets/icons_material/baseline-battery_20-24px.svg";
            } else if (myDevice.deviceBattery < 60) {
                imageBattery.source = "qrc:/assets/icons_material/baseline-battery_50-24px.svg";
            } else if (myDevice.deviceBattery < 60) {
                imageBattery.source = "qrc:/assets/icons_material/baseline-battery_50-24px.svg";
            } else if (myDevice.deviceBattery < 95) {
                imageBattery.source = "qrc:/assets/icons_material/baseline-battery_90-24px.svg";
            } else {
                imageBattery.source = "qrc:/assets/icons_material/baseline-battery_full-24px.svg";
            }
        } else {
            imageBattery.visible = false
            textBattery.visible = false
        }

        if (myDevice.deviceAddress.charAt(0) !== '{')
            textAddr.text = " [" + myDevice.deviceAddress + "]"

        if (myDevice.deviceLocationName !== "")
            textInputLocation.text = myDevice.deviceLocationName
        else
            textInputLocation.text = qsTr("Location")

        // Plant sensor?
        if ((myDevice.deviceCapabilities & 64) != 0) {
            labelPlant.visible = true
            textInputPlant.visible = true

            if (myDevice.devicePlantName !== "")
                textInputPlant.text = myDevice.devicePlantName
            else
                textInputPlant.text = qsTr("Plant")

            rectangleHeader.height = 160
            labelStatus.anchors.top = labelPlant.bottom
        } else {
            labelPlant.visible = false
            labelStatus.anchors.top = labelLocation.bottom
            textInputPlant.visible = false
            rectangleHeader.height = 140
        }

        if (!myDevice.deviceFirmwareUpToDate) {
            imageFw.visible = true
            textFw.visible = true
        } else {
            imageFw.visible = false
            textFw.visible = false
        }

        // Update sub header
        updateLastUpdateText()

        if (myDevice.updating) {
            refreshRotation.start()
        } else {
            refreshRotation.stop()
        }
    }

    Rectangle {
        id: rectangleBody
        anchors.fill: parent

        Connections {
            target: myDevice
            onStatusUpdated: updateStatus()
            onDatasUpdated: rectangleDeviceDatas.updateDatas()
        }

        Rectangle {
            id: rectangleHeader
            height: 160
            color: Theme.colorMaterialLightGrey

            anchors.right: parent.right
            anchors.rightMargin: 0
            anchors.left: parent.left
            anchors.leftMargin: 0
            anchors.top: parent.top
            anchors.topMargin: 0

            Text {
                id: textDeviceName
                x: 12
                y: 12
                color: "#454B54"
                anchors.left: parent.left
                anchors.leftMargin: 12
                font.pixelSize: 23
                text: myDevice.deviceName
                font.capitalization: Font.AllUppercase
                anchors.top: parent.top
                anchors.topMargin: 12
            }
            Text {
                id: textAddr
                y: 48
                color: "#454B54"
                text: myDevice.deviceAddress
                anchors.left: labelFw.right
                anchors.leftMargin: 8
                anchors.verticalCenter: labelFw.verticalCenter
                font.pixelSize: 16
            }

            Text {
                id: labelFw
                anchors.topMargin: 10
                anchors.leftMargin: 12
                anchors.left: parent.left
                anchors.top: textDeviceName.bottom

                text: qsTr("Address")
                font.pixelSize: 15
            }
            Text {
                id: textFw
                height: 26
                color: "#454B54"
                text: myDevice.deviceFirmware
                verticalAlignment: Text.AlignVCenter
                anchors.left: imageFw.right
                anchors.leftMargin: 8
                anchors.verticalCenter: textDeviceName.verticalCenter
                font.pixelSize: 15

                property string saveText: myDevice.deviceFirmware
            }
            Image {
                id: imageFw
                width: 24
                height: 24
                anchors.verticalCenterOffset: 0
                anchors.verticalCenter: textDeviceName.verticalCenter
                anchors.left: textBattery.right
                anchors.leftMargin: 8

                source: "qrc:/assets/icons_material/baseline-new_releases-24px.svg"
                sourceSize.width: width
                sourceSize.height: height

                ColorOverlay {
                    anchors.fill: parent
                    source: parent
                    color: Theme.colorDarkGrey
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: {
                        textFw.saveText = textFw.text
                        textFw.text = qsTr("Update available!")
                    }
                    onExited: {
                        textFw.text = textFw.saveText
                    }
                }
            }
            Text {
                id: textBattery
                height: 26
                color: "#454B54"
                text: myDevice.deviceBattery + "%"
                verticalAlignment: Text.AlignVCenter
                anchors.left: imageBattery.right
                anchors.leftMargin: 8
                anchors.verticalCenter: imageBattery.verticalCenter
                font.pixelSize: 15
            }
            Image {
                id: imageBattery
                width: 32
                height: 32
                rotation: 90
                anchors.verticalCenter: textDeviceName.verticalCenter
                anchors.left: textDeviceName.right
                anchors.leftMargin: 16

                source: "qrc:/assets/icons_material/baseline-battery_unknown-24px.svg"
                sourceSize.width: width
                sourceSize.height: height

                ColorOverlay {
                    anchors.fill: parent
                    source: parent
                    color: Theme.colorDarkGrey
                }
            }

            Text {
                id: labelPlant
                anchors.top: labelLocation.bottom
                anchors.topMargin: 10
                anchors.left: parent.left
                anchors.leftMargin: 12

                text: qsTr("Plant")
                font.pixelSize: 15
            }
            TextInput {
                id: textInputPlant
                anchors.verticalCenter: labelPlant.verticalCenter
                anchors.left: labelPlant.right
                anchors.leftMargin: 8
                padding: 4

                text: ""
                color: "#454B54"
                font.pixelSize: 18
                onEditingFinished: {
                    myDevice.setPlantName(text)
                    focus = false
                    imageEditPlant.visible = false
                }

                Image {
                    id: imageEditPlant
                    width: 24
                    height: 24
                    anchors.left: parent.right
                    anchors.leftMargin: 6
                    anchors.verticalCenterOffset: 0
                    anchors.verticalCenter: parent.verticalCenter
                    visible: false
                    source: "qrc:/assets/icons_material/baseline-edit-24px.svg"
                    sourceSize.width: width
                    sourceSize.height: height

                    ColorOverlay {
                        anchors.fill: parent
                        source: parent
                        color: Theme.colorDarkGrey
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    propagateComposedEvents: true

                    onEntered: { imageEditPlant.visible = true; }
                    onExited: { imageEditPlant.visible = false; }

                    onClicked:  {
                        imageEditPlant.visible = true;
                        mouse.accepted = false;
                    }
                    onPressed:  {
                        imageEditPlant.visible = true;
                        mouse.accepted = false;
                    }
                    onReleased: mouse.accepted = false;
                    onDoubleClicked: mouse.accepted = false;
                    onPositionChanged: mouse.accepted = false;
                    onPressAndHold: mouse.accepted = false;
                }
            }

            Text {
                id: labelLocation
                text: qsTr("Location")
                anchors.top: labelFw.bottom
                anchors.topMargin: 10
                anchors.left: parent.left
                anchors.leftMargin: 12
                font.pixelSize: 15
            }
            TextInput {
                id: textInputLocation
                anchors.verticalCenter: labelLocation.verticalCenter
                anchors.left: labelLocation.right
                anchors.leftMargin: 8
                padding: 4

                text: ""
                color: "#454B54"
                font.pixelSize: 18
                onEditingFinished: {
                    myDevice.setLocationName(text)
                    focus = false
                    imageEditLocation.visible = false
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    propagateComposedEvents: true

                    onEntered: { imageEditLocation.visible = true; }
                    onExited: { imageEditLocation.visible = false; }

                    onClicked: {
                        imageEditLocation.visible = true;
                        mouse.accepted = false;
                    }
                    onPressed: {
                        imageEditLocation.visible = true;
                        mouse.accepted = false;
                    }
                    onReleased: mouse.accepted = false;
                    onDoubleClicked: mouse.accepted = false;
                    onPositionChanged: mouse.accepted = false;
                    onPressAndHold: mouse.accepted = false;
                }

                Image {
                    id: imageEditLocation
                    width: 24
                    height: 24
                    anchors.left: parent.right
                    anchors.leftMargin: 6
                    anchors.verticalCenterOffset: 0
                    anchors.verticalCenter: parent.verticalCenter
                    visible: false
                    source: "qrc:/assets/icons_material/baseline-edit-24px.svg"
                    sourceSize.width: width
                    sourceSize.height: height

                    ColorOverlay {
                        anchors.fill: parent
                        source: parent
                        color: Theme.colorDarkGrey
                    }
                }
            }

            Text {
                id: labelStatus
                text: qsTr("Status")
                anchors.top: labelPlant.bottom
                anchors.left: parent.left
                anchors.topMargin: 10
                anchors.leftMargin: 12
                font.pixelSize: 15
            }

            Text {
                id: textLastUpdate
                color: "#454b54"
                text: qsTr("Loading...")
                font.pixelSize: 18
                anchors.verticalCenter: labelStatus.verticalCenter
                anchors.left: labelStatus.right
                anchors.leftMargin: 12
            }

            Rectangle {
                id: rectangleSubHeader
                width: 120
                height: 94
                color: Theme.colorMaterialLightGrey
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right

                Rectangle {
                    id: buttonRefresh
                    width: 100
                    height: 32
                    color: "#e0e0e0"
                    anchors.top: buttonLimits.bottom
                    anchors.topMargin: 10
                    anchors.left: parent.left
                    anchors.leftMargin: 10

                    Text {
                        id: textRefresh
                        height: 32
                        color: "#202020"
                        text: qsTr("Refresh")
                        verticalAlignment: Text.AlignVCenter
                        anchors.right: parent.right
                        font.pixelSize: 16
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: imageRefresh.right
                        anchors.rightMargin: 0
                        anchors.leftMargin: 8
                    }

                    Image {
                        id: imageRefresh
                        width: 22
                        height: 22
                        anchors.left: parent.left
                        anchors.leftMargin: 6
                        anchors.verticalCenter: parent.verticalCenter
                        source: "qrc:/assets/icons_material/baseline-refresh-24px.svg"
                        sourceSize.width: width
                        sourceSize.height: height

                        ColorOverlay {
                            anchors.fill: parent
                            source: parent
                            color: Theme.colorDarkGrey
                        }

                        NumberAnimation on rotation {
                            id: refreshRotation
                            duration: 3000;
                            from: 0;
                            to: 360;
                            loops: Animation.Infinite
                            running: false
                        }
                    }

                    MouseArea {
                        anchors.fill: parent

                        hoverEnabled: true
                        onEntered: buttonRefresh.color = "#eaeaea"
                        onExited: buttonRefresh.color = "#e0e0e0"

                        onClicked: {
                            refreshRotation.start()
                            myDevice.refreshDatas()
                        }
                    }
                }

                Rectangle {
                    id: buttonLimits
                    width: 100
                    height: 32
                    color: "#e0e0e0"
                    anchors.top: parent.top
                    anchors.topMargin: 10
                    anchors.left: parent.left
                    anchors.leftMargin: 10

                    function initButton() {
                        if (rectangleContent.state === "limits") {
                            textLimits.text = qsTr("Datas")
                            imageLimits.source = "qrc:/assets/icons_material/baseline-insert_chart_outlined-24px.svg"
                        } else {
                            textLimits.text = qsTr("Limits")
                            imageLimits.source = "qrc:/assets/icons_material/baseline-iso-24px.svg"
                        }
                    }

                    Text {
                        id: textLimits
                        height: 32
                        color: "#202020"
                        text: qsTr("Limits")
                        verticalAlignment: Text.AlignVCenter
                        anchors.right: parent.right
                        font.pixelSize: 16
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: imageLimits.right
                        anchors.rightMargin: 0
                        anchors.leftMargin: 8
                    }

                    Image {
                        id: imageLimits
                        width: 22
                        height: 22
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.left: parent.left
                        anchors.leftMargin: 6
                        source: "qrc:/assets/icons_material/baseline-iso-24px.svg"
                        sourceSize.width: width
                        sourceSize.height: height

                        ColorOverlay {
                            anchors.fill: parent
                            source: parent
                            color: Theme.colorDarkGrey
                        }
                    }

                    MouseArea {
                        anchors.fill: parent

                        hoverEnabled: true
                        onEntered: buttonLimits.color = "#eaeaea"
                        onExited: buttonLimits.color = "#e0e0e0"

                        onClicked: {
                            if (rectangleContent.state === "datas") {
                                rectangleContent.state = "limits"
                                textLimits.text = qsTr("Datas")
                                imageLimits.source = "qrc:/assets/icons_material/baseline-insert_chart_outlined-24px.svg"
                            } else {
                                rectangleContent.state = "datas"
                                textLimits.text = qsTr("Limits")
                                imageLimits.source = "qrc:/assets/icons_material/baseline-iso-24px.svg"

                                // Update color bars with new limits
                                rectangleDeviceDatas.updateDatas()
                            }
                        }
                    }
                }
            }
        }

        Rectangle {
            id: rectangleContent
            color: "#ffffff"
            anchors.topMargin: 0
            anchors.bottom: parent.bottom

            anchors.top: rectangleHeader.bottom
            anchors.left: parent.left
            anchors.right: parent.right

            DeviceScreenDatas {
                anchors.fill: parent
                id: rectangleDeviceDatas
            }
            DeviceScreenLimits {
                anchors.fill: parent
                id: rectangleDeviceLimits
            }

            state: "datas"
            states: [
                State {
                    name: "datas"
                    PropertyChanges {
                        target: rectangleDeviceDatas
                        visible: true
                    }
                    PropertyChanges {
                        target: rectangleDeviceLimits
                        visible: false
                    }
                },
                State {
                    name: "limits"
                    PropertyChanges {
                        target: rectangleDeviceDatas
                        visible: false
                    }
                    PropertyChanges {
                        target: rectangleDeviceLimits
                        visible: true
                    }
                }
            ]
        }
    }
}
