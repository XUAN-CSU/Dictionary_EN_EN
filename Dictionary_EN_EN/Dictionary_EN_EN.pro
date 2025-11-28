QT       += core gui
QT       += network
QT       += multimedia multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES +=

RESOURCES += \
    Resource.qrc \
    Resource.qrc

DESTDIR = ./

CONFIG -= debug_and_release


#程序版本
VERSION = 0.0.1
#程序图标
RC_ICONS = $$PWD\images\app_icon.ico
#公司名称
QMAKE_TARGET_COMPANY ="WLC"
#程序说明
QMAKE_TARGET_DESCRIPTION = "Dictionary_EN_EN"
#版权信息
QMAKE_TARGET_COPYRIGHT = "Copyright(C) 2025 WLC Co.,Ltd."
#程序名称
QMAKE_TARGET_PRODUCT = "Dictionary_EN_EN"
#程序语言
#0x0800代表和系统当前语言一致
RC_LANG = 0x0800

# In Windows cmd /c stops after the first command finishes unless you explicitly chain with &&.
QMAKE_POST_LINK += cmd /c $$PWD\\bat\\copy_images.bat && cmd /c $$PWD\\bat\\\copy_Auto_Add_Version_and_Delete_self_To_release_folder.bat
