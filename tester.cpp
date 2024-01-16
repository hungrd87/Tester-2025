#include "tester.h"
#include "ui_tester.h"

Tester::Tester(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Tester)
{
    ui->setupUi(this);
}

Tester::~Tester()
{
    delete ui;
}

