#include "tester.h"
#include "ui_tester.h"
#include <QTimer>
#include <QDebug>
#include <QThread>

Tester::Tester(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Tester)
    , m_dialog(nullptr), m_module(-1)
{
    ui->setupUi(this);

    //ToolBar

    //Class
    m_udp = new Udp;

    m_thread = new QThread(this);
    m_thread->setObjectName("TesterThread");
    m_udp->moveToThread(m_thread);
    connect(m_thread, &QThread::finished, m_udp, &QObject::deleteLater);
    m_thread->start();

    //Connections
    connect(this, &Tester::signalStartTest, m_udp, &Udp::slotStartTest);

    //Timer
    QTimer::singleShot(1000, this, SLOT(showDialog()));
}

Tester::~Tester()
{
    m_thread->quit();
    m_thread->wait();

    delete ui;
}

void Tester::showDialog()
{
    if(m_dialog==nullptr){
        m_dialog = new Dialog();
        connect(m_dialog, SIGNAL(moduleSelected(int)), this, SLOT(currentModuleChanged(int)));
    }
    m_dialog->show();

}

void Tester::currentModuleChanged(int index)
{
    m_module = index;
    qDebug() <<  index;
}

void Tester::on_pushButton_clicked()
{
    emit signalStartTest(m_module);
}

void Tester::on_actionSelect_triggered()
{
    showDialog();
}
