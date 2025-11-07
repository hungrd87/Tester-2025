#include <QApplication>
#include "ComponentMapperWidget.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    //Check Arguments
    bool isAdmin = false;
    for(int i=0; i<argc; i++){
        if(QString::fromLatin1(argv[i]).contains("--admin") || QString::fromLatin1(argv[i]).contains("-a"))
            isAdmin = true;
    }

    // Khởi tạo và hiển thị widget của chúng ta
    ComponentMapperWidget widget;
    widget.show(); // Hiển thị nó như một cửa sổ

    widget.setAdmin(isAdmin);

    return a.exec();
}
