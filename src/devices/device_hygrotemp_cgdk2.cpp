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
 * \date      2021
 * \author    Emeric Grange <emeric.grange@gmail.com>
 */

#include "device_hygrotemp_cgdk2.h"
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

DeviceHygrotempCGDK2::DeviceHygrotempCGDK2(QString &deviceAddr, QString &deviceName, QObject *parent):
    DeviceSensor(deviceAddr, deviceName, parent)
{
    m_deviceType = DeviceUtils::DEVICE_THERMOMETER;
    m_deviceCapabilities += DeviceUtils::DEVICE_REALTIME;
    m_deviceSensors += DeviceUtils::SENSOR_TEMPERATURE;
    m_deviceSensors += DeviceUtils::SENSOR_HUMIDITY;
}

DeviceHygrotempCGDK2::DeviceHygrotempCGDK2(const QBluetoothDeviceInfo &d, QObject *parent):
    DeviceSensor(d, parent)
{
    m_deviceType = DeviceUtils::DEVICE_THERMOMETER;
    m_deviceCapabilities += DeviceUtils::DEVICE_REALTIME;
    m_deviceSensors += DeviceUtils::SENSOR_TEMPERATURE;
    m_deviceSensors += DeviceUtils::SENSOR_HUMIDITY;
}

DeviceHygrotempCGDK2::~DeviceHygrotempCGDK2()
{
    delete serviceInfos;
    delete serviceData;
}

/* ************************************************************************** */
/* ************************************************************************** */

void DeviceHygrotempCGDK2::serviceScanDone()
{
    qDebug() << "DeviceHygrotempCGDK2::serviceScanDone(" << m_deviceAddress << ")";

    if (serviceInfos)
    {
        if (m_deviceFirmware.isEmpty() || m_deviceFirmware == "UNKN")
        {
            if (serviceInfos->state() == QLowEnergyService::DiscoveryRequired)
            {
                connect(serviceInfos, &QLowEnergyService::stateChanged, this, &DeviceHygrotempCGDK2::serviceDetailsDiscovered_infos);

                // Windows hack, see: QTBUG-80770 and QTBUG-78488
                QTimer::singleShot(0, this, [=] () { serviceInfos->discoverDetails(); });
            }
        }
    }

    if (serviceData)
    {
        if (serviceData->state() == QLowEnergyService::DiscoveryRequired)
        {
            connect(serviceData, &QLowEnergyService::stateChanged, this, &DeviceHygrotempCGDK2::serviceDetailsDiscovered_data);
            connect(serviceData, &QLowEnergyService::characteristicChanged, this, &DeviceHygrotempCGDK2::bleReadNotify);

            // Windows hack, see: QTBUG-80770 and QTBUG-78488
            QTimer::singleShot(0, this, [=] () { serviceData->discoverDetails(); });
        }
    }
}

/* ************************************************************************** */

void DeviceHygrotempCGDK2::addLowEnergyService(const QBluetoothUuid &uuid)
{
    //qDebug() << "DeviceHygrotempCGDK2::addLowEnergyService(" << uuid.toString() << ")";

    if (uuid.toString() == "{0000180a-0000-1000-8000-00805f9b34fb}") // Device Information service
    {
        delete serviceInfos;
        serviceInfos = nullptr;

        serviceInfos = m_bleController->createServiceObject(uuid);
        if (!serviceInfos)
            qWarning() << "Cannot create service (infos) for uuid:" << uuid.toString();
    }

    if (uuid.toString() == "{22210000-554a-4546-5542-46534450464d}") // (custom) data service
    {
        delete serviceData;
        serviceData = nullptr;

        serviceData = m_bleController->createServiceObject(uuid);
        if (!serviceData)
            qWarning() << "Cannot create service (data) for uuid:" << uuid.toString();
    }
}

/* ************************************************************************** */

void DeviceHygrotempCGDK2::serviceDetailsDiscovered_infos(QLowEnergyService::ServiceState newState)
{
    if (newState == QLowEnergyService::ServiceDiscovered)
    {
        //qDebug() << "DeviceHygrotempCGDK2::serviceDetailsDiscovered_infos(" << m_deviceAddress << ") > ServiceDiscovered";

        if (serviceInfos)
        {
            // Characteristic "Firmware Revision String"
            QBluetoothUuid c(QString("00002a26-0000-1000-8000-00805f9b34fb")); // handle 0x0b
            QLowEnergyCharacteristic chc = serviceInfos->characteristic(c);
            if (chc.value().size() > 0)
            {
               QString fw = chc.value();
               setFirmware(fw);
            }

            if (m_deviceFirmware.size() == 10)
            {
                if (Version(m_deviceFirmware) >= Version(LATEST_KNOWN_FIRMWARE_HYGROTEMP_EINK))
                {
                    m_firmware_uptodate = true;
                    Q_EMIT sensorUpdated();
                }
            }
        }
    }
}

