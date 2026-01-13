// plugins/Slice/SlicePlugin.h
#pragma once
#include <QObject>
#include <QTemporaryDir>
#include "plugins/IPlugin.h"
#include "model/AnalyseResult.h"

class SlicePlugin : public QObject, public IPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID IPlugin_iid FILE "SlicePlugin.json")
    Q_INTERFACES(IPlugin)
public:
    QString id()   const override { return "slice"; }
    QString name() const override { return "Program Slicer"; }
    QStringList supportedKinds() const override { return {"slice.v1"}; }
    QList<CommandDescriptor> commands() const override;
    // {
    //     return {{ "slice.run", "Сделать срез…", "Editor/Context/Slice", "Ctrl+Alt+S", "", "slice.v1", {} }};
    // }
    QVariant runAnalysis(QString kind, const EditorContext& ctx, const QVariantMap& params) override;
private:
    SliceResult runSlice(const QString& input, const QString& criterion) const;
};
