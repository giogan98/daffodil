#include "serialcontroller.h"
#include "ISettings.h"
#include "autologger.h"

#include <QDebug>
#include <QTimer>
#include <QVariant>
#include <QVector>
#include <string>

SerialController::SerialController()
{
    serial = new QSerialPort ;
    qba_requestedBytes = ""  ;
}
//------------------------------------------------------------------------------
SerialController::~SerialController()
{
    serial->deleteLater(); // @hint check if this create problems or not
}
//------------------------------------------------------------------------------
QString SerialController::openSerialPort()
{
    QString str_temp="";
    if (serial->isOpen())
    {
        closeSerialPort();
    }
    serial->setPortName   (iSettings.load(iSettings.SET_COMPORT).toString())    ;
    serial->setDataBits   (static_cast<QSerialPort::DataBits>
                           (iSettings.load(iSettings.SET_DATABITS).toInt()))    ;
    serial->setParity     (static_cast<QSerialPort::Parity>
                           (iSettings.load(iSettings.SET_PARITY).toInt()))      ;
    serial->setStopBits   (static_cast<QSerialPort::StopBits>
                           (iSettings.load(iSettings.SET_STOPBITS).toInt()))    ;
    serial->setBaudRate   (static_cast<QSerialPort::BaudRate>
                           (iSettings.load(iSettings.SET_BAUDRATE).toInt()))    ;
    serial->setFlowControl(static_cast<QSerialPort::FlowControl>
                           (iSettings.load(iSettings.SET_FLOWCONTROL).toInt())) ;

    if (serial->open(QIODevice::ReadWrite))
    {
        aLog.log("Device connected");
        str_temp.append("Device connected");
    }
    else
    {
        //qDebug()<<"[!]Device not connected"             ;
        aLog.log("[!]Device not connected");
        str_temp.append("Device not connected");
    }
    return  str_temp;
}
//------------------------------------------------------------------------------
/**
 * @brief SerialController::closeSerialPort
 * @brief check if the serial is open, if so the serial is now closed
 */
void SerialController::closeSerialPort()
{
    serial->close() ;
    aLog.log("Device disconnected");
}
//------------------------------------------------------------------------------
void SerialController::setSafetyStop(QString str_todo)
{
    //FILE SYNTAX: [!DO$.0] or [!DO$.1]
    bool bSucc = true;

    if (!str_todo.contains("[") || !str_todo.contains("]"))
    {
        bSucc = false;
    }

    if (!str_todo.contains("!DO") || !str_todo.contains("."))
    {
        bSucc = false;
    }

    QStringList strl_splitted = str_todo.split("$");

    if (strl_splitted.length() != 2)
    {
        bSucc = false;
    }
    if (bSucc)
    {
        strl_safeStop.clear();
        for (int ii = 0; ii < strl_splitted.length(); ii++)
        {
            strl_splitted[ii].remove("$");
            strl_safeStop.append(strl_splitted[ii]);
        }
    }
}
//------------------------------------------------------------------------------
/**
 * @brief SerialController::emergencyShutdown
 * @brief iterate all pins and set their status to the one chosen with a file or default to low
 */
void SerialController::emergencyShutdown()
{
    if (strl_safeStop.length() == 2)
    {
        iterateAllPins(strl_safeStop[0].toUtf8(), strl_safeStop[1].toUtf8());
    }
    else
    {
        iterateAllPins("[!DO",".0]");
    }
}
//------------------------------------------------------------------------------
/**
 * @brief SerialController::read
 * @brief iterate all pins and read their status
 * @return
 */
bool SerialController::read()
{
    qba_requestedBytes = qba_requestedBytes + serial->readAll();
    QSerialPort::SerialPortError error = serial->error();
    if (error != QSerialPort::SerialPortError::NoError)
    {
        aLog.log(serial->errorString()) ;
        return false ;
    }
    else
    {
        return true ;
    }
}
//------------------------------------------------------------------------------
/**
 * @brief SerialController::write
 * @brief after some controls that verify the correct syntax of the message, the message is written
 * @param qba_toWrite
 * @return
 */
bool SerialController::write(QByteArray qba_toWrite)
{
    int controller=0;
    if(qba_toWrite.indexOf("!") < 0 && qba_toWrite.indexOf("?") < 0)
    {
        return false;
    }

    if (qba_toWrite.indexOf("[") < 0 || qba_toWrite.indexOf("]") < 0)
    {
        return false;
    }

    for (int ii = PIN_ONE; ii < PIN_NUMEL; ii++)
    {
        if (qba_toWrite.indexOf(QByteArray::number(ii))>-1)
        {
            controller++;
        }
    }

    if (controller>0)
    {
        serial->write(qba_toWrite);
        return true;
    }

    return false;
}
//------------------------------------------------------------------------------
/**
 * @brief SerialController::iterateAllPins
 * @brief given the first part and the last part of the command, iterate all pins and write the command
 * @param qba_query1
 * @param qba_query2
 * @return
 */
QByteArray SerialController::iterateAllPins(QByteArray qba_query1, QByteArray qba_query2)
{
    QByteArray qba_read = "", qba_pin = "" ;
    for (int ii = PIN_ONE ; ii < PIN_NUMEL; ii++)
    {
        qba_pin = QString::number(ii).toUtf8() ;
        serial->write(qba_query1 + qba_pin + qba_query2);
    }
    qba_read = qba_read + serial->readAll()+ "\n" ;
    return qba_read ;
}
//------------------------------------------------------------------------------
/**
* @brief Log some text
* @param u8aMsg
* @param lSz
*/
void SerialController::log(char * u8aMsg, long lSz)
{
    char u8aMsg256[256]={0,};
    if (lSz > 0 && u8aMsg != nullptr)
    {
        snprintf(u8aMsg256,sizeof(u8aMsg256),"SUP       ; %s",u8aMsg);
        aLog.log(u8aMsg256);
    }
}
//------------------------------------------------------------------------------
QByteArray SerialController::getRequestedQba(void)
{
    QByteArray qba_temp = qba_requestedBytes;
    qba_requestedBytes.clear();
    return qba_temp;
}
//------------------------------------------------------------------------------
