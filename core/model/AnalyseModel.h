#pragma once
#include <QObject>
#include "core/model/AnalyseResult.h"
#include "core/events/AnalysisTypes.h"

class AnalyseModel : public QObject {
    Q_OBJECT
public:
    explicit AnalyseModel(QObject* parent=nullptr);
    const AnalyseResult& state() const;

public slots:
    void apply(const AnalysisResultEnvelope& env);

signals:
    void changed(const AnalyseResult& s);

private:
    AnalyseResult s_; //composition
};
