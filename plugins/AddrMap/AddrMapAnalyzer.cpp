// plugins/AddrMap/AddrMapAnalyzer.cpp
#include "AddrMapAnalyzer.h"

#include "AddrMapMemory.h"
#include "AddrMapReport.h"
#include "AddrMapUtils.h"

#include <QMap>
#include <QMultiMap>
#include <QRegularExpression>
#include <QVector>

namespace addrmap {

QString AddrMapAnalyzer::analyze(const QString& code) const {
    State st;

    //Regex набор (простой парсер по строкам)
    const QRegularExpression reDeclPtr(R"(^\s*\w+\s*\*\s*([A-Za-z_]\w*)\s*;)");
    // Pointer declaration with initializer: T* x = y;
    // (needed for: Node *p = head;)
    const QRegularExpression reDeclInitAlias(
        R"(^\s*(?:[A-Za-z_]\w*\s+)*[A-Za-z_]\w*\s*\*\s*([A-Za-z_]\w*)\s*=\s*([A-Za-z_]\w*)\s*;)");

    const QRegularExpression reAssignAlias(R"(^\s*([A-Za-z_]\w*)\s*=\s*([A-Za-z_]\w*)\s*;)");
    const QRegularExpression reFieldAssign1(R"(^\s*([A-Za-z_]\w*)\s*->\s*([A-Za-z_]\w*)\s*=\s*([A-Za-z_]\w*|NULL|0|nullptr)\s*;)");
    const QRegularExpression reFieldAssignChain(R"(^\s*([A-Za-z_]\w*)\s*->\s*([A-Za-z_]\w*)\s*=\s*([A-Za-z_]\w*(?:\s*->\s*[A-Za-z_]\w*)+)\s*;)");
    const QRegularExpression reMalloc(R"(^\s*([A-Za-z_]\w*)\s*=\s*(?:\([^)]*\)\s*)?malloc\s*\()");
    const QRegularExpression reFree(R"(^\s*free\s*\(\s*([A-Za-z_]\w*)\s*\)\s*;)");
    const QRegularExpression reAssignNull(R"(^\s*([A-Za-z_]\w*)\s*=\s*(NULL|0|nullptr)\s*;)");

    const QRegularExpression reIfEqNull(R"(^\s*if\s*\(\s*([A-Za-z_]\w*)\s*==\s*(NULL|0|nullptr)\s*\))");
    const QRegularExpression reIfNeNull(R"(^\s*if\s*\(\s*([A-Za-z_]\w*)\s*!=\s*(NULL|0|nullptr)\s*\))");
    const QRegularExpression reIfChainEqNull(R"(^\s*if\s*\(\s*([A-Za-z_]\w*(?:\s*->\s*[A-Za-z_]\w*)*)\s*==\s*(NULL|0|nullptr)\s*\))");
    const QRegularExpression reIfChainNeNull(R"(^\s*if\s*\(\s*([A-Za-z_]\w*(?:\s*->\s*[A-Za-z_]\w*)*)\s*!=\s*(NULL|0|nullptr)\s*\))");

    const QRegularExpression reAssignChainGet(R"(^\s*([A-Za-z_]\w*)\s*=\s*([A-Za-z_]\w*(?:\s*->\s*[A-Za-z_]\w*)+)\s*;)");

    // провести анализ до main()
    const QRegularExpression reMainDecl(R"(^\s*(?:(?:static|extern)\s+)?(?:[A-Za-z_]\w*\s+)*main\s*\()");

    // if (x == q->right->left)
    const QRegularExpression reIfEqAny(R"(^\s*if\s*\(\s*([A-Za-z_]\w*(?:\s*->\s*[A-Za-z_]\w*)*)\s*==\s*([A-Za-z_]\w*(?:\s*->\s*[A-Za-z_]\w*)*)\s*\))");
    const QRegularExpression reIfNeAny(R"(^\s*if\s*\(\s*([A-Za-z_]\w*(?:\s*->\s*[A-Za-z_]\w*)*)\s*!=\s*([A-Za-z_]\w*(?:\s*->\s*[A-Za-z_]\w*)*)\s*\))");

    // Линейное представление исходного кода (main() пропускаем, чтобы не засорять репорт)
    struct LineInfo {
        int index = 0;   // индекс в анализируемом "потоке" строк
        int lineNo = 0;  // оригинальный номер строки в исходнике
        QString text;
    };

    QVector<LineInfo> lines;
    lines.reserve(code.count('\n') + 1);

    const auto braceDelta = [](const QString& s) -> int {
        int d = 0;
        for (const QChar ch : s) {
            if (ch == QChar('{')) ++d;
            else if (ch == QChar('}')) --d;
        }
        return d;
    };

    bool mainSigSeen = false; // увидели строку с "int main(" и ждём '{'
    bool inMain = false;      // внутри тела main()
    int mainDepth = 0;        // баланс скобок для тела main()

    int lineNo = 0;
    int idx = 0;
    for (const QString& raw : code.split('\n')) {
        ++lineNo;
        const QString line = norm(trimComment(raw));

        if (!inMain) {
            if (mainSigSeen) {
                // ждём открытия тела main()
                if (line.contains('{') || line.startsWith('{')) {
                    inMain = true;
                    mainSigSeen = false;
                    mainDepth += braceDelta(line);
                    if (mainDepth <= 0) { // main() может закрыться на той же строке
                        inMain = false;
                        mainDepth = 0;
                    }
                }
                continue; // пропускаем строки сигнатуры/пустые строки перед '{'
            }

            if (reMainDecl.match(line).hasMatch()) {
                // сигнатура main() — пропускаем и либо входим в тело (если '{' здесь),
                // либо ждём '{' на следующих строках
                if (line.contains('{')) {
                    inMain = true;
                    mainDepth += braceDelta(line);
                    if (mainDepth <= 0) {
                        inMain = false;
                        mainDepth = 0;
                    }
                } else {
                    mainSigSeen = true;
                }
                continue;
            }

            LineInfo li;
            li.index = idx++;
            li.lineNo = lineNo;
            li.text = line;
            lines.push_back(li);
        } else {
            // внутри main(): пропускаем, но отслеживаем закрывающую '}'
            mainDepth += braceDelta(line);
            if (mainDepth <= 0) {
                inMain = false;
                mainDepth = 0;
            }
            continue;
        }
    }

    // После удаления main() анализируем весь оставшийся код.
    const int mainStartIdx = lines.size();

    auto seedInputPointerParams = [&]() {
        auto startsWithKw = [](const QString& s, const char* kw) -> bool {
            const QString k = QString::fromLatin1(kw);
            if (!s.startsWith(k)) return false;
            if (s.size() == k.size()) return true;
            const QChar ch = s[k.size()];
            return ch.isSpace() || ch == '(';
        };

        for (int i = 0; i < lines.size() && i < mainStartIdx; ++i) {
            const QString text = lines[i].text;
            if (text.isEmpty()) continue;
            if (startsWithKw(text, "if") || startsWithKw(text, "for") || startsWithKw(text, "while") ||
                startsWithKw(text, "switch") || startsWithKw(text, "return")) {
                continue;
            }

            const int lparen = text.indexOf('(');
            const int rparen = text.lastIndexOf(')');
            if (lparen < 0 || rparen < lparen) continue;

            bool isDef = text.contains('{');
            if (!isDef) {
                for (int j = i + 1; j < lines.size() && j < mainStartIdx; ++j) {
                    const QString next = lines[j].text;
                    if (next.isEmpty()) continue;
                    if (next.startsWith('{')) isDef = true;
                    break;
                }
            }
            if (!isDef) continue;

            const QString params = text.mid(lparen + 1, rparen - lparen - 1).trimmed();
            if (params.isEmpty() || params == QStringLiteral("void"))
                continue;

            const QRegularExpression reLastIdent(R"(([A-Za-z_]\w*)\s*$)");
            for (const QString& rawParam : params.split(',', Qt::SkipEmptyParts)) {
                const QString p = rawParam.trimmed();
                if (!p.contains('*')) continue;

                const auto mm = reLastIdent.match(p);
                if (!mm.hasMatch()) continue;
                const QString name = mm.captured(1);
                if (name.isEmpty()) continue;
                if (isNullLike(name)) continue;

                (void)ensureVar(st, name);
            }
        }
    };
    seedInputPointerParams();

    // 1-й проход: SymTab + AliasTab (препроцессор алиасов)
    struct SymEntry {
        int varDef = -1;            // последняя строка, где var присваивали
        QMap<QString, int> fieldDef; // последняя строка, где var->field присваивали (1-й уровень)
    };
    QMap<QString, SymEntry> symTab;

    struct Chain { QString base; QStringList f; };
    auto parseChain = [&](const QString& s) -> Chain {
        const QStringList parts = parseChainExpr(s);
        Chain c;
        if (parts.isEmpty()) return c;
        c.base = parts.front();
        c.f = parts.mid(1);
        return c;
    };

    struct AliasOp {
        int placeIndex = 0; // индекс строки, ПЕРЕД которой выполнить alias
        Chain lhs;          // кого «привязать» (переменная ИЛИ поле-цепочка)
        Chain rhs;          // к кому привязать (цепочка)
        int condLineNo = 0; // номер условной строки для лога
    };
    QVector<AliasOp> aliasOps;

    // Собираем определения и планируем alias’ы
    for (const LineInfo& li : lines) {
        if (li.index >= mainStartIdx) break;
        const QString& line = li.text;
        if (line.isEmpty()) continue;

        // T* x = y;
        if (auto m = reDeclInitAlias.match(line); m.hasMatch()) {
            const QString x = m.captured(1);
            symTab[x].varDef = li.index;
            continue;
        }

        // x = malloc(...)
        if (auto m = reMalloc.match(line); m.hasMatch()) {
            const QString v = m.captured(1);
            symTab[v].varDef = li.index;
            continue;
        }

        // x = y;
        if (auto m = reAssignAlias.match(line); m.hasMatch()) {
            const QString x = m.captured(1);
            symTab[x].varDef = li.index;
            continue;
        }

        // x = y->f->g; (любой уровень >= 1)
        if (auto m = reAssignChainGet.match(line); m.hasMatch()) {
            const QString x = m.captured(1);
            symTab[x].varDef = li.index;
            continue;
        }

        // y->f = z;
        if (auto m = reFieldAssign1.match(line); m.hasMatch()) {
            const QString y = m.captured(1);
            const QString f = m.captured(2);
            symTab[y].fieldDef[f] = li.index;
            continue;
        }

        // y->f = x->g->h;
        if (auto m = reFieldAssignChain.match(line); m.hasMatch()) {
            const QString y = m.captured(1);
            const QString f = m.captured(2);
            symTab[y].fieldDef[f] = li.index;
            continue;
        }

        // x = NULL;
        if (auto m = reAssignNull.match(line); m.hasMatch()) {
            const QString v = m.captured(1);
            symTab[v].varDef = li.index;
            continue;
        }

        // if (expr == expr) — общий случай (цепочки)
        if (auto m = reIfEqAny.match(line); m.hasMatch()) {
            const QString Lraw = m.captured(1).trimmed();
            const QString Rraw = m.captured(2).trimmed();
            if (isNullLike(Lraw) || isNullLike(Rraw)) {
                // сравнение с NULL не превращаем в alias; это обработается как NULL-constraint во 2-м проходе
                continue;
            }
            const Chain L = parseChain(m.captured(1));
            const Chain R = parseChain(m.captured(2));

            // вычислим «последнюю модификацию» для баз
            auto lastDef = [&](const Chain& C) -> int {
                int last = symTab.contains(C.base) ? symTab[C.base].varDef : -1;
                if (!C.f.isEmpty()) {
                    const QString f0 = C.f.front();
                    if (symTab.contains(C.base) && symTab[C.base].fieldDef.contains(f0))
                        last = std::max(last, symTab[C.base].fieldDef[f0]);
                }
                return last;
            };

            const int lastL = lastDef(L);
            const int lastR = lastDef(R);
            int place = std::max(lastL, lastR);

            if (place < 0) {
                place = li.index;
            } else {
                place = place + 1;
                if (place > li.index)
                    place = li.index;
            }
            const int maxIdx = std::max(0, int(lines.size()) - 1);
            place = std::clamp(place, 0, maxIdx);

            AliasOp op;
            op.placeIndex = place;
            op.lhs = L;
            op.rhs = R;
            op.condLineNo = li.lineNo;
            aliasOps.push_back(op);
            continue;
        }
    }

    // индекс строки -> alias-операции
    QMultiMap<int, AliasOp> aliasBefore;
    for (const AliasOp& op : aliasOps)
        aliasBefore.insert(op.placeIndex, op);

    // 2-й проход: AddressMapper + выполнение aliasBefore
    for (const LineInfo& li : lines) {
        if (li.index >= mainStartIdx) break;
        const int idx = li.index;
        const int lineNo = li.lineNo;
        const QString& line = li.text;

        // 2.1. сначала выполняем alias’ы, запланированные перед этой строкой
        auto range = aliasBefore.equal_range(idx);
        for (auto it = range.first; it != range.second; ++it) {
            const AliasOp& op = it.value();

            auto toList = [&](const Chain& C) -> QStringList {
                QStringList r;
                if (C.base.isEmpty()) return r;
                r << C.base;
                for (const auto& x : C.f)
                    r << x;
                return r;
            };

            const QString rhsExpr = op.rhs.base + (op.rhs.f.isEmpty() ? QString() : QStringLiteral("->") + op.rhs.f.join(QStringLiteral("->")));
            const QString lhsExpr = op.lhs.base + (op.lhs.f.isEmpty() ? QString() : QStringLiteral("->") + op.lhs.f.join(QStringLiteral("->")));

            // адрес-источник (rhs) — вычисляем значение цепочки (на последнем шаге может быть NULL)
            const auto rhsRes = evalChainValue(st, toList(op.rhs),
                                               /*createIfClassI*/ true,
                                               /*createBaseIfMissing*/ true,
                                               /*markNonNull*/ true);
            if (!rhsRes.ok) {
                st.errors << QStringLiteral("[%1] alias rhs недоступен (%2)").arg(op.condLineNo).arg(rhsExpr);
                continue;
            }
            const int rhsAddr = rhsRes.value;

            auto isUnequal = [&](int a, int b) -> bool {
                if (a == 0 || b == 0) return false;
                if (!st.mem.contains(a) || !st.mem.contains(b)) return false;
                return st.mem[a].unequal.contains(b) || st.mem[b].unequal.contains(a);
            };

            if (op.lhs.f.isEmpty()) {
                // lhs — переменная
                const int lhsAddrCur = st.env.value(op.lhs.base, 0);
                if (isUnequal(lhsAddrCur, rhsAddr)) {
                    st.errors << QStringLiteral("[%1] alias(%2 := %3) противоречит неравенству")
                                     .arg(op.condLineNo).arg(lhsExpr, rhsExpr);
                }
                st.env[op.lhs.base] = rhsAddr;
            } else {
                // lhs — цепочка поля: p->f1->..->fk := rhs
                QStringList lhsBasePath;
                lhsBasePath << op.lhs.base;
                for (int i = 0; i + 1 < op.lhs.f.size(); ++i)
                    lhsBasePath << op.lhs.f[i];

                const int baseAddr = useChain(st, lhsBasePath,
                                              /*createIfClassI*/ true,
                                              /*createBaseIfMissing*/ true,
                                              /*markNonNull*/ true);
                if (baseAddr == 0 || !st.mem.contains(baseAddr) || !st.mem[baseAddr].active) {
                    st.errors << QStringLiteral("[%1] alias lhs база недоступна (%2)")
                                     .arg(op.condLineNo).arg(lhsExpr);
                    continue;
                }
                const QString lastField = op.lhs.f.back();
                const int existing = st.mem[baseAddr].fields.value(lastField, 0);
                if (isUnequal(existing, rhsAddr)) {
                    st.errors << QStringLiteral("[%1] alias(%2 := %3) противоречит неравенству")
                                     .arg(op.condLineNo).arg(lhsExpr, rhsExpr);
                }
                st.mem[baseAddr].fields[lastField] = rhsAddr;
                mirrorFieldWriteToSolution(st, baseAddr, lastField, rhsAddr); // χ ← ψ
            }

            st.constraints << QStringLiteral("[%1] alias(%2 := %3) [preprocess]")
                                  .arg(op.condLineNo)
                                  .arg(lhsExpr, rhsExpr);
        }

        if (line.isEmpty())
            continue;

        // САМИ ОПЕРАЦИИ

        // T* x = y;
        if (auto m = reDeclInitAlias.match(line); m.hasMatch()) {
            const QString x = m.captured(1);
            const QString y = m.captured(2);

            if (x != y) {
                if (isNullLike(y)) {
                    st.env[x] = 0;
                    st.constraints << QStringLiteral("[%1] %2 = NULL").arg(lineNo).arg(x);
                } else {
                    const int rhs = ensureVar(st, y);
                    const int lhs = st.env.value(x, 0);
                    if (lhs != 0 && rhs != 0 && st.mem.contains(lhs) && st.mem[lhs].unequal.contains(rhs)) {
                        st.errors << QStringLiteral("[%1] %2 := %3 противоречит неравенству").arg(lineNo).arg(x, y);
                    }
                    st.env[x] = rhs;
                }
            }
            continue;
        }

        // объявление указателя: T* x;
        if (auto m = reDeclPtr.match(line); m.hasMatch()) {
            (void)m;
            continue;
        }

        // x = malloc(.);  Create class II
        if (auto m = reMalloc.match(line); m.hasMatch()) {
            const QString v = m.captured(1);
            const int addr = createCell(st, /*classI*/ false);
            st.env[v] = addr;
            continue;
        }

        // free(x); Delete
        if (auto m = reFree.match(line); m.hasMatch()) {
            const QString v = m.captured(1);
            const int a = st.env.value(v, 0);
            if (a == 0) {
                st.errors << QStringLiteral("[%1] free(NULL)").arg(lineNo);
                continue;
            }
            if (st.mem.contains(a))
                st.mem[a].active = false;
            continue;
        }

        // x = NULL; Modify
        if (auto m = reAssignNull.match(line); m.hasMatch()) {
            const QString v = m.captured(1);
            const int cur = st.env.value(v, 0);
            if (cur != 0 && st.mem.contains(cur) && st.mem[cur].nullFlag == 2) {
                st.errors << QStringLiteral("[%1] попытка присвоить NULL узлу, помеченному non-NULL").arg(lineNo);
            }
            st.env[v] = 0;
            st.constraints << QStringLiteral("[%1] %2 = NULL").arg(lineNo).arg(v);
            continue;
        }

        // if (p->f->g == NULL) / if (p->f->g != NULL) — ограничения на NULL для цепочки
        if (auto m = reIfChainEqNull.match(line); m.hasMatch()) {
            const QString expr = m.captured(1).trimmed();
            QStringList chain = parseChainExpr(expr);
            if (chain.size() >= 2) {
                const QString lastField = chain.takeLast();
                const int base = useChain(st, chain,
                                          /*createIfClassI*/ true,
                                          /*createBaseIfMissing*/ true,
                                          /*markNonNull*/ true);
                if (base == 0 || !st.mem.contains(base) || !st.mem[base].active) {
                    st.errors << QStringLiteral("[%1] constraint %2 == NULL невозможно").arg(lineNo).arg(expr);
                } else {
                    st.mem[base].fields[lastField] = 0;
                    mirrorFieldWriteToSolution(st, base, lastField, 0);
                }
                st.constraints << QStringLiteral("[%1] %2 == NULL").arg(lineNo).arg(expr);
                continue;
            }
        }

        if (auto m = reIfChainNeNull.match(line); m.hasMatch()) {
            const QString expr = m.captured(1).trimmed();
            QStringList chain = parseChainExpr(expr);
            if (chain.size() >= 2) {
                const QString lastField = chain.takeLast();
                const int base = useChain(st, chain,
                                          /*createIfClassI*/ true,
                                          /*createBaseIfMissing*/ true,
                                          /*markNonNull*/ true);
                if (base == 0 || !st.mem.contains(base) || !st.mem[base].active) {
                    st.errors << QStringLiteral("[%1] constraint %2 != NULL невозможно").arg(lineNo).arg(expr);
                } else {
                    int cur = st.mem[base].fields.value(lastField, 0);
                    if (cur == 0) {
                        int child = 0;
                        if (st.mem[base].classI) {
                            child = ensureField(st, base, lastField, /*nextIsClassI*/ true);
                        } else {
                            child = createCell(st, /*classI*/ false);
                        }

                        if (child == 0) {
                            st.errors << QStringLiteral("[%1] %2 != NULL: не удалось создать адрес").arg(lineNo).arg(expr);
                        } else {
                            st.mem[base].fields[lastField] = child;
                            mirrorFieldWriteToSolution(st, base, lastField, child);
                            setNullFlag(st, child, 2);
                        }
                    } else {
                        setNullFlag(st, cur, 2);
                    }
                }
                st.constraints << QStringLiteral("[%1] %2 != NULL").arg(lineNo).arg(expr);
                continue;
            }
        }

        // if (x == NULL) / if (x != NULL) — свойства null (CONSTRAINTS)
        if (auto m = reIfEqNull.match(line); m.hasMatch()) {
            const QString var = m.captured(1);
            const int a = st.env.value(var, 0);
            if (a != 0)
                setNullFlag(st, a, 1);
            st.constraints << QStringLiteral("[%1] %2 == NULL").arg(lineNo).arg(var);
            continue;
        }

        if (auto m = reIfNeNull.match(line); m.hasMatch()) {
            const QString var = m.captured(1);
            const int a = st.env.value(var, 0);
            if (a == 0) {
                st.errors << QStringLiteral("[%1] %2 != NULL, но %2 уже NULL").arg(lineNo).arg(var);
            } else {
                setNullFlag(st, a, 2);
            }
            st.constraints << QStringLiteral("[%1] %2 != NULL").arg(lineNo).arg(var);
            continue;
        }

        // if (expr != expr) — CONSTRAINT INEQUALITY
        if (auto m = reIfNeAny.match(line); m.hasMatch()) {
            const QString Lexpr = m.captured(1);
            const QString Rexpr = m.captured(2);
            const QStringList L = parseChainExpr(Lexpr);
            const QStringList R = parseChainExpr(Rexpr);
            const int a = peekChain(st, L);
            const int b = peekChain(st, R);
            if (a != 0 && b != 0)
                addUnequal(st, a, b, QStringLiteral("[%1] %2 != %3").arg(lineNo).arg(Lexpr, Rexpr));
            st.constraints << QStringLiteral("[%1] %2 != %3").arg(lineNo).arg(Lexpr, Rexpr);
            continue;
        }

        // if (expr == expr) — после препроцессора это только фиксация ограничения
        if (auto m = reIfEqAny.match(line); m.hasMatch()) {
            st.constraints << QStringLiteral("[%1] %2 == %3").arg(lineNo).arg(m.captured(1), m.captured(2));
            continue;
        }

        // x = y->f;  x = y->f->g;  ... (чтение значения цепочки)
        if (auto m = reAssignChainGet.match(line); m.hasMatch()) {
            const QString x = m.captured(1);
            const QString rhsExpr = m.captured(2);

            const auto rhs = evalChainValue(st, parseChainExpr(rhsExpr),
                                            /*createIfClassI*/ true,
                                            /*createBaseIfMissing*/ true,
                                            /*markNonNull*/ true);
            if (!rhs.ok) {
                st.errors << QStringLiteral("[%1] разыменование %2 невозможно").arg(lineNo).arg(rhsExpr);
                continue;
            }

            const int lhsAddr = st.env.value(x, 0);
            if (lhsAddr != 0 && rhs.value != 0 && st.mem.contains(lhsAddr) && st.mem[lhsAddr].unequal.contains(rhs.value)) {
                st.errors << QStringLiteral("[%1] %2 := %3 противоречит неравенству").arg(lineNo).arg(x, rhsExpr);
            }
            st.env[x] = rhs.value; // может быть 0 (NULL)
            continue;
        }

        // y->f = x->g->h;  (присваивание из цепочки)
        if (auto m = reFieldAssignChain.match(line); m.hasMatch()) {
            const QString y = m.captured(1);
            const QString f = m.captured(2);
            const QString rhsExpr = m.captured(3);

            const int base = useChain(st, QStringList{y});
            if (base == 0 || !st.mem.contains(base) || !st.mem[base].active) {
                st.errors << QStringLiteral("[%1] base %2 недоступен для присваивания").arg(lineNo).arg(y);
                continue;
            }

            const auto rhs = evalChainValue(st, parseChainExpr(rhsExpr),
                                            /*createIfClassI*/ true,
                                            /*createBaseIfMissing*/ true,
                                            /*markNonNull*/ true);
            if (!rhs.ok) {
                st.errors << QStringLiteral("[%1] RHS %2 недоступен").arg(lineNo).arg(rhsExpr);
                continue;
            }

            const int prev = st.mem[base].fields.value(f, 0);
            if (prev != 0 && rhs.value != 0 && st.mem.contains(prev) && st.mem[prev].unequal.contains(rhs.value)) {
                st.errors << QStringLiteral("[%1] %2->%3 := %4 противоречит неравенству").arg(lineNo).arg(y, f, rhsExpr);
            }

            st.mem[base].fields[f] = rhs.value;
            mirrorFieldWriteToSolution(st, base, f, rhs.value);
            continue;
        }

        // y->f = z / NULL;
        if (auto m = reFieldAssign1.match(line); m.hasMatch()) {
            const QString y = m.captured(1);
            const QString f = m.captured(2);
            const QString z = m.captured(3);

            const int base = useChain(st, QStringList{y},
                                      /*createIfClassI*/ true,
                                      /*createBaseIfMissing*/ true,
                                      /*markNonNull*/ true);
            if (base == 0 || !st.mem.contains(base) || !st.mem[base].active) {
                st.errors << QStringLiteral("[%1] запись в %2->%3 невозможна").arg(lineNo).arg(y, f);
                continue;
            }
            if (st.mem[base].nullFlag == 1) {
                st.errors << QStringLiteral("[%1] разыменование NULL (%2)").arg(lineNo).arg(y);
                continue;
            }

            int target = 0;
            if (!isNullLike(z)) {
                target = ensureVar(st, z);
                if (target == 0)
                    st.errors << QStringLiteral("[%1] RHS %2 == NULL?").arg(lineNo).arg(z);
            }

            const int prev = st.mem[base].fields.value(f, 0);
            if (prev != 0 && target != 0 && st.mem.contains(prev) && st.mem[prev].unequal.contains(target)) {
                st.errors << QStringLiteral("[%1] %2->%3 := %4 противоречит неравенству")
                                 .arg(lineNo).arg(y, f, z);
            }

            st.mem[base].fields[f] = target;
            mirrorFieldWriteToSolution(st, base, f, target);
            continue;
        }

        // x = y;  (явный alias)
        if (auto m = reAssignAlias.match(line); m.hasMatch()) {
            const QString x = m.captured(1);
            const QString y = m.captured(2);
            if (x != y) {
                const int rhs = ensureVar(st, y);
                const int lhs = st.env.value(x, 0); // не создаём до присваивания
                if (lhs != 0 && rhs != 0 && st.mem.contains(lhs) && st.mem[lhs].unequal.contains(rhs)) {
                    st.errors << QStringLiteral("[%1] %2 := %3 противоречит неравенству").arg(lineNo).arg(x, y);
                }
                st.env[x] = rhs;
            }
            continue;
        }

        // прочее игнорируем
    }

    const bool valid = st.errors.isEmpty();
    return dumpReport(st, valid);
}

} // namespace addrmap
