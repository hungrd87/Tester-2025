#include "imagelabel.h"
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QApplication> // Để set/unset cursor
#include <QCursor>

ImageLabel::ImageLabel(QWidget *parent) : QLabel(parent)
{
    setMouseTracking(true); // Cần thiết để bắt sự kiện move ngay cả khi không nhấn

    setBackgroundRole(QPalette::Dark);
    setAutoFillBackground(true);
}

void ImageLabel::setComponentList(const QList<ComponentInfo> *components)
{
    m_pComponentList = components;
}

void ImageLabel::setLocalPixmap(const QPixmap &pixmap)
{
    m_pixmap = pixmap;
    m_highlightedIndices.clear();
    m_isDrawing = false;
    m_pixmapDisplayRect = QRect();
    update(); // Yêu cầu vẽ lại
}

void ImageLabel::setHighlightedIndices(const QList<int> &indices)
{
    if (m_highlightedIndices != indices) {
        m_highlightedIndices = indices;
        update(); // Yêu cầu vẽ lại
    }
}

void ImageLabel::setDrawingEnabled(bool enabled)
{
    m_drawingEnabled = enabled;
}

void ImageLabel::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) return;

    // === THAY ĐỔI: Ưu tiên 1. KÉO/THẢ (Drag/Resize) ===
    m_dragMode = getDragMode(event->pos()); // getDragMode() đã kiểm tra m_highlightedIndices

    if (m_dragMode != DragMode::None) {
        // Bắt đầu kéo/thả
        int index = m_highlightedIndices.first();
        m_originalDragRect = m_pComponentList->at(index).rect;
        m_editingRect = m_originalDragRect;

        QPoint pixmapPos = mapLabelToPixmap(event->pos());
        m_dragOffset = pixmapPos - m_originalDragRect.topLeft();

        m_isDrawing = false; // Đảm bảo không ở chế độ vẽ mới
        return; // Dừng
    }
    // ===============================================

    // === Ưu tiên 2. VẼ MỚI (Draw New) ===
    // (Chỉ chạy nếu m_dragMode == DragMode::None)
    if (m_drawingEnabled && m_pixmapDisplayRect.contains(event->pos()))
    {
        m_startPoint = mapLabelToPixmap(event->pos());
        m_currentRect = QRect(m_startPoint, QSize());
        m_isDrawing = true;
        m_dragMode = DragMode::None; // Đảm bảo không ở chế độ kéo
        return;
    }

    // Nếu không làm gì cả, reset
    m_dragMode = DragMode::None;
    m_isDrawing = false;
}

void ImageLabel::mouseMoveEvent(QMouseEvent *event)
{
    // 1. Đang VẼ MỚI
    if (m_isDrawing) {
        QPoint pixmapPos = mapLabelToPixmap(event->pos());
        m_currentRect = QRect(m_startPoint, pixmapPos).normalized();
        update();
        return; // Dừng
    }

    // 2. Đang KÉO/THẢ
    if (m_dragMode != DragMode::None) {
        QPoint pixmapPos = mapLabelToPixmap(event->pos());
        QRect newRect = m_originalDragRect;

        // Tính toán rect mới dựa trên chế độ kéo
        switch (m_dragMode) {
        case DragMode::Move:
            newRect.moveTo(pixmapPos - m_dragOffset);
            break;
        case DragMode::Resize_TL:
            newRect.setTopLeft(pixmapPos);
            break;
        case DragMode::Resize_T:
            newRect.setTop(pixmapPos.y());
            break;
        case DragMode::Resize_TR:
            newRect.setTopRight(pixmapPos);
            break;
        case DragMode::Resize_R:
            newRect.setRight(pixmapPos.x());
            break;
        case DragMode::Resize_BR:
            newRect.setBottomRight(pixmapPos);
            break;
        case DragMode::Resize_B:
            newRect.setBottom(pixmapPos.y());
            break;
        case DragMode::Resize_BL:
            newRect.setBottomLeft(pixmapPos);
            break;
        case DragMode::Resize_L:
            newRect.setLeft(pixmapPos.x());
            break;
        default: break;
        }

        m_editingRect = newRect.normalized();
        update(); // Yêu cầu vẽ lại
        return; // Dừng
    }

    // 3. Không kéo/vẽ: Chỉ cập nhật hình dạng con trỏ
    updateCursorShape(event->pos());
}

void ImageLabel::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) return;

    // 1. Kết thúc VẼ MỚI
    if (m_isDrawing) {
        m_isDrawing = false;
        if (m_currentRect.width() > 5 && m_currentRect.height() > 5) {
            emit rectDrawn(m_currentRect);
        }
        m_currentRect = QRect();
        update();
        return; // Dừng
    }

    // 2. Kết thúc KÉO/THẢ
    if (m_dragMode != DragMode::None) {
        int index = m_highlightedIndices.first();
        // Phát tín hiệu thay đổi (đã chuẩn hóa)
        emit componentGeometryChanged(index, m_editingRect.normalized());

        // Reset trạng thái
        m_dragMode = DragMode::None;
        m_editingRect = QRect();
        update();
        updateCursorShape(event->pos()); // Đặt lại con trỏ
        return; // Dừng
    }
}

