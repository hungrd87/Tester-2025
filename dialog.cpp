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
    QTreeWidgetItem *item=new QTreeWidgetItem(itemTopLevel);
    item->setText(0,"Mô đun Đo công suất");
    item->setData(0,Qt::UserRole,0);

    itemTopLevel = new QTreeWidgetItem(ui->treeWidget);
    ui->treeWidget->addTopLevelItem(itemTopLevel);
    itemTopLevel->setText(0,"Máy thu");
    itemTopLevel->setData(0,Qt::UserRole,-1);
    item=new QTreeWidgetItem(itemTopLevel);
    item->setText(0,"Mô đun Mã hóa-giải mã");
    item->setData(0,Qt::UserRole,1);

    itemTopLevel = new QTreeWidgetItem(ui->treeWidget);
    ui->treeWidget->addTopLevelItem(itemTopLevel);
    itemTopLevel->setText(0,"Điều khiển-kiểm tra");
    itemTopLevel->setData(0,Qt::UserRole,-1);
    item=new QTreeWidgetItem(itemTopLevel);
    item->setText(0,"Mô đun Kiểm tra");
    item->setData(0,Qt::UserRole,2);
    item=new QTreeWidgetItem(itemTopLevel);
    item->setText(0,"Mô đun Phối hợp");
    item->setData(0,Qt::UserRole,3);
    item=new QTreeWidgetItem(itemTopLevel);
    item->setText(0,"Mô đun Xử lý");
    item->setData(0,Qt::UserRole,4);

    itemTopLevel = new QTreeWidgetItem(ui->treeWidget);
    ui->treeWidget->addTopLevelItem(itemTopLevel);
    itemTopLevel->setText(0,"Mã mật");
    itemTopLevel->setData(0,Qt::UserRole,-1);
    item=new QTreeWidgetItem(itemTopLevel);
    item->setText(0,"Mô đun mã mật");
    item->setData(0,Qt::UserRole,5);

    itemTopLevel = new QTreeWidgetItem(ui->treeWidget);
    ui->treeWidget->addTopLevelItem(itemTopLevel);
    itemTopLevel->setText(0,"Điều khiển quay");
    itemTopLevel->setData(0,Qt::UserRole,-1);
    item=new QTreeWidgetItem(itemTopLevel);
    item->setText(0,"Mô đun điều khiển");
    item->setData(0,Qt::UserRole,6);
    item=new QTreeWidgetItem(itemTopLevel);
    item->setText(0,"Mô đun công suất");
    item->setData(0,Qt::UserRole,7);
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::on_buttonBox_clicked(QAbstractButton *button)
{
    if(ui->buttonBox->standardButton(button)==QDialogButtonBox::Ok){
        if (m_index>0)
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
