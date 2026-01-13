// plugins/AddrMap/AddrMapReport.h
#pragma once

#include <QString>

#include "AddrMapTypes.h"

namespace addrmap {

QString dumpReport(const State& st, bool valid);

} // namespace addrmap
