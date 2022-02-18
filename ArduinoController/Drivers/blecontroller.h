#ifndef BLECONTROLLER_H
#define BLECONTROLLER_H

#include <QtBluetooth>


class BLEcontroller : public QObject
{
    Q_OBJECT

public:
    QString m_error;
    QString m_info;
    QVector<float> vfAccelerometer;
    QVector<float> vfGyroscope;

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

public slots:
    void scanError(QBluetoothDeviceDiscoveryAgent::Error error);
    void startDeviceScan(void);
    void scanFinished(void);
    void getSelectedDevice(int iIndex);
    void serviceDiscovered(const QBluetoothUuid &gatt);
    void serviceScanDone(void);
    void serviceStateChanged(QLowEnergyService::ServiceState s);
    void updateSensorsData(const QLowEnergyCharacteristic &c, const QByteArray &value);

signals:
    void errorChanged();
    void infoChanged();
    void deviceListAvaiable(QList<QBluetoothDeviceInfo>);
    void newAccDataAvaiable(float);
    void newGyroDataAvaiable(float);

};

#endif // BLECONTROLLER_H
