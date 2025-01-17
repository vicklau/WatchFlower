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

#include "device_parrotpot.h"
#include "utils/utils_versionchecker.h"

#include <cstdint>
#include <cmath>

#include <QBluetoothUuid>
#include <QBluetoothServiceInfo>
#include <QLowEnergyService>

#include <QSqlQuery>
#include <QSqlError>

#include <QDebug>

/* ************************************************************************** */

DeviceParrotPot::DeviceParrotPot(QString &deviceAddr, QString &deviceName, QObject *parent):
    DeviceSensor(deviceAddr, deviceName, parent)
{
    m_deviceType = DeviceUtils::DEVICE_PLANTSENSOR;
    //m_deviceCapabilities += DeviceUtils::DEVICE_REALTIME;
    //m_deviceCapabilities += DeviceUtils::DEVICE_HISTORY;
    m_deviceCapabilities += DeviceUtils::DEVICE_BATTERY;
    m_deviceCapabilities += DeviceUtils::DEVICE_LED_STATUS;
    m_deviceCapabilities += DeviceUtils::DEVICE_WATER_TANK;
    m_deviceSensors += DeviceUtils::SENSOR_SOIL_MOISTURE;
    m_deviceSensors += DeviceUtils::SENSOR_SOIL_CONDUCTIVITY;
    m_deviceSensors += DeviceUtils::SENSOR_SOIL_TEMPERATURE;
    m_deviceSensors += DeviceUtils::SENSOR_TEMPERATURE;
    m_deviceSensors += DeviceUtils::SENSOR_LUMINOSITY;
    m_deviceSensors += DeviceUtils::SENSOR_WATER_LEVEL;

    m_watertank_capacity = 2.2;
}

DeviceParrotPot::DeviceParrotPot(const QBluetoothDeviceInfo &d, QObject *parent):
    DeviceSensor(d, parent)
{
    m_deviceType = DeviceUtils::DEVICE_PLANTSENSOR;
    //m_deviceCapabilities += DeviceUtils::DEVICE_REALTIME;
    //m_deviceCapabilities += DeviceUtils::DEVICE_HISTORY;
    m_deviceCapabilities += DeviceUtils::DEVICE_BATTERY;
    m_deviceCapabilities += DeviceUtils::DEVICE_LED_STATUS;
    m_deviceCapabilities += DeviceUtils::DEVICE_WATER_TANK;
    m_deviceSensors += DeviceUtils::SENSOR_SOIL_MOISTURE;
    m_deviceSensors += DeviceUtils::SENSOR_SOIL_CONDUCTIVITY;
    m_deviceSensors += DeviceUtils::SENSOR_SOIL_TEMPERATURE;
    m_deviceSensors += DeviceUtils::SENSOR_TEMPERATURE;
    m_deviceSensors += DeviceUtils::SENSOR_LUMINOSITY;
    m_deviceSensors += DeviceUtils::SENSOR_WATER_LEVEL;

    m_watertank_capacity = 2.2;
}

DeviceParrotPot::~DeviceParrotPot()
{
    delete serviceHistory;
    delete serviceClock;
    delete serviceWatering;
    delete serviceLive;
    delete serviceBattery;
    delete serviceInfos;
}

/* ************************************************************************** */
/* ************************************************************************** */

