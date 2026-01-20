// core/plugins/CommandDescriptor.h
#pragma once
#include <QString>
#include <QVariantMap>

struct ParamSpec {
    enum Kind { Identifier, Text, Choice, LineNumber };
    QString key;        // например: "variable"
    QString label;      // "Переменная"
    Kind kind;          // ParamSpec::Identifier
    QStringList choices; // для Choice
    bool editable = true; // можно ли вручную вписать своё
    bool optional = false;
};

struct CommandDescriptor {
    QString id;           // "slice.run"
    QString title;        // "Сделать срез…"
    QString menuPath;     // "Editor/Context/"
    QString shortcut;     // "Ctrl+Alt+S"
    QString when;         // условие видимости (expr на EditorContext) — можно оставить пустым
    QString kind;         // какой анализ запустить ("slice.v1")
    QVariantMap defaultParams; // дефолтные параметры анализа
    QVector<ParamSpec> paramSpecs;
};

