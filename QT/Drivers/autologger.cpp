#include "autologger.h"

#include <QDate>
#include <QTextStream>

AutoLogger aLog = AutoLogger::instance();
//------------------------------------------------------------------------------
AutoLogger::AutoLogger()
{
    init();
}
//------------------------------------------------------------------------------
void AutoLogger::init()
{
    file = new QFile;
    QString strFileName = getFileName();
    file->setFileName(strFileName);
    log("SESSION START");
}
//------------------------------------------------------------------------------
bool AutoLogger::openFile()
{
    bool bSucc = false;
    if (file != nullptr)
    {
        bSucc = file->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    }
    return bSucc;
}
//------------------------------------------------------------------------------
bool AutoLogger::closeFile()
{
    bool bSucc = false;
    if (file != nullptr)
    {
        file->close();
        bSucc = true;
    }
    return bSucc;
}
//------------------------------------------------------------------------------
QString AutoLogger::getFileName()
{
    return "./Log_" + QDate::currentDate().toString(Qt::DateFormat::ISODate) + ".txt";
}
//------------------------------------------------------------------------------
void AutoLogger::log(const QString &strToLog)
{
    if (openFile())
    {
        QString strTime = QTime::currentTime().toString("HH:mm:ss.zzz");
        QTextStream out(file);
        out<<strTime + " : " + strToLog + "\n";
        closeFile();
    }
}
//------------------------------------------------------------------------------
