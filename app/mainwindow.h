#pragma once
#include <QMainWindow>
#include "core/events/DomainEventBus.h"
#include "core/commands/CommandRegistry.h"
#include "core/model/AnalyseModel.h"
#include "core/run/PluginRunner.h"
#include "core/plugins/IPlugin.h"
#include "core/viewers/ResultViewer.h"
#include "app/CodeEditor.h"

class IDE : public QMainWindow {
    Q_OBJECT
public:
    explicit IDE(QWidget* parent=nullptr);

private slots:
    void openSourceFile();
    void onRightLineActivated(int line1based);

private:
    CodeEditor*           leftEditor  = nullptr;
    SliceResultViewer*    rightTabs   = nullptr;
    QMenu*                fileMenu    = nullptr;
    QMenu*                pluginsMenu = nullptr;

    DomainEventBus*       bus        = nullptr;
    CommandRegistry*      cmdReg     = nullptr;
    SliceModel*           sliceModel = nullptr;
    PluginRunner*         runner     = nullptr;
};
