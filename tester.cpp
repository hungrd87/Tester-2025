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
    , m_dialog(nullptr), m_module(-1), m_last_module(-1), m_step(0)
{
    ui->setupUi(this);
    QThread::currentThread()->setObjectName("MainThread");

    //MenuBar
    ui->menubar->hide();

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

    //StatusBar
    m_progressBar = new QProgressBar();
    m_progressBar->setMaximum(STAGE_GOOD);
    m_progressBar->setValue(0);
    m_progressBar->setAlignment(Qt::AlignCenter);
    ui->statusbar->addWidget(m_progressBar,1);

    ui->stackedWidget->setCurrentIndex(0);

    //Classes
    m_udp = new Udp;
    m_dialog = new Dialog(this);

    //Process
    m_process = new QProcess(this);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, QOverload<int, QProcess::ExitStatus>::of(&Tester::slotProcessFinished));

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
    connect(this, &Tester::signalIpChanged, m_udp, &Udp::slotIpChanged);
    connect(m_udp, &Udp::signalStageChanged, this, &Tester::slotStageChanged);

    //Initial
//    ui->actionSelect->setEnabled(false);
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
}

Tester::~Tester()
{
    m_thread->quit();
    m_thread->wait();

    delete ui;
}

void Tester::changeIp()
{
    QString iface ="";
    QFile file(qApp->applicationDirPath()+ "/networks.conf");
    if(file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        iface = QString(file.readLine());
    }
    file.close();

    QString program = "C:/Windows/System32/netsh.exe";
    QStringList arguments;
    arguments << "interface" << "ip" << "set" << "address" << iface << "static" << "192.168.2.248" << "255.255.255.0" << "192.168.2.1";
    m_process->start(program, arguments);
    m_process->waitForFinished();

    arguments.clear();
    arguments << "interface" << "ip" << "add" << "address" << iface << "192.168.1.12" << "255.255.255.0";
    m_process->start(program, arguments);
    m_process->waitForFinished();

    if(m_module == MODULE_PROCESS){
        arguments.clear();
        arguments << "interface" << "ip" << "add" << "address" << iface << "192.168.1.245" << "255.255.255.0";
        m_process->start(program, arguments);
        m_process->waitForFinished();

        arguments.clear();
        arguments << "interface" << "ip" << "add" << "address" << iface << "192.168.1.200" << "255.255.255.0";
        m_process->start(program, arguments);
        m_process->waitForFinished();
    }
    else {
        arguments.clear();
        arguments << "interface" << "ip" << "add" << "address" << iface << "192.168.1.240" << "255.255.255.0";
        m_process->start(program, arguments);
        m_process->waitForFinished();
    }
}

void Tester::loadStep()
{
    QFile file(qApp->applicationDirPath() + QString("/data/steps/%1/%2.html").arg(m_module).arg(m_step));
    if(file.open(QIODevice::ReadOnly | QIODevice::Text))
        ui->textBrowser->setHtml(file.readAll());

    QString s = qApp->applicationDirPath() + QString("/data/images/%1/%2.png").arg(m_module).arg(m_step);
    ui->labelImage->setPixmap(QPixmap(s));

    if(m_step==0){
        ui->pushButtonPrev->setEnabled(false);
        ui->pushButtonNext->setEnabled(true);
    }
    else if(m_step==3){
        ui->pushButtonPrev->setEnabled(true);
        ui->pushButtonNext->setEnabled(false);

        ui->actionStart->setEnabled(true);
        ui->actionStop->setEnabled(true);
    }
    else{
        ui->pushButtonPrev->setEnabled(true);
        ui->pushButtonNext->setEnabled(true);
    }
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
    m_module = index+1;

    if((m_last_module == -1) ||
       (m_last_module == MODULE_PROCESS && m_module != MODULE_PROCESS) ||
       (m_module == MODULE_PROCESS && m_last_module!= MODULE_PROCESS)) {
        changeIp();

        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        QThread::sleep(5);
        QApplication::restoreOverrideCursor();

        emit signalIpChanged();

    }
    m_last_module = m_module;

    ui->stackedWidget->setCurrentIndex(1);

    m_step = 0;
    loadStep();
}

