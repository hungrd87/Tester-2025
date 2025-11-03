#ifndef ERRORSETUPDIALOG_H
#define ERRORSETUPDIALOG_H

#include <QDialog>

class QTextEdit;
class QPushButton;
class QLineEdit;
class QComboBox;

class ErrorSetupDialog : public QDialog
{
    Q_OBJECT

public:
    // Mã trả về
    enum DialogCode {
        Accepted = QDialog::Accepted, // 1
        Rejected = QDialog::Rejected, // 0
        Delete = 2                    // Mã tùy chỉnh cho Xóa
    };

    explicit ErrorSetupDialog(QWidget *parent = nullptr);

    QString getTestName() const;
    QString getErrorMessage() const;
    int getTargetListIndex() const;

    void setErrorData(const QString &testName, const QString &msg);
    void setComponentNames(const QStringList &names);
    void setErrorMessage(const QString &msg);
    void setMode(bool isEditMode); // Chuyển sang chế độ Edit (thay nút Hủy)

private slots:
    void onDeleteClicked();

private:
    QLineEdit *m_testNameLineEdit;
    QTextEdit *m_componentsText; // Hiển thị danh sách linh kiện (chỉ đọc)
    QTextEdit *m_errorText;      // Nhập thông tin lỗi
    QComboBox *m_listTargetCombo;
    QPushButton *m_okButton;
    QPushButton *m_cancelButton;
};

#endif // ERRORSETUPDIALOG_H
