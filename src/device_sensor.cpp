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

#include "device_sensor.h"
#include "SettingsManager.h"
#include "DatabaseManager.h"
#include "DeviceManager.h"
#include "NotificationManager.h"
#include "utils/utils_versionchecker.h"

#include <QSqlQuery>
#include <QSqlError>

#include <QDateTime>
#include <QTimer>
#include <QDebug>

/* ************************************************************************** */

DeviceSensor::DeviceSensor(QString &deviceAddr, QString &deviceName, QObject *parent) :
    Device(deviceAddr, deviceName, parent)
{
    // Database
    DatabaseManager *db = DatabaseManager::getInstance();
    if (db)
    {
        m_dbInternal = db->hasDatabaseInternal();
        m_dbExternal = db->hasDatabaseExternal();
    }

    // Load device infos and limits
    if (m_dbInternal || m_dbExternal)
    {
        getSqlDeviceInfos();
        getSqlPlantLimits();
        // Load initial data into the GUI (if they are no more than 12h old)
        bool data = false;
        if (!data) data = getSqlPlantData(12*60);
        if (!data) data = getSqlSensorData(12*60);
    }

    // Configure timeout timer
    m_timeoutTimer.setSingleShot(true);
    connect(&m_timeoutTimer, &QTimer::timeout, this, &DeviceSensor::actionTimedout);

    // Configure update timer (only started on desktop)
    connect(&m_updateTimer, &QTimer::timeout, this, &DeviceSensor::refreshStart);
}

DeviceSensor::DeviceSensor(const QBluetoothDeviceInfo &d, QObject *parent) :
    Device(d, parent)
{
    // Database
    DatabaseManager *db = DatabaseManager::getInstance();
    if (db)
    {
        m_dbInternal = db->hasDatabaseInternal();
        m_dbExternal = db->hasDatabaseExternal();
    }

    // Load device infos and limits
    if (m_dbInternal || m_dbExternal)
    {
        getSqlDeviceInfos();
        getSqlPlantLimits();
        // Load initial data into the GUI (if they are no more than 12h old)
        bool data = false;
        if (!data) data = getSqlPlantData(12*60);
        if (!data) data = getSqlSensorData(12*60);
    }

    // Configure timeout timer
    m_timeoutTimer.setSingleShot(true);
    connect(&m_timeoutTimer, &QTimer::timeout, this, &DeviceSensor::actionTimedout);

    // Configure update timer (only started on desktop)
    connect(&m_updateTimer, &QTimer::timeout, this, &DeviceSensor::refreshStart);
}

DeviceSensor::~DeviceSensor()
{
    //
}

/* ************************************************************************** */
/* ************************************************************************** */

void DeviceSensor::refreshDataFinished(bool status, bool cached)
{
    //qDebug() << "DeviceSensor::refreshDataFinished()" << getAddress() << getName();

    Device::refreshDataFinished(status, cached);

    if (status == true)
    {
        // Plant sensor?
        if (hasSoilMoistureSensor())
        {
            SettingsManager *sm = SettingsManager::getInstance();

            // Reorder the device list by water level, if needed
            if (sm->getOrderBy() == "waterlevel")
            {
                if (parent()) static_cast<DeviceManager *>(parent())->invalidate();
            }

            // 'Water me' notification, if enabled
            if (sm->getNotifs())
            {
                // Only if the sensor has a plant
                if (m_soil_moisture > 0 && m_soil_moisture < m_limitHygroMin)
                {
                    NotificationManager *nm = NotificationManager::getInstance();
                    if (nm)
                    {
                        QString message;
                        if (!m_associatedName.isEmpty())
                            message = tr("You need to water your '%1' now!").arg(m_associatedName);
                        else if (!m_locationName.isEmpty())
                            message = tr("You need to water the plant near '%1'").arg(m_locationName);
                        else
                            message = tr("You need to water one of your (unnamed) plants!");

                        nm->setNotification(message);
                    }
                }
            }
        }
    }
}

void DeviceSensor::refreshHistoryFinished(bool status)
{
    //qDebug() << "DeviceSensor::refreshHistoryFinished()" << getAddress() << getName();

    Device::refreshHistoryFinished(status);

    m_history_entry_count = -1;
    m_history_entry_index = -1;
    m_history_session_count = -1;
    m_history_session_read = -1;

    if (m_lastHistorySync.isValid())
    {
        // Write last sync
        QSqlQuery updateDeviceLastSync;
        updateDeviceLastSync.prepare("UPDATE devices SET lastSync = :sync WHERE deviceAddr = :deviceAddr");
        updateDeviceLastSync.bindValue(":sync", m_lastHistorySync.toString("yyyy-MM-dd hh:mm:ss"));
        updateDeviceLastSync.bindValue(":deviceAddr", getAddress());
        if (updateDeviceLastSync.exec() == false)
            qWarning() << "> updateDeviceLastSync.exec() ERROR" << updateDeviceLastSync.lastError().type() << ":" << updateDeviceLastSync.lastError().text();
    }
}

/* ************************************************************************** */

