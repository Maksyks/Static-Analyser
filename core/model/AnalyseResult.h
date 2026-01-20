#pragma once
#include <QString>
#include <QMap>
#include <QPair>
#include <QMetaType>

struct AnalyseResult {
    QString custom;      // видимые строки исходника (срез)
    QString llvm2c;      // декомпилят
    QString dotVisible;  // DOT-граф «видимых строк»
    QString dotCfg;      // DOT CFG
    QMap<int, QPair<QString,int>> lineMap; // outLine -> {file, srcLine}
    QString addrmap;
};
Q_DECLARE_METATYPE(AnalyseResult)
