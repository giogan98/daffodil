#ifndef ISUPERVISOR_H
#define ISUPERVISOR_H

#include "Drivers/serialcontroller.h"
#include "Drivers/blecontroller.h"
#include "mainwindow.h"
#include "FilesProcessing/commandfileprocessor.h"

#include <QElapsedTimer>
#include <QTimer>

class ISupervisor
{

public:
    typedef enum
    {
        ST_IDLE  = 0,
        ST_INIT  = 1,
        ST_READ  = 2,
        ST_WRITE = 3,
        ST_ERROR = 4,
    }enumSvStates; /// enumeration for state enumeration

    typedef enum
    {
        SNS_NONE = 0,
        SNS_PINB = 1,
        SNS_BLE  = 2,
    }enumSensorTypes;

    QElapsedTimer chronoRead;
    QElapsedTimer chronoWrite;
    QVector<int> vec_intTime;
    QVector<int> vec_intRandomTimes;
    QVector<QByteArray> vec_qbaQueries;
    QString str_sensError;
    BLEcontroller *blecontroller = nullptr;

private:
    int iConsecutiveErrors;
    int iWriteDone;
    int iCyclesDone;
    int iCyclesToDo;
    int iRandomInterval;
    long lSzScript;
    QString str_message;
    QVector<int> iVec_DigReads;
    enumSvStates enStatus, enNxtStatus;
    SerialController serialcontroller;
    CommandFileProcessor fileprocessor;

public:
    static ISupervisor & instance(void)
    {
        static ISupervisor iSupervisor;
        return iSupervisor;
    }
    bool isReady(void);
    void init(void);
    void shutdownPins(void);
    void executeSM(void);
    bool isSerialOpen(void);
    void resetSensorErrorCount(void);
    void resetWriteDone(void);
    void resetCyclesDone(void);
    void refreshCyclesDone(void);
    void getRandomizedVector(void);
    void setCyclesToDo(int iNum);
    void loadNewFile(void);
    void updateScriptSize(void);
    void setRandomness(int iNumRandom);
    void setState(enumSvStates svState);
    void adjstWriteDoneAfterStop(void);
    QString returnMessage(void);
    void setSafetyStop(QString str_safe);
    inline enumSvStates getState(void){ return(enStatus); }
    ~ISupervisor();

private:
    ISupervisor();
    void st_idle(void);
    void st_init(void);
    void st_read(void);
    void st_write(void);
    void st_error(void);
    void to_idle(void);
    void to_init(void);
    void to_read(void);
    void to_write(void);
    void to_error(void);
    void dummyWrite(void);
    bool checkSensor(void);
    void handleNewCycle(void);
    void processSensorOutput(void);
    int randomizeNumber(int iNumber);
    void log(char * u8aMsg, unsigned long lSz);
    void setSensorError(bool error);

};

extern ISupervisor iSupervisor;

#endif // ISUPERVISOR_H
