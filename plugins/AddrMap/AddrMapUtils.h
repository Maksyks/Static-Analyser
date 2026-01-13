// plugins/AddrMap/AddrMapUtils.h
#pragma once

#include <QString>
#include <QStringList>

namespace addrmap {

QString trimComment(const QString& s);
QString norm(const QString& s);
bool isNullLike(const QString& s);

// "a->next->other" -> {"a","next","other"}
QStringList parseChainExpr(const QString& expr);

} // namespace addrmap
