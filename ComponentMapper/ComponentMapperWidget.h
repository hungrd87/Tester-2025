#ifndef COMPONENTMAPPERWIDGET_H
#define COMPONENTMAPPERWIDGET_H

#include <QWidget>
#include <QList>
#include <QByteArray>
#include "ComponentInfo.h"
#include "TestLogWidget.h"
#include "udp_def.h"

class ImageLabel;
class QListWidget;
class QListWidgetItem;
class QTreeWidget;
class QTreeWidgetItem;
class QScrollArea;
class QPushButton;
class QTabWidget;

class ComponentMapperWidget : public QWidget // Thay đổi chính ở đây
{
    Q_OBJECT

public:
    explicit ComponentMapperWidget(QWidget *parent = nullptr);
    // Không cần destructor

    void setAdmin(bool isAdmin);

public slots:
    void onStageUpdate(int next_stage, int old_result);
    bool loadProjectFromPath(const QString &filePath);

private slots:
    void onLoadImage();
    void onSaveProject();
    void onLoadProject();
    void onRectDrawn(const QRect &rect);

    void onComponentSelectionChanged();
    void onComponentDoubleClicked(QTreeWidgetItem *item, int column);

    void onSetupErrorClicked();
    void onErrorListSelectionChanged();
    void onErrorListDoubleClicked(QTreeWidgetItem *item, int column);
    void onTabChanged(int index);
    void onTestLogItemSelected(QVariantMap highlightData);
    void onRunTestClicked();

    void runSimulationStep();
    void onSimulationFinished();

    void onComponentGeometryChanged(int index, const QRect &newRect);
    void onErrorListRowsMoved(const QModelIndex &sourceParent, int sourceStart, int sourceEnd,
                              const QModelIndex &destinationParent, int destinationRow);

private:
    void setupUI();
    void updateComponentList();
    void updateErrorList_1();
    void updateErrorList_2();
    void clearProject();

    void handleComponentDeletion(int deletedIndex);
    void highlightComponents(const QList<int> &indices);
    void highlightError(int errorIndex, QTreeWidget* listWidget);

    // (0 = Reset, 1 = Pass, 2 = Fail)
    void setFinalStatus(int status);

    bool performLoadProject(const QString &filePath);

    void startTestSimulation(const QList<ErrorInfo> &errorList, int bitResult, bool forceGood, STAGE_ENUM originStage, const QString &dataType);

    // Dữ liệu (không đổi)
    QList<ComponentInfo> m_components;
    QList<ErrorInfo> m_errors_1;
    QList<ErrorInfo> m_errors_2;
    QByteArray m_currentImageBytes;

    // UI (không đổi)
    ImageLabel *m_imageLabel;
    QScrollArea *m_scrollArea;
    TestLogWidget *m_testLogWidget;

    QTabWidget *m_tabWidget;
    QTreeWidget *m_componentListWidget;
    QTreeWidget *m_errorListWidget_1;
    QTreeWidget *m_errorListWidget_2;

    QPushButton *m_loadButton;
    QPushButton *m_saveButton;
    QPushButton *m_setupErrorButton;
    QPushButton *m_loadProjectButton;
    QPushButton *m_runTestButton;

    QPushButton *m_statusPassButton;
    QPushButton *m_statusFailButton;

    QWidget *m_rightPanel;

    bool m_isAdmin;

    STAGE_ENUM m_currentStage;

    QTimer *m_simulationTimer;
    QList<ErrorInfo> m_simulationList;
    int m_simulationIndex;
    int m_simulationBitResult;    // Lưu giá trị old_result
    bool m_simulationForceGood;   // Dùng cho Stage_Good
    STAGE_ENUM m_simulationOriginStage; // Stage nào đã kích hoạt mô phỏng
    QString m_simulationDataType; // "error_1" hay "error_2"

    bool m_hasPendingCommand;
    int m_pendingStage;
    int m_pendingResult;
};

#endif // COMPONENTMAPPERWIDGET_H
