QT      += core
CONFIG  += c++17 plugin
TEMPLATE = lib
TARGET   = SlicePlugin

INCLUDEPATH += $$PWD/../../core $$PWD/../../

SOURCES += SlicePlugin.cpp
HEADERS += SlicePlugin.h
DISTFILES += SlicePlugin.json slice.sh

# Куда кладёт сам qmake библиотеку
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

PLUGINDIR = $$APP_BIN_DIR/plugins/slice

QMAKE_POST_LINK += \
    $$QMAKE_MKDIR_CMD \"$$PLUGINDIR\" && \
    $$QMAKE_COPY \"$$DLL_NAME\" \"$$PLUGINDIR\" && \
    $$QMAKE_COPY \"$$PWD/slice.sh\" \"$$PLUGINDIR\"
