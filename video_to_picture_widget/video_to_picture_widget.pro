QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17 app_bundle


# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

FORMS += \
    mainwindow.ui

INCLUDEPATH += /opt/homebrew/Cellar/opencv/4.7.0_2/include/opencv4

LIBS += /opt/homebrew/Cellar/opencv/4.7.0_2/lib/libopencv_core.dylib
LIBS += /opt/homebrew/Cellar/opencv/4.7.0_2/lib/libopencv_imgcodecs.dylib
LIBS += /opt/homebrew/Cellar/opencv/4.7.0_2/lib/libopencv_videoio.dylib
LIBS += /opt/homebrew/Cellar/gcc/12.2.0/lib/gcc/current/libgcc_s.1.1.dylib

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