bool DeviceSensor::getSqlDeviceInfos()
{
    //qDebug() << "DeviceSensor::getSqlDeviceInfos(" << m_deviceAddress << ")";
    bool status = Device::getSqlDeviceInfos();

    if ((m_deviceName == "Flower care" || m_deviceName == "Flower mate") && (m_deviceFirmware.size() == 5))
    {
        if (Version(m_deviceFirmware) >= Version(LATEST_KNOWN_FIRMWARE_FLOWERCARE))
        {
            m_firmware_uptodate = true;
            Q_EMIT sensorUpdated();
        }
    }
    else if ((m_deviceName.startsWith("Flower power")) && (m_deviceFirmware.size() == 5))
    {
        if (Version(m_deviceFirmware) >= Version(LATEST_KNOWN_FIRMWARE_FLOWERPOWER))
        {
            m_firmware_uptodate = true;
            Q_EMIT sensorUpdated();
        }
    }
    else if ((m_deviceName.startsWith("Parrot pot")) && (m_deviceFirmware.size() == 6))
    {
        if (Version(m_deviceFirmware) >= Version(LATEST_KNOWN_FIRMWARE_PARROTPOT))
        {
            m_firmware_uptodate = true;
            Q_EMIT sensorUpdated();
        }
    }
    else if ((m_deviceName == "ropot") && (m_deviceFirmware.size() == 5))
    {
        if (Version(m_deviceFirmware) >= Version(LATEST_KNOWN_FIRMWARE_ROPOT))
        {
            m_firmware_uptodate = true;
            Q_EMIT sensorUpdated();
        }
    }
    else if ((m_deviceName == "MJ_HT_V1") && (m_deviceFirmware.size() == 8))
    {
        if (Version(m_deviceFirmware) >= Version(LATEST_KNOWN_FIRMWARE_HYGROTEMP_LCD))
        {
            m_firmware_uptodate = true;
            Q_EMIT sensorUpdated();
        }
    }
    else if ((m_deviceName == "ClearGrass Temp & RH") && (m_deviceFirmware.size() == 10))
    {
        if (Version(m_deviceFirmware) >= Version(LATEST_KNOWN_FIRMWARE_HYGROTEMP_EINK))
        {
            m_firmware_uptodate = true;
            Q_EMIT sensorUpdated();
        }
    }
    else if ((m_deviceName.startsWith("Qingping Temp & RH")) && (m_deviceFirmware.size() == 10))
    {
        if (Version(m_deviceFirmware) >= Version(LATEST_KNOWN_FIRMWARE_HYGROTEMP_EINK))
        {
            m_firmware_uptodate = true;
            Q_EMIT sensorUpdated();
        }
    }
    else if ((m_deviceName == "LYWSD02") && (m_deviceFirmware.size() == 10))
    {
        if (Version(m_deviceFirmware) >= Version(LATEST_KNOWN_FIRMWARE_HYGROTEMP_CLOCK))
        {
            m_firmware_uptodate = true;
            Q_EMIT sensorUpdated();
        }
    }
    else if ((m_deviceName == "LYWSD03MMC") && (m_deviceFirmware.size() == 10))
    {
        if (Version(m_deviceFirmware) >= Version(LATEST_KNOWN_FIRMWARE_HYGROTEMP_SQUARE))
        {
            m_firmware_uptodate = true;
            Q_EMIT sensorUpdated();
        }
    }
    else if ((m_deviceName == "MHO-C401") && (m_deviceFirmware.size() == 10))
    {
        if (Version(m_deviceFirmware) >= Version(LATEST_KNOWN_FIRMWARE_HYGROTEMP_EINK2))
        {
            m_firmware_uptodate = true;
            Q_EMIT sensorUpdated();
        }
    }
    else if ((m_deviceName == "MHO-303") && (m_deviceFirmware.size() == 10))
    {
        if (Version(m_deviceFirmware) >= Version(LATEST_KNOWN_FIRMWARE_HYGROTEMP_ALARM))
        {
            m_firmware_uptodate = true;
            Q_EMIT sensorUpdated();
        }
    }

    return status;
}

bool DeviceSensor::getSqlPlantLimits()
{
    //qDebug() << "DeviceSensor::getSqlPlantLimits(" << m_deviceAddress << ")";
    bool status = false;

    QSqlQuery getLimits;
    getLimits.prepare("SELECT hygroMin, hygroMax, conduMin, conduMax, phMin, phMax, " \
                      " tempMin, tempMax, humiMin, humiMax, " \
                      " luxMin, luxMax, mmolMin, mmolMax " \
                      "FROM plantLimits WHERE deviceAddr = :deviceAddr");
    getLimits.bindValue(":deviceAddr", getAddress());
    getLimits.exec();
    while (getLimits.next())
    {
        m_limitHygroMin = getLimits.value(0).toInt();
        m_limitHygroMax = getLimits.value(1).toInt();
        m_limitConduMin = getLimits.value(2).toInt();
        m_limitConduMax = getLimits.value(3).toInt();
        m_limitPhMin = getLimits.value(4).toInt();
        m_limitPhMax = getLimits.value(5).toInt();
        m_limitTempMin = getLimits.value(6).toInt();
        m_limitTempMax = getLimits.value(7).toInt();
        m_limitHumiMin = getLimits.value(8).toInt();
        m_limitHumiMax = getLimits.value(9).toInt();
        m_limitLuxMin = getLimits.value(10).toInt();
        m_limitLuxMax = getLimits.value(11).toInt();
        m_limitMmolMin = getLimits.value(12).toInt();
        m_limitMmolMax = getLimits.value(13).toInt();

        status = true;
        Q_EMIT limitsUpdated();
    }

    return status;
}

