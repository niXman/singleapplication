
QT       -= core gui
TARGET   = singleapplication
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = \
    app

INCLUDEPATH += \
    include

SOURCES += \
    main.cpp \
    src/singleapplication.cpp

HEADERS += \
    include/singleapplication/singleapplication.hpp

LIBS += \
    -lpthread \
    -lrt \
    -lboost_system \
    -lboost_thread
