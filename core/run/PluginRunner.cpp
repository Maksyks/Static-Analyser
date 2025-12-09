#include "core/run/PluginRunner.h"
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
#include <QtConcurrent>
#else
#include <QtConcurrent/QtConcurrent>
#endif

PluginRunner::PluginRunner(const QList<IPlugin*>& plugins, DomainEventBus* bus, QObject* parent)
    : QObject(parent), plugins_(plugins), bus_(bus)
{
    qRegisterMetaType<EditorContext>("EditorContext");
    qRegisterMetaType<AnalysisResultEnvelope>("AnalysisResultEnvelope");
    connect(bus_, &DomainEventBus::analysisRequested, this, &PluginRunner::onRequested); //подписка на bus
}

//при событии в bus вызывается и выбирает плагин по supportedKinds
void PluginRunner::onRequested(QString kind, QVariantMap params, EditorContext ctx) {
    IPlugin* p = pick(kind);
    if (!p) {
        emit bus_->analysisFailed(kind, QStringLiteral("No plugin supports kind: %1").arg(kind), ctx);
        return;
    }
    QtConcurrent::run([=]{
        try {
            const QVariant payload = p->runAnalysis(kind, ctx, params); //вызов плагина
            AnalysisResultEnvelope env{kind, payload, {{"pluginId", p->id()}}};
            emit bus_->analysisCompleted(env, ctx);//возврат результата
        } catch (const std::exception& e) {
            emit bus_->analysisFailed(kind, QString::fromUtf8(e.what()), ctx);
        } catch (...) {
            emit bus_->analysisFailed(kind, QStringLiteral("Unknown error in plugin"), ctx);
        }
    });
}

IPlugin* PluginRunner::pick(const QString& kind) const {
    for (IPlugin* p : plugins_)
        if (p && p->supportedKinds().contains(kind))
            return p;
    return nullptr;
}
