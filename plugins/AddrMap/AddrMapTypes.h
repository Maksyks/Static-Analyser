// plugins/AddrMap/AddrMapTypes.h
#pragma once

#include <QMap>
#include <QSet>
#include <QString>
#include <QStringList>

namespace addrmap {

struct Cell {
    int id = 0;
    bool classI = false;  // class I (input/reserved) vs class II (heap)
    bool active = true;   // still allocated
    int nullFlag = 0;     // 0 unknown, 1 NULL, 2 non-NULL
    QSet<int> unequal;    // набор неравных друг другу адресов
    QMap<QString, int> fields; // field -> addr (0 == NULL)
};

struct State {
    int nextAddr = 1;
    QMap<int, Cell> mem;       // ψ
    QMap<QString, int> env;    // γ (var -> addr)

    QMap<int, Cell> solutionC; // χ (shadow of ψ for class I only)

    QStringList constraints;
    QStringList errors;
};

struct ChainValue {
    bool ok = false;
    int value = 0; // 0 == NULL
};

} // namespace addrmap
