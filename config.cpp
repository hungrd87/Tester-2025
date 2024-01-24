#include "config.h"
#include "ui_config.h"
#include <QNetworkInterface>
#include <QFile>
#include <QDialogButtonBox>
#include <QPushButton>

Config::Config(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Config)
{
    ui->setupUi(this);

    QList<QNetworkInterface> allInterfaces = QNetworkInterface::allInterfaces();
    foreach(QNetworkInterface iFace, allInterfaces) {
        ui->comboBox->addItem(iFace.humanReadableName());
    }

    ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Thiết lập");
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText("Hủy");
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

        emit signalConfigured();
    }
}
