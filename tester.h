#ifndef TESTER_H
#define TESTER_H

#include <QMainWindow>
#include "dialog.h"
#include "udp.h"

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
    void showDialog();
    void currentModuleChanged(int);

signals:
    void signalStartTest(int module);

private slots:
    void on_pushButton_clicked();

    void on_actionSelect_triggered();

private:
    Ui::Tester *ui;
    QDialog *m_dialog;
    Udp *m_udp;
    QThread *m_thread;
    int m_module;
};
#endif // TESTER_H
