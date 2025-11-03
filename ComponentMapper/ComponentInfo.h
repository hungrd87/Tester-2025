#ifndef COMPONENTINFO_H
#define COMPONENTINFO_H

#include <QRect>
#include <QString>
#include <QList>

// Cấu trúc lưu trữ thông tin của một linh kiện
struct ComponentInfo {
    QRect rect;       // Vị trí và kích thước trên ảnh
    QString idName;   // Tên định danh
    QString infoMsg; // Thông báo lỗi
};

struct ErrorInfo {
    QString testName;
    QList<int> componentIndices;
    QString errorMessage;
};

#endif // COMPONENTINFO_H
