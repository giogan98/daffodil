#ifndef LOGIN_H
#define LOGIN_H

#include <QDialog>
#include <QKeyEvent>
#include "mainwindow.h"

namespace Ui {
class LogIn;
}

class LogIn : public QDialog
{
    Q_OBJECT

public:


private:
    Ui::LogIn *ui;
    QString Username;
    QString Password;
    MainWindow *pNewWin;

public:
    explicit LogIn(QWidget *parent = nullptr);
    ~LogIn();

private:
    void keyPressEvent(QKeyEvent *e);
    void tryLogin(void);
};

#endif // LOGIN_H
