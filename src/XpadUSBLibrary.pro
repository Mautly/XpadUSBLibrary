#-------------------------------------------------
#
# Project created by QtCreator 2015-02-06T12:35:48
#
#-------------------------------------------------

QT       -= gui

TARGET = XpadUSBLibrary
TEMPLATE = lib

DEFINES += XPADUSBLIBRARY_LIBRARY

SOURCES += xpadusblibrary.cpp \
    tools.cpp \
    asynchronous.cpp \
    calibration.cpp \
    xpadcommands.cpp \
    writeexposefile.cpp \
    usbcontrol.cpp

HEADERS += xpadusblibrary.h\
        xpadusblibrary_global.h \
    tools.h \
    defines.h \
    asynchronous.h \
    calibration.h \
    xpadcommands.h \
    writeexposefile.h \
    usbcontrol.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}

unix: LIBS += -lquickusb

macx: LIBS += -L/Applications/QuickUSB/Library/MacOS/ -lquickusb
macx: INCLUDEPATH += /Applications/QuickUSB/Library/C/
macx: DEPENDPATH += /Applications/QuickUSB/Library/C/

win32{
    win32-msvc*:contains(QMAKE_HOST.arch, x86_64):{
        LIBS += "C:\Program Files (x86)\Bitwise Systems\QuickUsb\Library\DLL\amd64\QuickUSB.lib"
        INCLUDEPATH += "C:\Program Files (x86)\Bitwise Systems\QuickUsb\Library\C++ Builder"
        DEPENDPATH += "C:\Program Files (x86)\Bitwise Systems\QuickUsb\Library\C++ Builder"
    } else{
        LIBS += "C:\Program Files\Bitwise Systems\QuickUsb\Library\DLL\i386\QuickUSB.lib"
        INCLUDEPATH += "C:\Program Files\Bitwise Systems\QuickUsb\Library\C++ Builder"
        DEPENDPATH += "C:\Program Files\Bitwise Systems\QuickUsb\Library\C++ Builder"
    }
}

macx: QMAKE_MAC_SDK = macosx10.12
macx: QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.7
