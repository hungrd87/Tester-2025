#ifndef COMPONENTDIALOG_H
#define COMPONENTDIALOG_H

#include <QDialog>

class QLineEdit;
class QPushButton;

class ComponentDialog : public QDialog
{
    Q_OBJECT

public:
    // Enum để xác định kết quả trả về của dialog
    enum DialogCode {
        Accepted = QDialog::Accepted, // 1
        Rejected = QDialog::Rejected, // 0
        Delete = 2                    // Mã tùy chỉnh cho Xóa
    };

    explicit ComponentDialog(QWidget *parent = nullptr);

    QString getComponentID() const;
    QString getInfoMessage() const;

    // Cài đặt dialog cho chế độ Edit
    void setMode(bool isEditMode);
    // Đặt dữ liệu ban đầu cho dialog
    void setComponentData(const QString &id, const QString &info);
    void setDefaultComponentID(const QString &id, int prefixLength);

public slots:
    void accept() override;

private slots:
    // Slot tùy chỉnh cho nút "Xóa linh kiện"
    void onDeleteClicked();

private:
    bool validateInput(); // Hàm kiểm tra đầu vào

    QLineEdit *m_idLineEdit;
    QLineEdit *m_infoLineEdit;
    QPushButton *m_okButton;
    QPushButton *m_cancelButton;
};

#endif // COMPONENTDIALOG_H
