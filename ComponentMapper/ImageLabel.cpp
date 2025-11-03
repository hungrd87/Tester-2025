#include "imagelabel.h"
#include <QMouseEvent>
#include <QPainter>
#include <QPen>

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
    if (m_drawingEnabled && event->button() == Qt::LeftButton &&
        m_pixmapDisplayRect.contains(event->pos())) // Chỉ vẽ bên trong ảnh
    {
        // Chuyển từ tọa độ Label sang tọa độ Pixmap
        m_startPoint = mapLabelToPixmap(event->pos());
        m_currentRect = QRect(m_startPoint, QSize());
        m_isDrawing = true;
    }
}

void ImageLabel::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isDrawing) {
        // Chuyển từ tọa độ Label sang tọa độ Pixmap
        QPoint pixmapPos = mapLabelToPixmap(event->pos());

        m_currentRect = QRect(m_startPoint, pixmapPos).normalized();
        update();
    }
}

void ImageLabel::mouseReleaseEvent(QMouseEvent *event)
{
    if (m_isDrawing && event->button() == Qt::LeftButton) {
        m_isDrawing = false;
        if (m_currentRect.width() > 5 && m_currentRect.height() > 5) {
            // m_currentRect đã ở tọa độ Pixmap, sẵn sàng để emit
            emit rectDrawn(m_currentRect);
        }
        m_currentRect = QRect();
        update();
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

            // Vẽ hình chữ nhật
            painter.setPen(hRectPen);
            painter.drawRect(labelDrawRect);

            // Vẽ tên
            painter.setPen(textPen);
            QRect textRect = labelDrawRect;
            textRect.setHeight(20);
            textRect.moveCenter(labelDrawRect.center());
            painter.fillRect(textRect, QColor(0, 0, 0, 120));
            painter.drawText(labelDrawRect, Qt::AlignCenter, info.idName);
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
