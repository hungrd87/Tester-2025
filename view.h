#ifndef VIEW_H
#define VIEW_H

#include <QMainWindow>
#include <QtWidgets>
#include <QProcess>
#include "dialog.h"
#include "udp.h"
#include "config.h"
#include "tester.h"
#include "ComponentMapperWidget.h"

QT_BEGIN_NAMESPACE
namespace Ui { class View; }
QT_END_NAMESPACE

class View : public QMainWindow
{
    Q_OBJECT

public:
    View(QWidget *parent = nullptr);
    ~View();

    void changeIp();
    void loadStep();

public slots:
    void firstConfig();
    void showDialog();
    void currentModuleChanged(int);
    void slotProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void slotStageChanged(int);
    void slotConfigured();

signals:
    void signalStartTest(int module);
    void signalResetTest();
    void signalStopTest();
    void signalIpChanged();

protected:
    void timerEvent(QTimerEvent *) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    void on_actionSelect_triggered();

    void on_actionExit_triggered();

    void on_actionStart_triggered();

    void on_actionReset_triggered();

    void on_actionStop_triggered();

    void on_pushButtonPrev_clicked();

    void on_pushButtonNext_clicked();

private:
    Ui::View *ui;
    QDialog *m_dialog;
    Udp *m_udp;
    Tester *m_tester;
    QThread *m_thread;
    QProcess *m_process;
    QProgressBar *m_progressBar;
    ComponentMapperWidget *m_componentMapperWidget;
    int m_module, m_last_module, m_step;
    bool m_started;
};
#endif // VIEW_H
