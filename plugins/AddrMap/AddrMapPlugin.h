// plugins/AddrMap/AddrMapPlugin.h
#pragma once

#include <QObject>

#include "plugins/IPlugin.h"

class AddrMapPlugin : public QObject, public IPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID IPlugin_iid)
    Q_INTERFACES(IPlugin)

public:
    QString id() const override { return QStringLiteral("addrmap"); }
    QString name() const override { return QStringLiteral("Address Mapper"); }
    QStringList supportedKinds() const override { return { QStringLiteral("addrmap.v1") }; }

    QList<CommandDescriptor> commands() const override;
    QVariant runAnalysis(QString kind, const EditorContext& ctx, const QVariantMap& params) override;
};
