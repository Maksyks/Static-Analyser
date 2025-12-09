// plugins/AddrMap/AddrMapPlugin.h
#pragma once
#include <QObject>
#include <QSet>
#include <QMap>
#include <QString>
#include "plugins/IPlugin.h"

class AddrMapPlugin : public QObject, public IPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID IPlugin_iid FILE "AddrMapPlugin.json")
    Q_INTERFACES(IPlugin)
public:
    QString id()   const override { return "addrmap"; }
    QString name() const override { return "Address Mapper"; }
    QStringList supportedKinds() const override { return { "addrmap.v1" }; }
    QList<CommandDescriptor> commands() const override;
    QVariant runAnalysis(QString kind,
                         const EditorContext& ctx,
                         const QVariantMap& params) override;
private:
    QString analyze(const QString& code) const;

    struct Cell {
        int id = 0;
        bool classI = false;   // входной адрес
        bool active = true;    // не освобождён
        int  nullFlag = 0;     // 0=по умолчанию, 1=обязательно NULL, 2=обязательно не-NULL
        QSet<int> unequal;     // запреты псевдонима
        QMap<QString,int> fields; // имя поля -> адрес (0 == NULL)
    };

    struct State {
        int nextAddr = 1;
        QMap<int,Cell> mem;            // ψ : адрес -> ячейка
        QMap<QString,int> env;         // γ : переменная -> адрес (0 == NULL)
        QMap<int,Cell> solutionC;      // χ : только класс I, актуальная «тень» ψ
        QStringList constraints;       // диагностические записи ограничений
        QStringList errors;            // причины неосуществимости
    };

    static QString trimComment(const QString& line);
    static bool    isNullLike(const QString& t);

    // === Базовые операции над памятью ===
    int  createCell(State& st, bool classI) const;
    int  ensureVar(State& st, const QString& var) const; // создаёт класс I при первом ИСПОЛЬЗОВАНИИ (не при объявлении)
    int  ensureField(State& st, int addr, const QString& field, bool nextIsClassI) const;

    // === Трассировка цепочки ===
    // createIfClassI: создавать недостающие узлы class I при разыменовании
    // createBaseIfMissing: создавать сам "base"-узел переменной, если его ещё нет
    // markNonNull: помечать все пройденные узлы null=2
    int  useChain(State& st, const QStringList& chain,
                 bool createIfClassI=true,
                 bool createBaseIfMissing=true,
                 bool markNonNull=true) const;

    // Просмотр цепочки БЕЗ модификаций и без автосоздания (для CONSTRAINTS)
    int  peekChain(const State& st, const QStringList& chain) const;

    // === χ ← ψ синхронизация для class I ===
    void mirrorFieldWriteToSolution(State& st, int baseAddr, const QString& field, int targetAddr) const; // NEW
    void mirrorEnsureChildInSolution(State& st, int parentAddr, const QString& field, int childAddr) const; // NEW

    // === Ограничения ===
    void setNullFlag(State& st, int addr, int flag) const;
    void addUnequal(State& st, int a, int b) const; // NEW

    // === Парсинг выражений-цепочек ===
    static QStringList parseChainExpr(const QString& expr); // NEW (x  |  x->f  |  x->f->g)

    QString dumpReport(const State& st, bool valid) const;
};
