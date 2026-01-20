// plugins/AddrMap/AddrMapAnalyzer.h
#pragma once

#include <QString>

namespace addrmap {

class AddrMapAnalyzer {
public:
    QString analyze(const QString& code) const;
};

} // namespace addrmap
