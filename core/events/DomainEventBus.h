#pragma once
#include <QObject>
#include <QVariant>
#include "core/events/AnalysisTypes.h"
#include "core/EditorContext.h"

class QMenu; // forward

class DomainEventBus : public QObject {
    Q_OBJECT
public:
    using QObject::QObject;

signals:
    // Редактор/документы
    void documentOpened(QString uri);
    void selectionChanged(EditorContext ctx);

    // Контекстное меню
    void requestContextMenu(EditorContext ctx, QMenu* menu);

    // Анализ (универсально)
    void analysisRequested(QString kind, QVariantMap params, EditorContext ctx);
    void analysisProgress(QString kind, int percent, QString message);
    void analysisCompleted(AnalysisResultEnvelope result, EditorContext ctx);
    void analysisFailed(QString kind, QString error, EditorContext ctx);
};
