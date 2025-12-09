#pragma once
#include <QObject>
#include "core/model/SliceResult.h"
#include "core/events/AnalysisTypes.h"

class SliceModel : public QObject {
    Q_OBJECT
public:
    explicit SliceModel(QObject* parent=nullptr);
    const SliceResult& state() const;

public slots:
    void apply(const AnalysisResultEnvelope& env);

signals:
    void changed(const SliceResult& s);

private:
    SliceResult s_;
};
