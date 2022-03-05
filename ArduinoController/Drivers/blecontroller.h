#ifndef BLECONTROLLER_H
#define BLECONTROLLER_H

#include <QtBluetooth>


class BLEcontroller : public QObject
{
    Q_OBJECT

public:
    QString m_error;
    QString m_info;
    //Vettori per conservare il valore letto dai sensori:
    QVector<float> vfAccelerometer;
    QVector<float> vfGyroscope;
    //Vettori per conservare il valore di offset dei sensori:
    QVector<float> vfAccelerometerOffset;
    QVector<float> vfGyroscopeOffset;
    //Vettori per conservare il numero di misure lette durante la calibrazione:
    QVector<int> viAccelerometerOffsetCounter;
    QVector<int> viGyroscopeOffsetCounter;
    bool bConnected;
    bool bCalibrationInProgress;
    bool bCalibrated;
    typedef enum
    {
        AXIS_X = 0,
        AXIS_Y = 1,
        AXIS_Z = 2,
    }enAxes;

private:
    bool bFoundDataService;
    QLowEnergyController *ptr_leController = nullptr;
    QBluetoothDeviceDiscoveryAgent *ptr_deviceDiscoveryAgent = nullptr;
    QLowEnergyService *ptr_dataService = nullptr;
    QList<QBluetoothDeviceInfo> list_devicesInfos;
    QLowEnergyDescriptor leAccNotificationDescX;
    QLowEnergyDescriptor leAccNotificationDescY;
    QLowEnergyDescriptor leAccNotificationDescZ;
    QLowEnergyDescriptor leGyroNotificationDescX;
    QLowEnergyDescriptor leGyroNotificationDescY;
    QLowEnergyDescriptor leGyroNotificationDescZ;

public:
    BLEcontroller();
    void setError(const QString &error);
    void setInfo(const QString &info);

private:
    void initializeBluetoothDeviceDiscoveryAgent(void);
    void connectServicePointer(QLowEnergyService *ptr_service);
    float QByteArrayToFloat(const QByteArray &qba);
    void setDevice(QBluetoothDeviceInfo device);
    void calibrateSensor(float &fSensorAxisValues, const float &fValue, int &iSensorAxisCounter);

public slots:
    void scanError(QBluetoothDeviceDiscoveryAgent::Error error);
    void startDeviceScan(void);
    void scanFinished(void);
    void connectToSelectedDevice(int iIndex);
    void serviceDiscovered(const QBluetoothUuid &gatt);
    void serviceScanDone(void);
    void serviceStateChanged(QLowEnergyService::ServiceState s);
    void updateSensorsData(const QLowEnergyCharacteristic &c, const QByteArray &value);
    void startDeviceCalibration(int iTimer);
    void finishSensorCalibration(void);

signals:
    void errorChanged(void);
    void infoChanged(void);
    void deviceListAvaiable(QList<QBluetoothDeviceInfo>);
    void newAccDataAvaiable(float);
    void newGyroDataAvaiable(float);
    void calibrationPossible(void);

};

#endif // BLECONTROLLER_H
