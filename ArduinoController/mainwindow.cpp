#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ISupervisor.h"

#include <QMessageBox>
#include <QFile>
#include <QIcon>
#include <QDateTime>
#include <QTextStream>
#include <QStringList>
#include <QFileDialog>
#include <QSerialPort>
#include <QSerialPortInfo>

#define TIMER_CHRONO (20)
#define TIME_REFRESH_GUI (180 / TIMER_CHRONO)

//------------------------------------------------------------------------------
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setFixedSize(QSize(800,600));
    bStart = true;
    bMessageBox = true;
    //Disabilita lo stop del programma dopo N errori sensore consecutivi:
    iSettings.save(ISettings::SET_SENSOR_ERROR, false);
    fillCBComPort();
    handleGraphicInit();
    timerState = new QTimer;
    timerState->setTimerType(Qt::CoarseTimer);
    timerState->start(TIMER_CHRONO);
    connect(timerState,SIGNAL(timeout()),this,SLOT(executestatemachine()));

    if(!iSupervisor.blecontroller)
        return;
    connect(this, SIGNAL(startBleDeviceSearch()), iSupervisor.blecontroller, SLOT(startDeviceScan()));
    connect(iSupervisor.blecontroller, SIGNAL(deviceListAvaiable(QList<QBluetoothDeviceInfo>)), this, SLOT(displayBleDeviceList(QList<QBluetoothDeviceInfo>)));
    connect(this, SIGNAL(bleDeviceSelected(int)), iSupervisor.blecontroller, SLOT(connectToSelectedDevice(int)));
    connect(this, SIGNAL(startBleDeviceCalibration(int)), iSupervisor.blecontroller, SLOT(startDeviceCalibration(int)));
    connect(iSupervisor.blecontroller, SIGNAL(calibrationPossible()), this, SLOT(allowCalibration()));
    //connect(iSupervisor.blecontroller, SIGNAL(newAccDataAvaiable(float)), this, SLOT(updateAccelerometerData(float)));
    //connect(iSupervisor.blecontroller, SIGNAL(newGyroDataAvaiable(float)), this, SLOT(updateGyroscopeData(float)));
}
//------------------------------------------------------------------------------
MainWindow::~MainWindow()
{
    delete ui;
}
//------------------------------------------------------------------------------
/**
 * @brief MainWindow::on_pbn_openFile_clicked
 * 1) resetta i vettori contenenti i tempi e i comandi da eseguire
 * 2) seleziona e apre un file script
 * 3) salva il path del file
 * 4) processa il file script
 */
void MainWindow::on_pbn_openFile_clicked()
{
    iSupervisor.vec_intTime.clear();
    iSupervisor.vec_qbaQueries.clear();
    QString str_filePath = QFileDialog::getOpenFileName(this, "Open file containing commands",
                                                        "./","*.gal");
    if (str_filePath != "")
    {
        iSettings.save(ISettings::SET_FILEPATH, str_filePath);
        fileproc.processFile(str_filePath);
        ui->label_filePath->setText(str_filePath);
        ui->groupBox_setup->hide();
        ui->groupBox_bleSettings->hide();
        ui->label_filePath->show();
        ui->groupBox_main->show();
    }
}
//------------------------------------------------------------------------------
/**
 * @brief MainWindow::on_pbn_startPause_clicked
 */