void ImageLabel::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    // 1. Không gọi QLabel::paintEvent(event) nữa
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (m_pixmap.isNull()) {
        painter.fillRect(rect(), palette().brush(QPalette::Dark));
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter, "Vui lòng tải ảnh...");
        return;
    }

    // 2. Tự tính toán tỉ lệ và vẽ Pixmap
    QPixmap scaledPixmap = m_pixmap.scaled(this->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

    // Tính toán vị trí để căn giữa (letter-boxing)
    int x = (this->width() - scaledPixmap.width()) / 2;
    int y = (this->height() - scaledPixmap.height()) / 2;

    // Lưu lại vị trí này để dùng cho việc map tọa độ
    m_pixmapDisplayRect = QRect(x, y, scaledPixmap.width(), scaledPixmap.height());

    // Vẽ nền
    painter.fillRect(rect(), palette().brush(QPalette::Dark));
    // Vẽ ảnh đã scale
    painter.drawPixmap(m_pixmapDisplayRect, scaledPixmap);

    // 3. Vẽ TẤT CẢ các linh kiện được HIGHLIGHT
    if (m_pComponentList) {
        QPen hRectPen(Qt::red, 3, Qt::SolidLine);
        QPen textPen(Qt::green);
        QFont textFont("Arial", 11, QFont::Bold);
        painter.setFont(textFont);

        // Lặp qua danh sách các index được chọn
        for (int index : m_highlightedIndices) {
            if (index < 0 || index >= m_pComponentList->size()) continue;

            const ComponentInfo &info = m_pComponentList->at(index);
            QRect labelDrawRect = mapPixmapToLabel(info.rect);

            // 3a. Nếu đang kéo/sửa, vẽ m_editingRect
            if (m_dragMode != DragMode::None && index == m_highlightedIndices.first()) {
                labelDrawRect = mapPixmapToLabel(m_editingRect);
            } else {
                // 3b. Nếu không, vẽ rect gốc
                labelDrawRect = mapPixmapToLabel(info.rect);
            }

            // Vẽ hình chữ nhật
            painter.setPen(hRectPen);
            painter.drawRect(labelDrawRect);

            // Vẽ tên
            painter.setPen(textPen);
            QRect textRect = labelDrawRect;
            textRect.setHeight(26);
            textRect.moveCenter(labelDrawRect.center());
            // painter.fillRect(textRect, QColor(0, 0, 0, 120));
            painter.drawText(labelDrawRect, Qt::AlignCenter, info.idName);

            // 3c. === THÊM MỚI: Vẽ tay nắm (handles) ===
            // Chỉ vẽ nếu chọn 1 item VÀ không đang vẽ mới
            if (m_highlightedIndices.count() == 1 && !m_isDrawing) {
                QPen handlePen(QColor(255, 255, 255, 150), 1);
                QBrush handleBrush(QColor(255, 0, 0, 200)); // Đỏ bán trong
                painter.setPen(handlePen);
                painter.setBrush(handleBrush);

                // Lấy 8 tay nắm và vẽ
                foreach (const QRect &handle, getHandles(labelDrawRect)) {
                    painter.drawRect(handle);
                }
                painter.setBrush(Qt::NoBrush); // Reset
            }
        }
    }

    // 4. Vẽ hình chữ nhật MỚI (đang được kéo)
    if (m_isDrawing) {
        // Chuyển từ tọa độ Pixmap (m_currentRect) sang tọa độ Label (để vẽ)
        QRect labelDrawRect = mapPixmapToLabel(m_currentRect);

        QPen newRectPen(Qt::green, 2, Qt::SolidLine);
        painter.setPen(newRectPen);
        painter.drawRect(labelDrawRect);
    }
}

QPoint ImageLabel::mapLabelToPixmap(const QPoint &labelPos) const
{
    if (m_pixmap.isNull() || m_pixmapDisplayRect.width() == 0 || m_pixmapDisplayRect.height() == 0) {
        return QPoint();
    }

    // Tỉ lệ scale
    double scaleX = (double)m_pixmap.width() / m_pixmapDisplayRect.width();
    double scaleY = (double)m_pixmap.height() / m_pixmapDisplayRect.height();

    // Tọa độ tương đối bên trong m_pixmapDisplayRect
    int relX = labelPos.x() - m_pixmapDisplayRect.x();
    int relY = labelPos.y() - m_pixmapDisplayRect.y();

    // Chuyển về tọa độ ảnh gốc (pixmap)
    int pixmapX = qRound(relX * scaleX);
    int pixmapY = qRound(relY * scaleY);

    // Giới hạn trong kích thước ảnh gốc
    pixmapX = qBound(0, pixmapX, m_pixmap.width());
    pixmapY = qBound(0, pixmapY, m_pixmap.height());

    return QPoint(pixmapX, pixmapY);
}

