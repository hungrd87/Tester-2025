#include <QApplication>
#include "ComponentMapperWidget.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Khởi tạo và hiển thị widget của chúng ta
    ComponentMapperWidget widget;
    widget.show(); // Hiển thị nó như một cửa sổ

    return a.exec();
}
