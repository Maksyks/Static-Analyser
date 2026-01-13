// plugins/AddrMap/AddrMapMemory.cpp
#include "AddrMapMemory.h"

#include <QtGlobal>

namespace addrmap {

int createCell(State& st, bool classI) {
    const int id = st.nextAddr++;
    Cell c;
    c.id = id;
    c.classI = classI;
    c.active = true;
    c.nullFlag = 0;
    st.mem[id] = c;
    if (classI)
        st.solutionC[id] = c;
    return id;
}

int ensureVar(State& st, const QString& var) {
    int addr = st.env.value(var, 0);
    if (addr == 0) {
        addr = createCell(st, /*classI*/ true);
        st.env[var] = addr;
    }
    return addr;
}

void ensureCellInSolution(State& st, int addr) {
    if (!st.mem.contains(addr)) return;
    const Cell& c = st.mem[addr];
    if (!c.classI) return;
    if (!st.solutionC.contains(addr))
        st.solutionC[addr] = c;
}

int ensureField(State& st, int baseAddr, const QString& field, bool nextIsClassI) {
    if (!st.mem.contains(baseAddr))
        return 0;
    Cell& c = st.mem[baseAddr];
    int next = c.fields.value(field, 0);
    if (next == 0) {
        next = createCell(st, nextIsClassI);
        c.fields[field] = next;
        if (c.classI)
            mirrorFieldWriteToSolution(st, baseAddr, field, next);
    }
    return next;
}

void mirrorFieldWriteToSolution(State& st, int baseAddr, const QString& field, int targetAddr) {
    if (!st.mem.contains(baseAddr))
        return;
    const Cell& base = st.mem[baseAddr];
    if (!base.classI)
        return;
    ensureCellInSolution(st, baseAddr);
    st.solutionC[baseAddr].fields[field] = targetAddr;
}

void setNullFlag(State& st, int addr, int flag) {
    if (!st.mem.contains(addr)) return;
    Cell& c = st.mem[addr];

    // согласование: 2 (не-NULL) конфликтует с 1 (NULL)
    if ((c.nullFlag == 1 && flag == 2) || (c.nullFlag == 2 && flag == 1)) {
        st.errors << QStringLiteral("Противоречивое ограничение NULL по адресу a%1").arg(addr);
        return;
    }
    c.nullFlag = flag;
    if (c.classI) {
        ensureCellInSolution(st, addr);
        st.solutionC[addr].nullFlag = flag;
    }
}

bool addUnequal(State& st, int a, int b, const QString& origin) {
    if (a == 0 || b == 0)
        return true; // NULL мы не кодируем адресом, игнор
    if (a == b) {
        if (!origin.isEmpty())
            st.errors << QStringLiteral("%1: противоречие, требуется a%2 != a%2").arg(origin).arg(a);
        else
            st.errors << QStringLiteral("Противоречие: требуется a%1 != a%1").arg(a);
        return false;
    }
    if (!st.mem.contains(a) || !st.mem.contains(b))
        return true;
    st.mem[a].unequal.insert(b);
    st.mem[b].unequal.insert(a);
    if (st.mem[a].classI) {
        ensureCellInSolution(st, a);
        st.solutionC[a].unequal.insert(b);
    }
    if (st.mem[b].classI) {
        ensureCellInSolution(st, b);
        st.solutionC[b].unequal.insert(a);
    }
    return true;
}

int useChain(State& st, const QStringList& chain,
             bool createIfClassI,
             bool createBaseIfMissing,
             bool markNonNull)
{
    if (chain.isEmpty()) return 0;
    int cur = 0;
    if (createBaseIfMissing)
        cur = ensureVar(st, chain.front());
    else
        cur = st.env.value(chain.front(), 0);
    if (cur == 0) return 0;

    if (markNonNull)
        setNullFlag(st, cur, 2);

    for (int i = 1; i < chain.size(); ++i) {
        const QString f = chain[i];
        if (!st.mem.contains(cur)) return 0;
        Cell& c = st.mem[cur];
        if (!c.active) return 0;
        if (c.nullFlag == 1) return 0;
        if (markNonNull && c.nullFlag != 2)
            setNullFlag(st, c.id, 2);

        int next = c.fields.value(f, 0);
        if (next == 0 && c.classI && createIfClassI) {
            next = ensureField(st, c.id, f, /*nextIsClassI*/ true);
        }
        if (next == 0)
            return 0; // NULL или отсутствует поле у class II
        cur = next;
    }
    return cur;
}

int peekChain(const State& st, const QStringList& chain) {
    if (chain.isEmpty()) return 0;
    int cur = st.env.value(chain.front(), 0);
    if (cur == 0) return 0;
    for (int i = 1; i < chain.size(); ++i) {
        if (!st.mem.contains(cur)) return 0;
        const Cell& c = st.mem.value(cur);
        if (!c.active) return 0;
        if (c.nullFlag == 1) return 0;
        int next = c.fields.value(chain[i], 0);
        if (next == 0) return 0;
        cur = next;
    }
    return cur;
}

ChainValue evalChainValue(State& st, const QStringList& chain,
                          bool createIfClassI,
                          bool createBaseIfMissing,
                          bool markNonNull)
{
    ChainValue r;
    if (chain.isEmpty())
        return r;

    int cur = 0;
    if (createBaseIfMissing)
        cur = ensureVar(st, chain.front());
    else
        cur = st.env.value(chain.front(), 0);

    // чтение указателя без разыменования
    if (chain.size() == 1) {
        r.ok = true;
        r.value = cur;
        return r;
    }

    // цепочка длиной >1 требует разыменования base
    if (cur == 0)
        return r;

    if (markNonNull)
        setNullFlag(st, cur, 2);

    for (int i = 1; i < chain.size(); ++i) {
        const QString f = chain[i];
        if (!st.mem.contains(cur))
            return r;
        Cell& c = st.mem[cur];
        if (!c.active)
            return r;
        if (c.nullFlag == 1)
            return r;
        if (markNonNull && c.nullFlag != 2)
            setNullFlag(st, c.id, 2);

        int next = c.fields.value(f, 0);
        if (next == 0 && c.classI && createIfClassI) {
            next = ensureField(st, c.id, f, /*nextIsClassI*/ true);
        }

        const bool isLast = (i == chain.size() - 1);
        if (isLast) {
            r.ok = true;
            r.value = next; // может быть 0 (NULL)
            return r;
        }
        if (next == 0)
            return r;
        cur = next;
    }

    // unreachable
    r.ok = true;
    r.value = cur;
    return r;
}

} // namespace addrmap
