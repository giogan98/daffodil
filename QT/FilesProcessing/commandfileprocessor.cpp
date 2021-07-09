#include "commandfileprocessor.h"
#include "ISupervisor.h"

#include <QFile>
#include <QTextStream>
#include <QDebug>

CommandFileProcessor::CommandFileProcessor()
{

}
//------------------------------------------------------------------------------
void CommandFileProcessor::processFile(QString str_fileToProcess)
{
    QVector<int> * vec_int;
    QVector<QByteArray> * vec_qba;
    vec_int = &iSupervisor.vec_intTime;
    vec_qba = &iSupervisor.vec_qbaQueries;
    QFile file(str_fileToProcess);
    if (!file.open(QIODevice::ReadWrite | QIODevice::Text))
    {
        //qDebug()<<"Failed to open file";
    }
    QTextStream stream(&file);
    while(!stream.atEnd())
    {
        QString line = stream.readLine();
        line.replace(" ",""); //space
        line.replace("  ",""); //tab
        bool bSuccessful = controlLine(line);
        if(bSuccessful)
        {
            QStringList strl_temp = line.split(";");
            if (strl_temp[0].toInt()<1)
            {
                vec_int->append(1);
            }
            else
            {
                vec_int->append(strl_temp[0].toInt());
            }
            vec_qba->append(strl_temp[1].toUtf8());
        }
    }
    iSupervisor.getRandomizedVector();
    iSupervisor.loadNewFile();
}
//------------------------------------------------------------------------------
bool CommandFileProcessor::controlLine(QString str_line)
{
    bool bSuccessful= true;
    if(!str_line.contains(";"))
    {
        bSuccessful=false;
    }
    QStringList strl_temp;
    strl_temp = str_line.split(";");
    if(!(strl_temp[0].toInt())){
        if(strl_temp[0].toInt()==0)
        {
            return bSuccessful;
        }
        bSuccessful=false;
    }
    return bSuccessful;
}
//------------------------------------------------------------------------------
