#include "blecontroller.h"

#include <QTimer>
#include <QtMath>

#define BLE_UUID_DATA_SERVICE                   "5aaeb650-c2cb-44d1-b4ab-7144e08aed2e"

#define BLE_UUID_ACCELEROMETER_CHARACTERISTIC_X "f4055745-6f5a-4e2b-8433-2704337cc3b5"
#define BLE_UUID_ACCELEROMETER_CHARACTERISTIC_Y "ac7a390c-abb3-48c8-8c54-73c3a6a4bc73"
#define BLE_UUID_ACCELEROMETER_CHARACTERISTIC_Z "3c71aaec-128b-4f88-bb60-28a2026498be"

//------------------------------------------------------------------------------
BLEcontroller::BLEcontroller()
{
    initializeBluetoothDeviceDiscoveryAgent();

    bFoundDataService = false;
    bConnected = false;

    bAccX = false;
    bAccZ = false;

    vdAccelerometerOffset = QVector<double>(3, 0);
    viAccelerometerOffsetCounter = QVector<int>(3, 0);
}
//------------------------------------------------------------------------------
/**
 * @brief BLEcontroller::initializeBluetoothDeviceDiscoveryAgent
 * 1) Creates a new pointer of type QBLuetoothDeviceDiscoveryAgent, used to scan
 * for nearby Bluetooth devices.
 * 2) Sets the maximum search time for Bluetooth Low Energy device search to
 * timeout in milliseconds.
 * 3) Connects the signals 'errorOccurred()', 'finished()', 'canceled()' to
 * their appropriate slots.
 */
void BLEcontroller::initializeBluetoothDeviceDiscoveryAgent(void)
{
    ptr_deviceDiscoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);

    //Sets the maximum search time for BLE device search to timeout in ms:
    ptr_deviceDiscoveryAgent->setLowEnergyDiscoveryTimeout(2000);

    //This signal is emitted when an error occurs during Bluetooth device
    //discovery. The error parameter describes the error that occurred.
    connect(ptr_deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::errorOccurred,
            this, &BLEcontroller::scanError);

    //'finished' signal is emitted when Bluetooth device discovery completes.
    //The signal is not going to be emitted if the device discovery finishes
    //with an error.
    connect(ptr_deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::finished,
            this, &BLEcontroller::scanFinished);

    //'canceled' signal is emitted when device discovery is aborted by a call to stop().
    connect(ptr_deviceDiscoveryAgent, &QBluetoothDeviceDiscoveryAgent::canceled,
            this, &BLEcontroller::scanFinished);

    connect(this, SIGNAL(checkAvaiableDegree()), this, SLOT(accelerationToDegrees()));
}
//------------------------------------------------------------------------------
/**
 * @brief BLEcontroller::scanError
 * @param error describes the error that occurred.
 */
void BLEcontroller::scanError(QBluetoothDeviceDiscoveryAgent::Error error)
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
/**
 * @brief BLEcontroller::setError
 * @param error
 */
void BLEcontroller::setError(const QString &error)
{
    if (m_error != error)
    {
        m_error = error;
        emit errorChanged();
    }
}
//------------------------------------------------------------------------------
/**
 * @brief BLEcontroller::setInfo
 * @param info
 */
void BLEcontroller::setInfo(const QString &info)
{
    if (m_info != info)
    {
        m_info = info;
        emit infoChanged();
    }
}
//------------------------------------------------------------------------------
void BLEcontroller::startDeviceScan(void)
{
    if(!ptr_deviceDiscoveryAgent)
        return;
    ptr_deviceDiscoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod);
}
//------------------------------------------------------------------------------
/**
 * @brief BLEcontroller::scanFinished
 * @return a list of the valid LE device scanned
 */