bool DeviceSensor::getSqlPlantData(int minutes)
{
    //qDebug() << "DeviceSensor::getSqlPlantData(" << m_deviceAddress << ")";
    bool status = false;

    QSqlQuery cachedData;
    if (m_dbInternal) // sqlite
    {
        cachedData.prepare("SELECT ts_full, soilMoisture, soilConductivity, soilTemperature, soilPH, temperature, humidity, luminosity, watertank " \
                           "FROM plantData " \
                           "WHERE deviceAddr = :deviceAddr AND ts_full >= datetime('now', 'localtime', '-" + QString::number(minutes) + " minutes');");
    }
    else if (m_dbExternal) // mysql
    {
        cachedData.prepare("SELECT DATE_FORMAT(ts_full, '%Y-%m-%e %H:%i:%s'), soilMoisture, soilConductivity, soilTemperature, soilPH, temperature, humidity, luminosity, watertank " \
                           "FROM plantData " \
                           "WHERE deviceAddr = :deviceAddr AND ts_full >= TIMESTAMPADD(MINUTE,-" + QString::number(minutes) + ",NOW());");
    }
    cachedData.bindValue(":deviceAddr", getAddress());

    if (cachedData.exec() == false)
    {
        qWarning() << "> cachedData.exec() ERROR" << cachedData.lastError().type() << ":" << cachedData.lastError().text();
    }
    else
    {
#ifndef QT_NO_DEBUG
        qDebug() << "* Device loaded:" << getAddress();
#endif
    }

    while (cachedData.next())
    {
        m_soil_moisture =  cachedData.value(1).toInt();
        m_soil_conductivity = cachedData.value(2).toInt();
        m_soil_temperature = cachedData.value(3).toFloat();
        m_soil_ph = cachedData.value(4).toFloat();
        m_temperature = cachedData.value(5).toFloat();
        m_humidity =  cachedData.value(6).toFloat();
        m_luminosity = cachedData.value(7).toInt();
        m_watertank_level = cachedData.value(8).toFloat();

        QString datetime = cachedData.value(0).toString();
        m_lastUpdateDatabase = m_lastUpdate = QDateTime::fromString(datetime, "yyyy-MM-dd hh:mm:ss");
/*
        qDebug() << ">> timestamp" << m_lastUpdate;
        qDebug() << "- m_soil_moisture:" << m_soil_moisture;
        qDebug() << "- m_soil_conductivity:" << m_soil_conductivity;
        qDebug() << "- m_soil_temperature:" << m_soil_temperature;
        qDebug() << "- m_soil_ph:" << m_soil_ph;
        qDebug() << "- m_temperature:" << m_temperature;
        qDebug() << "- m_humidity:" << m_humidity;
        qDebug() << "- m_luminosity:" << m_luminosity;
        qDebug() << "- m_watertank_level:" << m_watertank_level;
*/
        status = true;
    }

    refreshDataFinished(status, true);
    return status;
}

/* ************************************************************************** */

bool DeviceSensor::getSqlSensorLimits()
{
    //qDebug() << "DeviceSensor::getSqlSensorLimits(" << m_deviceAddress << ")";
    bool status = false;

    // TODO

    return status;
}

bool DeviceSensor::getSqlSensorData(int minutes)
{
    //qDebug() << "DeviceSensor::getSqlSensorData(" << m_deviceAddress << ")";
    bool status = false;

    QSqlQuery cachedData;
    if (m_dbInternal) // sqlite
    {
        cachedData.prepare("SELECT timestamp, temperature, humidity, pressure, luminosity, uv, sound, water, windDirection, windSpeed, " \
                             "pm1, pm25, pm10, o2, o3, co, co2, no2, so2, voc, hcho, geiger " \
                           "FROM sensorData " \
                           "WHERE deviceAddr = :deviceAddr AND timestamp >= datetime('now', 'localtime', '-" + QString::number(minutes) + " minutes');");
    }
    else if (m_dbExternal) // mysql
    {
        cachedData.prepare("SELECT DATE_FORMAT(timestamp, '%Y-%m-%e %H:%i:%s'), temperature, humidity, pressure, luminosity, uv, sound, water, windDirection, windSpeed, " \
                             "pm1, pm25, pm10, o2, o3, co, co2, no2, so2, voc, hcho, geiger " \
                           "FROM sensorData " \
                           "WHERE deviceAddr = :deviceAddr AND timestamp >= TIMESTAMPADD(MINUTE,-" + QString::number(minutes) + ",NOW());");
    }
    cachedData.bindValue(":deviceAddr", getAddress());

    if (cachedData.exec() == false)
    {
        qWarning() << "> cachedData.exec() ERROR" << cachedData.lastError().type() << ":" << cachedData.lastError().text();
    }
    else
    {
#ifndef QT_NO_DEBUG
        qDebug() << "* Device loaded:" << getAddress();
#endif
    }

    while (cachedData.next())
    {
        // hygrometer data
        m_temperature = cachedData.value(1).toFloat();
        m_humidity = cachedData.value(2).toFloat();
        // environmental data
        m_pressure = cachedData.value(3).toFloat();
        m_luminosity = cachedData.value(4).toInt();
        m_uv = cachedData.value(5).toFloat();
        m_sound_level = cachedData.value(6).toFloat();
        m_water_level = cachedData.value(7).toFloat();
        m_wind_direction = cachedData.value(8).toFloat();
        m_wind_speed = cachedData.value(9).toFloat();
        m_pm_1 = cachedData.value(10).toFloat();
        m_pm_25 = cachedData.value(11).toFloat();
        m_pm_10 = cachedData.value(12).toFloat();
        m_o2 = cachedData.value(13).toFloat();
        m_o3 = cachedData.value(14).toFloat();
        m_co = cachedData.value(15).toFloat();
        m_co2 = cachedData.value(16).toFloat();
        m_no2 = cachedData.value(17).toFloat();
        m_so2 = cachedData.value(18).toFloat();
        m_voc = cachedData.value(19).toFloat();
        m_hcho = cachedData.value(20).toFloat();
        m_rh = m_rm = m_rs = cachedData.value(21).toFloat();

        QString datetime = cachedData.value(0).toString();
        m_lastUpdateDatabase = m_lastUpdate = QDateTime::fromString(datetime, "yyyy-MM-dd hh:mm:ss");
/*
        qDebug() << ">> timestamp" << m_lastUpdate;
        qDebug() << "- m_temperature:" << m_temperature;
        qDebug() << "- m_humidity:" << m_humidity;
        qDebug() << "- m_pressure:" << m_pressure;
        qDebug() << "- m_luminosity:" << m_luminosity;
        qDebug() << "- m_uv:" << m_uv;
        qDebug() << "- m_luminosity:" << m_luminosity;
        qDebug() << "- m_water_level:" << m_water_level;
        qDebug() << "- m_sound_level:" << m_sound_level;
        qDebug() << "- m_wind_direction:" << m_wind_direction;
        qDebug() << "- m_wind_speed:" << m_wind_speed;
        qDebug() << "- m_pm_1:" << m_pm_1;
        qDebug() << "- m_pm_25:" << m_pm_25;
        qDebug() << "- m_pm_10:" << m_pm_10;
        qDebug() << "- m_o2:" << m_o2;
        qDebug() << "- m_o3:" << m_o3;
        qDebug() << "- m_co:" << m_co;
        qDebug() << "- m_co2:" << m_co2;
        qDebug() << "- m_no2:" << m_no2;
        qDebug() << "- m_so2:" << m_so2;
        qDebug() << "- m_voc:" << m_voc;
        qDebug() << "- m_hcho:" << m_hcho;
        qDebug() << "- m_rm:" << m_rm;
*/
        status = true;
    }

    refreshDataFinished(status, true);
    return status;
}