void DeviceParrotPot::serviceScanDone()
{
    //qDebug() << "DeviceParrotPot::serviceScanDone(" << m_deviceAddress << ")";

    if (serviceInfos)
    {
        if (serviceInfos->state() == QLowEnergyService::DiscoveryRequired)
        {
            connect(serviceInfos, &QLowEnergyService::stateChanged, this, &DeviceParrotPot::serviceDetailsDiscovered_infos);
            //connect(serviceInfos, &QLowEnergyService::characteristicRead, this, &DeviceParrotPot::bleReadDone);

            // Windows hack, see: QTBUG-80770 and QTBUG-78488
            QTimer::singleShot(0, this, [=] () { serviceInfos->discoverDetails(); });
        }
    }

    if (serviceBattery)
    {
        if (serviceBattery->state() == QLowEnergyService::DiscoveryRequired)
        {
            connect(serviceBattery, &QLowEnergyService::stateChanged, this, &DeviceParrotPot::serviceDetailsDiscovered_battery);
            //connect(serviceBattery, &QLowEnergyService::characteristicRead, this, &DeviceParrotPot::bleReadDone);

            // Windows hack, see: QTBUG-80770 and QTBUG-78488
            QTimer::singleShot(0, this, [=] () { serviceBattery->discoverDetails(); });
        }
    }

    if (serviceClock)
    {
        if (serviceClock->state() == QLowEnergyService::DiscoveryRequired)
        {
            connect(serviceClock, &QLowEnergyService::stateChanged, this, &DeviceParrotPot::serviceDetailsDiscovered_clock);
            //connect(serviceClock, &QLowEnergyService::characteristicRead, this, &DeviceParrotPot::bleReadDone);

            // Windows hack, see: QTBUG-80770 and QTBUG-78488
            QTimer::singleShot(0, this, [=] () { serviceClock->discoverDetails(); });
        }
    }

    if (serviceHistory)
    {
        if (serviceHistory->state() == QLowEnergyService::DiscoveryRequired)
        {
            connect(serviceHistory, &QLowEnergyService::stateChanged, this, &DeviceParrotPot::serviceDetailsDiscovered_history);
            //connect(serviceHistory, &QLowEnergyService::characteristicRead, this, &DeviceParrotPot::bleReadDone);
            //connect(serviceHistory, &QLowEnergyService::characteristicWritten, this, &DeviceParrotPot::bleWriteDone);

            // Windows hack, see: QTBUG-80770 and QTBUG-78488
            QTimer::singleShot(0, this, [=] () { serviceHistory->discoverDetails(); });
        }
    }

    if (serviceWatering)
    {
        if (serviceWatering->state() == QLowEnergyService::DiscoveryRequired)
        {
            connect(serviceWatering, &QLowEnergyService::stateChanged, this, &DeviceParrotPot::serviceDetailsDiscovered_watering);
            //connect(serviceWatering, &QLowEnergyService::characteristicRead, this, &DeviceParrotPot::bleReadDone);

            // Windows hack, see: QTBUG-80770 and QTBUG-78488
            QTimer::singleShot(0, this, [=] () { serviceWatering->discoverDetails(); });
        }
    }

    if (serviceLive)
    {
        if (serviceLive->state() == QLowEnergyService::DiscoveryRequired)
        {
            connect(serviceLive, &QLowEnergyService::stateChanged, this, &DeviceParrotPot::serviceDetailsDiscovered_live);
            //connect(serviceLive, &QLowEnergyService::characteristicRead, this, &DeviceParrotPot::bleReadDone);

            // Windows hack, see: QTBUG-80770 and QTBUG-78488
            QTimer::singleShot(0, this, [=] () { serviceLive->discoverDetails(); });
        }
    }
}

/* ************************************************************************** */

