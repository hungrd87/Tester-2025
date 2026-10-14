#include "ComponentMapperWidget.h"
#include "imagelabel.h"
#include "componentdialog.h"
#include "errorsetupdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QListWidget>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>
#include <QScrollArea>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QLabel>
#include <QTimer>
#include <QAbstractItemView>
#include <QItemSelectionModel>
#include <QTreeWidget>
#include <QHeaderView>
#include <memory>
#include <QDebug>
#include <QSignalBlocker>

// Constructor
ComponentMapperWidget::ComponentMapperWidget(QWidget *parent)
    : QWidget(parent),
    m_isAdmin(false), // <<< Khởi tạo
    m_currentStage(STAGE_IDLE),       // <<< Khởi tạo
    m_isDragDropping(false),   // <<< Khởi tạo
    m_simulationIndex(0), // <<< Khởi tạo
    m_simulationBitResult(0), // <<< THÊM
    m_simulationForceGood(false),        // <<< THÊM
    m_hasPendingCommand(false),
    m_pendingStage(0),
    m_pendingResult(0)
{
    setupUI();
    setAdmin(m_isAdmin);

    m_imageLabel->setComponentList(&m_components);

    m_simulationTimer = new QTimer(this);
    m_simulationTimer->setInterval(500); // 500 ms
    connect(m_simulationTimer, &QTimer::timeout, this, &ComponentMapperWidget::runSimulationStep);
}

void ComponentMapperWidget::setAdmin(bool isAdmin)
{
    m_isAdmin = isAdmin;

    if (m_rightPanel) {
        m_rightPanel->setVisible(isAdmin);
    }

    if (!m_currentImageBytes.isEmpty()) {
        m_imageLabel->setDrawingEnabled(m_isAdmin);
    }

    qDebug() << "Chế độ Admin:" << (m_isAdmin ? "Bật (Hiện panel)" : "Tắt (Ẩn panel)");
}

void ComponentMapperWidget::onStageUpdate(int next_stage, int old_result)
{
    // === THAY ĐỔI: Kiểm tra trạng thái bận ===
    if (m_simulationTimer->isActive()) {
        qDebug() << "[External] ĐANG BẬN. Ghi đè lệnh chờ: stage=" << next_stage;
        // Ghi đè bất kỳ lệnh chờ nào trước đó bằng lệnh MỚI NHẤT
        m_hasPendingCommand = true;
        m_pendingStage = next_stage;
        m_pendingResult = old_result;
        return; // Chỉ lưu lại và thoát, không thực hiện
    }

    qDebug() << "[External] Nhận tín hiệu: next_stage =" << next_stage
             << ", prev_result =" << old_result;

    STAGE_ENUM newStage = static_cast<STAGE_ENUM>(next_stage);

    // Dừng mô phỏng cũ NẾU nó đang chạy (tránh xung đột)
    if (m_simulationTimer->isActive()) {
        m_simulationTimer->stop();
        qDebug() << "Mô phỏng bị ngắt bởi lệnh mới.";
    }

    // Xử lý các trạng thái KHÔNG chạy mô phỏng
    switch (newStage) {
    case STAGE_IDLE:
        m_testLogWidget->clearLog();
        m_testLogWidget->setTestProgressText("Trạng thái chờ");
        m_testLogWidget->resetTestProgress();
        setFinalStatus(0);
        break;

    case STAGE_ARM_PREPARE:
        // Đây là bước duy nhất được phép xóa log
        m_testLogWidget->clearLog();
        setFinalStatus(0);
        m_testLogWidget->setTestProgressText("Chuẩn bị phần cứng...");
        m_testLogWidget->setTestProgress(30);
        break;

    case STAGE_FPGA_PREPARE:
        m_testLogWidget->setTestProgress(65);
        break;

    case STAGE_FPGA_TEST:
        m_testLogWidget->setTestProgress(100);
        m_testLogWidget->setTestProgressText("Hoàn tất phần cứng...");
        break;

        // Xử lý các trạng thái CHẠY MÔ PHỎNG
    case STAGE_COMBINE_TEST:
        m_testLogWidget->setTestProgressText("Kiểm tra lần lượt...");
        // Yêu cầu: old_result = 0, chạy List 1 báo TỐT
        if (m_currentStage == STAGE_FPGA_TEST) {
            qDebug() << "Logic: FPGA_Test -> Combined_Test (List 1 PASS)";
            startTestSimulation(m_errors_1, 0, true, newStage, "error_1");
        } else {
            qWarning() << "Stage_Combined_Test nhận từ trạng thái không mong muốn:" << m_currentStage;
        }
        break;

    case STAGE_GOOD:
        // Yêu cầu: Chạy List 2 báo TỐT, kết thúc TỐT
        if (m_currentStage == STAGE_COMBINE_TEST) {
            qDebug() << "Logic: Combined_Test -> Good (List 2 PASS)";
            startTestSimulation(m_errors_2, 0, true, newStage, "error_2");
        } else {
            qWarning() << "Stage_Good nhận từ trạng thái không mong muốn:" << m_currentStage;
        }
        break;

    case STAGE_BAD:
        // Yêu cầu: Kết thúc HỎNG
        if (m_currentStage == STAGE_FPGA_TEST) {
            m_testLogWidget->setTestProgressText("Kiểm tra lần lượt...");
            // Chạy List 1, dùng old_result
            qDebug() << "Logic: FPGA_Test -> Bad (List 1 FAIL)";
            startTestSimulation(m_errors_1, old_result, false, newStage, "error_1");
        } else if (m_currentStage == STAGE_COMBINE_TEST) {
            // Chạy List 2, dùng old_result
            qDebug() << "Logic: Combined_Test -> Bad (List 2 FAIL)";
            startTestSimulation(m_errors_2, old_result, false, newStage, "error_2");
        } else {
            qWarning() << "Stage_Bad nhận từ trạng thái không mong muốn:" << m_currentStage;
            // setFinalStatus(2); // Báo HỎNG ngay
        }
        break;

    default:
        qWarning() << "[Warning] Trạng thái 'next_stage' không xác định:" << next_stage;
        break;
    }

    m_currentStage = newStage; // Lưu trạng thái hiện tại
}

bool ComponentMapperWidget::loadProjectFromPath(const QString &filePath)
{
    if (filePath.isEmpty()) {
        qWarning() << "loadProjectFromPath: Đường dẫn tệp rỗng.";
        return false;
    }

    // Gọi hàm helper
    return performLoadProject(filePath);
}

