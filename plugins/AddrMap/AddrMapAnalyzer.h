// plugins/AddrMap/AddrMapAnalyzer.h
#pragma once

#include <QString>

namespace addrmap {

// Основной анализатор Address Mapper (алгоритм из статьи + адаптации проекта).
class AddrMapAnalyzer {
public:
    QString analyze(const QString& code) const;
};

} // namespace addrmap