void BLEcontroller::scanFinished(void)
{
    //Get the list of all the bluetooth discovered devices:
    list_devicesInfos = ptr_deviceDiscoveryAgent->discoveredDevices();

    //Cycle the list of all the bluetooth discovered devices and remove the ones
    //that are not Low Energy:
    QMutableListIterator<QBluetoothDeviceInfo> iter(list_devicesInfos);
    while (iter.hasNext())
    {
        if (!(iter.next().coreConfigurations() & QBluetoothDeviceInfo::LowEnergyCoreConfiguration))
            iter.remove();
    }
    emit deviceListAvaiable(list_devicesInfos);
}
//------------------------------------------------------------------------------
void BLEcontroller::connectToSelectedDevice(int iIndex)
{
    QBluetoothDeviceInfo bdiTemp;
    if (list_devicesInfos.length() > iIndex)
    {
        bdiTemp = list_devicesInfos[iIndex];
        setDevice(bdiTemp);
    }
    else
    {
        qDebug()<<"Error: list_devicesInfos.lenght() is inferior to cb index";
    }
}
//------------------------------------------------------------------------------
/**
 * @brief BLEcontroller::setDevice
 * @param device set as the one to connect to
 */
void BLEcontroller::setDevice(QBluetoothDeviceInfo device)
{
    ptr_leController = QLowEnergyController::createCentral(device, this);

    //When a service is discovered, go to the slot "serviceDiscovered"
    connect(ptr_leController, &QLowEnergyController::serviceDiscovered,
            this, &BLEcontroller::serviceDiscovered);

    //When the services discovery is finished, go to slot "serviceScanDone"
    connect(ptr_leController, &QLowEnergyController::discoveryFinished,
            this, &BLEcontroller::serviceScanDone);

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

    //Connect to the selected device
    ptr_leController->connectToDevice();
}
//------------------------------------------------------------------------------
/**
 * @brief BLEcontroller::serviceDiscovered
 * @param gatt Bluetooth UUID that we filter to find the wanted service
 */
void BLEcontroller::serviceDiscovered(const QBluetoothUuid &gatt)
{
    //Search for data service UUID
    if (gatt == QBluetoothUuid(BLE_UUID_DATA_SERVICE))
    {
        bFoundDataService = true;
        qDebug()<<"Found data service";
        return;
    }
    bFoundDataService = false;
    qDebug()<<"Data service not found";
}
//------------------------------------------------------------------------------
/**
 * @brief BLEcontroller::serviceScanDone
 * When the service scan is done, if data service is found create a service
 * object
 */