/* ************************************************************************** */
/* ************************************************************************** */

bool DeviceSensor::hasData() const
{
    QString tableName;

    if (isPlantSensor() || isThermometer())
    {
        // If we have immediate data (<12h old)
        if (m_soil_moisture > 0 || m_soil_conductivity > 0 || m_soil_temperature > 0 ||
            m_temperature > -20.f || m_humidity > 0 || m_luminosity > 0)
            return true;

        tableName = "plantData";
    }
    else if (isEnvironmentalSensor())
    {
        // If we have immediate data (<12h old)
        if (m_temperature > -20.f || m_humidity > 0 || m_luminosity > 0 ||
            m_pm_10 > 0 || m_co2 > 0 || m_voc > 0 || m_rm > 0)
            return true;

        tableName = "sensorData";
    }

    // Otherwise, check if we have stored data
    if (m_dbInternal || m_dbExternal)
    {
        QSqlQuery hasData;
        hasData.prepare("SELECT COUNT(*) FROM " + tableName + " WHERE deviceAddr = :deviceAddr;");
        hasData.bindValue(":deviceAddr", getAddress());

        if (hasData.exec() == false)
            qWarning() << "> hasData.exec() ERROR" << hasData.lastError().type() << ":" << hasData.lastError().text();

        while (hasData.next())
        {
            if (hasData.value(0).toInt() > 0) // data count
                return true;
        }
    }

    return false;
}

bool DeviceSensor::hasData(const QString &dataName) const
{
    QString tableName;

    if (isPlantSensor() || isThermometer())
    {
        // If we have immediate data (<12h old)
        if (dataName == "soilMoisture" && m_soil_moisture > 0)
            return true;
        else if (dataName == "soilConductivity" && m_soil_conductivity > 0)
            return true;
        else if (dataName == "soilTemperature" && m_soil_temperature > 0)
            return true;
        else if (dataName == "temperature" && m_temperature > -20.f)
            return true;
        else if (dataName == "humidity" && m_humidity > 0)
            return true;
        else if (dataName == "luminosity" && m_luminosity > 0)
            return true;

        tableName = "plantData";
    }
    else if (isEnvironmentalSensor())
    {
        // If we have immediate data (<12h old)
        if (dataName == "temperature" && m_temperature > -20.f)
            return true;
        else if (dataName == "humidity" && m_humidity > 0)
            return true;

        tableName = "sensorData";
    }

    // Otherwise, check if we have stored data
    if (m_dbInternal || m_dbExternal)
    {
        QSqlQuery hasData;
        hasData.prepare("SELECT COUNT(" + dataName + ") FROM " + tableName + " WHERE deviceAddr = :deviceAddr AND " + dataName + " > 0;");
        hasData.bindValue(":deviceAddr", getAddress());

        if (hasData.exec() == false)
            qWarning() << "> hasData.exec() ERROR" << hasData.lastError().type() << ":" << hasData.lastError().text();

        while (hasData.next())
        {
            if (hasData.value(0).toInt() > 0) // data count
                return true;
        }
    }

    return false;
}

int DeviceSensor::countData(const QString &dataName, int days) const
{
    // Count stored data
    if (m_dbInternal || m_dbExternal)
    {
        QString tableName = "plantData";
        if (isEnvironmentalSensor()) tableName = "sensorData";

        QSqlQuery dataCount;
        if (m_dbInternal) // sqlite
        {
            dataCount.prepare("SELECT COUNT(" + dataName + ")" \
                              "FROM " + tableName + " " \
                              "WHERE deviceAddr = :deviceAddr " \
                                "AND " + dataName + " > -20 AND ts >= datetime('now','-" + QString::number(days) + " day');");
        }
        else if (m_dbExternal) // mysql
        {
            dataCount.prepare("SELECT COUNT(" + dataName + ")" \
                              "FROM " + tableName + " " \
                              "WHERE deviceAddr = :deviceAddr " \
                                "AND " + dataName + " > -20 AND ts >= DATE_SUB(NOW(), INTERVAL " + QString::number(days) + " DAY);");
        }
        dataCount.bindValue(":deviceAddr", getAddress());

        if (dataCount.exec() == false)
            qWarning() << "> dataCount.exec() ERROR" << dataCount.lastError().type() << ":" << dataCount.lastError().text();

        while (dataCount.next())
        {
            return dataCount.value(0).toInt();
        }
    }
    else
    {
        // No database
        if (m_soil_moisture > 0 || m_soil_conductivity > 0 || m_soil_temperature > 0 ||
            m_temperature > -20.f || m_humidity > 0 || m_luminosity > 0)
        return 1;
    }

    return 0;
}

/* ************************************************************************** */
/* ************************************************************************** */

float DeviceSensor::getTemp() const
{
    SettingsManager *s = SettingsManager::getInstance();
    if (s->getTempUnit() == "F")
        return getTempF();

    return getTempC();
}

QString DeviceSensor::getTempString() const
{
    QString tempString;

    SettingsManager *s = SettingsManager::getInstance();
    if (s->getTempUnit() == "F")
        tempString = QString::number(getTempF(), 'f', 1) + "°F";
    else
        tempString = QString::number(getTempC(), 'f', 1) + "°C";

    return tempString;
}

/* ************************************************************************** */