void ComponentMapperWidget::setupUI()
{
    // Đặt tiêu đề (chỉ có tác dụng nếu nó là cửa sổ độc lập)
    setWindowTitle("Trình biên tập lỗi linh kiện");

    // === Panel 1: Log ===
    m_testLogWidget = new TestLogWidget(this);

    // === Panel 2: Ảnh ===
    m_imageLabel = new ImageLabel;
    m_imageLabel->setMinimumSize(400, 400);
    m_imageLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_scrollArea = new QScrollArea;
    m_scrollArea->setWidget(m_imageLabel);
    m_scrollArea->setWidgetResizable(true);

    m_statusPassButton = new QPushButton("TỐT");
    m_statusFailButton = new QPushButton("HỎNG");
    m_statusPassButton->setMinimumSize(160, 40);
    m_statusFailButton->setMinimumSize(160, 40);

    QHBoxLayout *statusLayout = new QHBoxLayout;
    statusLayout->addStretch(1); // Đẩy từ bên trái
    statusLayout->addWidget(m_statusPassButton);
    statusLayout->addWidget(m_statusFailButton);
    statusLayout->addStretch(1); // Đẩy từ bên phải
    statusLayout->setSpacing(80);

    QWidget *statusContainer = new QWidget;
    statusContainer->setLayout(statusLayout);

    QWidget *middlePanel = new QWidget;
    QVBoxLayout *middleLayout = new QVBoxLayout(middlePanel);
    middleLayout->addWidget(m_scrollArea, 1); // Ảnh (chiếm nhiều không gian)
    middleLayout->addWidget(statusContainer); // Nút (chiếm ít không gian)
    middleLayout->setContentsMargins(0, 0, 0, 0);

    // === Phía phải: Cấu trúc lại với Tab ===
    m_rightPanel = new QWidget;
    QVBoxLayout *rightLayout = new QVBoxLayout(m_rightPanel);

    // 1. Các nút
    m_loadButton = new QPushButton("1. Tải ảnh mạch");
    m_setupErrorButton = new QPushButton("2. Thiết lập lỗi");
    m_runTestButton = new QPushButton("Chạy kiểm tra (Demo)");
    m_saveButton = new QPushButton("Lưu dự án");
    m_loadProjectButton = new QPushButton("Tải dự án");

    m_setupErrorButton->setEnabled(false); // Vô hiệu hóa ban đầu
    m_saveButton->setEnabled(false);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addWidget(m_saveButton);
    buttonLayout->addWidget(m_loadProjectButton);

    // 2. Tab Widget
    m_tabWidget = new QTabWidget(this);

    // 2a. Tab Linh kiện (thành QTreeWidget)
    m_componentListWidget = new QTreeWidget(this);
    m_componentListWidget->setColumnCount(2);
    m_componentListWidget->setHeaderLabels(QStringList() << "Linh kiện" << "Thông tin");
    m_componentListWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_componentListWidget->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_componentListWidget->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_componentListWidget->setRootIsDecorated(false); // Dạng bảng
    m_tabWidget->addTab(m_componentListWidget, "Linh kiện");

    // 2b. Tab Danh sách lỗi 1
    m_errorListWidget_1 = new QTreeWidget(this);
    m_errorListWidget_1->setColumnCount(2);
    m_errorListWidget_1->setHeaderLabels(QStringList() << "Tên lỗi" << "Nội dung");
    m_errorListWidget_1->setSelectionMode(QAbstractItemView::SingleSelection);
    m_errorListWidget_1->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_errorListWidget_1->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_errorListWidget_1->setRootIsDecorated(false);
    m_errorListWidget_1->setDragEnabled(true);
    m_errorListWidget_1->setAcceptDrops(true);
    m_errorListWidget_1->setDropIndicatorShown(true);
    m_errorListWidget_1->setDefaultDropAction(Qt::MoveAction);
    m_errorListWidget_1->setDragDropMode(QAbstractItemView::InternalMove);
    m_tabWidget->addTab(m_errorListWidget_1, "Danh sách lỗi 1"); // <<< Đổi tên

    // 2c. Tab Danh sách lỗi 2 (MỚI)
    m_errorListWidget_2 = new QTreeWidget(this);
    m_errorListWidget_2->setColumnCount(2);
    m_errorListWidget_2->setHeaderLabels(QStringList() << "Tên lỗi" << "Nội dung");
    m_errorListWidget_2->setSelectionMode(QAbstractItemView::SingleSelection);
    m_errorListWidget_2->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_errorListWidget_2->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_errorListWidget_2->setRootIsDecorated(false);
    m_errorListWidget_2->setDragEnabled(true);
    m_errorListWidget_2->setAcceptDrops(true);
    m_errorListWidget_2->setDropIndicatorShown(true);
    m_errorListWidget_2->setDefaultDropAction(Qt::MoveAction);
    m_errorListWidget_2->setDragDropMode(QAbstractItemView::InternalMove);
    m_tabWidget->addTab(m_errorListWidget_2, "Danh sách lỗi 2"); // <<< Thêm tab

    // 3. Thêm vào layout
    rightLayout->addWidget(m_loadButton);
    rightLayout->addWidget(m_setupErrorButton);
    rightLayout->addWidget(m_runTestButton);
    rightLayout->addLayout(buttonLayout);
    rightLayout->addWidget(m_tabWidget); // Thêm QTabWidget (thay vì QListWidget)

    // 1. Gộp Ảnh+Status (middlePanel) và Điều khiển (rightPanel)
    QSplitter *rightSplitter = new QSplitter(Qt::Horizontal);
    rightSplitter->addWidget(middlePanel); // <<< THAY ĐỔI
    rightSplitter->addWidget(m_rightPanel);
    rightSplitter->setSizes(QList<int>() << 800 << 250);

    // 2. Gộp Log (m_testLogWidget) và phần còn lại
    QSplitter *mainSplitter = new QSplitter(Qt::Horizontal);
    mainSplitter->addWidget(m_testLogWidget);
    mainSplitter->addWidget(rightSplitter);
    mainSplitter->setSizes(QList<int>() << 300 << 1050);

    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    mainLayout->addWidget(mainSplitter);
    resize(1350, 768);

    // === Kết nối Signals/Slots (Không thay đổi) ===
    connect(m_loadButton, &QPushButton::clicked, this, &ComponentMapperWidget::onLoadImage);
    connect(m_saveButton, &QPushButton::clicked, this, &ComponentMapperWidget::onSaveProject);
    connect(m_loadProjectButton, &QPushButton::clicked, this, &ComponentMapperWidget::onLoadProject);
    connect(m_imageLabel, &ImageLabel::rectDrawn, this, &ComponentMapperWidget::onRectDrawn);
    connect(m_imageLabel, &ImageLabel::componentGeometryChanged, this, &ComponentMapperWidget::onComponentGeometryChanged);

    connect(m_setupErrorButton, &QPushButton::clicked, this, &ComponentMapperWidget::onSetupErrorClicked);
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &ComponentMapperWidget::onTabChanged);
    connect(m_componentListWidget->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ComponentMapperWidget::onComponentSelectionChanged);
    connect(m_componentListWidget, &QTreeWidget::itemDoubleClicked, this, &ComponentMapperWidget::onComponentDoubleClicked);
    connect(m_errorListWidget_1->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ComponentMapperWidget::onErrorListSelectionChanged);
    connect(m_errorListWidget_1, &QTreeWidget::itemDoubleClicked, this, &ComponentMapperWidget::onErrorListDoubleClicked);
    connect(m_errorListWidget_2->selectionModel(), &QItemSelectionModel::selectionChanged, this, &ComponentMapperWidget::onErrorListSelectionChanged);
    connect(m_errorListWidget_2, &QTreeWidget::itemDoubleClicked, this, &ComponentMapperWidget::onErrorListDoubleClicked);
    connect(m_testLogWidget, &TestLogWidget::itemSelected, this, &ComponentMapperWidget::onTestLogItemSelected);
    connect(m_runTestButton, &QPushButton::clicked, this, &ComponentMapperWidget::onRunTestClicked);

    // connect(m_errorListWidget_1->model(), &QAbstractItemModel::rowsMoved, this, &ComponentMapperWidget::onErrorListRowsMoved);
    // connect(m_errorListWidget_2->model(), &QAbstractItemModel::rowsMoved, this, &ComponentMapperWidget::onErrorListRowsMoved);

    connect(m_errorListWidget_1->model(),
            SIGNAL(rowsAboutToBeRemoved(const QModelIndex&,int,int)),
            this,
            SLOT(onRowsAboutToBeRemoved(const QModelIndex&,int,int)));
    connect(m_errorListWidget_1->model(),
            SIGNAL(rowsInserted(const QModelIndex&,int,int)),
            this,
            SLOT(onRowsInserted(const QModelIndex&,int,int)));

    connect(m_errorListWidget_2->model(),
            SIGNAL(rowsAboutToBeRemoved(const QModelIndex&,int,int)),
            this,
            SLOT(onRowsAboutToBeRemoved(const QModelIndex&,int,int)));
    connect(m_errorListWidget_2->model(),
            SIGNAL(rowsInserted(const QModelIndex&,int,int)),
            this,
            SLOT(onRowsInserted(const QModelIndex&,int,int)));

    // === Khởi tạo trạng thái ban đầu ===
    setFinalStatus(0);
}

