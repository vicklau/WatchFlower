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

#include "device_wp6003.h"
#include "utils/utils_versionchecker.h"

#include <cstdint>
#include <cmath>

#include <QBluetoothUuid>
#include <QBluetoothServiceInfo>
#include <QLowEnergyService>

#include <QSqlQuery>
#include <QSqlError>

#include <QDateTime>
#include <QDebug>

/* ************************************************************************** */

DeviceWP6003::DeviceWP6003(QString &deviceAddr, QString &deviceName, QObject *parent):
    DeviceSensor(deviceAddr, deviceName, parent)
{
    m_deviceType = DeviceUtils::DEVICE_ENVIRONMENTAL;
    m_deviceSensors += DeviceUtils::SENSOR_TEMPERATURE;
    m_deviceSensors += DeviceUtils::SENSOR_eCO2;
    m_deviceSensors += DeviceUtils::SENSOR_VOC;
    m_deviceSensors += DeviceUtils::SENSOR_HCHO;
}

DeviceWP6003::DeviceWP6003(const QBluetoothDeviceInfo &d, QObject *parent):
    DeviceSensor(d, parent)
{
    m_deviceType = DeviceUtils::DEVICE_ENVIRONMENTAL;
    m_deviceSensors += DeviceUtils::SENSOR_TEMPERATURE;
    m_deviceSensors += DeviceUtils::SENSOR_eCO2;
    m_deviceSensors += DeviceUtils::SENSOR_VOC;
    m_deviceSensors += DeviceUtils::SENSOR_HCHO;
}

DeviceWP6003::~DeviceWP6003()
{
    if (m_bleController) m_bleController->disconnectFromDevice();
    delete serviceData;
}

/* ************************************************************************** */
/* ************************************************************************** */

void DeviceWP6003::serviceScanDone()
{
    //qDebug() << "DeviceWP6003::serviceScanDone(" << m_deviceAddress << ")";

    if (serviceData)
    {
        if (serviceData->state() == QLowEnergyService::DiscoveryRequired)
        {
            connect(serviceData, &QLowEnergyService::stateChanged, this, &DeviceWP6003::serviceDetailsDiscovered_data);
            connect(serviceData, &QLowEnergyService::characteristicChanged, this, &DeviceWP6003::bleReadNotify);
            //connect(serviceData, &QLowEnergyService::characteristicRead, this, &DeviceWP6003::bleReadDone);
            //connect(serviceData, &QLowEnergyService::characteristicWritten, this, &DeviceWP6003::bleWriteDone);

            // Windows hack, see: QTBUG-80770 and QTBUG-78488
            QTimer::singleShot(0, this, [=] () { serviceData->discoverDetails(); });
        }
    }
}

/* ************************************************************************** */

void DeviceWP6003::addLowEnergyService(const QBluetoothUuid &uuid)
{
    //qDebug() << "DeviceWP6003::addLowEnergyService(" << uuid.toString() << ")";

    if (uuid.toString() == "{0000fff0-0000-1000-8000-00805f9b34fb}") // data
    {
        delete serviceData;
        serviceData = nullptr;

        serviceData = m_bleController->createServiceObject(uuid);
        if (!serviceData)
            qWarning() << "Cannot create service (data) for uuid:" << uuid.toString();
    }
}

/* ************************************************************************** */

void DeviceWP6003::serviceDetailsDiscovered_data(QLowEnergyService::ServiceState newState)
{
    if (newState == QLowEnergyService::ServiceDiscovered)
    {
        //qDebug() << "DeviceWP6003::serviceDetailsDiscovered_data(" << m_deviceAddress << ") > ServiceDiscovered";

        if (serviceData)
        {
            QBluetoothUuid uuid_tx(QString("0000FFF1-0000-1000-8000-00805F9B34FB"));
            QBluetoothUuid uuid_rx(QString("0000FFF4-0000-1000-8000-00805F9B34FB"));

            // Characteristic "RX" // NOTIFY
            {
                QLowEnergyCharacteristic crx = serviceData->characteristic(uuid_rx);
                m_notificationDesc = crx.descriptor(QBluetoothUuid::ClientCharacteristicConfiguration);
                serviceData->writeDescriptor(m_notificationDesc, QByteArray::fromHex("0100"));
            }

            // Characteristic "TX" // WRITE
            {
                QLowEnergyCharacteristic ctx = serviceData->characteristic(uuid_tx);

                // send initialize command "ee"
                //serviceData->writeCharacteristic(ctx, QByteArray::fromHex("ee"), QLowEnergyService::WriteWithoutResponse);

                // send command "aa" + datetime
                QDateTime cdt = QDateTime::currentDateTime();
                QByteArray cmd(QByteArray::fromHex("aa"));
                cmd.push_back(cdt.date().year()%100);
                cmd.push_back(cdt.date().month());
                cmd.push_back(cdt.date().day());
                cmd.push_back(cdt.time().hour());
                cmd.push_back(cdt.time().minute());
                cmd.push_back(cdt.time().second());
                serviceData->writeCharacteristic(ctx, cmd, QLowEnergyService::WriteWithoutResponse);

                // set notify interval "ae" + interval
                //serviceData->writeCharacteristic(ctx, QByteArray::fromHex("ae0101"), QLowEnergyService::WriteWithoutResponse);

                // send notify request "ab"
                serviceData->writeCharacteristic(ctx, QByteArray::fromHex("ab"), QLowEnergyService::WriteWithoutResponse);
            }
        }
    }
}

