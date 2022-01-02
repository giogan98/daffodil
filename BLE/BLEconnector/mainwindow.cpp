#include "mainwindow.h"
#include "ui_mainwindow.h"

#define BLE_UUID_DATA_SERVICE                 "5aaeb650-c2cb-44d1-b4ab-7144e08aed2e"
#define BLE_UUID_GYROSCOPE_CHARACTERISTIC     "9936153d-65bc-4479-b079-aa25569f9ab1"
#define BLE_UUID_ACCELEROMETER_CHARACTERISTIC "f4055745-6f5a-4e2b-8433-2704337cc3b5"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("BLEconnector");
    blecontroller = new BLEcontroller();
    setupBLEcontroller();
}
//------------------------------------------------------------------------------
MainWindow::~MainWindow()
{
    delete blecontroller;
    delete ui;
}
//------------------------------------------------------------------------------
void MainWindow::setupBLEcontroller(void)
{
    if(!blecontroller)
        return;
    connect(this, SIGNAL(startDeviceSearch()), blecontroller, SLOT(startDeviceScan()));

    connect(blecontroller, SIGNAL(deviceListAvaiable(QList<QBluetoothDeviceInfo>)), this, SLOT(displayDeviceList(QList<QBluetoothDeviceInfo>)));

    connect(this, SIGNAL(deviceSelected(int)), blecontroller, SLOT(getSelectedDevice(int)));

    connect(blecontroller, SIGNAL(newAccDataAvaiable(float)), this, SLOT(updateAccelerometerData(float)));

    connect(blecontroller, SIGNAL(newGyroDataAvaiable(float)), this, SLOT(updateGyroscopeData(float)));
}
//------------------------------------------------------------------------------
void MainWindow::displayDeviceList(QList<QBluetoothDeviceInfo> list_devicesInfos)
{
    for (const auto &element : list_devicesInfos)
    {
        ui->cb_devices->addItem(element.name());
    }
}
//------------------------------------------------------------------------------
void MainWindow::on_pbn_search_clicked()
{
    ui->cb_devices->clear();
    emit startDeviceSearch();
}
//------------------------------------------------------------------------------
void MainWindow::on_pbn_okDevice_clicked()
{
    int iIndex = ui->cb_devices->currentIndex();
    emit deviceSelected(iIndex);
}
//------------------------------------------------------------------------------
void MainWindow::updateGyroscopeData(float fValue)
{
    addDataToPlainTextEdit(ui->pte_gyro, QString::number(fValue));
}
//------------------------------------------------------------------------------
void MainWindow::updateAccelerometerData( float fValue)
{
    addDataToPlainTextEdit(ui->pte_acc, QString::number(fValue));
}
//------------------------------------------------------------------------------
void MainWindow::addDataToPlainTextEdit(QPlainTextEdit *pte, const QString &data)
{
    if(pte)
    {
        pte->appendPlainText(data + "\n");
    }
}
//------------------------------------------------------------------------------
