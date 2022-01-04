#ifndef COMMANDFILEPROCESSOR_H
#define COMMANDFILEPROCESSOR_H

#include <QString>
#include <QVector>
#include <QVariant>

class CommandFileProcessor
{
public:
    QVector<int>        vec_relTimes;
    QVector<QByteArray> vec_qbaQueries;
private:

public:
    CommandFileProcessor();
    void processFile(QString str_fileToProcess);

private:
    bool controlLine(QString str_line);
};

#endif // COMMANDFILEPROCESSOR_H
