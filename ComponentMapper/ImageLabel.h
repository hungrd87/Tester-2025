#ifndef IMAGELABEL_H
#define IMAGELABEL_H

#include <QLabel>
#include <QPoint>
#include <QRect>
#include <QList>
#include "ComponentInfo.h"

class QMouseEvent;
class QPaintEvent;

class ImageLabel : public QLabel
{
    Q_OBJECT

public:
    enum class DragMode {
        None,
        Move,
        Resize_TL, Resize_T, Resize_TR,
        Resize_R, Resize_BR, Resize_B,
        Resize_BL, Resize_L
    };

    explicit ImageLabel(QWidget *parent = nullptr);

    // Cung cấp con trỏ tới danh sách linh kiện (do MainWindow sở hữu)
    void setComponentList(const QList<ComponentInfo> *components);

    // Đặt ảnh pixmap và reset trạng thái
    void setLocalPixmap(const QPixmap &pixmap);

public slots:
    // Đặt index của linh kiện cần highlight
    void setHighlightedIndices(const QList<int> &indices);

    // Bật/tắt chế độ vẽ
    void setDrawingEnabled(bool enabled);

signals:
    // Phát ra khi người dùng vẽ xong một hình chữ nhật
    void rectDrawn(const QRect &rect);
    void componentGeometryChanged(int index, const QRect &newRect);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QPoint mapLabelToPixmap(const QPoint &labelPos) const;
    QRect mapPixmapToLabel(const QRect &pixmapRect) const;

    // Lấy 8 tay nắm (handles) xung quanh một hình chữ nhật (tọa độ label)
    QList<QRect> getHandles(const QRect &labelRect) const;
    // Lấy chế độ kéo dựa trên vị trí chuột (tọa độ label)
    DragMode getDragMode(const QPoint &labelPos) const;
    // Cập nhật con trỏ chuột (mũi tên, di chuyển, đổi kích thước)
    void updateCursorShape(const QPoint &labelPos);

    QPixmap m_pixmap; // Ảnh gốc
    QRect m_pixmapDisplayRect;

    const QList<ComponentInfo> *m_pComponentList = nullptr; // Con trỏ tới data
    QList<int> m_highlightedIndices;

    bool m_isDrawing = false;      // Cờ: đang vẽ
    bool m_drawingEnabled = false; // Cờ: cho phép vẽ
    QPoint m_startPoint;           // Điểm bắt đầu kéo
    QRect m_currentRect;           // Hình chữ nhật đang vẽ

    DragMode m_dragMode = DragMode::None;
    QPoint m_dragOffset;      // Tọa độ Pixmap
    QRect m_originalDragRect; // Tọa độ Pixmap (lưu rect gốc khi bắt đầu kéo)
    QRect m_editingRect;      // Tọa độ Pixmap (lưu rect đang trong quá trình sửa)
};

#endif // IMAGELABEL_H
