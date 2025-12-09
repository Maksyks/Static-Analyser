#pragma once
#include <QObject>
#include <QList>
#include "core/events/DomainEventBus.h"
#include "core/plugins/IPlugin.h"
#include "core/EditorContext.h"

class PluginRunner : public QObject {
    Q_OBJECT
public:
    PluginRunner(const QList<IPlugin*>& plugins, DomainEventBus* bus, QObject* parent=nullptr);

private slots:
    void onRequested(QString kind, QVariantMap params, EditorContext ctx);

private:
    IPlugin* pick(const QString& kind) const;

    QList<IPlugin*> plugins_;
    DomainEventBus* bus_;
};
