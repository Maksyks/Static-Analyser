#pragma once
#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QMetaType>

struct AnalysisResultEnvelope {
    QString     kind;    // пример: "slice.v1"
    QVariant    payload; // результат плагина в QVariant (метатип или JSON)
    QVariantMap meta;    // произвольные метаданные
};
Q_DECLARE_METATYPE(AnalysisResultEnvelope)
