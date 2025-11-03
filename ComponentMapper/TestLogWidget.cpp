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
    m_model->setHorizontalHeaderLabels({"Trạng thái", "Bước kiểm tra", "Kết quả"});

    m_view = new QTreeView(this);
    m_view->setModel(m_model);
    m_view->setEditTriggers(QAbstractItemView::NoEditTriggers); // Không cho sửa
    m_view->setRootIsDecorated(false); // Hiển thị dạng bảng (flat)
    m_view->setAlternatingRowColors(true); // Dễ nhìn
    m_view->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_view->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_view->header()->setSectionResizeMode(2, QHeaderView::Stretch);

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
    QStandardItem *itemStatus = new QStandardItem(m_iconRunning, "Đang chạy");
    QStandardItem *itemName = new QStandardItem(stepName);
    QStandardItem *itemResult = new QStandardItem("...");

    // Lưu dữ liệu highlight vào item đầu tiên (cột 0)
    itemStatus->setData(highlightData, LogDataRole::HighlightDataRole);
    itemStatus->setData("Đang kiểm tra...", LogDataRole::ResultDetailRole);

    QList<QStandardItem*> rowItems = {itemStatus, itemName, itemResult};

    // Đảm bảo không item nào có thể bị sửa
    for(QStandardItem* item : rowItems) {
        item->setEditable(false);
    }

    m_model->appendRow(rowItems);

    int newRow = m_model->rowCount() - 1;
    m_view->scrollToBottom(); // Tự cuộn xuống
    return newRow;
}

void TestLogWidget::updateTestStep(int row, bool passed, const QString &resultMessage)
{
    if (row < 0 || row >= m_model->rowCount()) return;

    QStandardItem *itemStatus = m_model->item(row, 0);
    QStandardItem *itemResult = m_model->item(row, 2);

    if (passed) {
        itemStatus->setIcon(m_iconPass);
        itemStatus->setText("Tốt");
        itemResult->setText("Tốt");
        itemStatus->setData("Tốt", LogDataRole::ResultDetailRole);
    } else {
        itemStatus->setIcon(m_iconFail);
        itemStatus->setText("Hỏng");
        itemResult->setText("Lỗi");
        itemStatus->setData(resultMessage, LogDataRole::ResultDetailRole);
    }
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
    // Lấy item ở cột 0 của hàng được click
    QStandardItem *itemStatus = m_model->item(index.row(), 0);
    if (!itemStatus) return;

    QVariantMap data = itemStatus->data(LogDataRole::HighlightDataRole).toMap();
    if (!data.isEmpty()) {
        emit itemSelected(data);
    }

    QString detail = itemStatus->data(LogDataRole::ResultDetailRole).toString();
    m_errorDetailEdit->setText(detail);
}
