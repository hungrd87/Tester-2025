#include "componentdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>

ComponentDialog::ComponentDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Nhập thông tin linh kiện");
    setModal(true); // Đảm bảo nó chặn cửa sổ chính

    m_idLineEdit = new QLineEdit(this);
    m_infoLineEdit = new QLineEdit(this);

    QFormLayout *formLayout = new QFormLayout;
    formLayout->addRow("Linh kiện:", m_idLineEdit);
    formLayout->addRow("Thông tin:", m_infoLineEdit);

    m_okButton = new QPushButton("OK", this);
    m_cancelButton = new QPushButton("Hủy bỏ", this);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_okButton);
    buttonLayout->addWidget(m_cancelButton);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(formLayout);
    mainLayout->addLayout(buttonLayout);

    connect(m_okButton, &QPushButton::clicked, this, &ComponentDialog::accept);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

QString ComponentDialog::getComponentID() const
{
    return m_idLineEdit->text().trimmed();
}

QString ComponentDialog::getInfoMessage() const
{
    return m_infoLineEdit->text().trimmed();
}

void ComponentDialog::setMode(bool isEditMode)
{
    if (isEditMode) {
        setWindowTitle("Chỉnh sửa linh kiện");
        m_cancelButton->setText("Xóa linh kiện");

        // Ngắt kết nối 'reject' và kết nối 'onDeleteClicked'
        disconnect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
        connect(m_cancelButton, &QPushButton::clicked, this, &ComponentDialog::onDeleteClicked);
    }
    // Nếu không, nó giữ nguyên trạng thái "Tạo mới" từ constructor
}

void ComponentDialog::setComponentData(const QString &id, const QString &info)
{
    m_idLineEdit->setText(id);
    m_infoLineEdit->setText(info);
}

void ComponentDialog::setDefaultComponentID(const QString &id, int prefixLength)
{
    m_idLineEdit->setText(id);

    // Đảm bảo con trỏ ở line edit
    m_idLineEdit->setFocus();

    // Bôi đen phần số (từ vị trí 'prefixLength' đến hết)
    if (id.length() > prefixLength) {
        m_idLineEdit->setSelection(prefixLength, id.length() - prefixLength);
    }
}

bool ComponentDialog::validateInput()
{
    if (getComponentID().isEmpty()) {
        QMessageBox::warning(this, "Lỗi đầu vào", "Tên định danh không được để trống.");
        m_idLineEdit->setFocus();
        return false;
    }
    return true;
}

void ComponentDialog::accept()
{
    // Chỉ accept (và đóng dialog) nếu đầu vào hợp lệ
    if (validateInput()) {
        QDialog::accept(); // Gọi hàm accept() của lớp cha
    }
}

void ComponentDialog::onDeleteClicked()
{
    // Trả về mã tùy chỉnh
    done(ComponentDialog::Delete);
}
