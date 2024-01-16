#ifndef TESTER_H
#define TESTER_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class Tester; }
QT_END_NAMESPACE

class Tester : public QMainWindow
{
    Q_OBJECT

public:
    Tester(QWidget *parent = nullptr);
    ~Tester();

private:
    Ui::Tester *ui;
};
#endif // TESTER_H