void MainWindow::on_pbn_startPause_clicked()
{
    iSettings.save(ISettings::SET_SENSOR_ERROR, bool(false));
    ui->groupBox_menu->hide();

    //check sul vettore contenente i vari tempi in cui trasmettere i messaggi via seriale
    if (iSupervisor.vec_intTime.length() == 0)
    {
        QString str_fileToTest = iSettings.load(ISettings::SET_FILEPATH).toString();
        if (QFile(str_fileToTest).exists())
        {
            fileproc.processFile(str_fileToTest);
            ui->label_filePath->setText(str_fileToTest);
            ui->label_filePath->show();
        }
    }

    if (!iSupervisor.isSerialOpen())
    {
        saveSafeStop();
        iSupervisor.setState(ISupervisor::ST_INIT);
    }

    if (iSupervisor.isReady())
    {
        if (bStart)
        {
            QString str_safeStop = iSettings.load(ISettings::SET_SAFE_STOP).toString();
            iSupervisor.setSafetyStop(str_safeStop);
            iSupervisor.setState(iSupervisor.ST_READ);
            bMessageBox = true;
            iSupervisor.chronoRead.start();
            iSupervisor.chronoWrite.start();
            iSupervisor.resetSensorErrorCount();
        }
        else
        {
            iSupervisor.setState(iSupervisor.ST_IDLE);
            iSupervisor.shutdownPins();
            //iSupervisor.resetWriteDone(); // @hint delete this to block restart of the cycle
        }
        bStart = !bStart;
    }
}
//------------------------------------------------------------------------------
/**
 * @brief MainWindow::on_pbn_connect_clicked
 * 1) prende il nome della COM port dalla combobox
 * 2) salva questo nome nel file .settings
 * 3) setta lo state della state machine in INIT
 */
