#-------------------------------------------------
#
# Project created by QtCreator 2011-04-08T14:26:48
#
#-------------------------------------------------

QT       += core gui
TARGET = imageA
TEMPLATE = app

win32-g++ {
LIBS += -L. -lboost_thread-mgw45-mt-1_45.dll \
     -L. -lboost_program_options-mgw45-mt-1_45.dll
}

INCLUDEPATH += c:/frank/myprog/include
SOURCES += main.cpp\
        mainwindow.cpp \
    sliceview.cpp \
    filebrowser.cpp \
    prog_interface.cpp

HEADERS  += mainwindow.h \
    sliceview.h \
    filebrowser.h \
    mat_file.hpp \
    prog_interface_static_link.h \

FORMS    += mainwindow.ui \
    filebrowser.ui

RESOURCES += \
    icons.qrc
