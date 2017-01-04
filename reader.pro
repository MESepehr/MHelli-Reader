TARGET = reader
TEMPLATE = lib
CONFIG += shared dll c++11
DESTDIR += .
SOURCES += object.cpp \
        reader.cpp
HEADERS += object.h

QMAKE_LFLAGS += -Wl,-rpath,'./mhelli-reader'
INCLUDEPATH = /usr/local/include
LIBS = -L/usr/local/lib -lzbar

osx {
    QMAKE_INFO_PLIST = osx/Info.plist
}
