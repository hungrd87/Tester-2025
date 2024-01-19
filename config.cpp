#include "config.h"
#include "ui_config.h"
#include <QNetworkInterface>
#include <QFile>

Config::Config(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Config)
{
    ui->setupUi(this);

    QList<QNetworkInterface> allInterfaces = QNetworkInterface::allInterfaces();
    foreach(QNetworkInterface iFace, allInterfaces) {
        ui->comboBox->addItem(iFace.humanReadableName());
    }
}

Config::~Config()
{
    delete ui;
}

void Config::on_buttonBox_clicked(QAbstractButton *button)
{
    if(ui->buttonBox->standardButton(button)==QDialogButtonBox::Ok){
        QString interface = ui->comboBox->currentText();
        QFile file(qApp->applicationDirPath()+ "/networks.conf");
        if(file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            file.write(interface.toUtf8());
        }
        file.close();

        //
        QString eth0 = QString("netsh interface ip set address \"%1\" static 192.168.2.248 255.255.255.0 192.168.2.1\n"
                               "netsh interface ip add address \"%1\" 192.168.1.100 255.255.255.0\n"
                               "netsh interface ip add address \"%1\" 192.168.1.240 255.255.255.0")
                               .arg(interface);
        QString eth1 = QString("netsh interface ip set address \"%1\" static 192.168.2.248 255.255.255.0 192.168.2.1\n"
                               "netsh interface ip add address \"%1\" 192.168.1.100 255.255.255.0\n"
                               "netsh interface ip add address \"%1\" 192.168.1.245 255.255.255.0\n"
                               "netsh interface ip add address \"%1\" 192.168.1.200 255.255.255.0")
                               .arg(interface);

        QFile f0(qApp->applicationDirPath()+ "/eth0.bat");
        if(f0.open(QIODevice::WriteOnly | QIODevice::Text)) {
            f0.write(eth0.toUtf8());
        }
        f0.close();

        QFile f1(qApp->applicationDirPath()+ "/eth1.bat");
        if(f1.open(QIODevice::WriteOnly | QIODevice::Text)) {
            f1.write(eth1.toUtf8());
        }
        f1.close();
    }
}
