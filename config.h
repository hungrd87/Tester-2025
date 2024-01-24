#ifndef CONFIG_H
#define CONFIG_H

#include <QDialog>
#include <QAbstractButton>

namespace Ui {
class Config;
}

class Config : public QDialog
{
    Q_OBJECT

public:
    explicit Config(QWidget *parent = nullptr);
    ~Config();

signals:
    void signalConfigured();

private slots:
    void on_buttonBox_clicked(QAbstractButton *button);

private:
    Ui::Config *ui;
};

#endif // CONFIG_H
