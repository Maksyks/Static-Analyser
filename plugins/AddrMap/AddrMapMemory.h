// plugins/AddrMap/AddrMapMemory.h
#pragma once

#include "AddrMapTypes.h"

namespace addrmap {

int createCell(State& st, bool classI);
int ensureVar(State& st, const QString& var);
int ensureField(State& st, int baseAddr, const QString& field, bool nextIsClassI = true);

void ensureCellInSolution(State& st, int addr);
void mirrorFieldWriteToSolution(State& st, int baseAddr, const QString& field, int targetAddr);

void setNullFlag(State& st, int addr, int flag);

// Добавляет неравенство a != b.
// Возвращает false, если обнаружено противоречие.
bool addUnequal(State& st, int a, int b, const QString& origin);

// USE(γ, ψ, chain): гарантирует, что цепочка разыменований существует и заканчивается
// НЕ-NULL адресом (иначе возвращает 0).
int useChain(State& st, const QStringList& chain,
             bool createIfClassI = true,
             bool createBaseIfMissing = true,
             bool markNonNull = true);

// PEEK: безопасно пройти по цепочке без создания резервных адресов.
// Возвращает 0, если цепочка неразрешима.
int peekChain(const State& st, const QStringList& chain);

// EVAL: вычисляет значение цепочки. Для последнего шага допускает NULL.
ChainValue evalChainValue(State& st, const QStringList& chain,
                          bool createIfClassI = true,
                          bool createBaseIfMissing = true,
                          bool markNonNull = true);

} // namespace addrmap
