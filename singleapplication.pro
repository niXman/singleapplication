
QT       -= core gui

TARGET   = singleapplication
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += \
	main.cpp \
    singleapplication.cpp

LIBS += \
	-lpthread \
	-lrt \
	-lboost_system \
	-lboost_thread

HEADERS += \
    singleapplication.hpp