void ComponentMapperWidget::clearProject()
{
    m_components.clear();
    m_errors_1.clear(); // <<< THAY ĐỔI
    m_errors_2.clear();
    m_currentImageBytes.clear();

    m_imageLabel->setLocalPixmap(QPixmap()); // Dùng setLocalPixmap thay vì setPixmap
    updateComponentList();
    updateErrorList_1(); // <<< THAY ĐỔI
    updateErrorList_2();

    m_saveButton->setEnabled(false);
    m_setupErrorButton->setEnabled(false);

    m_testLogWidget->clearLog(); // (đã tự gọi resetTestProgress)
    setFinalStatus(0);
}

void ComponentMapperWidget::handleComponentDeletion(int deletedIndex)
{
    // 1. Xóa linh kiện (không đổi)
    m_components.removeAt(deletedIndex);
    delete m_componentListWidget->takeTopLevelItem(deletedIndex);

    // === THAY ĐỔI: Cập nhật CẢ HAI list lỗi ===
    // 3a. Cập nhật m_errors_1
    QList<int> errorsToRemove_1;
    for (int i = m_errors_1.size() - 1; i >= 0; --i) {
        ErrorInfo &error = m_errors_1[i];
        error.componentIndices.removeAll(deletedIndex);
        for (int j = 0; j < error.componentIndices.size(); ++j) {
            if (error.componentIndices[j] > deletedIndex) error.componentIndices[j]--;
        }
        if (error.componentIndices.isEmpty()) errorsToRemove_1.append(i);
    }
    for (int index : errorsToRemove_1) m_errors_1.removeAt(index);

    // 3b. Cập nhật m_errors_2
    QList<int> errorsToRemove_2;
    for (int i = m_errors_2.size() - 1; i >= 0; --i) {
        ErrorInfo &error = m_errors_2[i];
        error.componentIndices.removeAll(deletedIndex);
        for (int j = 0; j < error.componentIndices.size(); ++j) {
            if (error.componentIndices[j] > deletedIndex) error.componentIndices[j]--;
        }
        if (error.componentIndices.isEmpty()) errorsToRemove_2.append(i);
    }
    for (int index : errorsToRemove_2) m_errors_2.removeAt(index);
    // ======================================

    // 5. Cập nhật lại UI
    updateErrorList_1();
    updateErrorList_2();

    m_imageLabel->setHighlightedIndices({});
    m_imageLabel->update();
}

void ComponentMapperWidget::highlightComponents(const QList<int> &indices)
{
    // 1. Highlight ảnh
    m_imageLabel->setHighlightedIndices(indices);

    // 2. Chuyển tab
    m_tabWidget->setCurrentWidget(m_componentListWidget);

    // 3. Chọn item trong list (Chặn signal để tránh vòng lặp)
    bool oldSignalState = m_componentListWidget->selectionModel()->signalsBlocked();
    m_componentListWidget->selectionModel()->blockSignals(true);

    m_componentListWidget->clearSelection();
    for (int index : indices) {
        if (index >= 0 && index < m_componentListWidget->topLevelItemCount()) {
            // QTreeWidget dùng 'topLevelItem(index)' thay vì 'item(index)'
            QTreeWidgetItem *item = m_componentListWidget->topLevelItem(index);
            if(item) {
                item->setSelected(true);
            }
        }
    }

    m_componentListWidget->selectionModel()->blockSignals(oldSignalState);
}

void ComponentMapperWidget::highlightError(int errorIndex, QTreeWidget* listWidget)
{
    // Xác định list dữ liệu
    QList<ErrorInfo>* errorList = nullptr;
    if (listWidget == m_errorListWidget_1) {
        errorList = &m_errors_1;
    } else if (listWidget == m_errorListWidget_2) {
        errorList = &m_errors_2;
    } else {
        return; // Không phải list lỗi
    }

    if (errorIndex < 0 || errorIndex >= errorList->size()) return;

    // 1. Highlight ảnh
    m_imageLabel->setHighlightedIndices(errorList->at(errorIndex).componentIndices);

    // 2. Chuyển tab (nếu cần)
    m_tabWidget->setCurrentWidget(listWidget);

    // 3. Chọn item (chặn signal)
    bool oldSignalState = listWidget->selectionModel()->signalsBlocked();
    listWidget->selectionModel()->blockSignals(true);

    QTreeWidgetItem *item = listWidget->topLevelItem(errorIndex);
    if (item) {
        listWidget->setCurrentItem(item);
    }

    listWidget->selectionModel()->blockSignals(oldSignalState);
}

void ComponentMapperWidget::setFinalStatus(int status)
{
    // CSS cho các trạng thái
    const QString passStyle = "background-color: #2ecc71; color: white; font-weight: bold; border: none; padding: 5px;";
    const QString failStyle = "background-color: #e74c3c; color: white; font-weight: bold; border: none; padding: 5px;";
    const QString defaultStyle = "background-color: #bdc3c7; color: #7f8c8d; font-weight: bold; border: none; padding: 5px;";

    // Reset
    m_statusPassButton->setEnabled(false);
    m_statusFailButton->setEnabled(false);
    m_statusPassButton->setStyleSheet(defaultStyle);
    m_statusFailButton->setStyleSheet(defaultStyle);

    if (status == 1) { // Pass
        m_statusPassButton->setEnabled(true);
        m_statusPassButton->setStyleSheet(passStyle);
    } else if (status == 2) { // Fail
        m_statusFailButton->setEnabled(true);
        m_statusFailButton->setStyleSheet(failStyle);
    }
    // (status == 0 là trạng thái Reset ở trên)
}