/* ************************************************************************** */

void DeviceWP6003::bleWriteDone(const QLowEnergyCharacteristic &c, const QByteArray &value)
{
    //qDebug() << "DeviceWP6003::bleWriteDone(" << m_deviceAddress << ")";
    //qDebug() << "DATA: 0x" << value.toHex();

    Q_UNUSED(c)
    Q_UNUSED(value)
}

void DeviceWP6003::bleReadDone(const QLowEnergyCharacteristic &c, const QByteArray &value)
{
    //qDebug() << "DeviceWP6003::bleReadDone(" << m_deviceAddress << ") on" << c.name() << " / uuid" << c.uuid() << value.size();
    //qDebug() << "DATA: 0x" << value.toHex();

    Q_UNUSED(c)
    Q_UNUSED(value)
}

void DeviceWP6003::bleReadNotify(const QLowEnergyCharacteristic &c, const QByteArray &value)
{
    //qDebug() << "DeviceWP6003::bleReadNotify(" << m_deviceAddress << ") on" << c.name() << " / uuid" << c.uuid() << value.size();
    //qDebug() << "DATA: 0x" << value.toHex();

    QBluetoothUuid uuid_tx(QString("0000FFF1-0000-1000-8000-00805F9B34FB"));
    QBluetoothUuid uuid_rx(QString("0000FFF4-0000-1000-8000-00805F9B34FB"));

    if (c.uuid() == uuid_rx)
    {
        const quint8 *data = reinterpret_cast<const quint8 *>(value.constData());

        if (data[0] == 170) // 0xaa
        {
            //qDebug() << "* DeviceWP6003 update:" << getAddress();
            //qDebug() << "- data?" << data[6];
        }
        else if (data[0] == 10) // 0x0a
        {
            QDate d(2000 + data[1], data[2], data[3]);
            QTime t(data[4], data[5]);
            QDateTime tmcd(d, t);

            int16_t temp = static_cast<int16_t>((data[6] << 8) + data[7]);
            uint16_t voc = static_cast<uint16_t>((data[10] << 8) + data[11]);
            uint16_t hcho = static_cast<uint16_t>((data[12] << 8) + data[13]);
            uint16_t co2 = static_cast<uint16_t>((data[16] << 8) + data[17]);

            if (voc < 16383 && hcho < 16383)
            {
                m_voc = voc;
                m_hcho = hcho;
            }
            m_co2 = co2;
            m_temperature = temp / 10.f;

            m_lastUpdate = QDateTime::currentDateTime();

            if (m_dbInternal || m_dbExternal)
            {
                // SQL date format YYYY-MM-DD HH:MM:SS
                QString tsStr = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

                QSqlQuery addData;
                addData.prepare("REPLACE INTO sensorData (deviceAddr, timestamp, temperature, co2, voc, hcho)"
                                " VALUES (:deviceAddr, :ts, :temp, :co2, :voc, :hcho)");
                addData.bindValue(":deviceAddr", getAddress());
                addData.bindValue(":ts", tsStr);
                addData.bindValue(":temp", m_temperature);
                addData.bindValue(":co2", m_co2);
                addData.bindValue(":voc", m_voc);
                addData.bindValue(":hcho", m_hcho);
                if (addData.exec() == false)
                    qWarning() << "> addData.exec() ERROR" << addData.lastError().type() << ":" << addData.lastError().text();

                m_lastUpdateDatabase = m_lastUpdate;
            }

            refreshDataFinished(true);
            m_bleController->disconnectFromDevice();

#ifndef QT_NO_DEBUG
            //qDebug() << "* DeviceWP6003 update:" << getAddress();
            //qDebug() << "- timecode:" << tmcd;
            //qDebug() << "- temperature:" << m_temperature;
            //qDebug() << "- TVOC:" << m_voc;
            //qDebug() << "- HCHO:" << m_hcho;
            //qDebug() << "- eCO2:" << m_co2;
#endif
        }
    }
}

/* ************************************************************************** */
