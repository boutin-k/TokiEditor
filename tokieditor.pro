QT += widgets xml
requires(qtConfig(filedialog))

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++14

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
DEFINES += QT_USE_QSTRINGBUILDER

TEMPLATE = app
TRANSLATIONS = languages/tokieditor_en.ts languages/tokieditor_fr.ts

HEADERS       = mainwindow.h \
                mdichild.h \
                settingsdialog.h \
                tkdata.h \
                tkflowlayout.h \
                tkgriditem.h \
                tkgridlayout.h \
                tklabel.h \
                tkutils.h
SOURCES       = main.cpp \
                mainwindow.cpp \
                mdichild.cpp \
                settingsdialog.cpp \
                tkflowlayout.cpp \
                tkgriditem.cpp \
                tkgridlayout.cpp \
                tklabel.cpp
RESOURCES     = \
    tokieditor.qrc

ICON = myappico.ico
