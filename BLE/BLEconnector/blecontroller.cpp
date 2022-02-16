#include "blecontroller.h"

#define BLE_UUID_DATA_SERVICE                   "5aaeb650-c2cb-44d1-b4ab-7144e08aed2e"

#define BLE_UUID_ACCELEROMETER_CHARACTERISTIC_X "f4055745-6f5a-4e2b-8433-2704337cc3b5"
#define BLE_UUID_ACCELEROMETER_CHARACTERISTIC_Y "ac7a390c-abb3-48c8-8c54-73c3a6a4bc73"
#define BLE_UUID_ACCELEROMETER_CHARACTERISTIC_Z "3c71aaec-128b-4f88-bb60-28a2026498be"

#define BLE_UUID_GYROSCOPE_CHARACTERISTIC_X     "9936153d-65bc-4479-b079-aa25569f9ab1"
#define BLE_UUID_GYROSCOPE_CHARACTERISTIC_Y     "ef1cdf9d-56fd-437d-8c4e-3a77cf8c8265"
#define BLE_UUID_GYROSCOPE_CHARACTERISTIC_Z     "69911b28-ffcc-4a65-85de-1b501f7a5e40"

//------------------------------------------------------------------------------
BLEcontroller::BLEcontroller()
{
    initializeBluetoothDeviceDiscoveryAgent();
    //Need this to be initialized to false, its value will be changed if a data
    //service is found:
    bFoundDataService = false;
    vfAccelerometer = QVector<float>(3, 0); //vfAccelerometer = [X, Y, Z]
    vfGyroscope = QVector<float>(3, 0); //vfGyroscope = [X, Y, Z]
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
void BLEcontroller::getSelectedDevice(int iIndex)
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
    bFoundDataService = false; //@todo ha senso introdurre questo??
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

        const QLowEnergyCharacteristic gyroCharX = ptr_dataService->characteristic(QBluetoothUuid(BLE_UUID_GYROSCOPE_CHARACTERISTIC_X));
        const QLowEnergyCharacteristic gyroCharY = ptr_dataService->characteristic(QBluetoothUuid(BLE_UUID_GYROSCOPE_CHARACTERISTIC_Y));
        const QLowEnergyCharacteristic gyroCharZ = ptr_dataService->characteristic(QBluetoothUuid(BLE_UUID_GYROSCOPE_CHARACTERISTIC_Z));

        if (!accCharX.isValid() || !accCharY.isValid() || !accCharZ.isValid() ||
                !gyroCharX.isValid() || !gyroCharY.isValid() || !gyroCharZ.isValid())
        {
            setError("Data not found.");
            break;
        }

        leAccNotificationDescX = accCharX.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
        leAccNotificationDescY = accCharY.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
        leAccNotificationDescZ = accCharZ.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);

        leGyroNotificationDescX = gyroCharX.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
        leGyroNotificationDescY = gyroCharY.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);
        leGyroNotificationDescZ = gyroCharZ.descriptor(QBluetoothUuid::DescriptorType::ClientCharacteristicConfiguration);

        if (leAccNotificationDescX.isValid() && leAccNotificationDescY.isValid()
                && leAccNotificationDescZ.isValid() && leGyroNotificationDescX.isValid()
                && leGyroNotificationDescY.isValid() && leGyroNotificationDescZ.isValid())
        {
            connect(ptr_dataService, &QLowEnergyService::characteristicChanged, this, &BLEcontroller::updateSensorsData);

            //Enable notifications:
            ptr_dataService->writeDescriptor(leAccNotificationDescX, QByteArray::fromHex("0100"));
            ptr_dataService->writeDescriptor(leAccNotificationDescY, QByteArray::fromHex("0100"));
            ptr_dataService->writeDescriptor(leAccNotificationDescZ, QByteArray::fromHex("0100"));
            ptr_dataService->writeDescriptor(leGyroNotificationDescX, QByteArray::fromHex("0100"));
            ptr_dataService->writeDescriptor(leGyroNotificationDescY, QByteArray::fromHex("0100"));
            ptr_dataService->writeDescriptor(leGyroNotificationDescZ, QByteArray::fromHex("0100"));
        }
        break;
    }

    default:
        break;
    }
}
//------------------------------------------------------------------------------
void BLEcontroller::updateSensorsData(const QLowEnergyCharacteristic &c, const QByteArray &value)
{
    float fValue = QByteArrayToFloat(value);

    if (c.uuid() == QBluetoothUuid(BLE_UUID_ACCELEROMETER_CHARACTERISTIC_X))
    {
        vfAccelerometer[0] = fValue;
        emit newAccDataAvaiable(fValue);
    }
    else if (c.uuid() == QBluetoothUuid(BLE_UUID_ACCELEROMETER_CHARACTERISTIC_Y))
    {
        vfAccelerometer[1] = fValue;
        emit newAccDataAvaiable(fValue);
    }
    else if (c.uuid() == QBluetoothUuid(BLE_UUID_ACCELEROMETER_CHARACTERISTIC_Z))
    {
        vfAccelerometer[2] = fValue;
        emit newAccDataAvaiable(fValue);
    }
    else if (c.uuid() == QBluetoothUuid(BLE_UUID_GYROSCOPE_CHARACTERISTIC_X))
    {
        vfGyroscope[0] = fValue;
        emit newGyroDataAvaiable(fValue);
    }
    else if (c.uuid() == QBluetoothUuid(BLE_UUID_GYROSCOPE_CHARACTERISTIC_Y))
    {
        vfGyroscope[1] = fValue;
        emit newGyroDataAvaiable(fValue);
    }
    else if (c.uuid() == QBluetoothUuid(BLE_UUID_GYROSCOPE_CHARACTERISTIC_Z))
    {
        vfGyroscope[2] = fValue;
        emit newGyroDataAvaiable(fValue);
    }

}
//------------------------------------------------------------------------------
float BLEcontroller::QByteArrayToFloat(const QByteArray &qba)
{
    return *(reinterpret_cast<const float*>(qba.constData()));
}
//------------------------------------------------------------------------------
