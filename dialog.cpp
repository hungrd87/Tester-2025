#include "dialog.h"
#include "ui_dialog.h"
#include <QDebug>
#include <QDialogButtonBox>
#include <QPushButton>
#include <functional>

/**
 * @brief Thêm một item cấp cao nhất (top-level) vào QTreeWidget và cấu hình nó bằng lambda.
 * @param parentTree Widget QTreeWidget cha.
 * @param configFunc Lambda (hoặc std::function) nhận vào QTreeWidgetItem* để cấu hình.
 * @return Con trỏ tới item vừa được tạo.
 */
QTreeWidgetItem* addTopLevelItem(QTreeWidget* parentTree,
                                 std::function<void(QTreeWidgetItem*)> configFunc)
{
    QTreeWidgetItem* item = new QTreeWidgetItem(parentTree);
    if (configFunc) {
        configFunc(item);
    }
    return item;
}

/**
 * @brief Thêm một item con vào một QTreeWidgetItem cha và cấu hình nó bằng lambda.
 * @param parentItem Item QTreeWidgetItem cha.
 * @param configFunc Lambda (hoặc std::function) nhận vào QTreeWidgetItem* để cấu hình.
 * @return Con trỏ tới item vừa được tạo.
 */
QTreeWidgetItem* addChildItem(QTreeWidgetItem* parentItem,
                              std::function<void(QTreeWidgetItem*)> configFunc)
{
    QTreeWidgetItem* item = new QTreeWidgetItem(parentItem);
    if (configFunc) {
        configFunc(item);
    }
    return item;
}

// Định nghĩa trước một số Role để code dễ đọc hơn
const int ModuleRole = Qt::UserRole + 1;

Dialog::Dialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);

//     ui->treeWidget->clear();
//     QTreeWidgetItem *itemTopLevel = new QTreeWidgetItem(ui->treeWidget);
//     ui->treeWidget->addTopLevelItem(itemTopLevel);
//     itemTopLevel->setText(0,"Máy phát");
//     itemTopLevel->setData(0,Qt::UserRole,-1);
// //    itemTopLevel->setIcon(0,QIcon(":/icons/resources/icons8-hand-right-32.png"));
//     QTreeWidgetItem *item=new QTreeWidgetItem(itemTopLevel);
//     item->setText(0,"Mô đun Đo công suất");
//     item->setData(0,Qt::UserRole,0);
//     item->setIcon(0,QIcon(":/icons/resources/icons8-microcontroller-32.png"));

//     itemTopLevel = new QTreeWidgetItem(ui->treeWidget);
//     ui->treeWidget->addTopLevelItem(itemTopLevel);
//     itemTopLevel->setText(0,"Máy thu");
//     itemTopLevel->setData(0,Qt::UserRole,-1);
// //    itemTopLevel->setIcon(0,QIcon(":/icons/resources/icons8-hand-right-32.png"));
//     item=new QTreeWidgetItem(itemTopLevel);
//     item->setText(0,"Mô đun Mã hóa-giải mã");
//     item->setData(0,Qt::UserRole,1);
//     item->setIcon(0,QIcon(":/icons/resources/icons8-microcontroller-32.png"));

//     itemTopLevel = new QTreeWidgetItem(ui->treeWidget);
//     ui->treeWidget->addTopLevelItem(itemTopLevel);
//     itemTopLevel->setText(0,"Điều khiển-kiểm tra");
//     itemTopLevel->setData(0,Qt::UserRole,-1);
// //    itemTopLevel->setIcon(0,QIcon(":/icons/resources/icons8-hand-right-32.png"));
//     item=new QTreeWidgetItem(itemTopLevel);
//     item->setText(0,"Mô đun Kiểm tra");
//     item->setData(0,Qt::UserRole,2);
//     item->setIcon(0,QIcon(":/icons/resources/icons8-microcontroller-32.png"));
//     item=new QTreeWidgetItem(itemTopLevel);
//     item->setText(0,"Mô đun Phối hợp");
//     item->setData(0,Qt::UserRole,3);
//     item->setIcon(0,QIcon(":/icons/resources/icons8-microcontroller-32.png"));
//     item=new QTreeWidgetItem(itemTopLevel);
//     item->setText(0,"Mô đun Xử lý");
//     item->setData(0,Qt::UserRole,4);
//     item->setIcon(0,QIcon(":/icons/resources/icons8-electronics-48.png"));

//     itemTopLevel = new QTreeWidgetItem(ui->treeWidget);
//     ui->treeWidget->addTopLevelItem(itemTopLevel);
//     itemTopLevel->setText(0,"Mã mật");
//     itemTopLevel->setData(0,Qt::UserRole,-1);
// //    itemTopLevel->setIcon(0,QIcon(":/icons/resources/icons8-hand-right-32.png"));
//     item=new QTreeWidgetItem(itemTopLevel);
//     item->setText(0,"Mô đun mã mật");
//     item->setData(0,Qt::UserRole,5);
//     item->setIcon(0,QIcon(":/icons/resources/icons8-microcontroller-32.png"));

