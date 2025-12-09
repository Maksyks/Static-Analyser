// core/plugins/IPlugin.h
#pragma once
#include <QString>
#include <QStringList>
#include <QVariant>
#include "plugins/CommandDescriptor.h"
#include "EditorContext.h"

class IPlugin {
public:
    virtual ~IPlugin() = default;
    virtual QString id() const = 0;
    virtual QString name() const = 0;
    virtual QStringList supportedKinds() const = 0;        // например ["slice.v1"]
    virtual QList<CommandDescriptor> commands() const = 0; // декларативные команды
    virtual QVariant runAnalysis(QString kind,
                                 const EditorContext& ctx,
                                 const QVariantMap& params) = 0; // без UI
};
#define IPlugin_iid "com.myide.plugins.IPlugin.v1"
Q_DECLARE_INTERFACE(IPlugin, IPlugin_iid)