float DeviceSensor::getHeatIndex() const
{
    float hi = getTemp();

    if (getTempC() >= 27.f && getHumidity() >= 40.0)
    {
        float T = getTemp();
        float R = getHumidity();

        float c1, c2, c3, c4, c5, c6, c7, c8, c9;
        SettingsManager *s = SettingsManager::getInstance();
        if (s->getTempUnit() == "F")
        {
            c1 = -42.379;
            c2 = 2.04901523;
            c3 = 10.14333127;
            c4 = -0.22475541;
            c5 = -6.83783e-03;
            c6 = -5.481717e-02;
            c7 = 1.22874e-03;
            c8 = 8.5282e-04;
            c9 = -1.99e-06;
        }
        else
        {
            c1 = -8.78469475556;
            c2 = 1.61139411;
            c3 = 2.33854883889;
            c4 = -0.14611605;
            c5 = -0.012308094;
            c6 = -0.0164248277778;
            c7 = 0.002211732;
            c8 = 0.00072546;
            c9 = -0.000003582;
        }

        // Compute heat index (https://en.wikipedia.org/wiki/Heat_index)
        hi = c1 + c2*T + c3*R + c4*T*R + c5*(T*T) + c6*(R*R) +c7*(T*T)*R + c8*T*(R*R) + c9*(T*T)*(R*R);
    }

    return hi;
}

QString DeviceSensor::getHeatIndexString() const
{
    QString hiString;

    SettingsManager *s = SettingsManager::getInstance();
    if (s->getTempUnit() == "F")
        hiString = QString::number(getHeatIndex(), 'f', 1) + "°F";
    else
        hiString = QString::number(getHeatIndex(), 'f', 1) + "°C";

    return hiString;
}

/* ************************************************************************** */

bool DeviceSensor::setDbLimits()
{
    bool status = false;

    if (m_dbInternal || m_dbExternal)
    {
        QSqlQuery updateLimits;
        updateLimits.prepare("REPLACE INTO plantLimits (deviceAddr, hygroMin, hygroMax, conduMin, conduMax, phMin, phMax, tempMin, tempMax, humiMin, humiMax, luxMin, luxMax, mmolMin, mmolMax)"
                             " VALUES (:deviceAddr, :hygroMin, :hygroMax, :conduMin, :conduMax, :phMin, :phMax, :tempMin, :tempMax, :humiMin, :humiMax, :luxMin, :luxMax, :mmolMin, :mmolMax)");
        updateLimits.bindValue(":deviceAddr", getAddress());
        updateLimits.bindValue(":hygroMin", m_limitHygroMin);
        updateLimits.bindValue(":hygroMax", m_limitHygroMax);
        updateLimits.bindValue(":conduMin", m_limitConduMin);
        updateLimits.bindValue(":conduMax", m_limitConduMax);
        updateLimits.bindValue(":phMin", m_limitPhMin);
        updateLimits.bindValue(":phMax", m_limitPhMax);
        updateLimits.bindValue(":tempMin", m_limitTempMin);
        updateLimits.bindValue(":tempMax", m_limitTempMax);
        updateLimits.bindValue(":humiMin", m_limitHumiMin);
        updateLimits.bindValue(":humiMax", m_limitHumiMax);
        updateLimits.bindValue(":luxMin", m_limitLuxMin);
        updateLimits.bindValue(":luxMax", m_limitLuxMax);
        updateLimits.bindValue(":mmolMin", m_limitMmolMin);
        updateLimits.bindValue(":mmolMax", m_limitMmolMax);

        status = updateLimits.exec();
        if (status == false)
            qWarning() << "> updateLimits.exec() ERROR" << updateLimits.lastError().type() << ":" << updateLimits.lastError().text();

        Q_EMIT limitsUpdated();
    }

    return status;
}

/* ************************************************************************** */
/* ************************************************************************** */

QVariantList DeviceSensor::getBackgroundDays(float maxValue, int maxDays)
{
    QVariantList background;

    while (background.size() < maxDays)
    {
        background.append(maxValue);
    }

    return background;
}

/*!
 * \return Last 30 days
 *
 * First day is always xxx
 */
QVariantList DeviceSensor::getLegendDays(int maxDays)
{
    QVariantList legend;
    QString legendFormat = "dd";
    if (maxDays <= 7) legendFormat = "dddd";

    // first day is always today
    QDate currentDay = QDate::currentDate();
    QString d = currentDay.toString(legendFormat);
    if (maxDays <= 7)
    {
        d.truncate(3);
        d += ".";
    }
    legend.push_front(d);

    // then fill the days before that
    while (legend.size() < maxDays)
    {
        currentDay = currentDay.addDays(-1);
        d = currentDay.toString(legendFormat);
        if (maxDays <= 7)
        {
            d.truncate(3);
            d += ".";
        }
        legend.push_front(d);
    }

    return legend;
}

QVariantList DeviceSensor::getDataDays(const QString &dataName, int maxDays)
{
    QVariantList graphData;
    QDate currentDay = QDate::currentDate(); // today
    QDate previousDay;
    QDate firstDay;

    if (m_dbInternal || m_dbExternal)
    {
        QSqlQuery sqlData;
        if (m_dbInternal) // sqlite
        {
            sqlData.prepare("SELECT strftime('%Y-%m-%d', ts), avg(" + dataName + ") as 'avg'" \
                            "FROM plantData " \
                            "WHERE deviceAddr = :deviceAddr " \
                            "GROUP BY strftime('%Y-%m-%d', ts) " \
                            "ORDER BY ts DESC;");
        }
        else if (m_dbExternal) // mysql
        {
            sqlData.prepare("SELECT DATE_FORMAT(ts, '%Y-%m-%d'), avg(" + dataName + ") as 'avg'" \
                                "FROM plantData " \
                                "WHERE deviceAddr = :deviceAddr " \
                                "GROUP BY DATE_FORMAT(ts, '%Y-%m-%d') " \
                                "ORDER BY ts DESC;");
        }
        sqlData.bindValue(":deviceAddr", getAddress());

        if (sqlData.exec() == false)
        {
            qWarning() << "> dataPerMonth.exec() ERROR" << sqlData.lastError().type() << ":" << sqlData.lastError().text();
        }

        while (sqlData.next())
        {
            QDate datefromsql = sqlData.value(0).toDate();

            // missing day(s)?
            if (previousDay.isValid())
            {
                int diff = datefromsql.daysTo(previousDay);
                for (int i = diff; i > 1; i--)
                {
                    //qDebug() << "> filling hole for day" << datefromsql.daysTo(previousDay);
                    graphData.push_front(0);
                }
            }

            // data
            graphData.push_front(sqlData.value(1));
            previousDay = datefromsql;
            if (!firstDay.isValid()) firstDay = datefromsql;
            //qDebug() << "> we have data (" << sqlData.value(1) << ") for date" << datefromsql;

            // max days reached?
            if (graphData.size() >= maxDays) break;
        }

        // missing day(s) front?
        while (graphData.size() < maxDays)
        {
            graphData.push_front(0);
        }
        // missing day(s) back?
        int missing = maxDays;
        if (firstDay.isValid()) missing = firstDay.daysTo(currentDay);
        for (int i = missing; i > 0; i--)
        {
            if (graphData.size() >= maxDays) graphData.pop_front();
            graphData.push_back(0);
        }
    }
/*
    // debug
    qDebug() << "Data (" << dataName << "/" << graphData.size() << ") : ";
    for (auto d: graphData) qDebug() << d;
*/
    return graphData;
}

