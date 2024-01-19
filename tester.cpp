#include "tester.h"
#include "ui_tester.h"
#include <QTimer>
#include <QDebug>
#include <QThread>
#include <QNetworkInterface>
#include <QFile>
#include <QProcess>

Tester::Tester(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Tester)
    , m_dialog(nullptr), m_module(-1)
{
    ui->setupUi(this);
    QThread::currentThread()->setObjectName("MainThread");

    //ToolBar
    ui->toolBar->layout()->setContentsMargins(0,0,0,0);
    ui->toolBar->setFloatable(false);
    ui->toolBar->setMovable(false);
    ui->actionSelect->setIcon(QApplication::style()->standardIcon(QStyle::SP_FileDialogListView));
    ui->actionExit->setIcon(QApplication::style()->standardIcon(QStyle::SP_MessageBoxCritical));
    ui->actionStart->setIcon(QApplication::style()->standardIcon(QStyle::SP_ArrowForward));
    ui->actionReset->setIcon(QApplication::style()->standardIcon(QStyle::SP_BrowserReload));
    ui->actionStop->setIcon(QApplication::style()->standardIcon(QStyle::SP_BrowserStop));
    ui->actionContents->setIcon(QApplication::style()->standardIcon(QStyle::SP_MessageBoxQuestion));
    ui->actionAbout->setIcon(QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation));

    ui->toolButton_1->setIcon(ui->actionSelect->icon());
    ui->toolButton_2->setIcon(ui->actionStart->icon());
    ui->toolButton_3->setIcon(ui->actionReset->icon());
    ui->toolButton_4->setIcon(ui->actionStop->icon());

    //Classes
    m_udp = new Udp;
    m_dialog = new Dialog(this);

    //Threads
    m_thread = new QThread(this);
    m_thread->setObjectName("UdpThread");
    m_udp->moveToThread(m_thread);
    connect(m_thread, &QThread::finished, m_udp, &QObject::deleteLater);
    m_thread->start();

    //Connections
    connect(m_dialog, SIGNAL(moduleSelected(int)), this, SLOT(currentModuleChanged(int)));
    connect(this, &Tester::signalStartTest, m_udp, &Udp::slotStartTest);
    connect(this, &Tester::signalResetTest, m_udp, &Udp::slotResetTest);
    connect(this, &Tester::signalStopTest, m_udp, &Udp::slotStopTest);

    //Initial
    ui->actionSelect->setEnabled(false);
    ui->actionStart->setEnabled(false);
    ui->actionReset->setEnabled(false);
    ui->actionStop->setEnabled(false);

    //Timer
//    QTimer::singleShot(1000, this, &Tester::on_actionSelect_triggered);
    startTimer(250);

    //Config
    if(!QFile::exists(qApp->applicationDirPath() + "/networks.conf")){
        QTimer::singleShot(500, this, &Tester::firstConfig);
    }

    //Test
    QString program = "C:/Windows/System32/netsh.exe";
    QStringList arguments;
    arguments << "interface" << "ip" << "set" << "address" << "Ethernet" << "static" << "192.168.2.248" << "255.255.255.0" << "192.168.2.1";
    QProcess *myProcess = new QProcess(this);
    connect(myProcess, &QProcess::stateChanged, this, &Tester::stateChanged);
    myProcess->start(program, arguments);
}

Tester::~Tester()
{
    m_thread->quit();
    m_thread->wait();

    delete ui;
}

void Tester::stateChanged(QProcess::ProcessState newState)
{
    QProcess *process = (QProcess*)sender();
    qDebug() << "State change to:" << newState << "exitstatus" << process->exitStatus() << "exitcode" << process->exitCode();

}

void Tester::firstConfig()
{
    Config *config = new Config(this);
    config->setAttribute(Qt::WA_DeleteOnClose);
    config->setAttribute(Qt::WA_AlwaysStackOnTop);
    config->show();
}

void Tester::showDialog()
{
    m_dialog->show();
}

void Tester::currentModuleChanged(int index)
{
    m_module = index;
    ui->stackedWidget->setCurrentIndex(1);
}

int g_status = 0;
void Tester::timerEvent(QTimerEvent *)
{
    if(m_udp->armOnline() && m_udp->fpgaOnline())
        ui->statusbar->showMessage("Connected!");
    else if (!m_udp->armOnline() && !m_udp->fpgaOnline())
        ui->statusbar->showMessage("No connection");
    else if (!m_udp->fpgaOnline())
        ui->statusbar->showMessage("No connection to FPGA");
    else
        ui->statusbar->showMessage("No connection to ARM");
}

void Tester::on_actionSelect_triggered()
{
    ui->stackedWidget->setCurrentIndex(0);
    showDialog();
}

void Tester::on_actionExit_triggered()
{
    this->close();
}

void Tester::on_actionStart_triggered()
{
    emit signalStartTest(m_module);
}

void Tester::on_actionReset_triggered()
{
    emit signalResetTest();
}

void Tester::on_actionStop_triggered()
{
    emit signalStopTest();
    ui->stackedWidget->setCurrentIndex(0);
}

void Tester::on_actionContents_triggered()
{

}

void Tester::on_actionAbout_triggered()
{

}
