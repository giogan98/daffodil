#include "ISupervisor.h"
#include "Drivers/autologger.h"
#include "mainwindow.h"

#include "random"
#include <QObject>
#include <limits.h>
#include <QtGlobal>

#define PIN_CHECK_1 10
#define PIN_CHECKER 1
#define MAX_CONSEC_ERRORS 15

ISupervisor iSupervisor = ISupervisor::instance();
//-----------------------------------------------------------------------------
ISupervisor::ISupervisor()
{
    iWriteDone = 0;
    iCyclesDone = 0;
    iConsecutiveErrors = 0;
    iCyclesToDo = 10000;
    str_message = "";
    iRandomInterval = 0;
    iVec_DigReads.fill(0,13);
    str_sensError = "";
    blecontroller = new BLEcontroller();
    init();
}
//------------------------------------------------------------------------------
ISupervisor::~ISupervisor()
{
    delete blecontroller;
}
//------------------------------------------------------------------------------
void ISupervisor::st_idle()
{
    //Do nothing, wait for something to happen
}
//------------------------------------------------------------------------------
void ISupervisor::refreshCyclesDone()
{
    iCyclesDone = iSettings.load(ISettings::SET_CYCLESDONE).toInt();
}
//------------------------------------------------------------------------------
void ISupervisor::st_init()
{
    refreshCyclesDone();
    str_message = serialcontroller.openSerialPort();
    to_idle();
}
//------------------------------------------------------------------------------
void ISupervisor::st_read()
{
    if (chronoRead.elapsed() < 1) // atm this is an useless control, previously it was 1500.0f
    {
        to_write();
        return;
    }
    if (serialcontroller.read())
    {
        chronoRead.start();
        to_write();
    }
    else
    {
        to_error();
    }
}
//------------------------------------------------------------------------------
void ISupervisor::st_write()
{
    //     lSzScript = vec_intTime.length(); // @hint like this the program recalculate it every 40ms

    dummyWrite();

    handleNewCycle();

    if (iCyclesDone > iCyclesToDo)
    {
        to_idle();
        return;
    }

    float fDelta = 0.0f;
    const float elapsed = chronoWrite.elapsed();
    int iCounter = 3;
    while (iWriteDone < lSzScript && iCounter > 0)
    {
        fDelta = fDelta + vec_intRandomTimes[iWriteDone];
        if (elapsed >= fDelta)
        {
            serialcontroller.write(vec_qbaQueries[iWriteDone]);
            chronoWrite.start();
            iWriteDone++;
        }
        else
        {
            to_read();
            break;
        }
        iCounter--;
    }
}
//------------------------------------------------------------------------------
void ISupervisor::st_error()
{
    serialcontroller.emergencyShutdown();
    serialcontroller.closeSerialPort();
    aLog.log("[!] STATE_MACHINE_ERROR: STATE_ERROR [!]");
    to_init();
}
//------------------------------------------------------------------------------
void ISupervisor::to_idle()
{
    serialcontroller.emergencyShutdown();
    setState(ST_IDLE);
}
//------------------------------------------------------------------------------
void ISupervisor::to_init()
{
    setState(ST_INIT);
}
//------------------------------------------------------------------------------
void ISupervisor::to_read()
{
    setState(ST_READ);
}
//------------------------------------------------------------------------------
void ISupervisor::to_write()
{
    if (serialcontroller.isOpen())
    {
        setState(ST_WRITE);
    }
    else
    {
        setState(ST_ERROR);
    }
}
//------------------------------------------------------------------------------
void ISupervisor::to_error()
{
    setState(ST_ERROR);
}
//------------------------------------------------------------------------------
/**
 * @brief ISupervisor::dummyWrite
 * @brief write something useless just to prevent arduino from shutting down all its pins
 */
void ISupervisor::dummyWrite()
{
    static int iEntered = 0;

    if (iEntered%20 == 0)
    {
        serialcontroller.write("[?DO13]");
        iEntered = 0;
    }

    iEntered++;
}
//------------------------------------------------------------------------------
/**
* @brief Initialize State Machine
*/
void ISupervisor:: init(void)
{
    enStatus = ST_IDLE ;
}
//------------------------------------------------------------------------------
/**
* @brief ISupervisor::execute one cycle of state Machine
*/
void ISupervisor::executeSM(void)
{
    enStatus = enNxtStatus;
    switch(enStatus)
    {
    case ST_IDLE       :
        st_idle      ();
        break;
    case ST_INIT       :
        st_init      ();
        break;
    case ST_READ       :
        st_read      ();
        break;
    case ST_WRITE      :
        st_write     ();
        break;
    case ST_ERROR      :
        st_error     ();
        break;
    }
}
//------------------------------------------------------------------------------
bool ISupervisor::isSerialOpen()
{
    return serialcontroller.isOpen();
}
//------------------------------------------------------------------------------
void ISupervisor::shutdownPins()
{
    serialcontroller.emergencyShutdown();
}
//------------------------------------------------------------------------------
/**
 * @brief ISupervisor::isReady
 * @brief checks if the serialport is open and if a file has been selected by the user
 * @return
 */
