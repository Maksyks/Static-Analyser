// core/viewers/IResultViewerFactory.h
#pragma once
#include <QString>
#include <QWidget>
#include "events/DomainEventBus.h"

class IResultViewerFactory {
public:
    virtual ~IResultViewerFactory() = default;
    virtual QString kind() const = 0;
    virtual QWidget* create(QWidget* parent) = 0;
    virtual void render(QWidget* view, const AnalysisResultEnvelope& env) = 0;
};
