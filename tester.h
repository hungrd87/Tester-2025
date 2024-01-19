#ifndef TESTER_H
#define TESTER_H

#include <QMainWindow>
#include <QProcess>
#include "dialog.h"
#include "udp.h"
#include "config.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Tester; }
QT_END_NAMESPACE

class Tester : public QMainWindow
{
    Q_OBJECT

public:
    Tester(QWidget *parent = nullptr);
    ~Tester();    

public slots:
    void stateChanged(QProcess::ProcessState newState);
    void firstConfig();
    void showDialog();
    void currentModuleChanged(int);

signals:
    void signalStartTest(int module);
    void signalResetTest();
    void signalStopTest();

protected:
    void timerEvent(QTimerEvent *) override;

private slots:
    void on_actionSelect_triggered();

    void on_actionExit_triggered();

    void on_actionStart_triggered();

    void on_actionReset_triggered();

    void on_actionStop_triggered();

    void on_actionContents_triggered();

    void on_actionAbout_triggered();

private:
    Ui::Tester *ui;
    QDialog *m_dialog;
    Udp *m_udp;
    QThread *m_thread;
    int m_module;
};
#endif // TESTER_H
