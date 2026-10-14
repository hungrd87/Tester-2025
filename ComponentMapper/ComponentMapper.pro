# Chỉ định các module Qt cần thiết
QT       += core gui widgets

# Tên của tệp thực thi (executable)
TARGET = ComponentMapper

# Đây là một ứng dụng (application)
TEMPLATE = app

# Báo cho qmake biết thư mục chứa header là thư mục hiện tại
INCLUDEPATH += .
INCLUDEPATH += ..

# Các tệp nguồn
SOURCES += \
    ErrorSetupDialog.cpp \
    TestLogWidget.cpp \
    main.cpp \
    ComponentDialog.cpp \
    ImageLabel.cpp \
    ComponentMapperWidget.cpp

# Các tệp header
HEADERS += \
    ComponentInfo.h \
    ComponentDialog.h \
    ErrorSetupDialog.h \
    ImageLabel.h \
    ComponentMapperWidget.h \
    TestLogWidget.h

# --- Cấu hình đường dẫn Build ---
# Đây là phần quan trọng để đáp ứng yêu cầu của bạn.
# Tất cả các tệp build sẽ được đặt trong thư mục ../build (tức là Tester/build)
# và được sắp xếp gọn gàng theo tên của TARGET (ComponentMapperTest).

# Thư mục gốc cho các tệp build (so với tệp .pro này)
BUILD_DIR = ../build

# Thư mục cho tệp .exe
DESTDIR = $$BUILD_DIR/bin

# Thư mục cho các tệp đối tượng (.o)
OBJECTS_DIR = $$BUILD_DIR/obj

# Thư mục cho các tệp do MOC tạo ra
MOC_DIR = $$BUILD_DIR/moc

# Thư mục cho các tệp .rcc (nếu có)
# RCC_DIR = $$BUILD_DIR/rcc

# Thư mục cho các tệp .ui (nếu có)
# UI_DIR = $$BUILD_DIR/ui