bool ISupervisor::isReady()
{
    bool bReady = true;
    if (!serialcontroller.isOpen())
    {
        bReady = false;
    }
    if (vec_intTime.length()<1)
    {
        bReady = false;
    }
    if (iSettings.load(iSettings.SET_CHECKSENSOR).toInt() == SNS_BLE)
    {
        bReady = (blecontroller->bConnected && blecontroller->bCalibrated);
    }
    return bReady;
}
//------------------------------------------------------------------------------
/**
* @brief set status State Machine
* @param svState   new status for the State Machine
*/
void ISupervisor:: setState(enumSvStates svState)
{
    enNxtStatus = svState;
    aLog.log("TRANS "+ QString::number(enStatus)+" -> "+ QString::number(svState));
}
//------------------------------------------------------------------------------
/**
 * @brief ISupervisor::adjstWriteDoneAfterStop
 * @brief if stop button is pressed, write done diminish by one
 */
void ISupervisor::adjstWriteDoneAfterStop()
{
    if (iWriteDone > 0)
    {
        iWriteDone--;
    }
}
//------------------------------------------------------------------------------
/**
* @brief Log some text
* @param u8aMsg
* @param lSz
*/
void ISupervisor::log(char * u8aMsg, unsigned long lSz)
{
    char u8aMsg256[256]={0,};
    if (lSz > 0 && u8aMsg != nullptr)
    {
        snprintf(u8aMsg256,sizeof(u8aMsg256),"SUP       ; %s",u8aMsg);
        aLog.log(u8aMsg256);
    }
}
//------------------------------------------------------------------------------
void ISupervisor::setCyclesToDo(int iNum)
{
    if (iNum > 0 && iNum <= 100000)
    {
        iCyclesToDo = iNum;
    }
}
//------------------------------------------------------------------------------
void ISupervisor::loadNewFile()
{
    resetWriteDone();
    updateScriptSize();
}
//------------------------------------------------------------------------------
void ISupervisor::updateScriptSize()
{
    lSzScript = vec_intTime.length();
}
//------------------------------------------------------------------------------
void ISupervisor::resetCyclesDone(void)
{
    iCyclesDone=0;
}
//------------------------------------------------------------------------------
/**
 * @brief ISupervisor::returnMessage
 * returns message that confirm that openserialport has succeded
 * @return
 */
QString ISupervisor::returnMessage(void)
{
    return str_message;
}
//------------------------------------------------------------------------------
void ISupervisor::setSafetyStop(QString str_safe)
{
    serialcontroller.setSafetyStop(str_safe);
}
//------------------------------------------------------------------------------
void ISupervisor::resetWriteDone()
{
    iWriteDone = 0;
}
//------------------------------------------------------------------------------
/**
 * @brief ISupervisor::setRandomness
 * @brief checks that the randomness inserted by the user is between some specific values
 * @param iNumRandom
 */
void ISupervisor::setRandomness(int iNumRandom)
{
    if (iNumRandom < 0)
    {
        iRandomInterval = 0;
    }
    else if(iNumRandom > 2000)
    {
        iRandomInterval = 2000;
    }
    else
    {
        iRandomInterval = iNumRandom;
    }
}
//------------------------------------------------------------------------------
/**
 * @brief ISupervisor::randomizeNumber
 * given a number, return a random number in the interval selected
 * @param iNumber
 * @return
 */
int ISupervisor::randomizeNumber(int iNumber)
{
    if (iRandomInterval > 0)
    {
        int iRandDelta = (rand() % iRandomInterval)- (iRandomInterval/2) ;
        iNumber = iNumber + iRandDelta ;
        if (iNumber < 1)
        {
            iNumber = 1;
        }
    }
    return iNumber;
}
//------------------------------------------------------------------------------
/**
 * @brief ISupervisor::checkSensor
 * @brief if enabled, check if the sensor returns correct data or if there are problems
 * like the machine is currently not working and so stays still
 * @return
 */
bool ISupervisor::checkSensor(void)
{
    static unsigned long ulCounter = 0;
    processSensorOutput();
    bool bCheck =  iVec_DigReads[PIN_CHECK_1-1] != PIN_CHECKER;
    if (bCheck)
    {
        setSensorError(bCheck);
        if (ulCounter < MAX_CONSEC_ERRORS)
        {
            ulCounter++;
            return true;
        }
        ulCounter = 0; //@hint remove to disable reset of error count after restart
        return false;
    }
    setSensorError(bCheck);
    ulCounter = 0;
    return true;
}
//------------------------------------------------------------------------------
/**
 * @brief ISupervisor::checkBleSensor
 * @return true if movement is detected, false otherwise
 */
