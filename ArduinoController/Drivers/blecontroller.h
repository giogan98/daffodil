#ifndef BLECONTROLLER_H
#define BLECONTROLLER_H

#include <QtBluetooth>


class BLEcontroller : public QObject
{
    Q_OBJECT

public:
    QString m_error;
    QString m_info;
    //Double per conservare il valore dell'accelerometro letto dai sensori:
    double dAccelerometerX;
    double dAccelerometerY;
    double dAccelerometerZ;
    //Vettore per conservare il valore di offset dell'accelerometro:
    QVector<double> vdAccelerometerOffset;
    //Vettore per conservare il numero di misure lette durante la calibrazione:
    QVector<int> viAccelerometerOffsetCounter;
    //Vettore per il controllo dei gradi del macchinario
    QVector<double> vdDegrees;
    bool bConnected;
    bool bCalibrationInProgress;
    bool bCalibrated;
    bool bAccX;
    bool bAccZ;
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

public:
    BLEcontroller();
    void setError(const QString &error);
    void setInfo(const QString &info);
    void clearDegreeValues(void);

private:
    void initializeBluetoothDeviceDiscoveryAgent(void);
    void connectServicePointer(QLowEnergyService *ptr_service);
    float QByteArrayToFloat(const QByteArray &qba);
    void setDevice(QBluetoothDeviceInfo device);
    void calibrateSensor(double &dSensorAxisValues, const double &dValue, int &iSensorAxisCounter);

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
    void accelerationToDegrees(void);

signals:
    void errorChanged(void);
    void infoChanged(void);
    void deviceListAvaiable(QList<QBluetoothDeviceInfo>);
    void calibrationPossible(void);
    void checkAvaiableDegree(void);
};

#endif // BLECONTROLLER_H
