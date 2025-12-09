QT += core gui widgets concurrent webenginewidgets webchannel
CONFIG += c++17

INCLUDEPATH += $$PWD/.. \
               $$PWD/../core \
               $$PWD/../core/events \
               $$PWD/../core/plugins \
               $$PWD/../core/commands \
               $$PWD/../core/run \
               $$PWD/../core/model \
               $$PWD/../core/viewers

SOURCES += \
    main.cpp \
    MainWindow.cpp \
    CodeEditor.cpp \
    CHighlighter.cpp \
    ../core/commands/CommandRegistry.cpp \
    ../core/run/PluginRunner.cpp \
    ../core/model/SliceModel.cpp \
    ../core/viewers/SliceResultViewer.cpp \
    ../core/viewers/GraphTab.cpp

HEADERS += \
    MainWindow.h \
    CodeEditor.h \
    CHighlighter.h \
    ../core/EditorContext.h \
    ../core/events/AnalysisTypes.h \
    ../core/events/DomainEventBus.h \
    ../core/plugins/IPlugin.h \
    ../core/plugins/CommandDescriptor.h \
    ../core/commands/CommandRegistry.h \
    ../core/run/PluginRunner.h \
    ../core/model/SliceResult.h \
    ../core/model/SliceModel.h \
    ../core/viewers/IResultViewerFactory.h \
    ../core/viewers/SliceResultViewer.h \
    ../core/viewers/GraphTab.h

RESOURCES += resources.qrc