bool ComponentMapperWidget::performLoadProject(const QString &filePath)
{
    QFile loadFile(filePath);
    if (!loadFile.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Lỗi", "Không thể mở tệp dự án.");
        return false;
    }

    QByteArray data = loadFile.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        QMessageBox::warning(this, "Lỗi", "Tệp dự án không hợp lệ.");
        return false;
    }

    // Xóa dự án hiện tại
    clearProject();

    QJsonObject mainObject = doc.object();

    // === THAY ĐỔI: Tải ảnh từ Base64 ===
    if (!mainObject.contains("imageData")) {
        QMessageBox::warning(this, "Lỗi", "Tệp dự án không chứa dữ liệu ảnh.");
        clearProject();
        return false;
    }

    // Đọc chuỗi Base64
    QString base64String = mainObject["imageData"].toString();
    // Chuyển về QByteArray (dữ liệu Base64)
    // Dùng toLatin1() an toàn và nhanh hơn toUtf8() cho Base64/ASCII
    QByteArray base64Bytes = base64String.toLatin1();
    // Giải mã Base64
    QByteArray imageBytes = QByteArray::fromBase64(base64Bytes);

    QPixmap pixmap;
    if (imageBytes.isEmpty() || !pixmap.loadFromData(imageBytes)) {
        QMessageBox::warning(this, "Lỗi", "Dữ liệu ảnh trong tệp dự án bị hỏng.");
        clearProject();
        return false;
    }

    m_currentImageBytes = imageBytes;
    m_imageLabel->setLocalPixmap(pixmap);

    // 2. Tải danh sách linh kiện
    QJsonArray componentsArray = mainObject["components"].toArray();
    for (const QJsonValue &value : componentsArray) {
        QJsonObject compObject = value.toObject();
        ComponentInfo info;
        info.idName = compObject["id"].toString();
        info.infoMsg = compObject["info"].toString();

        QJsonArray rectArray = compObject["rect"].toArray();
        if (rectArray.size() == 4) {
            info.rect = QRect(rectArray[0].toInt(), rectArray[1].toInt(), rectArray[2].toInt(), rectArray[3].toInt());
        }
        m_components.append(info);
    }

    // === THAY ĐỔI: Tải 2 list lỗi ===
    // 1. Tải errors_1
    if (mainObject.contains("errors_1")) {
        QJsonArray errorsArray = mainObject["errors_1"].toArray();
        for (const QJsonValue &value : errorsArray) {
            QJsonObject errorObject = value.toObject();
            ErrorInfo error;
            error.testName = errorObject["testName"].toString("Lỗi (chưa có tên)");
            error.errorMessage = errorObject["message"].toString();
            QJsonArray indicesArray = errorObject["indices"].toArray();
            for (const QJsonValue &indexVal : indicesArray) error.componentIndices.append(indexVal.toInt());
            m_errors_1.append(error);
        }
    } else if (mainObject.contains("errors")) { // Hỗ trợ load file cũ
        // (Copy-paste logic của errors_1, trỏ vào "errors")
    }

    // 2. Tải errors_2
    if (mainObject.contains("errors_2")) {
        QJsonArray errorsArray = mainObject["errors_2"].toArray();
        for (const QJsonValue &value : errorsArray) {
            QJsonObject errorObject = value.toObject();
            ErrorInfo error;
            error.testName = errorObject["testName"].toString("Lỗi (chưa có tên)");
            error.errorMessage = errorObject["message"].toString();
            QJsonArray indicesArray = errorObject["indices"].toArray();
            for (const QJsonValue &indexVal : indicesArray) error.componentIndices.append(indexVal.toInt());
            m_errors_2.append(error);
        }
    }

    updateErrorList_1();
    updateErrorList_2();

    // 3. Cập nhật UI
    updateComponentList();

    m_imageLabel->update(); // Vẽ lại các hình chữ nhật đã tải
    m_imageLabel->setDrawingEnabled(m_isAdmin);
    m_saveButton->setEnabled(true);

    return true;
}

void ComponentMapperWidget::startTestSimulation(const QList<ErrorInfo> &errorList, int bitResult, bool forceGood, STAGE_ENUM originStage, const QString &dataType)
{
    // KHÔNG reset Tốt/Hỏng, KHÔNG xóa log

    // Thiết lập các biến trạng thái
    m_simulationList = errorList;
    m_simulationIndex = 0;
    m_simulationBitResult = bitResult;
    m_simulationForceGood = forceGood;
    m_simulationOriginStage = originStage; // Lưu lại stage đã GỌI ta
    m_simulationDataType = dataType;

    m_simulationAllFailedSoFar = true;
    m_testLogWidget->setTestProgress(0);

    // Nếu list trống, gọi 'Hoàn tất' ngay lập tức
    if (m_simulationList.isEmpty()) {
        qWarning() << "startTestSimulation: danh sách lỗi trống.";
        onSimulationFinished(); // Gọi hoàn tất ngay
        return;
    }

    m_simulationTimer->start(); // Bắt đầu timer 500ms
}

void ComponentMapperWidget::onLoadImage()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Tải ảnh", "", "Image Files (*.png *.jpg *.bmp)");
    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Lỗi", "Không thể mở tệp ảnh.");
        return;
    }
    QByteArray imageBytes = file.readAll();
    file.close();

    QPixmap pixmap;
    if (!pixmap.loadFromData(imageBytes)) { // Load từ QByteArray
        QMessageBox::warning(this, "Lỗi", "Không thể tải dữ liệu ảnh. Tệp có thể bị hỏng.");
        return;
    }

    // Tải ảnh mới sẽ reset dự án
    clearProject();
    m_currentImageBytes = imageBytes;
    m_imageLabel->setLocalPixmap(pixmap);

    m_imageLabel->setDrawingEnabled(m_isAdmin);

    m_saveButton->setEnabled(true);
    QMessageBox::information(this, "Sẵn sàng", "Ảnh đã được tải. Hãy nhấn 'Thêm linh kiện' để bắt đầu vẽ.");
}

void ComponentMapperWidget::onRectDrawn(const QRect &rect)
{
    ComponentDialog dialog(this);

    // 1. Xác định tiền tố (prefix)
    // Mặc định là "U" nếu không có linh kiện nào
    QString prefix = "U";
    int prefixLength = 1;

    // Nếu đã có linh kiện, lấy tiền tố của linh kiện CUỐI CÙNG
    if (!m_components.isEmpty()) {
        const QString lastID = m_components.last().idName;

        // Tìm phần chữ cái ở đầu
        int i = 0;
        while (i < lastID.length() && lastID[i].isLetter()) {
            i++;
        }

        if (i > 0) { // Nếu tìm thấy (ví dụ: "U", "C", "Res")
            prefix = lastID.left(i);
            prefixLength = i;
        }
    }

    // 2. Tìm chỉ số (index) lớn nhất cho tiền tố này
    int maxIndex = 0;
    for (const ComponentInfo &info : m_components) {
        // Chỉ tìm các ID bắt đầu bằng đúng tiền tố (không phân biệt chữ hoa/thường)
        if (info.idName.startsWith(prefix, Qt::CaseInsensitive)) {

            // Lấy phần số
            QString numberPart = info.idName.mid(prefixLength);
            bool ok;
            int index = numberPart.toInt(&ok);

            // Nếu là số hợp lệ và lớn hơn max, cập nhật max
            if (ok && index > maxIndex) {
                maxIndex = index;
            }
        }
    }

    // 3. Tạo ID mới và đặt vào dialog
    QString nextID = QString("%1%2").arg(prefix).arg(maxIndex + 1);
    dialog.setDefaultComponentID(nextID, prefixLength);


    // 4. Hiển thị dialog (logic cũ không đổi)
    if (dialog.exec() == QDialog::Accepted)
    {
        ComponentInfo info;
        info.rect = rect;
        // Lấy tên mà người dùng đã nhập (hoặc giữ nguyên mặc định)
        info.idName = dialog.getComponentID();
        info.infoMsg = dialog.getInfoMessage();

        m_components.append(info);
        updateComponentList();

        m_saveButton->setEnabled(true);
    }

    m_imageLabel->update();
}

