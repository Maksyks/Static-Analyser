// core/model/SliceModel.cpp
#include "core/model/SliceModel.h"
#include <QMetaType>
#include <QVariant>

SliceModel::SliceModel(QObject* parent) : QObject(parent) {
    qRegisterMetaType<SliceResult>("SliceResult");
}

const SliceResult& SliceModel::state() const { return s_; }

void SliceModel::apply(const AnalysisResultEnvelope& env) {
    if (env.kind == QLatin1String("slice.v1")) {
        if (!env.payload.canConvert<SliceResult>()) return;
        s_ = env.payload.value<SliceResult>();
        emit changed(s_);
        return;
    }
    if (env.kind == QLatin1String("addrmap.v1")) {
        s_.addrmap = env.payload.toString();
        emit changed(s_);
        return;
    }
}
