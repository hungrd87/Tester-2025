#include "view.h"
#include "ui_view.h"
#include <QTimer>
#include <QDebug>
#include <QThread>
#include <QNetworkInterface>
#include <QFile>
#include <QProcess>

View::View(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::View)
    , m_dialog(nullptr), m_module(-1), m_last_module(-1), m_step(1)
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

    // ui->toolButton_1->setIcon(ui->actionSelect->icon());
    // ui->toolButton_2->setIcon(ui->actionStart->icon());
    // ui->toolButton_3->setIcon(ui->actionReset->icon());
    // ui->toolButton_4->setIcon(ui->actionStop->icon());

    ui->toolButton_1->setDefaultAction(ui->actionSelect);
    ui->toolButton_2->setDefaultAction(ui->actionStart);
    ui->toolButton_3->setDefaultAction(ui->actionReset);
    ui->toolButton_4->setDefaultAction(ui->actionStop);

    //StatusBar
    m_progressBar = new QProgressBar();
    m_progressBar->setMaximum(STAGE_GOOD);
    m_progressBar->setValue(0);
    m_progressBar->setAlignment(Qt::AlignCenter);
    ui->statusbar->addWidget(m_progressBar,1);

    ui->groupBox_3->setVisible(false);  //Không dùng trong phiên bản mới
    m_progressBar->setVisible(false);   //Không dùng trong phiên bản mới

    // Setup ComponentMapperWidget on page_3
    m_componentMapperWidget = new ComponentMapperWidget(ui->page_3);
    QVBoxLayout *page3Layout = new QVBoxLayout(ui->page_3);
    page3Layout->setContentsMargins(0, 0, 0, 0);
    page3Layout->addWidget(m_componentMapperWidget);
    ui->page_3->setLayout(page3Layout);

    ui->stackedWidget->setCurrentIndex(0);
    ui->labelLogo->setPixmap(QPixmap(qApp->applicationDirPath() + QString("/data/images/logo.png")));

    //Classes
    m_dialog = new Dialog(this);
    m_udp = new Udp;
    m_tester = new Tester;

    //Process
    m_process = new QProcess(this);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, QOverload<int, QProcess::ExitStatus>::of(&View::slotProcessFinished));

    //Threads
    m_thread = new QThread(this);
    m_thread->setObjectName("TesterThread");
    m_tester->moveToThread(m_thread);
    connect(m_thread, &QThread::finished, m_tester, &QObject::deleteLater);
    m_thread->start();

    //Connections
    connect(m_dialog, SIGNAL(moduleSelected(int)), this, SLOT(currentModuleChanged(int)));
    connect(this, &View::signalIpChanged, m_udp, &Udp::slotIpChanged);
    connect(this, &View::signalStartTest, m_tester, &Tester::slotStartTest);
    connect(this, &View::signalResetTest, m_tester, &Tester::slotResetTest);
    connect(this, &View::signalStopTest, m_tester, &Tester::slotStopTest);
    connect(m_udp, &Udp::signalInterconnectOk, m_tester, &Tester::slotInterconnectOk);
    connect(m_udp, &Udp::signalProcessOk, m_tester, &Tester::slotProcessOk);
    // connect(m_tester, &Tester::signalStageChanged, this, &View::slotStageChanged);
    connect(m_tester, &Tester::signalStageUpdate, m_componentMapperWidget, &ComponentMapperWidget::onStageUpdate);
    connect(m_tester, &Tester::signalSendToArm, m_udp, &Udp::slotSendToArm);
    connect(m_tester, &Tester::signalSendToInterconnect, m_udp, &Udp::slotSendToInterconnect);


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
        QTimer::singleShot(500, this, &View::firstConfig);
    }

    qDebug() << "STARTED";
}

View::~View()
{
    m_thread->quit();
    m_thread->wait();

    delete ui;
}

void View::changeIp()
{
    QString iface ="";
    QFile file(qApp->applicationDirPath()+ "/networks.conf");
    if(file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        iface = QString(file.readLine());
    }
    file.close();

    QString program = "C:/Windows/System32/netsh.exe";
    QStringList arguments;
    arguments << "interface" << "ip" << "set" << "address" << iface << "static" << "192.168.1.12" << "255.255.255.0" << "192.168.1.1";
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

void View::loadStep()
{
    QFile file(qApp->applicationDirPath() + QString("/data/steps/%1/%2.html").arg(m_module).arg(m_step));
    if(file.open(QIODevice::ReadOnly | QIODevice::Text))
        ui->textBrowser->setHtml(file.readAll());

    QString s = qApp->applicationDirPath() + QString("/data/images/%1/%2.png").arg(m_module).arg(m_step);
    // QPixmap pixmap = QPixmap(s);
    QPixmap pixmap = QPixmap(s).scaled(ui->labelImage->size(),Qt::KeepAspectRatio);
    ui->labelImage->setPixmap(pixmap);

    if(m_step==1){
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

void View::setAdmin(bool is_admin)
{
    m_componentMapperWidget->setAdmin(is_admin);
}

void View::firstConfig()
{
    Config *config = new Config(this);
    config->setAttribute(Qt::WA_DeleteOnClose);
    config->setAttribute(Qt::WA_AlwaysStackOnTop);
    connect(config, &Config::signalConfigured, this, &View::slotConfigured);

    config->show();
}

void View::showDialog()
{
    m_dialog->show();
}

void View::currentModuleChanged(int index)
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
    ui->actionSelect->setEnabled(false);

    m_step = 1;
    loadStep();
}

void View::slotProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode)
    Q_UNUSED(exitStatus)
}