void DeviceParrotPot::addLowEnergyService(const QBluetoothUuid &uuid)
{
    //qDebug() << "DeviceParrotPot::addLowEnergyService(" << uuid.toString() << ")";

    if (uuid.toString() == "{0000180a-0000-1000-8000-00805f9b34fb}") // Device Information service
    {
        delete serviceInfos;
        serviceInfos = nullptr;

        if (m_ble_action == DeviceUtils::ACTION_UPDATE &&
            (m_deviceFirmware.isEmpty() || m_deviceFirmware == "UNKN"))
        {
            serviceInfos = m_bleController->createServiceObject(uuid);
            if (!serviceInfos)
                qWarning() << "Cannot create service (infos) for uuid:" << uuid.toString();
        }
    }

    if (uuid.toString() == "{0000180f-0000-1000-8000-00805f9b34fb}") // Battery service
    {
        delete serviceBattery;
        serviceBattery = nullptr;

        if (m_ble_action == DeviceUtils::ACTION_UPDATE)
        {
            serviceBattery = m_bleController->createServiceObject(uuid);
            if (!serviceBattery)
                qWarning() << "Cannot create service (battery) for uuid:" << uuid.toString();
        }
    }

    if (uuid.toString() == "{39e1fc00-84a8-11e2-afba-0002a5d5c51b}") // (custom) History service
    {
        delete serviceHistory;
        serviceHistory = nullptr;

        if (m_ble_action == DeviceUtils::ACTION_UPDATE_HISTORY)
        {
            serviceHistory = m_bleController->createServiceObject(uuid);
            if (!serviceHistory)
                qWarning() << "Cannot create service (history) for uuid:" << uuid.toString();
        }
    }

    if (uuid.toString() == "{39e1fd00-84a8-11e2-afba-0002a5d5c51b}") // (custom) Clock service
    {
        delete serviceClock;
        serviceClock = nullptr;

        if (m_ble_action == DeviceUtils::ACTION_UPDATE_HISTORY)
        {
            serviceClock = m_bleController->createServiceObject(uuid);
            if (!serviceClock)
                qWarning() << "Cannot create service (clock) for uuid:" << uuid.toString();
        }
    }

    if (uuid.toString() == "{39e1f900-84a8-11e2-afba-0002a5d5c51b}") // (custom) Watering service
    {
        delete serviceWatering;
        serviceWatering = nullptr;

        if (m_ble_action == DeviceUtils::ACTION_UPDATE ||
            m_ble_action == DeviceUtils::ACTION_WATERING)
        {
            serviceWatering = m_bleController->createServiceObject(uuid);
            if (!serviceWatering)
                qWarning() << "Cannot create service (watering) for uuid:" << uuid.toString();
        }
    }

    if (uuid.toString() == "{39e1fa00-84a8-11e2-afba-0002a5d5c51b}") // (custom) Live service
    {
        delete serviceLive;
        serviceLive = nullptr;

        if (m_ble_action != DeviceUtils::ACTION_UPDATE_HISTORY)
        {
            serviceLive = m_bleController->createServiceObject(uuid);
            if (!serviceLive)
                qWarning() << "Cannot create service (data) for uuid:" << uuid.toString();
        }
    }
}

/* ************************************************************************** */

void DeviceParrotPot::serviceDetailsDiscovered_infos(QLowEnergyService::ServiceState newState)
{
    if (newState == QLowEnergyService::ServiceDiscovered)
    {
        //qDebug() << "DeviceParrotPot::serviceDetailsDiscovered_infos(" << m_deviceAddress << ") > ServiceDiscovered";

        if (serviceInfos)
        {
            QBluetoothUuid uuid_fw(QString("00002a26-0000-1000-8000-00805f9b34fb"));
            QLowEnergyCharacteristic cfw = serviceInfos->characteristic(uuid_fw);
            if (cfw.value().size() > 0)
            {
                m_deviceFirmware = cfw.value();
                m_deviceFirmware =  m_deviceFirmware.split('_')[1].split('-')[1];
            }

            if (m_deviceFirmware.size() == 6)
            {
                if (Version(m_deviceFirmware) >= Version(LATEST_KNOWN_FIRMWARE_PARROTPOT))
                {
                    m_firmware_uptodate = true;
                }
            }

            if (m_dbInternal || m_dbExternal)
            {
                QSqlQuery updateDevice;
                updateDevice.prepare("UPDATE devices SET deviceFirmware = :firmware WHERE deviceAddr = :deviceAddr");
                updateDevice.bindValue(":firmware", m_deviceFirmware);
                updateDevice.bindValue(":deviceAddr", getAddress());
                if (updateDevice.exec() == false)
                    qWarning() << "> updateDevice.exec() ERROR" << updateDevice.lastError().type() << ":" << updateDevice.lastError().text();
            }

            Q_EMIT sensorUpdated();
        }
    }
}

void DeviceParrotPot::serviceDetailsDiscovered_battery(QLowEnergyService::ServiceState newState)
{
    if (newState == QLowEnergyService::ServiceDiscovered)
    {
        //qDebug() << "DeviceParrotPot::serviceDetailsDiscovered_battery(" << m_deviceAddress << ") > ServiceDiscovered";

        if (serviceBattery)
        {
            // Characteristic "Battery Level"
            QBluetoothUuid uuid_batterylevel(QString("00002a19-0000-1000-8000-00805f9b34fb"));
            QLowEnergyCharacteristic cbat = serviceBattery->characteristic(uuid_batterylevel);

            if (cbat.value().size() == 1)
            {
                int lvl = static_cast<uint8_t>(cbat.value().constData()[0]);
                setBattery(lvl);
            }
        }
    }
}