void BLEcontroller::serviceScanDone(void)
{
    setInfo("Service scan done.");

    // Delete old data service if available:
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
/**
 * @brief BLEcontroller::connectServicePointer
 * @param ptr_service
 * If the pointer to the service is not nullptr, connect its SINGNAL
 * 'stateChanged()' to the SLOT 'serviceStateChanged()'.
 * Also, discover the detail of the service.
 */
void BLEcontroller::connectServicePointer(QLowEnergyService *ptr_service)
{
    if (ptr_service)
    {
        connect(ptr_service, &QLowEnergyService::stateChanged, this, &BLEcontroller::serviceStateChanged);
        ptr_service->discoverDetails();
    }
    else
    {
        setError("Service not found.");
    }
}
//------------------------------------------------------------------------------
/**
 * @brief BLEcontroller::serviceStateChanged
 * @param leServiceState
 * This is a SLOT, called when the service emits its SIGNAL 'stateChanged()'
 * If the data service is newly discovered, enable notifications from its
 * characteristics 'Gyroscope' and 'Accelerometer'.
 */
void BLEcontroller::serviceStateChanged(QLowEnergyService::ServiceState leServiceState)
{
    switch (leServiceState)
    {
    case QLowEnergyService::RemoteServiceDiscovering:
        setInfo(tr("Discovering services..."));
        break;

    case QLowEnergyService::RemoteServiceDiscovered:
    {
        setInfo(tr("Service discovered."));

        const QLowEnergyCharacteristic accCharX = ptr_dataService->characteristic(QBluetoothUuid(BLE_UUID_ACCELEROMETER_CHARACTERISTIC_X));
        const QLowEnergyCharacteristic accCharY = ptr_dataService->characteristic(QBluetoothUuid(BLE_UUID_ACCELEROMETER_CHARACTERISTIC_Y));
        const QLowEnergyCharacteristic accCharZ = ptr_dataService->characteristic(QBluetoothUuid(BLE_UUID_ACCELEROMETER_CHARACTERISTIC_Z));

        if (!accCharX.isValid() || !accCharY.isValid() || !accCharZ.isValid())
        {
            setError("Data not found.");
            break;
        }

        leAccNotificationDescX = accCharX.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
        leAccNotificationDescY = accCharY.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
        leAccNotificationDescZ = accCharZ.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);

        if (leAccNotificationDescX.isValid() && leAccNotificationDescY.isValid()
                && leAccNotificationDescZ.isValid())
        {
            connect(ptr_dataService, &QLowEnergyService::characteristicChanged, this, &BLEcontroller::updateSensorsData);

            //Enable notifications:
            ptr_dataService->writeDescriptor(leAccNotificationDescX, QByteArray::fromHex("0100"));
            ptr_dataService->writeDescriptor(leAccNotificationDescY, QByteArray::fromHex("0100"));
            ptr_dataService->writeDescriptor(leAccNotificationDescZ, QByteArray::fromHex("0100"));
        }
        break;
    }

    default:
        break;
    }
}
//------------------------------------------------------------------------------
//Quando viene detectata una characteristic change il programma entra qui
//ad ogni giro i dei sensori devono essere cancellati

//offset:
//acc 0 0 -1
//gyro 0 0 0

//valori letti con offset:
//acc 0.5 0.3 0.2
//gyor 10 20 30

//faccio soglia : acc[0]*100 > 50 -> moving

// problema: il check viene fatto a fine ciclo, quando il macchinario è fermo
//allora per risolvere faccio così: continuo a contare quanti valori letti,
void BLEcontroller::updateSensorsData(const QLowEnergyCharacteristic &c, const QByteArray &value)
{
    double dValue = static_cast<double>(QByteArrayToFloat(value));

    static int iOnce = 0;
    if (iOnce < 1)
    {
        iOnce++;
        bConnected = true;
        emit connectionCompleted();
    }

    if (c.uuid() == QBluetoothUuid(BLE_UUID_ACCELEROMETER_CHARACTERISTIC_X))
    {
        qDebug()<<"ACCX :"<<dValue;
        dAccelerometerX = dValue;
        bAccX = true;
    }
    else if (c.uuid() == QBluetoothUuid(BLE_UUID_ACCELEROMETER_CHARACTERISTIC_Y))
    {
        dAccelerometerY = dValue;
    }
    else if (c.uuid() == QBluetoothUuid(BLE_UUID_ACCELEROMETER_CHARACTERISTIC_Z))
    {
        qDebug()<<"ACCZ :"<<dValue;
        dAccelerometerZ = dValue;
        bAccZ = true;
    }
    emit checkAvaiableDegree();
}
//------------------------------------------------------------------------------
float BLEcontroller::QByteArrayToFloat(const QByteArray &qba)
{
    return *(reinterpret_cast<const float*>(qba.constData()));
}
//------------------------------------------------------------------------------
void BLEcontroller::accelerationToDegrees(void)
{
    if (!(bAccX && bAccZ))
        return;
    double dDegrees = qRadiansToDegrees(qAtan(dAccelerometerX/dAccelerometerZ));
    vdDegrees.append(dDegrees);
    qDebug()<<"Degree: "<<dDegrees;
    bAccX = false;
    bAccZ = false;
}
//------------------------------------------------------------------------------
void BLEcontroller::clearDegreeValues(void)
{
    vdDegrees.clear();
}
//------------------------------------------------------------------------------