void Tester::slotProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode)
    Q_UNUSED(exitStatus)
}

void Tester::slotStageChanged(int stage)
{
    if(stage == STAGE_GOOD){
        ui->pushButtonGood->setEnabled(true);
        ui->pushButtonGood->setStyleSheet("background-color: rgb(0, 255, 0);");
        ui->pushButtonBad->setEnabled(false);
        ui->pushButtonBad->setStyleSheet("");
        ui->textEditResult->clear();
        ui->textEditResult->append("Hoàn tất kiểm tra module.");
        m_started = false;
    }
    else if(stage == STAGE_BAD){
        ui->pushButtonGood->setEnabled(false);
        ui->pushButtonGood->setStyleSheet("");
        ui->pushButtonBad->setEnabled(true);
        ui->pushButtonBad->setStyleSheet("background-color: rgb(255, 0, 0);");
        m_started = false;

        ui->textEditResult->clear();
        QFile file(qApp->applicationDirPath() + QString("/data/results/%1.html").arg(m_module));
        if(file.open(QIODevice::ReadOnly | QIODevice::Text))
            ui->textEditResult->setHtml(file.readAll());
    }
    else if(stage == STAGE_IDLE){
        ui->pushButtonGood->setEnabled(false);
        ui->pushButtonGood->setStyleSheet("");
        ui->pushButtonBad->setEnabled(false);
        ui->pushButtonBad->setStyleSheet("");
        ui->textEditResult->clear();
        ui->textEditResult->append("Sẵn sàng kiểm tra!");
        m_started = false;
    }
    else{
        m_started = true;
        ui->pushButtonGood->setEnabled(false);
        ui->pushButtonGood->setStyleSheet("");
        ui->pushButtonBad->setEnabled(false);
        ui->pushButtonBad->setStyleSheet("");
        ui->textEditResult->clear();
        ui->textEditResult->append("Đang tự động kiểm tra...");
        ui->statusbar->clearMessage();
    }
}

int g_status = 0;
void Tester::timerEvent(QTimerEvent *)
{
    if(!m_started){
        if(m_udp->armOnline() && m_udp->fpgaOnline()){
            ui->actionSelect->setEnabled(true);
            ui->frame->setEnabled(true);
            ui->statusbar->showMessage("Sẵn sàng kết nối!");
        }
        else{
            ui->actionSelect->setEnabled(false);
            ui->frame->setEnabled(false);
            if (!m_udp->armOnline() && !m_udp->fpgaOnline())
                ui->statusbar->showMessage("Không có kết nối");
            else if (!m_udp->fpgaOnline())
                ui->statusbar->showMessage("Không có kết nối Device 1");
            else
                ui->statusbar->showMessage("Không có kết nối Device 2");
        }
    }
    else{
        m_progressBar->setValue(m_udp->stage());
        m_progressBar->setFormat(QString("%p% - %1").arg(stages[m_udp->stage()]));

//        ui->statusbar->showMessage(stages[m_udp->stage()]);
    }
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
    ui->actionStart->setEnabled(false);
    ui->actionReset->setEnabled(true);
    ui->actionStop->setEnabled(true);
    ui->statusbar->clearMessage();

    ui->pushButtonPrev->setEnabled(false);
}

void Tester::on_actionReset_triggered()
{
    emit signalResetTest();
    ui->actionStart->setEnabled(false);
    ui->actionReset->setEnabled(true);
    ui->actionStop->setEnabled(true);
    ui->statusbar->clearMessage();
}

void Tester::on_actionStop_triggered()
{
    emit signalStopTest();
    ui->stackedWidget->setCurrentIndex(0);

    ui->actionStart->setEnabled(false);
    ui->actionReset->setEnabled(false);
    ui->actionStop->setEnabled(false);
    ui->statusbar->clearMessage();
}

void Tester::on_actionContents_triggered()
{

}

void Tester::on_actionAbout_triggered()
{

}

void Tester::on_pushButtonPrev_clicked()
{
    if(m_step>0)
        m_step--;
    loadStep();
}

void Tester::on_pushButtonNext_clicked()
{
    if(m_step<3)
        m_step++;
    loadStep();
}
