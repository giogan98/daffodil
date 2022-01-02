#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "blecontroller.h"

#include <QMainWindow>
#include <QPlainTextEdit>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:

private:
    Ui::MainWindow *ui;
    BLEcontroller *blecontroller = nullptr;

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    QBluetoothDeviceInfo getDeviceFromCBox(const int &iIndex);
    void addDataToPlainTextEdit(QPlainTextEdit *pte, const QString &data);
    void setupBLEcontroller(void);

public slots:
    void updateAccelerometerData(float);
    void updateGyroscopeData(float);
    void displayDeviceList(QList<QBluetoothDeviceInfo>);

private slots:
    void on_pbn_search_clicked();
    void on_pbn_okDevice_clicked();

signals:
    void startDeviceSearch();
    void deviceSelected(int);
};
#endif // MAINWINDOW_H
