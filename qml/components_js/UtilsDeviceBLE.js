// UtilsDeviceBLE.js
// Version 1

.import ThemeEngine 1.0 as ThemeEngine
.import DeviceUtils 1.0 as DeviceUtils

/* ************************************************************************** */

function getDeviceStatusText(deviceStatus) {
    var txt = ""

    if (deviceStatus === DeviceUtils.DeviceUtils.DEVICE_OFFLINE) {
        txt = qsTr("Offline")
    } else if (deviceStatus === DeviceUtils.DeviceUtils.DEVICE_QUEUED) {
        txt = qsTr("Queued")
    } else if (deviceStatus === DeviceUtils.DeviceUtils.DEVICE_CONNECTING) {
        txt = qsTr("Connecting...")
    } else if (deviceStatus === DeviceUtils.DeviceUtils.DEVICE_CONNECTED) {
        txt = qsTr("Connected")
    } else if (deviceStatus === DeviceUtils.DeviceUtils.DEVICE_WORKING) {
        txt = qsTr("Working...")
    } else if (deviceStatus === DeviceUtils.DeviceUtils.DEVICE_UPDATING ||
               deviceStatus === DeviceUtils.DeviceUtils.DEVICE_UPDATING_HISTORY ||
               deviceStatus === DeviceUtils.DeviceUtils.DEVICE_UPDATING_REALTIME) {
        txt = qsTr("Updating...")
    }

    return txt + " "
}

function getDeviceStatusColor(deviceStatus) {
    var clr = ""

    if (deviceStatus === DeviceUtils.DeviceUtils.DEVICE_OFFLINE) {
        clr = ThemeEngine.Theme.colorRed
    } else if (deviceStatus === DeviceUtils.DeviceUtils.DEVICE_QUEUED) {
        clr = ThemeEngine.Theme.colorYellow
    } else if (deviceStatus === DeviceUtils.DeviceUtils.DEVICE_CONNECTING) {
        clr = ThemeEngine.Theme.colorYellow
    } else if (deviceStatus === DeviceUtils.DeviceUtils.DEVICE_CONNECTED) {
        clr = ThemeEngine.Theme.colorGreen
    } else if (deviceStatus === DeviceUtils.DeviceUtils.DEVICE_WORKING) {
        clr = ThemeEngine.Theme.colorYellow
    } else if (deviceStatus === DeviceUtils.DeviceUtils.DEVICE_UPDATING ||
               deviceStatus === DeviceUtils.DeviceUtils.DEVICE_UPDATING_HISTORY ||
               deviceStatus === DeviceUtils.DeviceUtils.DEVICE_UPDATING_REALTIME) {
        clr = ThemeEngine.Theme.colorYellow
    }

    return clr
}

/* ************************************************************************** */

function getDeviceBatteryIcon(batteryLevel) {
    var src = ""

    if (batteryLevel > 95) {
        src = "qrc:/assets/icons_material/baseline-battery_full-24px.svg";
    } else if (batteryLevel > 85) {
        src = "qrc:/assets/icons_material/baseline-battery_90-24px.svg";
    } else if (batteryLevel > 75) {
        src = "qrc:/assets/icons_material/baseline-battery_80-24px.svg";
    } else if (batteryLevel > 55) {
        src = "qrc:/assets/icons_material/baseline-battery_60-24px.svg";
    } else if (batteryLevel > 45) {
        src = "qrc:/assets/icons_material/baseline-battery_50-24px.svg";
    } else if (batteryLevel > 25) {
        src = "qrc:/assets/icons_material/baseline-battery_30-24px.svg";
    } else if (batteryLevel > 15) {
        src = "qrc:/assets/icons_material/baseline-battery_20-24px.svg";
    } else if (batteryLevel > 1) {
        src = "qrc:/assets/icons_material/baseline-battery_10-24px.svg";
    } else if (batteryLevel >= 0) {
        src = "qrc:/assets/icons_material/baseline-battery_unknown-24px.svg";
    }

    return src
}

function getDeviceBatteryColor(batteryLevel) {
    var clr = ""

    if (batteryLevel <= 0) {
        clr = ThemeEngine.Theme.colorRed
    } else if (batteryLevel <= 10) {
        clr = ThemeEngine.Theme.colorYellow
    } else {
        clr = ThemeEngine.Theme.colorIcon
    }

    return clr
}

/* ************************************************************************** */
