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

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    QPoint mapLabelToPixmap(const QPoint &labelPos) const;
    QRect mapPixmapToLabel(const QRect &pixmapRect) const;

    QPixmap m_pixmap; // Ảnh gốc
    QRect m_pixmapDisplayRect;

    const QList<ComponentInfo> *m_pComponentList = nullptr; // Con trỏ tới data

    bool m_isDrawing = false;      // Cờ: đang vẽ
    bool m_drawingEnabled = false; // Cờ: cho phép vẽ
    QPoint m_startPoint;           // Điểm bắt đầu kéo
    QRect m_currentRect;           // Hình chữ nhật đang vẽ
    QList<int> m_highlightedIndices;
};

#endif // IMAGELABEL_H