void ComponentMapperWidget::onComponentSelectionChanged()
{
    QList<QTreeWidgetItem*> items = m_componentListWidget->selectedItems(); // <<< THAY ĐỔI
    m_setupErrorButton->setEnabled(items.count() > 0);

    if (m_tabWidget->currentIndex() == 0) {
        QList<int> selectedIndices;
        for (QTreeWidgetItem* item : items) { // <<< THAY ĐỔI
            selectedIndices.append(m_componentListWidget->indexOfTopLevelItem(item)); // <<< THAY ĐỔI
        }
        m_imageLabel->setHighlightedIndices(selectedIndices);
    }
}

void ComponentMapperWidget::onComponentDoubleClicked(QTreeWidgetItem *item, int column) // <<< THAY ĐỔI
{
    Q_UNUSED(column);
    int index = m_componentListWidget->indexOfTopLevelItem(item); // <<< THAY ĐỔI
    if (index < 0 || index >= m_components.size()) return;

    ComponentInfo &info = m_components[index];
    ComponentDialog dialog(this);
    dialog.setMode(true);
    dialog.setComponentData(info.idName, info.infoMsg);

    int result = dialog.exec();

    if (result == QDialog::Accepted) {
        info.idName = dialog.getComponentID();
        info.infoMsg = dialog.getInfoMessage();

        // Cập nhật cả 2 cột
        item->setText(0, info.idName); // <<< THAY ĐỔI
        item->setText(1, info.infoMsg); // <<< THAY ĐỔI

        m_imageLabel->update();
    }
    else if (result == ComponentDialog::Delete) {
        // === Người dùng nhấn Xóa linh kiện ===
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Xác nhận xóa",
                                      QString("Bạn có chắc chắn muốn xóa linh kiện '%1'?").arg(info.idName),
                                      QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            handleComponentDeletion(index);
        }
    }
    // Nếu (result == QDialog::Rejected) -> Người dùng nhấn 'X' trên dialog
    // không làm gì cả.
}

void ComponentMapperWidget::onSetupErrorClicked()
{
    QList<QTreeWidgetItem*> items = m_componentListWidget->selectedItems(); // <<< THAY ĐỔI
    if (items.isEmpty()) return;

    QStringList names;
    QList<int> indices;

    for (QTreeWidgetItem* item : items) { // <<< THAY ĐỔI
        int index = m_componentListWidget->indexOfTopLevelItem(item); // <<< THAY ĐỔI
        names.append(m_components[index].idName);
        indices.append(index);
    }

    ErrorSetupDialog dialog(this);
    dialog.setComponentNames(names); // Tự động điền "Kiểm tra [U1, U2]"

    if (dialog.exec() == QDialog::Accepted) {
        ErrorInfo newError;
        newError.testName = dialog.getTestName();
        newError.componentIndices = indices;
        newError.errorMessage = dialog.getErrorMessage();

        // === THAY ĐỔI: Kiểm tra xem lưu vào list nào ===
        if (dialog.getTargetListIndex() == 0) {
            m_errors_1.append(newError);
            updateErrorList_1();
        } else {
            m_errors_2.append(newError);
            updateErrorList_2();
        }

        m_saveButton->setEnabled(true);
    }
}

void ComponentMapperWidget::onErrorListSelectionChanged()
{
    // Xác định tab nào đang active (1 hay 2)
    int currentTab = m_tabWidget->currentIndex();
    if (currentTab != 1 && currentTab != 2) return; // Không phải tab lỗi

    QTreeWidget* activeList = (currentTab == 1) ? m_errorListWidget_1 : m_errorListWidget_2;
    QList<ErrorInfo>* errorList = (currentTab == 1) ? &m_errors_1 : &m_errors_2;

    QList<QTreeWidgetItem*> items = activeList->selectedItems();
    if (items.isEmpty()) {
        m_imageLabel->setHighlightedIndices({});
        return;
    }

    int index = activeList->indexOfTopLevelItem(items.first());
    if (index >= 0 && index < errorList->size()) {
        m_imageLabel->setHighlightedIndices(errorList->at(index).componentIndices);
    }
}

void ComponentMapperWidget::onErrorListDoubleClicked(QTreeWidgetItem *item, int column) // <<< THAY ĐỔI
{
    Q_UNUSED(column);
    if (!item) return;

    QTreeWidget* listWidget = qobject_cast<QTreeWidget*>(sender());
    if (!listWidget) return;

    QList<ErrorInfo>* errorList = nullptr;
    std::function<void()> updateListFunc;

    if (listWidget == m_errorListWidget_1) {
        errorList = &m_errors_1;
        updateListFunc = [this](){ updateErrorList_1(); };
    } else if (listWidget == m_errorListWidget_2) {
        errorList = &m_errors_2;
        updateListFunc = [this](){ updateErrorList_2(); };
    } else {
        return;
    }

    // === THAY ĐỔI LỚN: Lấy index từ "thẻ" UserRole ===
    // 'index' bây giờ là index của DATA, bất kể item ở vị trí nào trên UI
    int index = item->data(0, Qt::UserRole).toInt();
    // int index = listWidget->indexOfTopLevelItem(item); // <<< ĐÂY LÀ DÒNG GÂY LỖI
    // ===============================================

    if (index < 0 || index >= errorList->size()) {
        qWarning() << "Lỗi đồng bộ: item trỏ đến index" << index << "không hợp lệ.";
        updateListFunc(); // Cập nhật lại list để sửa lỗi
        return;
    }

    ErrorInfo &error = (*errorList)[index];

    // Lấy tên các linh kiện
    QStringList names;
    for (int compIndex : error.componentIndices) {
        if (compIndex >= 0 && compIndex < m_components.size()) {
            names.append(m_components[compIndex].idName);
        } else {
            names.append(QString("<Lỗi: Linh kiện không tồn tại (index %1)>").arg(compIndex));
        }
    }

    ErrorSetupDialog dialog(this);
    dialog.setMode(true);
    dialog.setComponentNames(names);
    dialog.setErrorData(error.testName, error.errorMessage);
    // (Không cho phép đổi list khi edit, giữ nguyên)

    int result = dialog.exec();

    if (result == QDialog::Accepted) {
        error.testName = dialog.getTestName();
        error.errorMessage = dialog.getErrorMessage();
        item->setText(0, error.testName);
        item->setText(1, error.errorMessage);
    }
    else if (result == ErrorSetupDialog::Delete) {
        // Người dùng nhấn Xóa lỗi
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(this, "Xác nhận xóa",
                                      QString("Bạn có chắc chắn muốn xóa '%1'?").arg(item->text(0)),
                                      QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            errorList->removeAt(index);
            updateListFunc(); // Cập nhật list tương ứng
            m_imageLabel->setHighlightedIndices({});
        }
    }
}

void ComponentMapperWidget::onTabChanged(int index)
{
    // Xóa highlight cũ
    m_imageLabel->setHighlightedIndices({});

    // Kích hoạt lại logic highlight cho tab mới dựa trên lựa chọn hiện tại
    if (index == 0) { // Linh kiện
        onComponentSelectionChanged();
    } else if (index == 1) { // Lỗi 1
        onErrorListSelectionChanged();
    } else if (index == 2) { // Lỗi 2
        onErrorListSelectionChanged();
    }
}

