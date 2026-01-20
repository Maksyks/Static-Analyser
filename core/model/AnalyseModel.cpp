#include "core/model/AnalyseModel.h"
#include <QMetaType>
#include <QVariant>

AnalyseModel::AnalyseModel(QObject* parent) : QObject(parent) {
    qRegisterMetaType<AnalyseResult>("AnalyseResult");
}

const AnalyseResult& AnalyseModel::state() const { return s_; }

void AnalyseModel::apply(const AnalysisResultEnvelope& env) {
    if (env.kind == QLatin1String("slice.v1")) {
        if (!env.payload.canConvert<AnalyseResult>()) return;
        s_ = env.payload.value<AnalyseResult>();
        emit changed(s_);
        return;
    }
    if (env.kind == QLatin1String("addrmap.v1")) {
        s_.addrmap = env.payload.toString();
        emit changed(s_);
        return;
    }
}