bool ISupervisor::checkBleSensor(void)
{
    static unsigned long ulCounter = 0;
    bool bMoving = false;
    double dMin;
    double dMax;
    if (blecontroller->vdDegrees.size() > 0)
    {
        dMin = blecontroller->vdDegrees[0];
        dMax = blecontroller->vdDegrees[0];
        for (const auto &element : blecontroller->vdDegrees)
        {
            if (element < dMin)
                dMin = element;
            if (element > dMax)
                dMax = element;
        }
        if (dMin < -80 && dMax > 80) //check fatto ad 80 gradi al posto di 90
            bMoving = true;
    }
    qDebug()<<blecontroller->vdDegrees;
    blecontroller->clearDegreeValues();

    if (!bMoving)
    {
        setSensorError(!bMoving);//bool error
        if (ulCounter < MAX_CONSEC_ERRORS)
        {
            ulCounter++;
            return true;
        }
        ulCounter = 0; //@hint remove to disable reset of error count after restart
        return false;
    }
    setSensorError(!bMoving);
    ulCounter = 0;
    return true;
}
//------------------------------------------------------------------------------
void ISupervisor::handleNewCycle()
{
    if (iWriteDone >= lSzScript) //start new cycle
    {
        if (iSettings.load(ISettings::SET_CHECKSENSOR) == SNS_PINB)
        {
            if (!checkSensor())
            {
                aLog.log("ERROR: check sensor: sensor did not move");
                iSettings.save(ISettings::SET_SENSOR_ERROR, bool(true));
                to_idle();
                return;
            }
        }
        else if (iSettings.load(ISettings::SET_CHECKSENSOR) == SNS_BLE)
        {
            if (!checkBleSensor())
            {
                aLog.log("ERROR: check BLE sensor: sensor did not move");
                iSettings.save(ISettings::SET_SENSOR_ERROR, bool(true));
                to_idle();
                return;
            }
        }
        iVec_DigReads.fill(0,13);
        iWriteDone = 0;
        getRandomizedVector();
        iCyclesDone++ ;
        aLog.log("Saving cycle number");
        iSettings.save(ISettings::SET_CYCLESDONE , iCyclesDone);
        aLog.log("Saved number");
    }
}
//------------------------------------------------------------------------------
void ISupervisor::getRandomizedVector()
{
    static int iAppo = 1;
    vec_intRandomTimes.clear();
    for (int ii = 0; ii < vec_intTime.length(); ii++)
    {
        iAppo = randomizeNumber(vec_intTime[ii]);
        vec_intRandomTimes.append(iAppo);
    }
}
//------------------------------------------------------------------------------
/**
 * @brief ISupervisor::processSensorOutput
 * @brief process the message retrieved from the read function to check the values transmitted by
 * the sensor
 */
void ISupervisor::processSensorOutput(void)
{
    //"[?DI07.1]\r\n[?DI05.1]\r\n"
    QString str_temp = QString(serialcontroller.getRequestedQba());
    if (str_temp == "")
    {
        aLog.log("[!] WARNING: Sensor data is null");
        return;
    }
    str_temp = str_temp.replace("\r\n","");
    QStringList strl_splitted = str_temp.split("][");
    for (int ii = 0 ; ii < strl_splitted.size() ; ii++)
    {
        strl_splitted[ii].remove("[");
        strl_splitted[ii].remove("]");
        strl_splitted[ii].remove("?");
        strl_splitted[ii].remove("D");
        strl_splitted[ii].remove("I");
        QStringList strl_appo = strl_splitted[ii].split(".");
        if (strl_appo.size() >= 2)
        {
            int iIndex = strl_appo[0].toInt()-1;
            int iValue = strl_appo[1].toInt();
            if (iIndex >=0 && iIndex < iVec_DigReads.size())
            {
                int iTemp = iVec_DigReads[iIndex];
                iTemp = iTemp + iValue;
                iVec_DigReads[iIndex] = iTemp;
            }
        }
    }
}
//------------------------------------------------------------------------------
void ISupervisor::setSensorError(bool error)
{
    if (error)
    {
        iConsecutiveErrors++;
        str_sensError = "Sensor Error: ";
        str_sensError.append(QString::number(iConsecutiveErrors));
    }
    else
    {
        iConsecutiveErrors--;
        if (iConsecutiveErrors < 0)
        {
            iConsecutiveErrors = 0;
        }
        str_sensError = "Sensor Error: ";
        str_sensError.append(QString::number(iConsecutiveErrors));
    }
}
//------------------------------------------------------------------------------
void ISupervisor::resetSensorErrorCount(void)
{
    str_sensError = "";
    iConsecutiveErrors = 0;
}
//------------------------------------------------------------------------------