void DeviceParrotPot::serviceDetailsDiscovered_live(QLowEnergyService::ServiceState newState)
{
    if (newState == QLowEnergyService::ServiceDiscovered)
    {
        //qDebug() << "DeviceParrotPot::serviceDetailsDiscovered_live(" << m_deviceAddress << ") > ServiceDiscovered";

        if (serviceLive && m_ble_action == DeviceUtils::ACTION_LED_BLINK)
        {
            // Make LED blink
            QBluetoothUuid led(QString("39e1fa07-84a8-11e2-afba-0002a5d5c51b"));
            QLowEnergyCharacteristic cled = serviceLive->characteristic(led);
            serviceLive->writeCharacteristic(cled, QByteArray::fromHex("01"), QLowEnergyService::WriteWithResponse);
        }

        if (serviceLive && m_ble_action == DeviceUtils::ACTION_UPDATE)
        {
            const quint8 *rawData = nullptr;
            double rawValue = 0;
            uint32_t rawValueCal;

            /////////
/*
            QBluetoothUuid lx(QString("39e1fa01-84a8-11e2-afba-0002a5d5c51b"));
            QLowEnergyCharacteristic chlx = serviceLive->characteristic(lx);

            rawData = reinterpret_cast<const quint8 *>(chlx.value().constData());
            rawValue = static_cast<uint16_t>(rawData[0] + (rawData[1] << 8));
*/
            /////////

            QBluetoothUuid sf(QString("39e1fa02-84a8-11e2-afba-0002a5d5c51b"));
            QLowEnergyCharacteristic chsf = serviceLive->characteristic(sf);

            rawData = reinterpret_cast<const quint8 *>(chsf.value().constData());
            rawValue = static_cast<uint16_t>(rawData[0] + (rawData[1] << 8));
            // sensor output (no soil: 2036) - (max observed: ?) wich maps to 0 - 10 (mS/cm)
            m_soil_conductivity = std::round(rawValue / 1.0);

            /////////

            QBluetoothUuid st(QString("39e1fa03-84a8-11e2-afba-0002a5d5c51b"));
            QLowEnergyCharacteristic chst = serviceLive->characteristic(st);

            rawData = reinterpret_cast<const quint8 *>(chst.value().constData());
            rawValue = static_cast<uint16_t>(rawData[0] + (rawData[1] << 8));
            m_soil_temperature = 0.00000003044 * std::pow(rawValue, 3.0) - 0.00008038 * std::pow(rawValue, 2.0) + rawValue * 0.1149 - 30.449999999999999;

            /////////

            QBluetoothUuid t(QString("39e1fa04-84a8-11e2-afba-0002a5d5c51b"));
            QLowEnergyCharacteristic cht = serviceLive->characteristic(t);

            rawData = reinterpret_cast<const quint8 *>(cht.value().constData());
            rawValue = static_cast<uint16_t>(rawData[0] + (rawData[1] << 8));
            m_temperature = 0.00000003044 * std::pow(rawValue, 3.0) - 0.00008038 * std::pow(rawValue, 2.0) + rawValue * 0.1149 - 30.449999999999999;
            if (m_temperature < -10.0) m_temperature = -10.0;
            if (m_temperature > 55.0) m_temperature = 55.0;

            /////////
/*
            QBluetoothUuid sm(QString("39e1fa05-84a8-11e2-afba-0002a5d5c51b"));
            QLowEnergyCharacteristic chsm = serviceLive->characteristic(sm);

            rawData = reinterpret_cast<const quint8 *>(chsm.value().constData());
            rawValue = static_cast<uint16_t>(rawData[0] + (rawData[1] << 8));
            double hygro1 = 11.4293 + (0.0000000010698 * std::pow(rawValue, 4.0) - 0.00000152538 * std::pow(rawValue, 3.0) + 0.000866976 * std::pow(rawValue, 2.0) - 0.169422 * rawValue);
            double hygro2 = 100.0 * (0.0000045 * std::pow(hygro1, 3.0) - 0.00055 * std::pow(hygro1, 2.0) + 0.0292 * hygro1 - 0.053);
            if (hygro2 < 0.0) hygro2 = 0.0;
            if (hygro2 > 60.0) hygro2 = 60.0;
            m_soil_moisture = std::round(hygro2);
*/
            /////////

            QBluetoothUuid sm_calibrated(QString("39e1fa09-84a8-11e2-afba-0002a5d5c51b")); // soil moisture
            QLowEnergyCharacteristic chsmc = serviceLive->characteristic(sm_calibrated);

            rawData = reinterpret_cast<const quint8 *>(chsmc.value().constData());
            rawValueCal = static_cast<uint32_t>(rawData[0] + (rawData[1] << 8) + (rawData[2] << 16) + (rawData[3] << 24));
            m_soil_moisture = std::round(*((float*)&rawValueCal));
/*
            QBluetoothUuid at_calibrated(QString("39e1fa0a-84a8-11e2-afba-0002a5d5c51b")); // air temp
            QLowEnergyCharacteristic chatc = serviceLive->characteristic(at_calibrated);

            rawData = reinterpret_cast<const quint8 *>(chatc.value().constData());
            rawValueCal = static_cast<uint32_t>(rawData[0] + (rawData[1] << 8) + (rawData[2] << 16) + (rawData[3] << 24));
            m_temperature = std::round(*((float*)&rawValueCal));
*/
            QBluetoothUuid dli_calibrated(QString("39e1fa0b-84a8-11e2-afba-0002a5d5c51b")); // sunlight?
            QLowEnergyCharacteristic chdlic = serviceLive->characteristic(dli_calibrated);

            rawData = reinterpret_cast<const quint8 *>(chdlic.value().constData());
            rawValueCal = static_cast<uint32_t>(rawData[0] + (rawData[1] << 8) + (rawData[2] << 16) + (rawData[3] << 24));
            m_luminosity = std::round(*((float*)&rawValueCal)) * 11.574 * 53.93;

            /////////

            m_lastUpdate = QDateTime::currentDateTime();

            // Sometimes, Parrot devices send obviously wrong data over ble
            if (m_soil_temperature > -10.f && m_temperature > -10.f &&
                m_soil_temperature < 100.f && m_temperature < 100.f)
            {
                if (m_dbInternal || m_dbExternal)
                {
                    // SQL date format YYYY-MM-DD HH:MM:SS
                    QString tsStr = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:00:00");
                    QString tsFullStr = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

                    QSqlQuery addData;
                    addData.prepare("REPLACE INTO plantData (deviceAddr, ts, ts_full, soilMoisture, soilConductivity, soilTemperature, temperature, luminosity, watertank)"
                                    " VALUES (:deviceAddr, :ts, :ts_full, :hygro, :condu, :stemp, :atemp, :lumi, :tank)");
                    addData.bindValue(":deviceAddr", getAddress());
                    addData.bindValue(":ts", tsStr);
                    addData.bindValue(":ts_full", tsFullStr);
                    addData.bindValue(":hygro", m_soil_moisture);
                    addData.bindValue(":condu", m_soil_conductivity);
                    addData.bindValue(":stemp", m_soil_temperature);
                    addData.bindValue(":atemp", m_temperature);
                    addData.bindValue(":lumi", m_luminosity);
                    addData.bindValue(":tank", m_watertank_level);
                    if (addData.exec() == false)
                        qWarning() << "> addData.exec() ERROR" << addData.lastError().type() << ":" << addData.lastError().text();
                }
            }
            else
            {
                qDebug() << "DeviceParrotPot::serviceDetailsDiscovered_live() values reported are wrong and won't be saved";
            }

            refreshDataFinished(true);
            m_bleController->disconnectFromDevice();

#ifndef QT_NO_DEBUG
            qDebug() << "* DeviceParrotPot update:" << getAddress();
            qDebug() << "- m_firmware:" << m_deviceFirmware;
            qDebug() << "- m_battery:" << m_deviceBattery;
            qDebug() << "- m_soil_moisture:" << m_soil_moisture;
            qDebug() << "- m_soil_conductivity:" << m_soil_conductivity;
            qDebug() << "- m_soil_temperature:" << m_soil_temperature;
            qDebug() << "- m_temperature:" << m_temperature;
            qDebug() << "- m_luminosity:" << m_luminosity;
#endif
        }
    }
}