/* ************************************************************************** */
/* ************************************************************************** */

QVariantList DeviceSensor::getDataHours(const QString &dataName)
{
    QVariantList graphData;
    QDateTime currentTime = QDateTime::currentDateTime(); // right now
    QDateTime previousTime;
    QDateTime firstTime;

    if (m_dbInternal || m_dbExternal)
    {
        QSqlQuery sqlData;
        if (m_dbInternal) // sqlite
        {
            sqlData.prepare("SELECT strftime('%Y-%m-%d %H:%m:%s', ts), avg(" + dataName + ") as 'avg'" \
                            "FROM plantData " \
                            "WHERE deviceAddr = :deviceAddr AND ts >= datetime('now','-1 day') " \
                            "GROUP BY strftime('%d-%H', ts) " \
                            "ORDER BY ts DESC;");
        }
        else if (m_dbExternal) // mysql
        {
            sqlData.prepare("SELECT DATE_FORMAT(ts, '%Y-%m-%d %H:%m:%s'), avg(" + dataName + ") as 'avg'" \
                            "FROM plantData " \
                            "WHERE deviceAddr = :deviceAddr AND ts >= datetime('now','-1 day') " \
                            "GROUP BY DATE_FORMAT(ts, '%d-%H') " \
                            "ORDER BY ts DESC;");
        }
        sqlData.bindValue(":deviceAddr", getAddress());

        if (sqlData.exec() == false)
        {
            qWarning() << "> dataPerHour.exec() ERROR" << sqlData.lastError().type() << ":" << sqlData.lastError().text();
        }

        while (sqlData.next())
        {
            QDateTime timefromsql = sqlData.value(0).toDateTime();

            // missing hour(s)?
            if (previousTime.isValid())
            {
                int diff = timefromsql.secsTo(previousTime) / 3600;
                for (int i = diff; i > 1; i--)
                {
                    //qDebug() << "> filling hole for hour" << diff;
                    graphData.push_front(0);
                }
            }

            // data
            graphData.push_front(sqlData.value(1));
            previousTime = timefromsql;
            if (!firstTime.isValid()) firstTime = timefromsql;
            //qDebug() << "> we have data (" << sqlData.value(1) << ") for hour" << timefromsql;

            // max hours reached?
            if (graphData.size() >= 24) break;
        }

        // missing hour(s) front?
        while (graphData.size() < 24)
        {
            graphData.push_front(0);
        }
        // missing hour(s) back?
        int missing = 24;
        if (firstTime.isValid()) missing = firstTime.secsTo(currentTime) / 3600;
        for (int i = missing; i > 0; i--)
        {
            if (graphData.size() >= 24) graphData.pop_front();
            graphData.push_back(0);
        }
    }
/*
    // debug
    qDebug() << "Data (" << dataName << "/" << graphData.size() << ") : ";
    for (auto d: graphData) qDebug() << d;
*/
    return graphData;
}

/*!
 * \return List of hours
 *
 * Two possibilities:
 * - We have data, so we go from last data available +24
 * - We don't have data, so we go from current hour to +24
 */
QVariantList DeviceSensor::getLegendHours()
{
    QVariantList legend;

    QTime now = QTime::currentTime();
    while (legend.size() < 24)
    {
        legend.push_front(now.hour());
        now = now.addSecs(-3600);
    }
/*
    // debug
    qDebug() << "Hours (" << legend.size() << ") : ";
    for (auto h: legend) qDebug() << h;
*/
    return legend;
}

QVariantList DeviceSensor::getBackgroundDaytime(float maxValue)
{
    QVariantList bgDaytime;

    QTime now = QTime::currentTime();
    while (bgDaytime.size() < 24)
    {
        if (now.hour() >= 21 || now.hour() <= 8)
            bgDaytime.push_front(0);
        else
            bgDaytime.push_front(maxValue);

        now = now.addSecs(-3600);
    }

    return bgDaytime;
}

QVariantList DeviceSensor::getBackgroundNighttime(float maxValue)
{
    QVariantList bgNighttime;

    QTime now = QTime::currentTime();
    while (bgNighttime.size() < 24)
    {
        if (now.hour() >= 21 || now.hour() <= 8)
            bgNighttime.push_front(maxValue);
        else
            bgNighttime.push_front(0);
        now = now.addSecs(-3600);
    }

    return bgNighttime;
}

/* ************************************************************************** */
/* ************************************************************************** */