void ComponentMapperWidget::onTestLogItemSelected(QVariantMap highlightData)
{
    if (highlightData.isEmpty()) {
        m_imageLabel->setHighlightedIndices({});

        // (Tùy chọn: Xóa lựa chọn trong các tab)
        // m_componentListWidget->clearSelection();
        // m_errorListWidget_1->clearSelection();
        // m_errorListWidget_2->clearSelection();

        return; // Dừng
    }

    QString type = highlightData.value("type").toString();
    QList<int> indices;
    QVariantList vList = highlightData.value("indices").toList();
    for(const QVariant &v : vList) indices.append(v.toInt());
    if (indices.isEmpty()) return;

    if (type == "component") {
        highlightComponents(indices);
    } else if (type == "error_1") {
        highlightError(indices.first(), m_errorListWidget_1);
    } else if (type == "error_2") {
        highlightError(indices.first(), m_errorListWidget_2);
    }
}

void ComponentMapperWidget::onRunTestClicked()
{
    // 0. Vô hiệu hóa nút để tránh click đúp
    m_runTestButton->setEnabled(false);

    // Dừng bất kỳ mô phỏng nào đang chạy
    if (m_simulationTimer->isActive()) {
        m_simulationTimer->stop();
    }

    // --- Bắt đầu chuỗi demo ---
    // Chúng ta sẽ mô phỏng một kịch bản "HỎNG" (Stage_Bad)
    // trong đó cả List 1 và List 2 đều có lỗi.

    // Demo: Giả sử List 1 có lỗi ở item 0 và 2 (bit 0, bit 2)
    // old_result = 0b101 = 5
    // int demoResultList1 = 5;
    int demoResultList1 = 0x34;
    long long delayList1 = (m_errors_1.size() + 2) * 500; // Ước tính thời gian chạy List 1

    // Demo: Giả sử List 2 có lỗi ở item 1 (bit 1)
    // old_result = 0b010 = 2
    int demoResultList2 = 2;
    long long delayList2 = (m_errors_2.size() + 2) * 500; // Ước tính thời gian chạy List 2

    // Tổng thời gian
    long long totalDelay = 0;
    const int stepDelay = 250; // Thời gian giữa các lệnh chuẩn bị

    // 1. (Time 0ms) - Bắt đầu, reset
    onStageUpdate(STAGE_IDLE, 0);
    totalDelay += stepDelay;

    // 2. (Time 250ms) - Chuẩn bị ARM
    QTimer::singleShot(totalDelay, this, [=]() {
        onStageUpdate(STAGE_ARM_PREPARE, 0);
    });
    totalDelay += stepDelay;

    // 3. (Time 500ms) - Chuẩn bị FPGA
    QTimer::singleShot(totalDelay, this, [=]() {
        onStageUpdate(STAGE_FPGA_READY, 0);
    });
    totalDelay += stepDelay;

    // // 4. (Time 750ms) - Bắt đầu Test
    // QTimer::singleShot(totalDelay, this, [=]() {
    //     onStageUpdate(STAGE_FPGA_TEST, 0);
    // });
    // totalDelay += stepDelay;

    // // 5. (Time 1000ms) - Chạy Test kết hợp (List 1)
    // // (Hàm này sẽ bắt đầu m_simulationTimer riêng của nó)
    // QTimer::singleShot(totalDelay, this, [=]() {
    //     qDebug() << "Demo: Bắt đầu Stage_Combined_Test (List 1) với kết quả =" << demoResultList1;
    //     onStageUpdate(STAGE_COMBINE_TEST, demoResultList1);
    // });
    // totalDelay += delayList1; // Chờ cho List 1 chạy xong

    // 4. (Time 750ms) - Bắt đầu Test
    QTimer::singleShot(totalDelay, this, [=]() {
        onStageUpdate(STAGE_FPGA_TEST, demoResultList1);
    });
    totalDelay += stepDelay;

    // 6. (Time 1000ms + delayList1) - Báo HỎNG (để kích hoạt List 2)
    // (Hàm này sẽ bắt đầu m_simulationTimer cho List 2)
    QTimer::singleShot(totalDelay, this, [=]() {
        qDebug() << "Demo: Bắt đầu Stage_Bad (List 2) với kết quả =" << demoResultList2;
        // Vì trạng thái trước đó là Stage_Combined_Test,
        // lệnh này sẽ kích hoạt kiểm tra Danh sách lỗi 2
        onStageUpdate(STAGE_BAD, demoResultList1);
        // onStageUpdate(STAGE_BAD, demoResultList2);
    });
    totalDelay += delayList2; // Chờ cho List 2 chạy xong

    // 7. (Time 1000ms + delayList1 + delayList2) - Kích hoạt lại nút
    QTimer::singleShot(totalDelay, this, [=]() {
        qDebug() << "Demo: Hoàn tất chuỗi demo.";
        m_runTestButton->setEnabled(true);
    });
}

void ComponentMapperWidget::runSimulationStep()
{
    int totalItems = m_simulationList.size();
    bool isList1 = (m_simulationDataType == "error_1");

    // === 1. Xử lý logic N-1 (Chỉ cho List 1) ===
    if (isList1 && totalItems > 0 && m_simulationIndex == (totalItems - 1)) // Đây là item N
    {
        if (m_simulationAllFailedSoFar == false) // (Nghĩa là 1 item Tốt)
        {
            qDebug() << "List 1 (N-1): Đã có item Tốt. Bỏ qua item N.";
            m_simulationTimer->stop();
            onSimulationFinished(); // Hoàn tất sớm
            return;
        }
        // (Nếu 'm_simulationAllFailedSoFar' là TRUE, tiếp tục chạy item N)
        qDebug() << "List 1 (N-1): Tất cả đều Lỗi. Tiếp tục chạy item N.";
    }

    // === 2. Xử lý dừng (Stop Condition) ===
    if (m_simulationIndex >= totalItems)
    {
        m_simulationTimer->stop();
        onSimulationFinished();
        return;
    }

    // === 3. Lấy item và chạy (Run Step) ===
    const ErrorInfo &error = m_simulationList[m_simulationIndex];
    QVariantMap data;
    data["type"] = m_simulationDataType;
    data["indices"] = QVariantList{m_simulationIndex};

    QString stepName = error.testName;
    int row = m_testLogWidget->addTestStep(stepName, data);

    m_testLogWidget->setTestProgressText(QString("Đang kiểm tra: %1").arg(stepName));

    // === 4. Quyết định Pass/Fail ===
    bool pass;
    QString resultDetail;

    // 4a. Logic ép Hỏng cho item N (nếu N-1 đều Hỏng)
    if (isList1 && m_simulationIndex == (totalItems - 1) && m_simulationAllFailedSoFar == true)
    {
        pass = false;
        resultDetail = error.errorMessage.isEmpty() ? "Lỗi tổng hợp" : error.errorMessage;
    }
    // 4b. Logic ép Tốt (dùng cho Stage_Combined_Test)
    else if (m_simulationForceGood)
    {
        pass = true;
        resultDetail = "Tốt";
    }
    // 4c. Logic chuẩn (dùng bitResult)
    else
    {
        bool isFail = (m_simulationBitResult >> m_simulationIndex) & 1;
        pass = !isFail;
        resultDetail = pass ? "Tốt" : error.errorMessage;
    }

    // === 5. Cập nhật cờ N-1 ===
    // (Chỉ cập nhật nếu là List 1 VÀ đang chạy trong (N-1) items)
    if (isList1 && m_simulationIndex < (totalItems - 1))
    {
        if (pass) {
            m_simulationAllFailedSoFar = false; // Phát hiện 1 item Tốt
        }
    }

    // === 6. Cập nhật UI ===
    m_testLogWidget->updateTestStep(row, pass, resultDetail);
    int progress = ((m_simulationIndex + 1) * 100) / totalItems;
    m_testLogWidget->setTestProgress(progress);
    m_testLogWidget->setTestProgressText(QString("Đã kiểm tra: %1").arg(stepName));

    // === 6b. Dừng sớm cho List 2 khi gặp lỗi đầu tiên ===
    if (!isList1 && !pass) {
        qDebug() << "List 2: Phát hiện lỗi tại item" << m_simulationIndex << "- Dừng mô phỏng.";
        m_simulationTimer->stop();
        onSimulationFinished();
        return;
    }

    // === 7. Chuẩn bị cho bước tiếp theo ===
    m_simulationIndex++;
}

