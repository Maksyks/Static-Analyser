QT      += core
CONFIG  += c++17 plugin
TEMPLATE = lib
TARGET   = AddrMapPlugin

INCLUDEPATH += $$PWD/../../core $$PWD/../../

SOURCES += AddrMapPlugin.cpp
HEADERS += AddrMapPlugin.h
DISTFILES += AddrMapPlugin.json

CONFIG(debug, debug|release) {
    DESTDIR = $$OUT_PWD/debug
    APP_BIN_DIR = $$OUT_PWD/../../app/debug
} else {
    DESTDIR = $$OUT_PWD/release
    APP_BIN_DIR = $$OUT_PWD/../../app/release
}

win32 {
    DLL_NAME = $$DESTDIR/$$TARGET.dll
} else:macx {
    DLL_NAME = $$DESTDIR/lib$$TARGET.dylib
} else {
    DLL_NAME = $$DESTDIR/lib$$TARGET.so
}

PLUGINDIR = $$APP_BIN_DIR/plugins/AddrMap

QMAKE_POST_LINK += \
    $$QMAKE_MKDIR_CMD \"$$PLUGINDIR\" && \
    $$QMAKE_COPY \"$$DLL_NAME\" \"$$PLUGINDIR\"