void DeviceSensor::updateChartData_environmentalVoc(int maxDays)
{
    qDeleteAll(m_chartData_env);
    m_chartData_env.clear();
    ChartDataVoc *previousdata = nullptr;

    if (m_dbInternal || m_dbExternal)
    {
        QSqlQuery graphData;
        if (m_dbInternal) // sqlite
        {
            graphData.prepare("SELECT strftime('%Y-%m-%d', timestamp), " \
                              " min(voc), avg(voc), max(voc), " \
                              " min(hcho), avg(hcho), max(hcho), " \
                              " min(co2), avg(co2), max(co2) " \
                              "FROM sensorData " \
                              "WHERE deviceAddr = :deviceAddr " \
                              "GROUP BY strftime('%Y-%m-%d', timestamp) " \
                              "ORDER BY timestamp DESC;");
        }
        else if (m_dbExternal) // mysql
        {
            graphData.prepare("SELECT DATE_FORMAT(timestamp, '%Y-%m-%d'), " \
                              " min(voc), avg(voc), max(voc), " \
                              " min(hcho), avg(hcho), max(hcho), " \
                              " min(co2), avg(co2), max(co2) " \
                              "FROM sensorData " \
                              "WHERE deviceAddr = :deviceAddr " \
                              "GROUP BY DATE_FORMAT(timestamp, '%Y-%m-%d') " \
                              "ORDER BY timestamp DESC;");
        }
        graphData.bindValue(":deviceAddr", getAddress());

        if (graphData.exec() == false)
        {
            qWarning() << "> graphData.exec() ERROR" << graphData.lastError().type() << ":" << graphData.lastError().text();
            return;
        }
        while (graphData.next())
        {
            // missing day(s)?
            if (previousdata)
            {
                QDate datefromsql = graphData.value(0).toDate();
                int diff = datefromsql.daysTo(previousdata->getDate());
                for (int i = diff; i > 1; i--)
                {
                    QDate fakedate(datefromsql.addDays(i-1));
                    m_chartData_env.push_front(new ChartDataVoc(fakedate, -99, -99, -99, -99, -99, -99, -99, -99, -99, this));
                }
            }

            // data

            ChartDataVoc *d = new ChartDataVoc(graphData.value(0).toDate(),
                                   graphData.value(1).toFloat(), graphData.value(2).toFloat(), graphData.value(3).toFloat(),
                                   graphData.value(4).toFloat(), graphData.value(5).toFloat(), graphData.value(6).toFloat(),
                                   graphData.value(7).toFloat(), graphData.value(8).toFloat(), graphData.value(9).toFloat(),
                                   this);
            m_chartData_env.push_front(d);
            previousdata = d;

            // max days reached?
            if (m_chartData_env.size() >= maxDays) break;
        }

        // missing day(s)?
        {
            QDate today = QDate::currentDate();
            int missing = maxDays;
            if (previousdata) missing = static_cast<ChartDataVoc *>(m_chartData_env.last())->getDate().daysTo(today);

            for (int i = missing - 1; i >= 0; i--)
            {
                QDate fakedate(today.addDays(-i));
                m_chartData_env.push_front(new ChartDataVoc(fakedate, -99, -99, -99, -99, -99, -99, -99, -99, -99, this));
            }
        }

        Q_EMIT chartDataEnvUpdated();
    }
}

/* ************************************************************************** */
/* ************************************************************************** */

void DeviceSensor::updateChartData_thermometerMinMax(int maxDays)
{
    qDeleteAll(m_chartData_minmax);
    m_chartData_minmax.clear();
    m_tempMin = 999.f;
    m_tempMax = -99.f;
    ChartDataMinMax *previousdata = nullptr;

    if (m_dbInternal || m_dbExternal)
    {
        QSqlQuery graphData;
        if (m_dbInternal) // sqlite
        {
            graphData.prepare("SELECT strftime('%Y-%m-%d', ts), " \
                              " min(temperature), avg(temperature), max(temperature), " \
                              " min(humidity), max(humidity) " \
                              "FROM plantData " \
                              "WHERE deviceAddr = :deviceAddr " \
                              "GROUP BY strftime('%Y-%m-%d', ts) " \
                              "ORDER BY ts DESC;");
        }
        else if (m_dbExternal) // mysql
        {
            graphData.prepare("SELECT DATE_FORMAT(ts, '%Y-%m-%d'), " \
                              " min(temperature), avg(temperature), max(temperature), " \
                              " min(humidity), max(humidity) " \
                              "FROM plantData " \
                              "WHERE deviceAddr = :deviceAddr " \
                              "GROUP BY DATE_FORMAT(ts, '%Y-%m-%d') " \
                              "ORDER BY ts DESC;");
        }
        graphData.bindValue(":deviceAddr", getAddress());

        if (graphData.exec() == false)
        {
            qWarning() << "> graphData.exec() ERROR" << graphData.lastError().type() << ":" << graphData.lastError().text();
            return;
        }

        while (graphData.next())
        {
            // missing day(s)?
            if (previousdata)
            {
                QDate datefromsql = graphData.value(0).toDate();
                int diff = datefromsql.daysTo(previousdata->getDate());
                for (int i = diff; i > 1; i--)
                {
                    QDate fakedate(datefromsql.addDays(i-1));
                    m_chartData_minmax.push_front(new ChartDataMinMax(fakedate, -99, -99, -99, -99, -99, this));
                }
            }

            // data
            if (graphData.value(1).toFloat() < m_tempMin) { m_tempMin = graphData.value(1).toFloat(); }
            if (graphData.value(3).toFloat() > m_tempMax) { m_tempMax = graphData.value(3).toFloat(); }
            if (graphData.value(4).toInt() < m_hygroMin) { m_hygroMin = graphData.value(4).toInt(); }
            if (graphData.value(5).toInt() > m_hygroMax) { m_hygroMax = graphData.value(5).toInt(); }

            ChartDataMinMax *d = new ChartDataMinMax(graphData.value(0).toDate(),
                                         graphData.value(1).toFloat(), graphData.value(2).toFloat(), graphData.value(3).toFloat(),
                                         graphData.value(4).toInt(), graphData.value(5).toInt(), this);
            m_chartData_minmax.push_front(d);
            previousdata = d;

            // max days reached?
            if (m_chartData_minmax.size() >= maxDays) break;
        }

        // missing day(s)?
        {
            QDate today = QDate::currentDate();
            int missing = maxDays;
            if (previousdata) missing = static_cast<ChartDataMinMax *>(m_chartData_minmax.last())->getDate().daysTo(today);

            for (int i = missing - 1; i >= 0; i--)
            {
                QDate fakedate(today.addDays(-i));
                m_chartData_minmax.push_back(new ChartDataMinMax(fakedate, -99, -99, -99, -99, -99, this));
            }
        }

        Q_EMIT minmaxUpdated();
        Q_EMIT chartDataMinMaxUpdated();
    }
    else
    {
        // No database, use fake values
        m_hygroMin = 0;
        m_hygroMax = 50;
        m_conduMin = 0;
        m_conduMax = 2000;
        m_soilTempMin = 0.f;
        m_soilTempMax = 36.f;
        m_soilPhMin = 0.f;
        m_soilPhMax = 15.f;
        m_tempMin = 0.f;
        m_tempMax = 36.f;
        m_humiMin = 0;
        m_humiMax = 100;
        m_luxMin = 0;
        m_luxMax = 10000;
        m_mmolMin = 0;
        m_mmolMax = 10000;
        Q_EMIT minmaxUpdated();
    }
}