void ComponentMapperWidget::onSimulationFinished()
{
    qDebug() << "Hoàn tất mô phỏng cho stage:" << m_simulationOriginStage;

    bool isList1 = (m_simulationDataType == "error_1");
    int totalItems = m_simulationList.size();

    // === THAY ĐỔI: Kiểm tra xem có dừng sớm không ===
    bool stoppedEarly = (isList1 && m_simulationIndex < totalItems);

    if (!stoppedEarly)
    {
        // Hoàn thành bình thường
        // m_testLogWidget->addTestStep("Hoàn tất", QVariantMap());
        m_testLogWidget->setTestProgress(100);
        m_testLogWidget->setTestProgressText("Hoàn tất");
    }
    else
    {
        // Dừng sớm (do List 1 có item Tốt)
        // m_testLogWidget->addTestStep("Dừng (phát hiện item Tốt)", QVariantMap());
        m_testLogWidget->setTestProgressText("Hoàn tất");
        // (Không set 100% vì chưa chạy hết)
        m_testLogWidget->setTestProgress(100);//Vẫn set 100%
    }

    // === Logic Quyết định TỐT/HỎNG (không đổi) ===
    if (m_simulationOriginStage == STAGE_GOOD) {
        setFinalStatus(1); // TỐT
    }
    else if (m_simulationOriginStage == STAGE_BAD) {
        setFinalStatus(2); // HỎNG
    }
    else if (m_simulationOriginStage == STAGE_COMBINE_TEST) {
        // (Dừng sớm hay không cũng không set status)
        qDebug() << "Mô phỏng (List 1 PASS) hoàn tất. Chờ lệnh tiếp theo...";
    }

    // === THÊM MỚI: Xử lý lệnh đang chờ ===
    if (m_hasPendingCommand) {
        qDebug() << "[External] Xử lý lệnh đang chờ: stage=" << m_pendingStage;

        // Lấy thông tin lệnh chờ
        int stage = m_pendingStage;
        int result = m_pendingResult;

        // Xóa cờ chờ (Quan trọng: Đặt lại trước khi gọi)
        m_hasPendingCommand = false;
        m_pendingStage = 0;
        m_pendingResult = 0;

        // Gọi lại hàm onExternalStageUpdate một cách an toàn
        // (Timer không còn active, nên nó sẽ thực thi ngay)
        // Dùng QTimer::singleShot(0) để tránh đệ quy sâu trên stack
        QTimer::singleShot(0, this, [=]() {
            onStageUpdate(stage, result);
        });

    }
    // Nếu không có lệnh chờ, không làm gì thêm
    // (Nút Run Demo sẽ tự quản lý việc bật lại)
}

void ComponentMapperWidget::onComponentGeometryChanged(int index, const QRect &newRect)
{
    if (index < 0 || index >= m_components.size()) return;

    // Cập nhật dữ liệu trong model
    m_components[index].rect = newRect;

    // Đánh dấu là cần lưu
    m_saveButton->setEnabled(true);

    qDebug() << "Cập nhật vị trí cho linh kiện" << index << "tới" << newRect;
}

void ComponentMapperWidget::onRowsAboutToBeRemoved(const QModelIndex &parent, int start, int end)
{
    // Khi người dùng bắt đầu KÉO, 'remove' được gọi.
    // Chúng ta chỉ cần đặt cờ.
    Q_UNUSED(parent);
    Q_UNUSED(start);
    Q_UNUSED(end);

    QObject* model = sender();

    // Đảm bảo đây là model chúng ta quan tâm
    if (model == m_errorListWidget_1->model() || model == m_errorListWidget_2->model()) {
        // Chỉ đặt cờ nếu đây là thao tác kéo (drag)
        QTreeWidget* listWidget = (model == m_errorListWidget_1->model()) ? m_errorListWidget_1 : m_errorListWidget_2;

        // Kiểm tra xem có phải 'InternalMove' đang diễn ra không
        if(listWidget->dragDropMode() == QAbstractItemView::InternalMove) {
            m_isDragDropping = true;
            qDebug() << "Bắt đầu Drag-Drop (Remove)...";
        }
    }
}

void ComponentMapperWidget::onRowsInserted(const QModelIndex &parent, int start, int end)
{
    // (Chúng ta không cần các tham số này)
    Q_UNUSED(parent); Q_UNUSED(start); Q_UNUSED(end);

    // === BƯỚC 1: KIỂM TRA CỜ ===
    // Nếu cờ này là 'false', đây là 'add' do code (ví dụ: load project)
    // Chúng ta phải bỏ qua nó.
    if (!m_isDragDropping) {
        return; // Bỏ qua
    }

    // === BƯỚC 2: RESET CỜ ===
    // (Thao tác 'move' đã hoàn tất, reset cờ)
    m_isDragDropping = false;
    qDebug() << "Kết thúc Drag-Drop (Insert). Đang đồng bộ hóa...";

    // === BƯỚC 3: Đồng bộ hóa (Logic v11/v16) ===

    // 3a. Xác định list nào đã di chuyển
    QObject* model = sender();
    QTreeWidget* listWidget = nullptr;
    QList<ErrorInfo>* errorList = nullptr;

    if (model == m_errorListWidget_1->model()) {
        listWidget = m_errorListWidget_1;
        errorList = &m_errors_1;
    } else if (model == m_errorListWidget_2->model()) {
        listWidget = m_errorListWidget_2;
        errorList = &m_errors_2;
    } else {
        return;
    }

    // 3b. Đọc UI -> Xây dựng lại Data
    QList<ErrorInfo> newOrderedList;
    for (int i = 0; i < listWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = listWidget->topLevelItem(i);
        int originalDataIndex = item->data(0, Qt::UserRole).toInt();
        if (originalDataIndex >= 0 && originalDataIndex < errorList->size()) {
            newOrderedList.append((*errorList)[originalDataIndex]);
        }
    }

    // 3c. Thay thế Data cũ
    *errorList = newOrderedList;

    // 3d. Cập nhật lại "thẻ" UserRole
    // (KHÔNG gọi updateListFunc() - để tránh vòng lặp)
    for (int i = 0; i < listWidget->topLevelItemCount(); ++i) {
        listWidget->topLevelItem(i)->setData(0, Qt::UserRole, i);
    }

    m_saveButton->setEnabled(true);
}

