QT       += core gui network widgets

TARGET = Tester

TEMPLATE = app

INCLUDEPATH += $$PWD/ComponentMapper

SOURCES += \
    ComponentMapper/ComponentDialog.cpp \
    ComponentMapper/ComponentMapperWidget.cpp \
    ComponentMapper/ErrorSetupDialog.cpp \
    ComponentMapper/ImageLabel.cpp \
    ComponentMapper/TestLogWidget.cpp \
    config.cpp \
    dialog.cpp \
    main.cpp \
    tester.cpp \
    udp.cpp \
    view.cpp

HEADERS += \
    ComponentMapper/ComponentDialog.h \
    ComponentMapper/ComponentInfo.h \
    ComponentMapper/ComponentMapperWidget.h \
    ComponentMapper/ErrorSetupDialog.h \
    ComponentMapper/ImageLabel.h \
    ComponentMapper/TestLogWidget.h \
    config.h \
    dialog.h \
    tester.h \
    udp.h \
    udp_def.h \
    view.h

FORMS += \
    config.ui \
    dialog.ui \
    view.ui

RC_FILE = Tester.rc

RESOURCES += \
    resources.qrc

# Thư mục gốc cho các tệp build (so với tệp .pro này)
BUILD_DIR = build/$$TARGET

# Thư mục cho tệp .exe
DESTDIR = $$BUILD_DIR/bin

# Thư mục cho các tệp đối tượng (.o)
OBJECTS_DIR = $$BUILD_DIR/obj

# Thư mục cho các tệp do MOC tạo ra
MOC_DIR = $$BUILD_DIR/moc

# Thư mục cho các tệp .rcc (nếu có)
RCC_DIR = $$BUILD_DIR/rcc

# Thư mục cho các tệp .ui (nếu có)
UI_DIR = $$BUILD_DIR/ui

# (Tùy chọn) Bật warnings đẹp
QMAKE_CXXFLAGS += -Wall -Wextra -Wpedantic

DISTFILES += \
    TEMP.txt
