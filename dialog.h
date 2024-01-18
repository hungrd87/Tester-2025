#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>
#include <QAbstractButton>
#include <QTreeWidgetItem>

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

public:
    explicit Dialog(QWidget *parent = nullptr);
    ~Dialog();

signals:
    void moduleSelected(int index);

private slots:
    void on_buttonBox_clicked(QAbstractButton *button);

    void on_treeWidget_clicked(const QModelIndex &index);

private:
    Ui::Dialog *ui;
    int m_index;
};

#endif // DIALOG_H