// void ComponentMapperWidget::onErrorListRowsMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd, const QModelIndex &destinationParent, int destinationRow)
// {
//     // Chúng ta không cần các tham số này, vì chúng ta sẽ đọc lại toàn bộ
//     Q_UNUSED(sourceParent); Q_UNUSED(sourceStart); Q_UNUSED(sourceEnd);
//     Q_UNUSED(destinationParent); Q_UNUSED(destinationRow);

//     qDebug() << "onErrorListRowsMoved";

//     // 1. Xác định list nào đã di chuyển
//     QObject* model = sender();
//     QTreeWidget* listWidget = nullptr;
//     QList<ErrorInfo>* errorList = nullptr;
//     std::function<void()> updateListFunc;

//     if (model == m_errorListWidget_1->model()) {
//         listWidget = m_errorListWidget_1;
//         errorList = &m_errors_1;
//         updateListFunc = [this](){ updateErrorList_1(); };
//     } else if (model == m_errorListWidget_2->model()) {
//         listWidget = m_errorListWidget_2;
//         errorList = &m_errors_2;
//         updateListFunc = [this](){ updateErrorList_2(); };
//     } else {
//         return;
//     }

//     // === Logic mới: Xây dựng lại Data từ UI ===

//     QList<ErrorInfo> newOrderedList;
//     // Tạo một bản sao của Data cũ để tìm kiếm (rất quan trọng)
//     QList<ErrorInfo> oldListCopy = *errorList;

//     // 2. Duyệt qua TreeWidget (UI) theo thứ tự MỚI
//     for (int i = 0; i < listWidget->topLevelItemCount(); ++i) {
//         QTreeWidgetItem* item = listWidget->topLevelItem(i);
//         QString uiTestName = item->text(0);
//         QString uiErrorMsg = item->text(1);

//         // 3. Tìm item này trong bản sao của Data cũ
//         int foundIndex = -1;
//         for(int j = 0; j < oldListCopy.size(); ++j) {
//             // (Chúng ta tìm dựa trên nội dung text)
//             if (oldListCopy[j].testName == uiTestName && oldListCopy[j].errorMessage == uiErrorMsg) {
//                 foundIndex = j;
//                 break;
//             }
//         }

//         if (foundIndex != -1) {
//             // 4. Thêm Data (từ bản sao) vào list mới
//             // và XÓA nó khỏi bản sao (để xử lý trường hợp trùng lặp)
//             newOrderedList.append(oldListCopy.takeAt(foundIndex));
//             qDebug() << "Đã tìm được index at: " << foundIndex;
//         } else {
//             // Trường hợp này không nên xảy ra nếu UI và Data khớp
//             qWarning() << "Lỗi đồng bộ khi kéo: không tìm thấy item" << uiTestName;
//         }
//     }

//     // 5. Thay thế hoàn toàn danh sách Data CŨ bằng danh sách MỚI
//     // (Đây là bước cập nhật vĩnh viễn m_errors_1/2)
//     *errorList = newOrderedList;

//     // 6. Xây dựng lại UI để reset các "thẻ" (UserRole)
//     // (Điều này đảm bảo Double-Click hoạt động)
//     updateListFunc();

//     m_saveButton->setEnabled(true);
//     qDebug() << "Đã đồng bộ lại Data theo UI (Fix v11).";
// }

void ComponentMapperWidget::updateComponentList()
{
    m_componentListWidget->clear();
    for (const ComponentInfo &info : m_components) {
        // Thêm item 2 cột
        QTreeWidgetItem *item = new QTreeWidgetItem(QStringList() << info.idName << info.infoMsg); // <<< THAY ĐỔI
        m_componentListWidget->addTopLevelItem(item); // <<< THAY ĐỔI
    }
}

void ComponentMapperWidget::updateErrorList_1()
{
    m_errorListWidget_1->clear();
    for (int i = 0; i < m_errors_1.size(); ++i) { // Dùng vòng lặp 'i'
        const ErrorInfo &error = m_errors_1[i];
        QTreeWidgetItem *item = new QTreeWidgetItem(QStringList() << error.testName << error.errorMessage);

        Qt::ItemFlags flags = item->flags();
        flags &= ~Qt::ItemIsDropEnabled;
        item->setFlags(flags);

        // === ĐÁNH DẤU: item này đại diện cho data ở index 'i' ===
        item->setData(0, Qt::UserRole, i);
        // =======================================================

        m_errorListWidget_1->addTopLevelItem(item);
    }
}

void ComponentMapperWidget::updateErrorList_2()
{
    m_errorListWidget_2->clear();
    for (int i = 0; i < m_errors_2.size(); ++i) { // Dùng vòng lặp 'i'
        const ErrorInfo &error = m_errors_2[i];
        QTreeWidgetItem *item = new QTreeWidgetItem(QStringList() << error.testName << error.errorMessage);

        Qt::ItemFlags flags = item->flags();
        flags &= ~Qt::ItemIsDropEnabled;
        item->setFlags(flags);

        // === ĐÁNH DẤU: item này đại diện cho data ở index 'i' ===
        item->setData(0, Qt::UserRole, i);
        // =======================================================

        m_errorListWidget_2->addTopLevelItem(item);
    }
}

void ComponentMapperWidget::onSaveProject()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Lưu dự án", "", "Project Files (*.json)");
    if (fileName.isEmpty()) {
        return;
    }

    QJsonObject mainObject;
    mainObject["imageData"] = QString(m_currentImageBytes.toBase64());

    QJsonArray componentsArray;
    for (const ComponentInfo &info : m_components) {
        QJsonObject compObject;
        compObject["id"] = info.idName;
        compObject["info"] = info.infoMsg;

        QJsonArray rectArray;
        rectArray.append(info.rect.x());
        rectArray.append(info.rect.y());
        rectArray.append(info.rect.width());
        rectArray.append(info.rect.height());
        compObject["rect"] = rectArray;

        componentsArray.append(compObject);
    }
    mainObject["components"] = componentsArray;

    // === THAY ĐỔI: Lưu 2 list lỗi ===
    // 1. Lưu errors_1
    QJsonArray errorsArray_1;
    for (const ErrorInfo &error : m_errors_1) {
        QJsonObject errorObject;
        errorObject["testName"] = error.testName;
        errorObject["message"] = error.errorMessage;
        QJsonArray indicesArray;
        for (int index : error.componentIndices) indicesArray.append(index);
        errorObject["indices"] = indicesArray;
        errorsArray_1.append(errorObject);
    }
    mainObject["errors_1"] = errorsArray_1;

    // 2. Lưu errors_2
    QJsonArray errorsArray_2;
    for (const ErrorInfo &error : m_errors_2) {
        QJsonObject errorObject;
        errorObject["testName"] = error.testName;
        errorObject["message"] = error.errorMessage;
        QJsonArray indicesArray;
        for (int index : error.componentIndices) indicesArray.append(index);
        errorObject["indices"] = indicesArray;
        errorsArray_2.append(errorObject);
    }
    mainObject["errors_2"] = errorsArray_2;

    QFile saveFile(fileName);
    if (!saveFile.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(this, "Lỗi", "Không thể lưu tệp.");
        return;
    }

    saveFile.write(QJsonDocument(mainObject).toJson(QJsonDocument::Indented));
    saveFile.close();
}

void ComponentMapperWidget::onLoadProject()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Tải dự án", "", "Project Files (*.json)");
    if (fileName.isEmpty()) {
        return; // Người dùng nhấn Hủy
    }

    // Gọi hàm helper
    performLoadProject(fileName);
}
