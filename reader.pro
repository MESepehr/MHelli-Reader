TARGET = reader
TEMPLATE = lib
CONFIG += shared dll
DESTDIR += .
SOURCES += object.cpp \
        reader.cpp
HEADERS += object.h

QMAKE_LFLAGS += -Wl,-rpath,'./mhelli-reader'

QMAKE_MAC_SDK = macosx10.12
