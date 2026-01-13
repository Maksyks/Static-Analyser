// plugins/AddrMap/AddrMapPlugin.cpp
#include "AddrMapPlugin.h"

#include "AddrMapAnalyzer.h"

QList<CommandDescriptor> AddrMapPlugin::commands() const {
    CommandDescriptor d;
    d.id = QStringLiteral("addrmap.run");
    d.title = QObject::tr("Проанализировать код (AddressMapper)");
    d.menuPath = QStringLiteral("Editor/Context/AddressMapper");
    d.shortcut = QStringLiteral("Ctrl+Alt+A");
    d.kind = QStringLiteral("addrmap.v1");
    return { d };
}

QVariant AddrMapPlugin::runAnalysis(QString kind, const EditorContext& ctx, const QVariantMap& params) {
    Q_UNUSED(kind);
    Q_UNUSED(params);
    addrmap::AddrMapAnalyzer analyzer;
    return analyzer.analyze(ctx.documentText);
}
