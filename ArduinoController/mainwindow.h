#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "FilesProcessing/commandfileprocessor.h"
#include "Drivers/ISettings.h"

#include <QMainWindow>
#include <QVector>
#include <QFile>
#include <QSerialPort>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    QTimer * timerState;
private:
    bool bStart;
    bool bMessageBox;
    Ui::MainWindow *ui            ;
    CommandFileProcessor fileproc ;

public:
    MainWindow(QWidget *parent = nullptr) ;
    ~MainWindow()                         ;

private slots:
    void executestatemachine()       ;
    void on_pbn_setUp_clicked()      ;
    void on_pbn_connect_clicked()    ;
    void on_pbn_openFile_clicked()   ;
    void on_pbn_startPause_clicked() ;
    void on_pbn_resetCycles_clicked();
    void on_spinBox_editingFinished();
    void closeEvent(QCloseEvent *event);
    void on_checkBox_sensor_stateChanged();
    void on_pbn_openMenu_clicked();
    void on_pbn_home_clicked();

private:
    void fillCBComPort();
    void handleGraphicInit();
    void refreshTemporizedGui();
    void saveInSettingsFile(QString str_comName);
    void saveSafeStop(void);
};
#endif // MAINWINDOW_H
