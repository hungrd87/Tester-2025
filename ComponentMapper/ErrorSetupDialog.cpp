#include "ErrorSetupDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextEdit>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>

ErrorSetupDialog::ErrorSetupDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("Thiết lập lỗi");
    setModal(true);
    setMinimumSize(400, 300);

    m_componentsText = new QTextEdit(this);
    m_componentsText->setReadOnly(true); // Chỉ đọc

    m_testNameLineEdit = new QLineEdit(this);
    m_testNameLineEdit->setText("Kiểm tra ");

    m_errorText = new QTextEdit(this);

    m_listTargetCombo = new QComboBox(this);
    m_listTargetCombo->addItem("Danh sách lỗi 1");
    m_listTargetCombo->addItem("Danh sách lỗi 2");

    m_okButton = new QPushButton("OK", this);
    m_cancelButton = new QPushButton("Hủy bỏ", this);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_okButton);
    buttonLayout->addWidget(m_cancelButton);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(new QLabel("Các linh kiện được áp dụng:"));
    mainLayout->addWidget(m_componentsText, 1); // 1 stretch
    mainLayout->addWidget(new QLabel("Tên bước kiểm tra:"));
    mainLayout->addWidget(m_testNameLineEdit);
    mainLayout->addWidget(new QLabel("Thông tin lỗi:"));
    mainLayout->addWidget(m_errorText, 1); // 1 stretch
    mainLayout->addWidget(new QLabel("Lưu vào:"));
    mainLayout->addWidget(m_listTargetCombo);
    mainLayout->addLayout(buttonLayout);

    connect(m_okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

QString ErrorSetupDialog::getTestName() const // <<< THÊM
{
    return m_testNameLineEdit->text().trimmed();
}

QString ErrorSetupDialog::getErrorMessage() const
{
    return m_errorText->toPlainText().trimmed();
}

int ErrorSetupDialog::getTargetListIndex() const
{
    return m_listTargetCombo->currentIndex();
}

void ErrorSetupDialog::setErrorData(const QString &testName, const QString &msg)
{
    m_testNameLineEdit->setText(testName);
    m_errorText->setText(msg);
}

void ErrorSetupDialog::setComponentNames(const QStringList &names)
{
    // Hiển thị mỗi tên trên một dòng
    m_componentsText->setText(names.join("\n"));

    QString currentName = m_testNameLineEdit->text(); // "Kiểm tra "
    QString newName = QString("%1[%2]").arg(currentName).arg(names.join(", "));
    m_testNameLineEdit->setText(newName);

    // Bôi đen phần tên linh kiện
    m_testNameLineEdit->setFocus();
    m_testNameLineEdit->setSelection(currentName.length(), newName.length() - currentName.length());
}

void ErrorSetupDialog::setErrorMessage(const QString &msg)
{
    m_errorText->setText(msg);
}

void ErrorSetupDialog::setMode(bool isEditMode)
{
    if (isEditMode) {
        setWindowTitle("Chỉnh sửa lỗi");
        m_cancelButton->setText("Xóa lỗi");

        disconnect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
        connect(m_cancelButton, &QPushButton::clicked, this, &ErrorSetupDialog::onDeleteClicked);
    }
}

void ErrorSetupDialog::onDeleteClicked()
{
    // Trả về mã tùy chỉnh
    done(ErrorSetupDialog::Delete);
}
