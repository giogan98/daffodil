#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("BLEconnector");
    initializeBluetoothDeviceDiscoveryAgent();
}
//------------------------------------------------------------------------------
MainWindow::~MainWindow()
{
    delete ui;
}
//------------------------------------------------------------------------------
void MainWindow::initializeBluetoothDeviceDiscoveryAgent(void)
{
    ptr_deviceDiscoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);

    //Sets the maximum search time for BLE device search to timeout in ms:
    ptr_deviceDiscoveryAgent->setLowEnergyDiscoveryTimeout(5000);

    //This signal is emitted when an error occurs during Bluetooth device
    //discovery. The error parameter describes the error that occurred.
    connect(ptr_deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::errorOccurred,
            this, &MainWindow::scanError);

    //'finished' signal is emitted when Bluetooth device discovery completes.
    //The signal is not going to be emitted if the device discovery finishes
    //with an error.
    connect(ptr_deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished,
            this, &MainWindow::scanFinished);

    //'canceled' signal is emitted when device discovery is aborted by a call to stop().
    connect(ptr_deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::canceled,
            this, &MainWindow::scanFinished);
}
//------------------------------------------------------------------------------
void MainWindow::scanError(QBluetoothDeviceDiscoveryAgent::Error error)
{
    if (error == QBluetoothDeviceDiscoveryAgent::PoweredOffError)
    {
        setError(tr("The Bluetooth adaptor is powered off."));
    }
    else if (error == QBluetoothDeviceDiscoveryAgent::InputOutputError)
    {
        setError(tr("Writing or reading from the device resulted in an error."));
    }
    else
    {
        setError(tr("An unknown error has occurred."));
    }
}
//------------------------------------------------------------------------------
void MainWindow::setError(const QString &error)
{
    if (m_error != error)
    {
        m_error = error;
        emit errorChanged();
    }
}
//------------------------------------------------------------------------------
void MainWindow::setInfo(const QString &info)
{
    if (m_info != info)
    {
        m_info = info;
        emit infoChanged();
    }
}
//------------------------------------------------------------------------------
void MainWindow::scanFinished(void)
{
    //Get the list of all the bluetooth discovered devices:
    list_devicesInfos = ptr_deviceDiscoveryAgent->discoveredDevices();

    //Cycle the list of all the bluetooth discovered devices and filter them
    //based on the low energy configuration
    //Display the LE devices in the combobox
    for (const auto &element : list_devicesInfos)
    {
        if (element.coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration)
        {
            ui->cb_devices->addItem(element.name());
        }
    }
}
//------------------------------------------------------------------------------
void MainWindow::on_pbn_search_clicked()
{
    //Starts Bluetooth device discovery, if it is not already started.
    ptr_deviceDiscoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
}
//------------------------------------------------------------------------------
void MainWindow::on_pbn_okDevice_clicked()
{
    //Get index of the selected item in the combobox
    int iIndex = ui->cb_devices->currentIndex();
    setDevice(getDevice(iIndex));
}
//------------------------------------------------------------------------------
QBluetoothDeviceInfo MainWindow::getDevice(const int &iIndex = 0)
{
    QBluetoothDeviceInfo bdiTemp;
    if (list_devicesInfos.length() > iIndex)
    {
        bdiTemp = list_devicesInfos[iIndex];
    }
    else
    {
        qDebug()<<"Error: list_devicesInfos.lenght() is inferior to cb index";
    }
    qDebug()<<"Name: "<<bdiTemp.name()<<" Adress: "<<bdiTemp.address();
    return bdiTemp;
}
//------------------------------------------------------------------------------
void MainWindow::setDevice(const QBluetoothDeviceInfo &device)
{
    ptr_leController = QLowEnergyController::createCentral(device, this);
    ptr_leController->connectToDevice();
}
//------------------------------------------------------------------------------
