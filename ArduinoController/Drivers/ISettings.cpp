/******************************************************************************
 * @file 		ISettings class implementation
 * @brief		This class is used to configure Application Settings
 * @author		IVAN Perletti - General Medical Merate.spa - Seriate - ITALY
 * @version		1.0
 * @date		July 25th, 2019
 * @post 		Nope
 * @bug			Not all memory is freed when deleting an object of this class.
 * @warning		Improper use can crash your application
 * @copyright 	GMM.spa - All Rights Reserved
 ******************************************************************************/
#include "ISettings.h"

#include <QDebug>
#include <QSerialPort>

ISettings iSettings = ISettings::instance();
//------------------------------------------------------------------------------
const ISettings::structRecordSettings ISettings::recordList[]
{
    {ISettings::SET_COMPORT     ,"ComPort"    , "Grp0", QString("COM7")            },
    {ISettings::SET_BAUDRATE    ,"BaudRate"   , "Grp0", QSerialPort::Baud9600      },
    {ISettings::SET_DATABITS    ,"DataBits"   , "Grp0", QSerialPort::Data8         },
    {ISettings::SET_PARITY      ,"Parity"     , "Grp0", QSerialPort::NoParity      },
    {ISettings::SET_STOPBITS    ,"StopBits"   , "Grp0", QSerialPort::OneStop       },
    {ISettings::SET_FLOWCONTROL ,"FlowControl", "Grp0", QSerialPort::NoFlowControl },
    {ISettings::SET_CYCLESDONE  ,"CyclesDone" , "Grp0", int(0)                     },
    //CHECKSENSOR: 0 NOSENSOR, 1 PINBALL, 2 BLE
    {ISettings::SET_CHECKSENSOR ,"CheckSensor", "Grp0", int(0)                     },
    {ISettings::SET_FILEPATH    ,"StrFilePath", "Grp0", QString("C:/users/giorg/desktop/new1.txt")},
    {ISettings::SET_SENSOR_ERROR,"SensorError", "Grp0", bool(false)                },
    {ISettings::SET_SAFE_STOP   ,"SafeStop"   , "Grp0", QString("[!DO$.0]")        }
};
//------------------------------------------------------------------------------
ISettings::ISettings()
{
    pSetting = nullptr;
    initSettings();
}
//------------------------------------------------------------------------------
/**
 * @brief save settings to file
 * @param key     variable to be saved
 * @param value   value to be saved
 */
void ISettings::save(const enumSettings key,
                     const QVariant &value)
{
    const QString strKey   = recordList[key].strParam;
    const QString strGroup = recordList[key].strGroup;
    saveParam(strKey,strGroup,value);
}
//------------------------------------------------------------------------------
/**
 * @brief load current settings belonging to variable
 * @param key            key value string to be searched into settings file
 * @param defaultValue   default value if variable is not found
 * @return value or default value if varible is not found
 */
QVariant ISettings::load(const enumSettings &key)
{
    QVariant variant = "-1"; // invalid
    if(key<SET_NUMEL)
    {
        const QString strKey   = recordList[key].strParam;
        const QString strGroup = recordList[key].strGroup;
        QVariant valDflt  = recordList[key].strDefault;
        variant = loadParam(strKey,strGroup,valDflt);
    }
    return(variant);
}
//------------------------------------------------------------------------------
void ISettings::initSettings(void)
{
    pSetting = new QSettings("./deviceConfiguration.ini",
                             QSettings::IniFormat);
}
//------------------------------------------------------------------------------
/**
 * @brief save settings to file
 * @param key     variable to be saved
 * @param value   value to be saved
 * @param group   group of the variable
 */
void ISettings::saveParam(const QString &key,
                          const QString &group,
                          const QVariant &value)
{
    if(pSetting!=nullptr)
    {
        pSetting->beginGroup(group);
        pSetting->setValue(key, value);
        pSetting->endGroup();
    }
}
//------------------------------------------------------------------------------
/**
 * @brief load current settings belonging to variable
 * @param key            key value string to be searched into settings file
 * @param group          belonging group of searched variable
 * @param defaultValue   default value if variable is not found
 * @return value or default value if varible is not found
 */
QVariant ISettings::loadParam(const QString &key,
                              const QString &group,
                              const QVariant &defaultValue)
{
    QVariant value ;
    if(pSetting!=nullptr)
    {
        pSetting->beginGroup(group);
        QString str1 =  pSetting->fileName();
        if(str1!=" ")
        {
            value = pSetting->value(key, defaultValue);
            pSetting->endGroup();
        }
    }
    return value;
}
//------------------------------------------------------------------------------
