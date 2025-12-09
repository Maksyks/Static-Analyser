#ifndef EDITORCONTEXT_H
#define EDITORCONTEXT_H

#pragma once
#include <QSet>
#include <QString>

struct EditorContext {
    int line1based = 0;          // номер строки
    QString lineText;            // текст строки
    QSet<QString> identifiers;   // идентификаторы, найденные в строке
    QString documentText;        // весь текст документа
};


#endif // EDITORCONTEXT_H
