#ifndef SERIALCONTROLLER_H
#define SERIALCONTROLLER_H

#include "autologger.h"
#include "ISettings.h"

#include <QElapsedTimer>
#include <QSerialPort>

class SerialController
{
public:
    QVector<int> vec_relTimes;
    QByteArray qba_requestedBytes;
    QVector<QByteArray> vec_qbaQueries;

private:
    QSerialPort * serial;
    QStringList strl_safeStop;
    typedef enum
    {
        PIN_ONE     = 1,
        PIN_TWO     = 2,
        PIN_THREE   = 3,
        PIN_FOUR    = 4,
        PIN_FIVE    = 5,
        PIN_SIX     = 6,
        PIN_SEVEN   = 7,
        PIN_EIGHT   = 8,
        PIN_NINE    = 9,
        PIN_TEN     = 10,
        PIN_ELEVEN  = 11,
        PIN_TWELVE  = 12,
        PIN_THIRTEEN= 13,
        PIN_NUMEL       ,
    }enPins;

public:
    SerialController();
    ~SerialController();
    bool read();
    QByteArray getRequestedQba();
    bool write(QByteArray qba_toWrite);
    QString openSerialPort();
    void closeSerialPort();
    void setSafetyStop(QString str_action);
    void emergencyShutdown();
    bool isOpen(){ return serial->isOpen(); }

private:
    void log(char *u8aMsg, long lSz);
    QByteArray iterateAllPins(QByteArray qba_query1, QByteArray qba_query2);

};

#endif // SERIALCONTROLLER_H