void DeviceHygrotempCGDK2::serviceDetailsDiscovered_data(QLowEnergyService::ServiceState newState)
{
    if (newState == QLowEnergyService::ServiceDiscovered)
    {
        //qDebug() << "DeviceHygrotempCGDK2::serviceDetailsDiscovered_data(" << m_deviceAddress << ") > ServiceDiscovered";

        if (serviceData)
        {
            // hygrotemp readings
            {
                QBluetoothUuid a(QString("00000100-0000-1000-8000-00805f9b34fb"));
                QLowEnergyCharacteristic cha = serviceData->characteristic(a);
                m_notificationDesc = cha.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
                serviceData->writeDescriptor(m_notificationDesc, QByteArray::fromHex("0100"));
            }
        }
    }
}
/* ************************************************************************** */

void DeviceHygrotempCGDK2::bleWriteDone(const QLowEnergyCharacteristic &, const QByteArray &)
{
    //qDebug() << "DeviceHygrotempCGDK2::bleWriteDone(" << m_deviceAddress << ")";
    //qDebug() << "DATA: 0x" << value.toHex();
}

void DeviceHygrotempCGDK2::bleReadDone(const QLowEnergyCharacteristic &, const QByteArray &)
{
    //qDebug() << "DeviceHygrotempCGDK2::bleReadDone(" << m_deviceAddress << ") on" << c.name() << " / uuid" << c.uuid() << value.size();
    //qDebug() << "DATA: 0x" << value.toHex();
}

void DeviceHygrotempCGDK2::bleReadNotify(const QLowEnergyCharacteristic &c, const QByteArray &value)
{
    //qDebug() << "DeviceHygrotempCGDK2::bleReadNotify(" << m_deviceAddress << ") on" << c.name() << " / uuid" << c.uuid() << value.size();
    //qDebug() << "DATA: 0x" << value.toHex();

    const quint8 *data = reinterpret_cast<const quint8 *>(value.constData());

    if (c.uuid().toString() == "{00000100-0000-1000-8000-00805f9b34fb}")
    {
        // sensor data

        if (value.size() == 7)
        {
            m_temperature = static_cast<int16_t>(data[2] + (data[3] << 8)) / 10.f;
            m_humidity = static_cast<int16_t>(data[4] + (data[5] << 8)) / 10;

            m_lastUpdate = QDateTime::currentDateTime();

            if (m_dbInternal || m_dbExternal)
            {
                // SQL date format YYYY-MM-DD HH:MM:SS
                QString tsStr = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:00:00");
                QString tsFullStr = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

                QSqlQuery addData;
                addData.prepare("REPLACE INTO plantData (deviceAddr, ts, ts_full, temperature, humidity)"
                                " VALUES (:deviceAddr, :ts, :ts_full, :temp, :humi)");
                addData.bindValue(":deviceAddr", getAddress());
                addData.bindValue(":ts", tsStr);
                addData.bindValue(":ts_full", tsFullStr);
                addData.bindValue(":temp", m_temperature);
                addData.bindValue(":humi", m_humidity);
                if (addData.exec() == false)
                    qWarning() << "> addData.exec() ERROR" << addData.lastError().type() << ":" << addData.lastError().text();
            }

            if (m_ble_action == DeviceUtils::ACTION_UPDATE_REALTIME)
            {
                refreshDataRealtime(true);
            }
            else
            {
                refreshDataFinished(true);
                m_bleController->disconnectFromDevice();
            }

#ifndef QT_NO_DEBUG
            qDebug() << "* DeviceHygrotempCGDK2 update:" << getAddress();
            qDebug() << "- m_firmware:" << m_deviceFirmware;
            qDebug() << "- m_battery:" << m_deviceBattery;
            qDebug() << "- m_temperature:" << m_temperature;
            qDebug() << "- m_humidity:" << m_humidity;
#endif
        }
    }
}

void DeviceHygrotempCGDK2::confirmedDescriptorWrite(const QLowEnergyDescriptor &d, const QByteArray &value)
{
    //qDebug() << "DeviceHygrotempCGDK2::confirmedDescriptorWrite!";

    if (d.isValid() && d == m_notificationDesc && value == QByteArray::fromHex("0000"))
    {
        //qDebug() << "confirmedDescriptorWrite() disconnect?!";

        //disabled notifications -> assume disconnect intent
        //m_control->disconnectFromDevice();
        //delete m_service;
        //m_service = nullptr;
    }
}