/* ************************************************************************** */
/* ************************************************************************** */

void DeviceSensor::getChartData_plantAIO(int maxDays,
                                   QtCharts::QDateTimeAxis *axis,
                                   QtCharts::QLineSeries *hygro, QtCharts::QLineSeries *condu,
                                   QtCharts::QLineSeries *temp, QtCharts::QLineSeries *lumi)
{
    if (!axis || !hygro || !condu || !temp || !lumi) return;

    if (m_dbInternal || m_dbExternal)
    {
        QString data = "soilMoisture";
        if (!hasSoilMoistureSensor()) data = "humidity";

        QString time = "datetime('now', 'localtime', '-" + QString::number(maxDays) + " days')";
        if (m_dbExternal)  time = "DATE_SUB(NOW(), INTERVAL " + QString::number(maxDays) + " DAY)";

        QSqlQuery graphData;
        graphData.prepare("SELECT ts_full, " + data + ", soilConductivity, temperature, luminosity " \
                          "FROM plantData " \
                          "WHERE deviceAddr = :deviceAddr AND ts_full >= " + time + ";");
        graphData.bindValue(":deviceAddr", getAddress());

        if (graphData.exec() == false)
        {
            qWarning() << "> graphData.exec() ERROR" << graphData.lastError().type() << ":" << graphData.lastError().text();
            return;
        }

        axis->setFormat("dd MMM");
        axis->setMax(QDateTime::currentDateTime());
        bool minSet = false;
        bool minmaxChanged = false;

        while (graphData.next())
        {
            QDateTime date = QDateTime::fromString(graphData.value(0).toString(), "yyyy-MM-dd hh:mm:ss");
            if (!minSet)
            {
                axis->setMin(date);
                minSet = true;
            }
            qint64 timecode = date.toMSecsSinceEpoch();

            hygro->append(timecode, graphData.value(1).toReal());
            condu->append(timecode, graphData.value(2).toReal());
            temp->append(timecode, graphData.value(3).toReal());
            lumi->append(timecode, graphData.value(4).toReal());

            if (graphData.value(1).toInt() < m_hygroMin) { m_hygroMin = graphData.value(1).toInt(); minmaxChanged = true; }
            if (graphData.value(2).toInt() < m_conduMin) { m_conduMin = graphData.value(2).toInt(); minmaxChanged = true; }
            if (graphData.value(3).toFloat() < m_tempMin) { m_tempMin = graphData.value(3).toFloat(); minmaxChanged = true; }
            if (graphData.value(4).toInt() < m_luxMin) { m_luxMin = graphData.value(4).toInt(); minmaxChanged = true; }

            if (graphData.value(1).toInt() > m_hygroMax) { m_hygroMax = graphData.value(1).toInt(); minmaxChanged = true; }
            if (graphData.value(2).toInt() > m_conduMax) { m_conduMax = graphData.value(2).toInt(); minmaxChanged = true; }
            if (graphData.value(3).toFloat() > m_tempMax) { m_tempMax = graphData.value(3).toFloat(); minmaxChanged = true; }
            if (graphData.value(4).toInt() > m_luxMax) { m_luxMax = graphData.value(4).toInt(); minmaxChanged = true; }
        }

        if (minmaxChanged) { Q_EMIT minmaxUpdated(); }
    }
    else
    {
        // No database, use fake values
        m_hygroMin = 0;
        m_hygroMax = 50;
        m_conduMin = 0;
        m_conduMax = 2000;
        m_soilTempMin = 0.f;
        m_soilTempMax = 36.f;
        m_soilPhMin = 0.f;
        m_soilPhMax = 15.f;
        m_tempMin = 0.f;
        m_tempMax = 36.f;
        m_humiMin = 0;
        m_humiMax = 100;
        m_luxMin = 0;
        m_luxMax = 10000;
        m_mmolMin = 0;
        m_mmolMax = 10000;
        Q_EMIT minmaxUpdated();
    }
}

/* ************************************************************************** */

int DeviceSensor::getHistoryUpdatePercent() const
{
    int p = 0;

    if (m_ble_status == DeviceUtils::DEVICE_UPDATING_HISTORY)
    {
        if (m_history_session_count > 0)
        {
            p = static_cast<int>((m_history_session_read / static_cast<float>(m_history_session_count)) * 100.f);
        }
    }

    //qDebug() << "DeviceSensor::getHistoryUpdatePercent(" << m_history_session_read << "/" <<  m_history_session_count << ")";

    return p;
}

QDateTime DeviceSensor::getLastMove() const
{
    if (m_device_lastmove > 0)
    {
        return QDateTime::fromSecsSinceEpoch(QDateTime::currentDateTime().toSecsSinceEpoch() - m_device_lastmove);
    }

    return QDateTime();
}

float DeviceSensor::getLastMove_days() const
{
    float days = (m_device_lastmove / 3600.f / 24.f);
    if (days < 0.f) days = 0.f;

    return days;
}

/* ************************************************************************** */
