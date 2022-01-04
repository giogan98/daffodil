#ifndef AUTOLOGGER_H
#define AUTOLOGGER_H

#include <QFile>


class AutoLogger
{
private:
    QFile *file;

public:

private:
    AutoLogger();
    void init();
    bool openFile();
    bool closeFile();
    QString getFileName();

public:
    static AutoLogger& instance()
    {
        static AutoLogger aLog;
        return aLog;
    }
    void log(const QString &strToLog);
};

extern AutoLogger aLog;

#endif // AUTOLOGGER_H
