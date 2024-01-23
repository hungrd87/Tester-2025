#ifndef TESTER_H
#define TESTER_H

#include <QMainWindow>
#include <QtWidgets>
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

    void changeIp();
    void loadStep();

public slots:
    void firstConfig();
    void showDialog();
    void currentModuleChanged(int);
    void slotProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void slotStageChanged(int);

signals:
    void signalStartTest(int module);
    void signalResetTest();
    void signalStopTest();
    void signalIpChanged();

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

    void on_pushButtonPrev_clicked();

    void on_pushButtonNext_clicked();

private:
    Ui::Tester *ui;
    QDialog *m_dialog;
    Udp *m_udp;
    QThread *m_thread;
    QProcess *m_process;
    QProgressBar *m_progressBar;
    int m_module, m_last_module, m_step;
    bool m_started;
};
#endif // TESTER_H
