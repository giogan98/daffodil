#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtBluetooth>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

private:
    Ui::MainWindow *ui;
    QBluetoothDeviceDiscoveryAgent *ptr_deviceDiscoveryAgent = nullptr;
    QList<QObject*> m_devices;
    QList<QBluetoothDeviceInfo> list_devicesInfos;
    QString m_error;
    QString m_info;
    QLowEnergyController *ptr_leController = nullptr;

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void setError(const QString &error);
    void setInfo(const QString &info);

private:
    void initializeBluetoothDeviceDiscoveryAgent(void);
    QBluetoothDeviceInfo getDevice(const int &iIndex);
    void setDevice(const QBluetoothDeviceInfo &device);

public slots:
    void scanError(QBluetoothDeviceDiscoveryAgent::Error error);
    void scanFinished(void);

signals:
    void scanningChanged(); //Device Finder Class
    void devicesChanged();  //Device Finder Class
    void errorChanged(); //Bluetooth Base Class
    void infoChanged();  //Bluetooth Base Class


private slots:
    void on_pbn_search_clicked();
    void on_pbn_okDevice_clicked();
};
#endif // MAINWINDOW_H
