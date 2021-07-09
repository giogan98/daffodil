#include "login.h"
#include "ui_login.h"
#include <QMessageBox>

LogIn::LogIn(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LogIn)
{
    ui->setupUi(this);
    setWindowTitle("Daffodil");
    Username = "user";
    Password = "pass";
    ui->le_username->setPlaceholderText("Username");
    ui->le_password->setPlaceholderText("Passowrd");
    ui->le_password->setEchoMode(QLineEdit::Password);
    this->setFixedSize(QSize(800,600));
}
//-----------------------------------------------------------------------------
LogIn::~LogIn()
{
    delete ui;
}
//-----------------------------------------------------------------------------
void LogIn::keyPressEvent(QKeyEvent *e)
{
    switch (e->key())
    {
    case Qt::Key_Enter:
    case Qt::Key_Return:
        tryLogin();
        break;
    default:
        break;
    }
}
//-----------------------------------------------------------------------------
void LogIn::tryLogin()
{
    QString str_user = ui->le_username->text().remove(" ");
    QString str_pass = ui->le_password->text().remove(" ");

    if (str_user == Username && str_pass == Password)
    {
        this->close();
        pNewWin = new MainWindow;
        pNewWin->show();
    }
    else
    {
        QMessageBox::warning(this,"Error","Username or password are incorrect");
    }
}
//-----------------------------------------------------------------------------