void MainWindow::on_pbn_connect_clicked()
{
    QString str_comName;
    str_comName = ui->cb_comPort->currentText();
    saveInSettingsFile(str_comName);
    iSupervisor.setState(ISupervisor::ST_INIT);
}
//------------------------------------------------------------------------------
void MainWindow::saveInSettingsFile(QString str_comName)
{
    iSettings.save(ISettings::SET_COMPORT    , str_comName)                ;
    iSettings.save(ISettings::SET_DATABITS   , QSerialPort::Data8)         ;
    iSettings.save(ISettings::SET_STOPBITS   , QSerialPort::OneStop)       ;
    iSettings.save(ISettings::SET_PARITY     , QSerialPort::NoParity)      ;
    //    iSettings.save(ISettings::SET_BAUDRATE   , QSerialPort::Baud9600)      ;
    iSettings.save(ISettings::SET_FLOWCONTROL, QSerialPort::NoFlowControl) ;
    saveSafeStop();
}
//------------------------------------------------------------------------------
void MainWindow::on_pbn_resetCycles_clicked()
{
    iSettings.save(ISettings::SET_CYCLESDONE , int(0));
    iSupervisor.resetWriteDone();
    iSupervisor.resetCyclesDone();
    iSupervisor.refreshCyclesDone();
}
//------------------------------------------------------------------------------
void MainWindow::refreshTemporizedGui()
{
    const int i = iSettings.load(ISettings::SET_CYCLESDONE).toInt();
    ui->label_sError->setText(iSupervisor.str_sensError);
    ui->lineEdit_completedCycles->clear();
    ui->statusBar->showMessage(iSupervisor.returnMessage());
    ui->lineEdit_completedCycles->setText(QString::number(i));

    if (iSettings.load(ISettings::SET_SENSOR_ERROR).toBool() && bMessageBox)
    {
        QDateTime time = QDateTime::currentDateTime();
        QString str_error = "Sensor error \nTime: " + time.toString("dd-MM-yyyy HH:mm:ss");
        QMessageBox::information(this,"Error",str_error);
        bMessageBox = !bMessageBox;
        bStart = true; // true -> show icon play button
    }

    if (!iSupervisor.isSerialOpen())
    {
        ui->pbn_startPause->setIcon(QIcon(":/icons/Images/Icons/magnet.png"));
        bStart = true;
    }
    else if (iSupervisor.isSerialOpen() && bStart)
    {
        ui->pbn_startPause->setIcon(QIcon(":/icons/Images/Icons/play-button.png"));
        ui->pbn_setUp->show();
        ui->pbn_resetCycles->show();
    }
    else if (iSupervisor.isSerialOpen() && !bStart)
    {
        ui->pbn_startPause->setIcon(QIcon(":/icons/Images/Icons/pause-button.png"));
        ui->pbn_setUp->hide();
        ui->pbn_resetCycles->hide();
    }
}
//------------------------------------------------------------------------------
void MainWindow::executestatemachine()
{
    static unsigned long ulVal = 0;
    iSupervisor.executeSM();
    if (ulVal % TIME_REFRESH_GUI)
    {
        refreshTemporizedGui();
    }
    ulVal++;
}
//------------------------------------------------------------------------------
void MainWindow::on_spinBox_editingFinished()
{
    iSupervisor.setCyclesToDo(ui->spinBox->value());
}
//------------------------------------------------------------------------------
void MainWindow::closeEvent(QCloseEvent *event)
{
    iSupervisor.shutdownPins();
}
//------------------------------------------------------------------------------
void MainWindow::on_rb_noSensor_clicked()
{
    bool bChecked = ui->rb_noSensor->isChecked();
    if(bChecked)
    {
        iSettings.save(ISettings::SET_CHECKSENSOR, iSupervisor.SNS_NONE);
    }
    qDebug()<<iSettings.load(ISettings::SET_CHECKSENSOR);

}
//------------------------------------------------------------------------------
void MainWindow::on_rb_pinballSensor_clicked()
{
    bool bChecked = ui->rb_pinballSensor->isChecked();
    if(bChecked)
    {
        iSettings.save(ISettings::SET_CHECKSENSOR, iSupervisor.SNS_PINB);
    }
    qDebug()<<iSettings.load(ISettings::SET_CHECKSENSOR);

}
//------------------------------------------------------------------------------
void MainWindow::on_rb_BLESensor_clicked()
{
    bool bChecked = ui->rb_BLESensor->isChecked();
    if(bChecked)
    {
        iSettings.save(ISettings::SET_CHECKSENSOR, iSupervisor.SNS_BLE);
    }
    qDebug()<<iSettings.load(ISettings::SET_CHECKSENSOR);
}
//------------------------------------------------------------------------------
void MainWindow::selectCorrectRadioButtonOnStartup(void)
{
    int iRadioButton = iSettings.load(iSettings.SET_CHECKSENSOR).toInt();
    switch(iRadioButton)
    {
    case iSupervisor.SNS_NONE:
        ui->rb_noSensor->setChecked(true);
        ui->rb_pinballSensor->setChecked(false);
        ui->rb_BLESensor->setChecked(false);
        break;
    case iSupervisor.SNS_PINB:
        ui->rb_noSensor->setChecked(false);
        ui->rb_pinballSensor->setChecked(true);
        ui->rb_BLESensor->setChecked(false);
        break;
    case iSupervisor.SNS_BLE:
        ui->rb_noSensor->setChecked(false);
        ui->rb_pinballSensor->setChecked(false);
        ui->rb_BLESensor->setChecked(true);
        break;
    default:
        qDebug()<<"Error in the rb sensor to check";
        break;
    }
}
//------------------------------------------------------------------------------
/**
 * @brief MainWindow::fillCBComPort
 * 1) trova le porte collegate al computer tramite USB, crea una lista di queste
 * 2) legge il nome della COM a cui connettersi salvato nel file .settings
 * 3) con un ciclo for fa passare la lista delle porte collegate al computer
 * 4) riempie una combobox con la lista delle porte disponibili
 * 5) se una di queste ha il nome salvato nel file .settings, salva il suo index
 * 6) setta l'index della combobox a quello appena trovato o in alternativa a 0
 */
void MainWindow::fillCBComPort()
{
    QList<QSerialPortInfo> list;
    list = QSerialPortInfo::availablePorts();
    int iIdx = 0;
    const QString strComSettings = iSettings.load(ISettings::SET_COMPORT).toString();
    for (int ii = 0 ; ii < list.length(); ii++)
    {
        QString strPortName = list[ii].portName();
        ui->cb_comPort->addItem(strPortName);
        if(strPortName.compare(strComSettings) == 0)
        {
            iIdx = ii;
        }
    }
    ui->cb_comPort->setCurrentIndex(iIdx);
}
//------------------------------------------------------------------------------
/**
 * @brief MainWindow::handleGraphicInit
 * Inizializza l'interfaccia grafica impostandone icone, nascondendo e mostrando
 * gli elementi desiderati.
 */