void View::slotStageChanged(int stage)
{
    if(stage == STAGE_GOOD){
        ui->pushButtonGood->setEnabled(true);
        ui->pushButtonGood->setStyleSheet("background-color: rgb(0, 255, 0);");
        ui->pushButtonBad->setEnabled(false);
        ui->pushButtonBad->setStyleSheet("");
        m_started = false;
    }
    else if(stage == STAGE_BAD){
        ui->pushButtonGood->setEnabled(false);
        ui->pushButtonGood->setStyleSheet("");
        ui->pushButtonBad->setEnabled(true);
        ui->pushButtonBad->setStyleSheet("background-color: rgb(255, 0, 0);");
        m_started = false;

        ui->groupBoxLeft->setTitle("Chẩn đoán và Hướng dẫn khắc phục");
        QFile file(qApp->applicationDirPath() + QString("/data/results/%1.html").arg(m_module));
        if(file.open(QIODevice::ReadOnly | QIODevice::Text))
            ui->textBrowser->setHtml(file.readAll());
    }
    else if(stage == STAGE_IDLE){
        ui->pushButtonGood->setEnabled(false);
        ui->pushButtonGood->setStyleSheet("");
        ui->pushButtonBad->setEnabled(false);
        ui->pushButtonBad->setStyleSheet("");
        m_started = false;
    }
    else{
        m_started = true;
        ui->pushButtonGood->setEnabled(false);
        ui->pushButtonGood->setStyleSheet("");
        ui->pushButtonBad->setEnabled(false);
        ui->pushButtonBad->setStyleSheet("");
        ui->statusbar->clearMessage();
    }
}

void View::slotConfigured()
{
    changeIp();

    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    QThread::sleep(10);
    QApplication::restoreOverrideCursor();

    qDebug() << "Changed IPs";

    emit signalIpChanged();
}

int g_status = 0;
void View::timerEvent(QTimerEvent *)
{
    if(!m_started){
        if(m_udp->armOnline() && m_udp->fpgaOnline()){
            if(ui->stackedWidget->currentIndex()==0)
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
        m_progressBar->setValue(m_tester->stage());
        // m_progressBar->setFormat(QString("%p% - %1").arg(stages[m_udp->stage()]));

//        ui->statusbar->showMessage(stages[m_udp->stage()]);
    }
}

void View::closeEvent(QCloseEvent *event)
{
    int btn = QMessageBox::warning(this,"Đóng ứng dụng", "Bạn có muốn kết thúc kiểm tra?","Đồng ý", "Hủy bỏ");
    if(btn!=0)
        event->ignore();
}

void View::on_actionSelect_triggered()
{
    ui->stackedWidget->setCurrentIndex(0);
    showDialog();
}

void View::on_actionExit_triggered()
{
    this->close();
}

void View::on_actionStart_triggered()
{
    emit signalStartTest(m_module);
    ui->actionStart->setEnabled(false);
    ui->actionReset->setEnabled(true);
    ui->actionStop->setEnabled(true);
    ui->statusbar->clearMessage();

    ui->pushButtonPrev->setEnabled(false);

    ui->stackedWidget->setCurrentIndex(2);
    QString path = qApp->applicationDirPath() +
                   QString("/data/errors/%1/%2.json").arg(m_module).arg(1);
    m_componentMapperWidget->loadProjectFromPath(path);
}

void View::on_actionReset_triggered()
{
    emit signalResetTest();
    ui->actionStart->setEnabled(false);
    ui->actionReset->setEnabled(true);
    ui->actionStop->setEnabled(true);
    ui->statusbar->clearMessage();

    ui->groupBoxLeft->setTitle("Các bước thực hiện");
    QFile file(qApp->applicationDirPath() + QString("/data/steps/%1/3.html").arg(m_module));
    if(file.open(QIODevice::ReadOnly | QIODevice::Text))
        ui->textBrowser->setHtml(file.readAll());
}

void View::on_actionStop_triggered()
{
    emit signalStopTest();
    ui->stackedWidget->setCurrentIndex(0);
    ui->actionSelect->setEnabled(true);

    ui->actionStart->setEnabled(false);
    ui->actionReset->setEnabled(false);
    ui->actionStop->setEnabled(false);
    ui->statusbar->clearMessage();

    ui->groupBoxLeft->setTitle("Các bước thực hiện");
    ui->textBrowser->clear();
}

void View::on_pushButtonPrev_clicked()
{
    if(m_step>1)
        m_step--;
    loadStep();
}

void View::on_pushButtonNext_clicked()
{
    if(m_step<3)
        m_step++;
    loadStep();
}
