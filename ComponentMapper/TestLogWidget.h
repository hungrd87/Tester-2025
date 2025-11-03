#ifndef TESTLOGWIDGET_H
#define TESTLOGWIDGET_H

#include <QWidget>
#include <QVariantMap>
#include <QIcon>

class QTreeView;
class QStandardItemModel;
class QModelIndex;
class QProgressBar;
class QTextEdit;

class TestLogWidget : public QWidget
{
    Q_OBJECT

public:
    explicit TestLogWidget(QWidget *parent = nullptr);

    // Dùng để lưu trữ dữ liệu liên kết với một bước
    enum LogDataRole {
        HighlightDataRole = Qt::UserRole + 1,
        ResultDetailRole = Qt::UserRole + 2
    };

public slots:
    // Xóa toàn bộ log
    void clearLog();

    // Thêm một bước mới, trả về ID (row) của bước đó
    int addTestStep(const QString &stepName, const QVariantMap &highlightData);

    // Cập nhật kết quả của một bước
    void updateTestStep(int row, bool passed, const QString &resultMessage);

    void setTestProgress(int value);
    void setTestProgressText(const QString &text);
    void resetTestProgress();

signals:
    // Phát ra khi người dùng click vào một bước, yêu cầu highlight
    void itemSelected(QVariantMap highlightData);

private slots:
    // Xử lý khi người dùng click
    void onLogItemClicked(const QModelIndex &index);

private:
    void setupUi();

    QTreeView *m_view;
    QStandardItemModel *m_model;
    QTextEdit *m_errorDetailEdit;
    QProgressBar *m_progressBar;

    // Icons
    QIcon m_iconRunning;
    QIcon m_iconPass;
    QIcon m_iconFail;
};

#endif // TESTLOGWIDGET_H
