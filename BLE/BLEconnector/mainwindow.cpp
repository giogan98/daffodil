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
    initializeBluetoothDeviceDiscoveryAgent();
    bFoundDataService = false;
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
    ui->cb_devices->clear();
    //Starts Bluetooth device discovery, if it is not already started.
    ptr_deviceDiscoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
}
//------------------------------------------------------------------------------
void MainWindow::on_pbn_okDevice_clicked()
{
    //Get index of the selected item in the combobox
    int iIndex = ui->cb_devices->currentIndex();
    setDevice(getDeviceFromCBox(iIndex));
}
//------------------------------------------------------------------------------
QBluetoothDeviceInfo MainWindow::getDeviceFromCBox(const int &iIndex = 0)
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

    //When a service is discovered, go to the slot "serviceDiscovered"
    connect(ptr_leController, &QLowEnergyController::serviceDiscovered,
            this, &MainWindow::serviceDiscovered);

    //When the services discovery is finished, go to slot "serviceScanDone"
    connect(ptr_leController, &QLowEnergyController::discoveryFinished,
            this, &MainWindow::serviceScanDone);

    connect(ptr_leController, &QLowEnergyController::errorOccurred, this,
            [this](QLowEnergyController::Error error) {
        Q_UNUSED(error);
        setError("Cannot connect to remote device.");
    });

    connect(ptr_leController, &QLowEnergyController::connected, this, [this]() {
        setInfo("Controller connected. Search services...");
        ptr_leController->discoverServices();
    });

    connect(ptr_leController, &QLowEnergyController::disconnected, this, [this]() {
        setError("LowEnergy controller disconnected");
    });

    ptr_leController->connectToDevice();
}
//------------------------------------------------------------------------------
void MainWindow::serviceDiscovered(const QBluetoothUuid &gatt)
{
    //Search for gyroscope data service UUID
    if (gatt == QBluetoothUuid(BLE_UUID_DATA_SERVICE))
    {
        qDebug()<<"Found data service";
        bFoundDataService = true;
        return;
    }
    qDebug()<<"Data service not found";
}
//------------------------------------------------------------------------------
void MainWindow::serviceScanDone(void)
{
    setInfo("Service scan done.");

    // Delete old gyro service if available
    if (ptr_dataService)
    {
        delete ptr_dataService;
        ptr_dataService = nullptr;
    }

    if (bFoundDataService)
    {
        ptr_dataService = ptr_leController->createServiceObject(QBluetoothUuid(BLE_UUID_DATA_SERVICE), this);
        connectServicePointer(ptr_dataService);
    }
}
//------------------------------------------------------------------------------
void MainWindow::connectServicePointer(QLowEnergyService *ptr_service)
{
    if (ptr_service)
    {
        connect(ptr_service, &QLowEnergyService::stateChanged, this, &MainWindow::serviceStateChanged);
        connect(ptr_service, &QLowEnergyService::characteristicChanged, this, &MainWindow::updateGyroscopeData);
        connect(ptr_service, &QLowEnergyService::characteristicChanged, this, &MainWindow::updateAccelerometerData);
        ptr_service->discoverDetails();
    }
    else
    {
        setError("Service not found.");
    }
}
//------------------------------------------------------------------------------
void MainWindow::serviceStateChanged(QLowEnergyService::ServiceState leServiceState)
{
    switch (leServiceState)
    {
    case QLowEnergyService::RemoteServiceDiscovering:
        setInfo(tr("Discovering services..."));
        break;

    case QLowEnergyService::RemoteServiceDiscovered:
    {
        setInfo(tr("Service discovered."));

        const QLowEnergyCharacteristic gyroChar = ptr_dataService->characteristic(QBluetoothUuid(BLE_UUID_GYROSCOPE_CHARACTERISTIC));
        const QLowEnergyCharacteristic accChar = ptr_dataService->characteristic(QBluetoothUuid(BLE_UUID_ACCELEROMETER_CHARACTERISTIC));

        if (!gyroChar.isValid() || !accChar.isValid())
        {
            setError("Data not found.");
            break;
        }

        leGyroNotificationDesc = gyroChar.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
        leAccNotificationDesc = accChar.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);

        if (leGyroNotificationDesc.isValid() && leAccNotificationDesc.isValid())
        {
            ptr_dataService->writeDescriptor(leGyroNotificationDesc, QByteArray::fromHex("0100"));
            ptr_dataService->writeDescriptor(leAccNotificationDesc, QByteArray::fromHex("0100"));
        }
        break;
    }

    default:
        break;
    }
}
//------------------------------------------------------------------------------
void MainWindow::updateGyroscopeData(const QLowEnergyCharacteristic &c, const QByteArray &value)
{
    if (c.uuid() != QBluetoothUuid(BLE_UUID_GYROSCOPE_CHARACTERISTIC))
        return;

    addDataToPlainTextEdit(ui->pte_gyro, value);
}
//------------------------------------------------------------------------------
void MainWindow::updateAccelerometerData(const QLowEnergyCharacteristic &c, const QByteArray &value)
{
    if (c.uuid() != QBluetoothUuid(BLE_UUID_ACCELEROMETER_CHARACTERISTIC))
        return;

    addDataToPlainTextEdit(ui->pte_acc, value);
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