void MainWindow::handleGraphicInit(void)
{
    const QSize btnSize = QSize(40, 30);
    ui->pbn_openMenu->setFixedSize(btnSize);
    ui->pbn_openMenu->setIcon(QIcon(":/icons/Images/Icons/horizontal-lines.png"));

    QSize sizeStartButton = ui->pbn_startPause->rect().size();
    ui->pbn_startPause->setIcon(QIcon(":/icons/Images/Icons/magnet.png"));
    ui->pbn_startPause->setIconSize(sizeStartButton);

    ui->pbn_advancedSettings->setFixedSize(QSize(50,50));
    QSize size_advSett_button = ui->pbn_advancedSettings->rect().size();

    ui->pbn_advancedSettings->setIcon(QIcon(":/icons/Images/Icons/settings.png"));
    ui->pbn_advancedSettings->setIconSize(size_advSett_button);

    ui->groupBox_setup->hide();
    ui->groupBox_bleSettings->hide();
    ui->label_filePath->hide();

    ui->pb_calibrateBLE->setEnabled(false);

    selectCorrectRadioButtonOnStartup();
}
//------------------------------------------------------------------------------
void MainWindow::on_pbn_setUp_clicked()
{
    ui->groupBox_main->hide();
    ui->groupBox_menu->hide();
    ui->groupBox_setup->show();
    ui->groupBox_bleSettings->show();
}
//------------------------------------------------------------------------------
void MainWindow::on_pbn_openMenu_clicked()
{
    if (ui->groupBox_menu->isHidden())
    {
        ui->groupBox_menu->show();
    }
    else
    {
        ui->groupBox_menu->hide();
    }
}
//------------------------------------------------------------------------------
void MainWindow::on_pbn_home_clicked()
{
    ui->groupBox_setup->hide();
    ui->groupBox_bleSettings->hide();
    ui->groupBox_menu->hide();
    if (ui->groupBox_main->isHidden())
    {
        ui->groupBox_main->show();
    }
}
//------------------------------------------------------------------------------
void MainWindow::saveSafeStop(void)
{
    if (iSettings.load(ISettings::SET_SAFE_STOP).toString() == QString("[!DO$.0]"))
    {
        iSettings.save(ISettings::SET_SAFE_STOP, QString("[!DO$.0]"));
    }
}
//------------------------------------------------------------------------------
/**
 * @brief MainWindow::on_pb_searchBLE_clicked
 * When the user presses the button, the search of ble devices is started
 * Signal is emitted to start the search
 */
void MainWindow::on_pb_searchBLE_clicked()
{
    emit startBleDeviceSearch();
}
//------------------------------------------------------------------------------
/**
 * @brief MainWindow::displayBleDeviceList
 * @param list_devicesInfos list of the ble devices found by scanning
 * When the signal 'deviceListAvaiable' is emitted, this slot is called
 */
void MainWindow::displayBleDeviceList(QList<QBluetoothDeviceInfo> list_devicesInfos)
{
    for (const auto &element : list_devicesInfos)
    {
        ui->cb_BLEdevices->addItem(element.name());
    }
}
//------------------------------------------------------------------------------
/**
 * @brief MainWindow::allowCalibration
 * when the ble device is connected, allow the button to be pressed to start
 * the calibration
 */
void MainWindow::allowCalibration(void)
{
    QMessageBox::information(this, "Connected", "Device connected");
    ui->pb_calibrateBLE->setEnabled(true);
    ui->pb_connectBLE->setText("Connect");
}
//------------------------------------------------------------------------------
/**
 * @brief MainWindow::on_pb_connectBLE_clicked
 * When the user select to which device the program has to connect, this emits
 * a signal to connect to that device
 */
void MainWindow::on_pb_connectBLE_clicked()
{
    int iIndex = ui->cb_BLEdevices->currentIndex();
    emit bleDeviceSelected(iIndex);
    ui->pb_connectBLE->setText("Connecting...");
}
//------------------------------------------------------------------------------
void MainWindow::on_pb_calibrateBLE_clicked()
{
    emit startBleDeviceCalibration(10000);
}
//------------------------------------------------------------------------------
