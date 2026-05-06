#include "testlogwidget.h"
#include <QVBoxLayout>
#include <QTreeView>
#include <QStandardItemModel>
#include <QHeaderView>
#include <QApplication>
#include <QStyle>
#include <QLabel>
#include <QProgressBar>
#include <QTextEdit>

TestLogWidget::TestLogWidget(QWidget *parent) : QWidget(parent)
{
    // Tải các icon chuẩn của Qt
    m_iconRunning = QApplication::style()->standardIcon(QStyle::SP_ArrowRight);
    m_iconPass = QApplication::style()->standardIcon(QStyle::SP_DialogApplyButton);
    m_iconFail = QApplication::style()->standardIcon(QStyle::SP_DialogCancelButton);

    setupUi();
}

void TestLogWidget::setupUi()
{
    m_model = new QStandardItemModel(0, 3, this); // 0 hàng, 3 cột
    m_model->setHorizontalHeaderLabels({"Thứ tự", "Nội dung kiểm tra", "Kết quả"});

    m_view = new QTreeView(this);
    m_view->setModel(m_model);
    m_view->setEditTriggers(QAbstractItemView::NoEditTriggers); // Không cho sửa
    m_view->setRootIsDecorated(false); // Hiển thị dạng bảng (flat)
    m_view->setAlternatingRowColors(true); // Dễ nhìn
    m_view->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents); // Cột 0 (Thứ tự) co lại
    m_view->header()->setSectionResizeMode(1, QHeaderView::Stretch); // Cột 1 (Nội dung)
    m_view->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents); // Cột 2 (Kết quả) co lại

    m_errorDetailEdit = new QTextEdit(this);
    m_errorDetailEdit->setReadOnly(true);
    // m_errorDetailEdit->setMaximumHeight(60); // Giới hạn chiều cao
    m_errorDetailEdit->setPlaceholderText("Chi tiết kết quả sẽ hiện ở đây...");

    // === THÊM MỚI: ProgressBar ===
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(true);
    // m_progressBar->setFormat("%p%");
    m_progressBar->setAlignment(Qt::AlignCenter);
    m_progressBar->setFormat("Sẵn sàng");

    QVBoxLayout *layout = new QVBoxLayout(this);
    // layout->addWidget(new QLabel("Tiến trình kiểm tra:"));
    layout->addWidget(m_view, 1); // <<< Thêm stretch factor
    layout->addWidget(m_errorDetailEdit, 1);
    layout->addWidget(m_progressBar); // <<< Thêm progress bar
    layout->setContentsMargins(0, 0, 0, 0);

    connect(m_view, &QTreeView::clicked, this, &TestLogWidget::onLogItemClicked);
}

void TestLogWidget::clearLog()
{
    // m_model->clear() sẽ xóa cả header
    m_model->removeRows(0, m_model->rowCount());
    m_errorDetailEdit->clear();
    resetTestProgress();
}

int TestLogWidget::addTestStep(const QString &stepName, const QVariantMap &highlightData)
{
    // === THAY ĐỔI: Logic tạo cột ===

    // 1. Lấy số thứ tự (1-based)
    int newRow = m_model->rowCount();
    QString orderNumber = QString::number(newRow + 1);

    // Cột 0: Thứ tự (Không có icon, lưu data ẩn ở đây)
    QStandardItem *itemOrder = new QStandardItem(orderNumber);
    itemOrder->setData(highlightData, LogDataRole::HighlightDataRole);
    itemOrder->setData("Đang kiểm tra...", LogDataRole::ResultDetailRole);

    // Cột 1: Nội dung
    QStandardItem *itemName = new QStandardItem(stepName);

    // Cột 2: Kết quả (Chuyển icon "Running" và text "..." sang đây)
    QStandardItem *itemResult = new QStandardItem(m_iconRunning, "...");

    // ==============================

    QList<QStandardItem*> rowItems = {itemOrder, itemName, itemResult};

    for(QStandardItem* item : rowItems) {
        item->setEditable(false);
    }

    m_model->appendRow(rowItems);

    m_view->scrollToBottom();
    return newRow; // Trả về index (0-based)
}

void TestLogWidget::updateTestStep(int row, bool passed, const QString &resultMessage)
{
    if (row < 0 || row >= m_model->rowCount()) return;

    // === THAY ĐỔI: Logic cập nhật cột ===

    // Cột 0: Thứ tự (Chứa data)
    QStandardItem *itemOrder = m_model->item(row, 0);

    // Cột 2: Kết quả (Cần cập nhật icon + text)
    QStandardItem *itemResult = m_model->item(row, 2);

    if (passed) {
        // Cập nhật Cột 2 (Kết quả)
        itemResult->setIcon(m_iconPass);
        itemResult->setText("Tốt");

        // Cập nhật data (ẩn trong Cột 0)
        itemOrder->setData("Tốt", LogDataRole::ResultDetailRole);
    } else {
        // Cập nhật Cột 2 (Kết quả)
        itemResult->setIcon(m_iconFail);
        itemResult->setText("Lỗi");

        // Cập nhật data (ẩn trong Cột 0)
        itemOrder->setData(resultMessage, LogDataRole::ResultDetailRole);
    }
    // ===================================
}

void TestLogWidget::setTestProgress(int value)
{
    m_progressBar->setValue(value);
}

void TestLogWidget::setTestProgressText(const QString &text)
{
    m_progressBar->setFormat(text);
}

void TestLogWidget::resetTestProgress()
{
    m_progressBar->setValue(0);
    m_progressBar->setFormat("Sẵn sàng");
}

void TestLogWidget::onLogItemClicked(const QModelIndex &index)
{
    // Cột 0: Thứ tự (Chứa data)
    QStandardItem *itemOrder = m_model->item(index.row(), 0);
    if (!itemOrder) return;

    // Cột 2: Kết quả (Để kiểm tra Tốt/Lỗi)
    QStandardItem *itemResult = m_model->item(index.row(), 2);
    if (!itemResult) return;

    // 1. Hiển thị chi tiết (Không đổi)
    QString detail = itemOrder->data(LogDataRole::ResultDetailRole).toString();
    m_errorDetailEdit->setText(detail);

    // 2. Lấy dữ liệu highlight
    QVariantMap data = itemOrder->data(LogDataRole::HighlightDataRole).toMap();

    // === THAY ĐỔI: Logic phát tín hiệu ===

    // Chỉ phát tín hiệu highlight nếu kết quả là "Lỗi"
    if (itemResult->text() == "Lỗi")
    {
        if (!data.isEmpty()) {
            emit itemSelected(data); // Gửi data để highlight
        }
    }
    else
    {
        // Nếu là "Tốt" hoặc "..." (Đang chạy),
        // chúng ta gửi tín hiệu RỖNG để xóa highlight
        emit itemSelected(QVariantMap{});
    }
    // ===================================
}
