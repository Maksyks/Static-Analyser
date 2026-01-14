// plugins/AddrMap/AddrMapUtils.cpp
#include "AddrMapUtils.h"

#include <QRegularExpression>

namespace addrmap {

QString trimComment(const QString& s) {
    QString out = s;
    const int i = out.indexOf(QStringLiteral("//"));
    if (i >= 0)
        out = out.left(i);
    return out.trimmed();
}

QString norm(const QString& s) {
    return s.trimmed();
}


bool isNullLike(const QString& s) {
    const QString t = s.trimmed();
    return t.isEmpty()
        || t == QStringLiteral("NULL")
        || t == QStringLiteral("0")
        || t == QStringLiteral("nullptr");
}

QStringList parseChainExpr(const QString& expr) {
    const QString n = norm(expr);
    QStringList parts;
    for (const QString& p : n.split(QStringLiteral("->"), Qt::SkipEmptyParts))
        parts << p.trimmed();
    return parts;
}

} // namespace addrmap