QRect ImageLabel::mapPixmapToLabel(const QRect &pixmapRect) const
{
    if (m_pixmap.isNull() || m_pixmap.width() == 0 || m_pixmap.height() == 0) {
        return QRect();
    }

    // Tỉ lệ nghịch (map từ pixmap -> label)
    double invScaleX = (double)m_pixmapDisplayRect.width() / m_pixmap.width();
    double invScaleY = (double)m_pixmapDisplayRect.height() / m_pixmap.height();

    int labelX = qRound(pixmapRect.x() * invScaleX) + m_pixmapDisplayRect.x();
    int labelY = qRound(pixmapRect.y() * invScaleY) + m_pixmapDisplayRect.y();
    int labelW = qRound(pixmapRect.width() * invScaleX);
    int labelH = qRound(pixmapRect.height() * invScaleY);

    return QRect(labelX, labelY, labelW, labelH);
}

QList<QRect> ImageLabel::getHandles(const QRect &labelRect) const
{
    const int handleSize = 8;
    const int h_2 = handleSize / 2;
    QList<QRect> handles;

    // 4 góc
    handles.append(QRect(labelRect.topLeft() - QPoint(h_2, h_2), QSize(handleSize, handleSize)));
    handles.append(QRect(labelRect.topRight() - QPoint(h_2, h_2), QSize(handleSize, handleSize)));
    handles.append(QRect(labelRect.bottomRight() - QPoint(h_2, h_2), QSize(handleSize, handleSize)));
    handles.append(QRect(labelRect.bottomLeft() - QPoint(h_2, h_2), QSize(handleSize, handleSize)));

    // 4 cạnh (tùy chọn, nếu bạn muốn)
    // handles.append(QRect(labelRect.left() - h_2, labelRect.center().y() - h_2, handleSize, handleSize)); // L
    // handles.append(QRect(labelRect.right() - h_2, labelRect.center().y() - h_2, handleSize, handleSize)); // R
    // ... (T, B)

    return handles;
}

void ImageLabel::updateCursorShape(const QPoint &labelPos)
{
    // Nếu đang kéo, con trỏ đã được set, không đổi
    if (m_dragMode != DragMode::None) return;

    DragMode mode = getDragMode(labelPos);

    switch (mode) {
    case DragMode::Move:
        setCursor(Qt::SizeAllCursor);
        break;
    case DragMode::Resize_TL:
    case DragMode::Resize_BR:
        setCursor(Qt::SizeFDiagCursor);
        break;
    case DragMode::Resize_TR:
    case DragMode::Resize_BL:
        setCursor(Qt::SizeBDiagCursor);
        break;
    // (Thêm Qt::SizeVerCursor và Qt::SizeHorCursor nếu bạn thêm tay nắm 4 cạnh)
    default:
        unsetCursor(); // Dùng con trỏ mặc định
        break;
    }
}

ImageLabel::DragMode ImageLabel::getDragMode(const QPoint &labelPos) const
{
    // Chỉ cho phép kéo/sửa khi:
    // 1. Chỉ chọn 1 item
    // 2. Không đang trong chế độ vẽ mới
    // 3. Ảnh đã được tải
    if (m_highlightedIndices.count() != 1 || m_isDrawing || m_pixmap.isNull()) {
        return DragMode::None;
    }

    int index = m_highlightedIndices.first();
    QRect pixmapRect = m_pComponentList->at(index).rect;
    QRect labelRect = mapPixmapToLabel(pixmapRect);

    // Kích thước tay nắm (cần khớp với getHandles)
    const int handleSize = 8;
    const int h_2 = handleSize / 2;

    // Kiểm tra các góc (thứ tự ngược với paintEvent để ưu tiên góc)
    QRect tlHandle(labelRect.topLeft() - QPoint(h_2, h_2), QSize(handleSize, handleSize));
    if (tlHandle.contains(labelPos)) return DragMode::Resize_TL;

    QRect trHandle(labelRect.topRight() - QPoint(h_2, h_2), QSize(handleSize, handleSize));
    if (trHandle.contains(labelPos)) return DragMode::Resize_TR;

    QRect brHandle(labelRect.bottomRight() - QPoint(h_2, h_2), QSize(handleSize, handleSize));
    if (brHandle.contains(labelPos)) return DragMode::Resize_BR;

    QRect blHandle(labelRect.bottomLeft() - QPoint(h_2, h_2), QSize(handleSize, handleSize));
    if (blHandle.contains(labelPos)) return DragMode::Resize_BL;

    // (Thêm 4 cạnh nếu bạn đã triển khai chúng)

    // Kiểm tra thân
    // (Mở rộng vùng thân một chút để dễ click)
    if (labelRect.adjusted(-3, -3, 3, 3).contains(labelPos)) {
        return DragMode::Move;
    }

    return DragMode::None;
}