void DeviceParrotPot::serviceDetailsDiscovered_watering(QLowEnergyService::ServiceState newState)
{
    if (newState == QLowEnergyService::ServiceDiscovered)
    {
        //qDebug() << "DeviceParrotPot::serviceDetailsDiscovered_watering(" << m_deviceAddress << ") > ServiceDiscovered";

        if (serviceWatering && m_ble_action == DeviceUtils::ACTION_UPDATE)
        {
            QBluetoothUuid uuid_wt_lvl(QString("39e1f907-84a8-11e2-afba-0002a5d5c51b"));
            QLowEnergyCharacteristic cwt = serviceWatering->characteristic(uuid_wt_lvl);
            if (cwt.value().size() > 0)
            {
                int water_percent = static_cast<uint8_t>(cwt.value().constData()[0]);
                m_watertank_level = (water_percent * m_watertank_capacity) / 100.0;

#ifndef QT_NO_DEBUG
                qDebug() << "* DeviceParrotPot water tank: " << m_water_level;
#endif
            }
        }

        if (serviceWatering && m_ble_action == DeviceUtils::ACTION_WATERING)
        {
            QBluetoothUuid uuid_wt_trigger(QString("39e1f906-84a8-11e2-afba-0002a5d5c51b"));
            QLowEnergyCharacteristic cwt = serviceWatering->characteristic(uuid_wt_trigger);
            serviceWatering->writeCharacteristic(cwt, QByteArray::fromHex("0800"));
        }
    }
}