//     itemTopLevel = new QTreeWidgetItem(ui->treeWidget);
//     ui->treeWidget->addTopLevelItem(itemTopLevel);
//     itemTopLevel->setText(0,"Điều khiển quay");
//     itemTopLevel->setData(0,Qt::UserRole,-1);
// //    itemTopLevel->setIcon(0,QIcon(":/icons/resources/icons8-hand-right-32.png"));
//     item=new QTreeWidgetItem(itemTopLevel);
//     item->setText(0,"Mô đun điều khiển");
//     item->setData(0,Qt::UserRole,6);
//     item->setIcon(0,QIcon(":/icons/resources/icons8-arduino-uno-board-32.png"));
//     item=new QTreeWidgetItem(itemTopLevel);
//     item->setText(0,"Mô đun công suất");
//     item->setData(0,Qt::UserRole,7);
//     item->setIcon(0,QIcon(":/icons/resources/icons8-arduino-uno-board-32.png"));

    ui->treeWidget->clear();
    ui->treeWidget->setColumnCount(1);
    ui->treeWidget->setHeaderLabel("Lựa chọn mô đun kiểm tra");

    // ----- Top Level 1: Máy phát -----
    auto* top1 = addTopLevelItem(ui->treeWidget, [&](QTreeWidgetItem* item) {
        item->setText(0, "Máy phát");
    });

    addChildItem(top1, [&](QTreeWidgetItem* item) {
        item->setText(0, "Mô đun Đo công suất (Thế hệ 1)");
        item->setData(0, ModuleRole, 0);
        item->setIcon(0,QIcon(":/icons/resources/icons8-microcontroller-32.png"));
    });
    addChildItem(top1, [&](QTreeWidgetItem* item) {
        item->setText(0, "Mô đun Đo công suất (Thế hệ 2)");
        item->setData(0, ModuleRole, 0);
        item->setIcon(0,QIcon(":/icons/resources/icons8-microcontroller-32.png"));
    });
    addChildItem(top1, [&](QTreeWidgetItem* item) {
        item->setText(0, "Mô đun Đo công suất (Thế hệ 3)");
        item->setData(0, ModuleRole, 0);
        item->setIcon(0,QIcon(":/icons/resources/icons8-microcontroller-32.png"));
    });

    // ----- Top Level 1: Máy thu -----
    auto* top2 = addTopLevelItem(ui->treeWidget, [&](QTreeWidgetItem* item) {
        item->setText(0, "Máy thu");
    });

    addChildItem(top2, [&](QTreeWidgetItem* item) {
        item->setText(0, "Mô đun Mã hóa-giải mã (Thế hệ 1)");
        item->setData(0, ModuleRole, 1);
        item->setIcon(0,QIcon(":/icons/resources/icons8-microcontroller-32.png"));
    });
    addChildItem(top2, [&](QTreeWidgetItem* item) {
        item->setText(0, "Mô đun Mã hóa-giải mã (Thế hệ 2)");
        item->setData(0, ModuleRole, 1);
        item->setIcon(0,QIcon(":/icons/resources/icons8-microcontroller-32.png"));
    });
    addChildItem(top2, [&](QTreeWidgetItem* item) {
        item->setText(0, "Mô đun Mã hóa-giải mã (Thế hệ 3)");
        item->setData(0, ModuleRole, 1);
        item->setIcon(0,QIcon(":/icons/resources/icons8-microcontroller-32.png"));
    });

    // ----- Top Level 3: Điều khiển-kiểm tra -----
    auto* top3 = addTopLevelItem(ui->treeWidget, [&](QTreeWidgetItem* item) {
        item->setText(0, "Điều khiển-kiểm tra");
    });

    addChildItem(top3, [&](QTreeWidgetItem* item) {
        item->setText(0, "Mô đun Kiểm tra (Thế hệ 1)");
        item->setData(0, ModuleRole, 2);
        item->setIcon(0,QIcon(":/icons/resources/icons8-microcontroller-32.png"));
    });
    addChildItem(top3, [&](QTreeWidgetItem* item) {
        item->setText(0, "Mô đun Kiểm tra (Thế hệ 2)");
        item->setData(0, ModuleRole, 2);
        item->setIcon(0,QIcon(":/icons/resources/icons8-microcontroller-32.png"));
    });
    addChildItem(top3, [&](QTreeWidgetItem* item) {
        item->setText(0, "Mô đun Kiểm tra (Thế hệ 3)");
        item->setData(0, ModuleRole, 2);
        item->setIcon(0,QIcon(":/icons/resources/icons8-microcontroller-32.png"));
    });
    addChildItem(top3, [&](QTreeWidgetItem* item) {
        item->setText(0, "Mô đun Phối hợp (Thế hệ 1)");
        item->setData(0, ModuleRole, 3);
        item->setIcon(0,QIcon(":/icons/resources/icons8-microcontroller-32.png"));
    });
    addChildItem(top3, [&](QTreeWidgetItem* item) {
        item->setText(0, "Mô đun Phối hợp (Thế hệ 2)");
        item->setData(0, ModuleRole, 3);
        item->setIcon(0,QIcon(":/icons/resources/icons8-microcontroller-32.png"));
    });
    addChildItem(top3, [&](QTreeWidgetItem* item) {
        item->setText(0, "Mô đun Phối hợp (Thế hệ 3)");
        item->setData(0, ModuleRole, 3);
        item->setIcon(0,QIcon(":/icons/resources/icons8-microcontroller-32.png"));
    });
    addChildItem(top3, [&](QTreeWidgetItem* item) {
        item->setText(0, "Mô đun Xử lý (Thế hệ 1)");
        item->setData(0, ModuleRole, 4);
        item->setIcon(0,QIcon(":/icons/resources/icons8-electronics-48.png"));
    });
    addChildItem(top3, [&](QTreeWidgetItem* item) {
        item->setText(0, "Mô đun Xử lý (Thế hệ 2)");
        item->setData(0, ModuleRole, 4);
        item->setIcon(0,QIcon(":/icons/resources/icons8-electronics-48.png"));
    });
    addChildItem(top3, [&](QTreeWidgetItem* item) {
        item->setText(0, "Mô đun Xử lý (Thế hệ 3)");
        item->setData(0, ModuleRole, 4);
        item->setIcon(0,QIcon(":/icons/resources/icons8-electronics-48.png"));
    });

    // ----- Top Level 1: Mã mật -----
    auto* top4 = addTopLevelItem(ui->treeWidget, [&](QTreeWidgetItem* item) {
        item->setText(0, "Mã mật");
    });

    addChildItem(top4, [&](QTreeWidgetItem* item) {
        item->setText(0, "Mô đun Mã mật (Thế hệ 1)");
        item->setData(0, ModuleRole, 5);
        item->setIcon(0,QIcon(":/icons/resources/icons8-microcontroller-32.png"));
    });
    addChildItem(top4, [&](QTreeWidgetItem* item) {
        item->setText(0, "Mô đun Mã mật (Thế hệ 2)");
        item->setData(0, ModuleRole, 5);
        item->setIcon(0,QIcon(":/icons/resources/icons8-microcontroller-32.png"));
    });
    addChildItem(top4, [&](QTreeWidgetItem* item) {
        item->setText(0, "Mô đun Mã mật (Thế hệ 3)");
        item->setData(0, ModuleRole, 5);
        item->setIcon(0,QIcon(":/icons/resources/icons8-microcontroller-32.png"));
    });

    // ----- Top Level 1: Điều khiển quay -----
    auto* top5 = addTopLevelItem(ui->treeWidget, [&](QTreeWidgetItem* item) {
        item->setText(0, "Điều khiển quay");
    });

    addChildItem(top5, [&](QTreeWidgetItem* item) {
        item->setText(0, "Mô đun điều khiển (Thế hệ 1)");
        item->setData(0, ModuleRole, 6);
        item->setIcon(0,QIcon(":/icons/resources/icons8-arduino-uno-board-32.png"));
    });
    addChildItem(top5, [&](QTreeWidgetItem* item) {
        item->setText(0, "Mô đun điều khiển (Thế hệ 2)");
        item->setData(0, ModuleRole, 6);
        item->setIcon(0,QIcon(":/icons/resources/icons8-arduino-uno-board-32.png"));
    });
    addChildItem(top5, [&](QTreeWidgetItem* item) {
        item->setText(0, "Mô đun điều khiển (Thế hệ 3)");
        item->setData(0, ModuleRole, 6);
        item->setIcon(0,QIcon(":/icons/resources/icons8-arduino-uno-board-32.png"));
    });
    addChildItem(top5, [&](QTreeWidgetItem* item) {
        item->setText(0, "Mô đun Công suất (Thế hệ 1)");
        item->setData(0, ModuleRole, 7);
        item->setIcon(0,QIcon(":/icons/resources/icons8-arduino-uno-board-32.png"));
    });
    addChildItem(top5, [&](QTreeWidgetItem* item) {
        item->setText(0, "Mô đun Công suất (Thế hệ 2)");
        item->setData(0, ModuleRole, 7);
        item->setIcon(0,QIcon(":/icons/resources/icons8-arduino-uno-board-32.png"));
    });
    addChildItem(top5, [&](QTreeWidgetItem* item) {
        item->setText(0, "Mô đun Công suất (Thế hệ 3)");
        item->setData(0, ModuleRole, 7);
        item->setIcon(0,QIcon(":/icons/resources/icons8-arduino-uno-board-32.png"));
    });


    ui->labelLogo->setPixmap(QPixmap(qApp->applicationDirPath() + QString("/data/images/logo.png")));

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
    int id = index.data(ModuleRole).toInt()/3;
    QPushButton *btn = ui->buttonBox->button(QDialogButtonBox::Ok);
    if(id<0){
        btn->setEnabled(false);
    }
    else{
        btn->setEnabled(true);
        m_index = id;
    }
}
