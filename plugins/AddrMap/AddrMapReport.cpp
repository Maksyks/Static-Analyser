// plugins/AddrMap/AddrMapReport.cpp
#include "AddrMapReport.h"

#include <QStringList>

namespace addrmap {

static QString addrStr(int a) {
    if (a == 0) return QStringLiteral("NULL");
    return QStringLiteral("a%1").arg(a);
}

static QString fieldsStr(const QMap<QString,int>& f) {
    QStringList parts;
    for (auto it = f.begin(); it != f.end(); ++it) {
        parts << QStringLiteral("%1->%2").arg(it.key(), addrStr(it.value()));
    }
    return QStringLiteral("{%1}").arg(parts.join(QStringLiteral(", ")));
}

static QString unequalStr(const QSet<int>& s) {
    QStringList parts;
    for (int a : s)
        parts << addrStr(a);
    return QStringLiteral("{%1}").arg(parts.join(QStringLiteral(",")));
}

QString dumpReport(const State& st, bool valid) {
    QString out;
    out += QStringLiteral("== AddressMapper report ==\n");
    out += QStringLiteral("Status: %1\n").arg(valid ? QStringLiteral("VALID") : QStringLiteral("INVALID"));
    if (!valid) {
        for (const QString& e : st.errors)
            out += QStringLiteral("  error: %1\n").arg(e);
    }

    out += QStringLiteral("\n[g] Environment (var -> addr)\n");
    for (auto it = st.env.begin(); it != st.env.end(); ++it)
        out += QStringLiteral("  %1 -> %2\n").arg(it.key(), addrStr(it.value()));

    out += QStringLiteral("\n[j] Memory (addr: {field->addr} | class active null | unequal)\n");
    for (auto it = st.mem.begin(); it != st.mem.end(); ++it) {
        const Cell& c = it.value();
        out += QStringLiteral("  %1: %2 | class=%3 active=%4 null=%5 unequal=%6\n")
                   .arg(addrStr(c.id))
                   .arg(fieldsStr(c.fields))
                   .arg(c.classI ? QStringLiteral("I") : QStringLiteral("II"))
                   .arg(c.active ? 1 : 0)
                   .arg(c.nullFlag)
                   .arg(unequalStr(c.unequal));
    }

    out += QStringLiteral("\n[c] Solution (class I only, shadow of ψ)\n");
    for (auto it = st.solutionC.begin(); it != st.solutionC.end(); ++it) {
        const Cell& c = it.value();
        out += QStringLiteral("  %1: %2\n").arg(addrStr(c.id), fieldsStr(c.fields));
    }

    out += QStringLiteral("\n[constraints]\n");
    for (const QString& c : st.constraints)
        out += QStringLiteral("  %1\n").arg(c);

    return out;
}

} // namespace addrmap