void DeviceParrotPot::serviceDetailsDiscovered_clock(QLowEnergyService::ServiceState newState)
{
    if (newState == QLowEnergyService::ServiceDiscovered)
    {
        //qDebug() << "DeviceParrotPot::serviceDetailsDiscovered_clock(" << m_deviceAddress << ") > ServiceDiscovered";

        if (serviceClock)
        {
            QBluetoothUuid uuid_clk(QString("39e1fd01-84a8-11e2-afba-0002a5d5c51b"));
            QLowEnergyCharacteristic cclk = serviceClock->characteristic(uuid_clk);
            if (cclk.value().size() > 0)
            {
                const quint8 *data = reinterpret_cast<const quint8 *>(cclk.value().constData());
                m_device_time = data[0] + (data[1] << 8) + (data[2] << 16) + (data[3] << 24);
                m_device_wall_time = QDateTime::currentSecsSinceEpoch() - m_device_time;

#ifndef QT_NO_DEBUG
                qDebug() << "* DeviceParrotPot clock: " << m_device_time;
#endif
            }
        }
    }
}

void DeviceParrotPot::serviceDetailsDiscovered_history(QLowEnergyService::ServiceState newState)
{
    if (newState == QLowEnergyService::ServiceDiscovered)
    {
        if (serviceHistory)
        {
            //qDebug() << "DeviceParrotPot::serviceDetailsDiscovered_history(" << m_deviceAddress << ") > ServiceDiscovered";
        }
    }
}

/* ************************************************************************** */

void DeviceParrotPot::bleWriteDone(const QLowEnergyCharacteristic &c, const QByteArray &value)
{
    //qDebug() << "DeviceParrotPot::bleWriteDone(" << m_deviceAddress << ") on" << c.name() << " / uuid" << c.uuid() << value.size();
    //qDebug() << "DATA: 0x" << value.toHex();

    Q_UNUSED(c)
    Q_UNUSED(value)
}

void DeviceParrotPot::bleReadDone(const QLowEnergyCharacteristic &c, const QByteArray &value)
{
    //qDebug() << "DeviceParrotPot::bleReadDone(" << m_deviceAddress << ") on" << c.name() << " / uuid" << c.uuid() << value.size();
    //qDebug() << "DATA: 0x" << value.toHex();

    Q_UNUSED(c)
    Q_UNUSED(value)
}

void DeviceParrotPot::bleReadNotify(const QLowEnergyCharacteristic &c, const QByteArray &value)
{
    //qDebug() << "DeviceParrotPot::bleReadNotify(" << m_deviceAddress << ") on" << c.name() << " / uuid" << c.uuid() << value.size();
    //qDebug() << "DATA: 0x" << value.toHex();

    Q_UNUSED(c)
    Q_UNUSED(value)
}

/* ************************************************************************** */
