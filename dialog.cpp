#include "dialog.h"
#include "ui_dialog.h"
#include <QDebug>
#include <QDialogButtonBox>
#include <QPushButton>

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);

    ui->treeWidget->clear();
    QTreeWidgetItem *itemTopLevel = new QTreeWidgetItem(ui->treeWidget);
    ui->treeWidget->addTopLevelItem(itemTopLevel);
    itemTopLevel->setText(0,"Máy phát");
    itemTopLevel->setData(0,Qt::UserRole,-1);
//    itemTopLevel->setIcon(0,QIcon(":/icons/resources/icons8-hand-right-32.png"));
    QTreeWidgetItem *item=new QTreeWidgetItem(itemTopLevel);
    item->setText(0,"Mô đun Đo công suất");
    item->setData(0,Qt::UserRole,0);
    item->setIcon(0,QIcon(":/icons/resources/icons8-microcontroller-32.png"));

    itemTopLevel = new QTreeWidgetItem(ui->treeWidget);
    ui->treeWidget->addTopLevelItem(itemTopLevel);
    itemTopLevel->setText(0,"Máy thu");
    itemTopLevel->setData(0,Qt::UserRole,-1);
//    itemTopLevel->setIcon(0,QIcon(":/icons/resources/icons8-hand-right-32.png"));
    item=new QTreeWidgetItem(itemTopLevel);
    item->setText(0,"Mô đun Mã hóa-giải mã");
    item->setData(0,Qt::UserRole,1);
    item->setIcon(0,QIcon(":/icons/resources/icons8-microcontroller-32.png"));

    itemTopLevel = new QTreeWidgetItem(ui->treeWidget);
    ui->treeWidget->addTopLevelItem(itemTopLevel);
    itemTopLevel->setText(0,"Điều khiển-kiểm tra");
    itemTopLevel->setData(0,Qt::UserRole,-1);
//    itemTopLevel->setIcon(0,QIcon(":/icons/resources/icons8-hand-right-32.png"));
    item=new QTreeWidgetItem(itemTopLevel);
    item->setText(0,"Mô đun Kiểm tra");
    item->setData(0,Qt::UserRole,2);
    item->setIcon(0,QIcon(":/icons/resources/icons8-microcontroller-32.png"));
    item=new QTreeWidgetItem(itemTopLevel);
    item->setText(0,"Mô đun Phối hợp");
    item->setData(0,Qt::UserRole,3);
    item->setIcon(0,QIcon(":/icons/resources/icons8-microcontroller-32.png"));
    item=new QTreeWidgetItem(itemTopLevel);
    item->setText(0,"Mô đun Xử lý");
    item->setData(0,Qt::UserRole,4);
    item->setIcon(0,QIcon(":/icons/resources/icons8-electronics-48.png"));

    itemTopLevel = new QTreeWidgetItem(ui->treeWidget);
    ui->treeWidget->addTopLevelItem(itemTopLevel);
    itemTopLevel->setText(0,"Mã mật");
    itemTopLevel->setData(0,Qt::UserRole,-1);
//    itemTopLevel->setIcon(0,QIcon(":/icons/resources/icons8-hand-right-32.png"));
    item=new QTreeWidgetItem(itemTopLevel);
    item->setText(0,"Mô đun mã mật");
    item->setData(0,Qt::UserRole,5);
    item->setIcon(0,QIcon(":/icons/resources/icons8-microcontroller-32.png"));

    itemTopLevel = new QTreeWidgetItem(ui->treeWidget);
    ui->treeWidget->addTopLevelItem(itemTopLevel);
    itemTopLevel->setText(0,"Điều khiển quay");
    itemTopLevel->setData(0,Qt::UserRole,-1);
//    itemTopLevel->setIcon(0,QIcon(":/icons/resources/icons8-hand-right-32.png"));
    item=new QTreeWidgetItem(itemTopLevel);
    item->setText(0,"Mô đun điều khiển");
    item->setData(0,Qt::UserRole,6);
    item->setIcon(0,QIcon(":/icons/resources/icons8-arduino-uno-board-32.png"));
    item=new QTreeWidgetItem(itemTopLevel);
    item->setText(0,"Mô đun công suất");
    item->setData(0,Qt::UserRole,7);
    item->setIcon(0,QIcon(":/icons/resources/icons8-arduino-uno-board-32.png"));

    ui->buttonBox->button(QDialogButtonBox::Ok)->setText("Chọn");
    ui->buttonBox->button(QDialogButtonBox::Cancel)->setText("Hủy");
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::on_buttonBox_clicked(QAbstractButton *button)
{
    if(ui->buttonBox->standardButton(button)==QDialogButtonBox::Ok){
        if (m_index>=0)
            emit moduleSelected(m_index);
    }
}

void Dialog::on_treeWidget_clicked(const QModelIndex &index)
{
    int id = index.data(Qt::UserRole).toInt();
    QPushButton *btn = ui->buttonBox->button(QDialogButtonBox::Ok);
    if(id<0){
        btn->setEnabled(false);
    }
    else{
        btn->setEnabled(true);
        m_index = id;
    }
}
