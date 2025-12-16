// plugins/AddrMap/AddrMapPlugin.cpp
#include "AddrMapPlugin.h"
#include <QRegularExpression>
#include <QStringList>
#include <QTextStream>
#include <QVector>
#include <QMultiMap>

QList<CommandDescriptor> AddrMapPlugin::commands() const {
    CommandDescriptor d;
    d.id = "addrmap.run";
    d.title = QObject::tr("Проанализировать срез (AddressMapper)");
    d.menuPath = "Editor/Context/AddressMapper";
    d.shortcut = "Ctrl+Alt+A";
    d.kind = "addrmap.v1";
    return { d };
}

QVariant AddrMapPlugin::runAnalysis(QString kind, const EditorContext& ctx, const QVariantMap& params) {
    Q_UNUSED(kind); Q_UNUSED(params);
    const QString code = ctx.documentText;
    return analyze(code);
}

// удаляем пробелы
static QString norm(const QString& s) {
    auto t = s; return t.trimmed();
}

// удаляем комменты
QString AddrMapPlugin::trimComment(const QString& line) {
    int p = line.indexOf("//");
    return (p>=0) ? line.left(p) : line;
}

// нормализация null
bool AddrMapPlugin::isNullLike(const QString& t) {
    const QString s = t.trimmed();
    return s.compare("NULL", Qt::CaseInsensitive) == 0 || s == "0" || s == "nullptr";
}

int AddrMapPlugin::createCell(State& st, bool classI) const {
    Cell c; c.id = st.nextAddr++; c.classI = classI; c.active = true; c.nullFlag = 0;
    st.mem[c.id] = c;
    if (classI) st.solutionC[c.id] = c; // создать «тень» узла
    return c.id;
}

int AddrMapPlugin::ensureVar(State& st, const QString& var) const {
    if (isNullLike(var)) return 0;

    // создаём адрес только если переменная вообще не встречалась
    if (!st.env.contains(var)) {
        int a = createCell(st, /*classI*/true);
        st.env[var] = a;
    }

    // если переменная была присвоена NULL, тут будет 0 — и это правильно
    return st.env.value(var, 0);
}


//синхронизация χ при появлении дочернего узла
void AddrMapPlugin::mirrorEnsureChildInSolution(State& st, int parentAddr, const QString& field, int childAddr) const {
    if (!st.mem.contains(parentAddr)) return;
    const auto& p = st.mem[parentAddr];
    if (!p.classI) return;
    if (!st.solutionC.contains(parentAddr)) return;
    auto &shadow = st.solutionC[parentAddr];
    // обновляем ссылку только если целевой — class I
    if (childAddr!=0 && st.mem.contains(childAddr) && st.mem[childAddr].classI) {
        shadow.fields[field] = childAddr;
    }
    if (childAddr==0) {
        shadow.fields[field] = 0;
    }
}

//синхронизация χ при присваивании поля
void AddrMapPlugin::mirrorFieldWriteToSolution(State& st, int baseAddr, const QString& field, int targetAddr) const {
    if (!st.mem.contains(baseAddr)) return;
    const auto& b = st.mem[baseAddr];
    if (!b.classI) return;                     // χ содержит только class I
    if (!st.solutionC.contains(baseAddr)) return;

    // class II в χ не попадает — оставляем предыдущее значение
    if (targetAddr!=0 && (!st.mem.contains(targetAddr) || !st.mem[targetAddr].classI))
        return;

    auto &shadow = st.solutionC[baseAddr];
    shadow.fields[field] = targetAddr;         // 0 (NULL) копируем, class I копируем
}

int AddrMapPlugin::ensureField(State& st, int addr, const QString& field, bool nextIsClassI) const {
    if (!st.mem.contains(addr)) return 0;
    Cell &c = st.mem[addr];
    if (!c.active) return 0;
    if (!c.fields.contains(field)) {
        if (!c.classI) {
            // класс II: неразрешённое разыменование без присваивания
            return 0;
        }
        int child = createCell(st, /*classI*/nextIsClassI);
        c.fields[field] = child;
        // NEW: поддержать χ как «тень»
        mirrorEnsureChildInSolution(st, addr, field, child);
    }
    return c.fields[field];
}

// parse "x" | "x->f" | "x->f->g"
QStringList AddrMapPlugin::parseChainExpr(const QString& expr) {
    QString e = expr.trimmed();
    // уберём внешние пробелы
    QStringList parts;
    for (const QString& t : e.split("->", Qt::SkipEmptyParts)) {
        parts << norm(t);
    }
    return parts;
}

// Пик без изменений (для CONSTRAINTS)
int AddrMapPlugin::peekChain(const State& st, const QStringList& chain) const {
    if (chain.isEmpty()) return 0;
    const QString base = chain.front();
    int cur = st.env.value(base, 0); // НЕ создаём
    if (cur==0) return 0;

    for (int i=1;i<chain.size();++i) {
        if (!st.mem.contains(cur)) return 0;
        const Cell& c = st.mem[cur];
        if (!c.active) return 0;
        if (c.nullFlag == 1) return 0;
        const QString f = chain[i];
        int next = c.fields.value(f, 0);
        if (next==0) return 0;
        cur = next;
    }
    return cur;
}

int AddrMapPlugin::useChain(State& st, const QStringList& chain,
                            bool createIfClassI,
                            bool createBaseIfMissing,
                            bool markNonNull) const {
    // chain: ["p","lt","rt", ...]
    if (chain.isEmpty()) return 0;
    int cur = 0;
    if (createBaseIfMissing) {
        cur = ensureVar(st, chain.front());
    } else {
        cur = st.env.value(chain.front(), 0);
    }
    if (cur == 0) return 0;

    // USE: base of a dereference chain must be non-NULL
    if (markNonNull) {
        setNullFlag(st, cur, 2);
    }

    for (int i=1;i<chain.size();++i) {
        const QString f = chain[i];
        if (!st.mem.contains(cur)) return 0;
        Cell& c = st.mem[cur];
        if (!c.active) return 0;
        if (c.nullFlag == 1) return 0;

        // стабильность разыменования: как только прошли через клетку — null=2
        if (markNonNull && c.nullFlag != 2) {
            // 2 конфликтует с 1 — setNullFlag проверит
            const_cast<AddrMapPlugin*>(this)->setNullFlag(st, c.id, 2);
        }

        int next = c.fields.value(f, 0);
        if (next==0 && c.classI && createIfClassI) {
            next = ensureField(st, c.id, f, /*nextIsClassI*/true);
        }
        if (next==0) return 0; // либо класс II, либо NULL — шаг невозможен
        cur = next;
    }
    return cur;
}

void AddrMapPlugin::setNullFlag(State& st, int addr, int flag) const {
    if (!st.mem.contains(addr)) return;
    Cell &c = st.mem[addr];
    // согласование: 2 (не-NULL) конфликтует с 1 (NULL)
    if (c.nullFlag != 0 && c.nullFlag != flag) {
        st.errors << QString("Противоречивое ограничение NULL по адресу a%1").arg(addr);
    }
    c.nullFlag = flag;
}

//добавить взаимное "не равны" и запрет алиаса
void AddrMapPlugin::addUnequal(State& st, int a, int b) const {
    if (a==0 || b==0) return;
    if (!st.mem.contains(a) || !st.mem.contains(b)) return;
    st.mem[a].unequal.insert(b);
    st.mem[b].unequal.insert(a);
}

QString AddrMapPlugin::dumpReport(const State& st, bool valid) const {
    QString out;
    QTextStream os(&out);

    os << "== AddressMapper report ==\n";
    os << "Status: " << (valid ? "VALID" : "INVALID") << "\n";
    if (!valid) {
        for (auto &e : st.errors) os << "  error: " << e << "\n";
    }
    os << "\n[g] Environment (var -> addr)\n";
    for (auto it=st.env.begin(); it!=st.env.end(); ++it) {
        os << "  " << it.key() << " -> " << (it.value()==0 ? "NULL" : QString("a%1").arg(it.value())) << "\n";
    }

    os << "\n[j] Memory (addr: {field->addr} | class active null | unequal)\n";
    for (auto it=st.mem.begin(); it!=st.mem.end(); ++it) {
        const auto &c = it.value();
        os << "  a" << c.id << ": {";
        bool first=true;
        for (auto fit=c.fields.begin(); fit!=c.fields.end(); ++fit) {
            if (!first) os << ", ";
            first=false;
            os << fit.key() << "->" << (fit.value()==0 ? "NULL" : QString("a%1").arg(fit.value()));
        }
        os << "} | class=" << (c.classI?"I":"II")
           << " active=" << (c.active?1:0)
           << " null=" << c.nullFlag
           << " unequal={";
        bool f2=true;
        for (int u : c.unequal) { if(!f2) os<<","; f2=false; os << "a" << u; }
        os << "}\n";
    }

    os << "\n[c] Solution (class I only, shadow of ψ)\n";
    for (auto it=st.solutionC.begin(); it!=st.solutionC.end(); ++it) {
        const auto &c = it.value();
        os << "  a" << c.id << ": {";
        bool first=true;
        for (auto fit=c.fields.begin(); fit!=c.fields.end(); ++fit) {
            if (!first) os << ", ";
            first=false;
            os << fit.key() << "->" << (fit.value()==0 ? "NULL" : QString("a%1").arg(fit.value()));
        }
        os << "}\n";
    }

    if (!st.constraints.isEmpty()) {
        os << "\n[constraints]\n";
        for (auto &s : st.constraints) os << "  " << s << "\n";
    }
    return out;
}

QString AddrMapPlugin::analyze(const QString& code) const {
    State st;

    const QRegularExpression reDeclPtr(R"(^\s*\w+\s*\*\s*([A-Za-z_]\w*)\s*;)");
    // Pointer declaration with initializer: T* x = y;
    // (needed for: Node *p = head;)
    const QRegularExpression reDeclInitAlias(
        R"(^\s*(?:[A-Za-z_]\w*\s+)*[A-Za-z_]\w*\s*\*\s*([A-Za-z_]\w*)\s*=\s*([A-Za-z_]\w*)\s*;)"
        );
    const QRegularExpression reAssignAlias(R"(^\s*([A-Za-z_]\w*)\s*=\s*([A-Za-z_]\w*)\s*;)");
    const QRegularExpression reAssignFieldGet1(R"(^\s*([A-Za-z_]\w*)\s*=\s*([A-Za-z_]\w*)\s*->\s*([A-Za-z_]\w*)\s*;)");
    const QRegularExpression reFieldAssign1(R"(^\s*([A-Za-z_]\w*)\s*->\s*([A-Za-z_]\w*)\s*=\s*([A-Za-z_]\w*|NULL|0|nullptr)\s*;)");
    const QRegularExpression reMalloc(R"(^\s*([A-Za-z_]\w*)\s*=\s*(?:\([^)]*\)\s*)?malloc\s*\()");
    const QRegularExpression reFree(R"(^\s*free\s*\(\s*([A-Za-z_]\w*)\s*\)\s*;)");
    const QRegularExpression reAssignNull(R"(^\s*([A-Za-z_]\w*)\s*=\s*(NULL|0|nullptr)\s*;)");
    const QRegularExpression reIfEqNull(R"(^\s*if\s*\(\s*([A-Za-z_]\w*)\s*==\s*(NULL|0|nullptr)\s*\))");
    const QRegularExpression reIfNeNull(R"(^\s*if\s*\(\s*([A-Za-z_]\w*)\s*!=\s*(NULL|0|nullptr)\s*\))");
    const QRegularExpression reIfChainEqNull(R"(^\s*if\s*\(\s*([A-Za-z_]\w*(?:\s*->\s*[A-Za-z_]\w*)*)\s*==\s*(NULL|0|nullptr)\s*\))");
    const QRegularExpression reIfChainNeNull(R"(^\s*if\s*\(\s*([A-Za-z_]\w*(?:\s*->\s*[A-Za-z_]\w*)*)\s*!=\s*(NULL|0|nullptr)\s*\))");
    const QRegularExpression reAssignChainGet(R"(^\s*([A-Za-z_]\w*)\s*=\s*([A-Za-z_]\w*(?:\s*->\s*[A-Za-z_]\w*)+)\s*;)");


    // stop analysis when main() definition starts
    const QRegularExpression reMainDecl(R"(^\s*int\s+main\s*\()" );


    // general chain equality/inequality: e.g. if (x == q->right->left)
    const QRegularExpression reIfEqAny(R"(^\s*if\s*\(\s*([A-Za-z_]\w*(?:\s*->\s*[A-Za-z_]\w*)*)\s*==\s*([A-Za-z_]\w*(?:\s*->\s*[A-Za-z_]\w*)*)\s*\))");
    const QRegularExpression reIfNeAny(R"(^\s*if\s*\(\s*([A-Za-z_]\w*(?:\s*->\s*[A-Za-z_]\w*)*)\s*!=\s*([A-Za-z_]\w*(?:\s*->\s*[A-Za-z_]\w*)*)\s*\))");

    // Линейное представление исходного кода
    struct LineInfo { int index=0; int lineNo=0; QString text; };
    QVector<LineInfo> lines; lines.reserve(code.count('\n') + 1);
    { int lineNo=0, idx=0;
        for (const QString& raw : code.split('\n')) {
            ++lineNo; const QString line = norm(trimComment(raw));
            LineInfo li; li.index=idx++; li.lineNo=lineNo; li.text=line; lines.push_back(li);
        }
    }

    // === find first main() DEFINITION and ignore everything after it ===
    int mainStartIdx = lines.size();  // default: "no main"
    for (int i = 0; i < lines.size(); ++i) {
        const QString &text = lines[i].text;
        if (!reMainDecl.match(text).hasMatch())
            continue;

        bool isDefinition = false;
        if (text.contains('{')) {
            isDefinition = true;
        } else {
            for (int j = i + 1; j < lines.size(); ++j) {
                const QString &nextText = lines[j].text;
                if (nextText.isEmpty())
                    continue;
                if (nextText.startsWith('{')) {
                    isDefinition = true;
                }
                break; // first non-empty line checked
            }
        }

        if (isDefinition) {
            mainStartIdx = lines[i].index;
            break;
        }
    }

    // === Seed γ with pointer parameters from the first function definition (before main) ===
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
            if (params.isEmpty() || params == "void") { continue; } // function with no pointer params

            const QRegularExpression reLastIdent(R"(([A-Za-z_]\w*)\s*$)");

            for (const QString& rawParam : params.split(',', Qt::SkipEmptyParts)) {
                QString p = rawParam.trimmed();
                if (!p.contains('*')) continue; // only pointer params

                auto mm = reLastIdent.match(p);
                if (!mm.hasMatch()) continue;
                const QString name = mm.captured(1);
                if (name.isEmpty()) continue;
                if (isNullLike(name)) continue;

                // In the paper, input parameters are class I and exist at entry.
                (void)ensureVar(st, name);
            }
            // continue: allow multiple function definitions in the slice
        }
    };
    seedInputPointerParams();

    // 1-й проход: SymTab + AliasTab (препроцессор алиасов)
    struct SymEntry {
        int varDef = -1;                         // последняя строка, где var присваивали
        QMap<QString,int> fieldDef;              // последняя строка, где var->field присваивали (первый уровень)
    };
    QMap<QString, SymEntry> symTab;

    struct Chain { QString base; QStringList f; };
    auto parseChain = [&](const QString& s)->Chain{
        QStringList parts = parseChainExpr(s);
        Chain c; if (parts.isEmpty()) return c;
        c.base = parts.front(); c.f = parts.mid(1); return c;
    };

    struct AliasOp {
        int placeIndex = 0;      // индекс строки, ПЕРЕД которой выполнить alias
        Chain lhs;               // кого «привязать» (переменная ИЛИ поле-цепочка)
        Chain rhs;               // к кому привязать (цепочка)
        int condLineNo = 0;      // номер условной строки для лога
    };
    QVector<AliasOp> aliasOps;

    // Собираем определения и планируем alias’ы
    for (const LineInfo& li : lines) {
        if (li.index >= mainStartIdx) break;
        const QString& line = li.text;
        if (line.isEmpty()) continue;

        // T* x = y;   (declaration with init) -> x was "defined" here
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

        // x = y->f;  (1 уровень)
        if (auto m = reAssignFieldGet1.match(line); m.hasMatch()) {
            const QString x = m.captured(1), y = m.captured(2), f = m.captured(3);
            Q_UNUSED(y); Q_UNUSED(f);
            symTab[x].varDef = li.index;
            continue;
        }

        // y->f = z;  (1 уровень)
        if (auto m = reFieldAssign1.match(line); m.hasMatch()) {
            const QString y = m.captured(1), f = m.captured(2);
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
            Chain L = parseChain(m.captured(1));
            Chain R = parseChain(m.captured(2));
            // вычислим «последнюю модификацию» для баз
            auto lastDef = [&](const Chain& C)->int {
                int last = symTab.contains(C.base) ? symTab[C.base].varDef : -1;
                if (!C.f.isEmpty()) {
                    const QString f0 = C.f.front(); // берём минимум — первый уровень, как в статье
                    if (symTab.contains(C.base) && symTab[C.base].fieldDef.contains(f0))
                        last = std::max(last, symTab[C.base].fieldDef[f0]);
                }
                return last;
            };
            const int lastL = lastDef(L);
            const int lastR = lastDef(R);
            int place = std::max(lastL, lastR);
            // IMPORTANT: placeIndex is the line index **before which** we execute the alias.
            // The paper inserts the explicit alias **after** the last modification of either side.
            // So when we know the last modification index, we must schedule at (last+1).
            // If we have no information (place < 0), keep it right before the condition line.
            if (place < 0) {
                place = li.index;
            } else {
                place = place + 1;
                if (place > li.index) place = li.index;
            }
            if (place < 0) place = 0;
            if (place >= lines.size()) place = lines.size()-1;

            AliasOp op; op.placeIndex=place; op.lhs=L; op.rhs=R; op.condLineNo=li.lineNo;
            aliasOps.push_back(op);
            continue;
        }
    }

    // индекс строки -> alias-операции
    QMultiMap<int, AliasOp> aliasBefore;
    for (const AliasOp& op : aliasOps) aliasBefore.insert(op.placeIndex, op);

    // 2-й проход: AddressMapper + выполнение aliasBefore
    for (const LineInfo& li : lines) {
        if (li.index >= mainStartIdx) break;
        const int idx    = li.index;
        const int lineNo = li.lineNo;
        const QString& line = li.text;

        // 2.1. сначала выполняем alias’ы, запланированные перед этой строкой
        auto range = aliasBefore.equal_range(idx);
        for (auto it = range.first; it != range.second; ++it) {
            const AliasOp& op = it.value();

            auto toList = [&](const Chain& C)->QStringList {
                QStringList r; if (C.base.isEmpty()) return r;
                r << C.base; for (auto &x: C.f) r << x; return r;
            };

            // адрес-источник (rhs) — создаём недостающее в class I
            int rhsAddr = useChain(st, toList(op.rhs), /*createIfClassI*/true, /*createBaseIfMissing*/true, /*markNonNull*/true);
            if (rhsAddr==0) { st.errors << QString("[%1] alias rhs недоступен").arg(op.condLineNo); continue; }

            // Запрет неравенства?
            // Если уже известно, что lhs и rhs неравны — путь невозможен
            // (lhs может быть переменной или полем)
            auto checkUnequal = [&](int a, int b)->bool {
                if (a==0 || b==0) return false;
                if (!st.mem.contains(a) || !st.mem.contains(b)) return false;
                return st.mem[a].unequal.contains(b) || st.mem[b].unequal.contains(a);
            };

            // применяем alias к lhs
            if (op.lhs.f.isEmpty()) {
                // lhs — переменная
                int lhsAddrCur = st.env.value(op.lhs.base, 0);
                if (checkUnequal(lhsAddrCur, rhsAddr)) {
                    st.errors << QString("[%1] alias(%2 := …) противоречит p!=q").arg(op.condLineNo).arg(op.lhs.base);
                }
                st.env[op.lhs.base] = rhsAddr;
            } else {
                // lhs — цепочка поля: p->f1->..->fk := rhs
                QStringList lhsBasePath; lhsBasePath << op.lhs.base;
                for (int i=0;i+1<op.lhs.f.size();++i) lhsBasePath << op.lhs.f[i];
                int baseAddr = useChain(st, lhsBasePath, /*createIfClassI*/true, /*createBaseIfMissing*/true, /*markNonNull*/true);
                if (baseAddr==0 || !st.mem.contains(baseAddr) || !st.mem[baseAddr].active) {
                    st.errors << QString("[%1] alias lhs база недоступна").arg(op.condLineNo); continue;
                }
                const QString lastField = op.lhs.f.back();
                int existing = st.mem[baseAddr].fields.value(lastField, 0);
                if (checkUnequal(existing, rhsAddr)) {
                    st.errors << QString("[%1] alias(%2->%3 := …) против p!=q").arg(op.condLineNo).arg(op.lhs.base, lastField);
                }
                st.mem[baseAddr].fields[lastField] = rhsAddr;
                mirrorFieldWriteToSolution(st, baseAddr, lastField, rhsAddr); // χ ← ψ
            }

            st.constraints << QString("[%1] alias(%2 := %3) [preprocess]")
                                  .arg(op.condLineNo)
                                  .arg(op.lhs.base + (op.lhs.f.isEmpty() ? "" : "->" + op.lhs.f.join("->")),
                                       op.rhs.base + (op.rhs.f.isEmpty() ? "" : "->" + op.rhs.f.join("->")));
        }

        if (line.isEmpty()) continue;

        // САМИ ОПЕРАЦИИ

        // T* x = y;  (explicit alias at declaration)
        if (auto m = reDeclInitAlias.match(line); m.hasMatch()) {
            const QString x = m.captured(1);
            const QString y = m.captured(2);

            if (x != y) {
                if (isNullLike(y)) {
                    st.env[x] = 0;
                    st.constraints << QString("[%1] %2 = NULL").arg(lineNo).arg(x);
                } else {
                    const int rhs = ensureVar(st, y);
                    const int lhs = st.env.value(x, 0);
                    if (lhs!=0 && rhs!=0 && st.mem.contains(lhs) && st.mem[lhs].unequal.contains(rhs)) {
                        st.errors << QString("[%1] %2 := %3 против p!=q").arg(lineNo).arg(x, y);
                    }
                    st.env[x] = rhs;
                }
            }
            continue;
        }

        // объявление указателя: T* x;  — НИЧЕГО НЕ ДЕЛАТЬ (из статьи)
        if (auto m = reDeclPtr.match(line); m.hasMatch()) {
            // no-op
            continue;
        }

        // x = malloc(.);  Create class II
        if (auto m = reMalloc.match(line); m.hasMatch()) {
            const QString v = m.captured(1);
            int addr = createCell(st, /*classI*/false);
            st.env[v] = addr;
            continue;
        }


        // free(x); Delete
        if (auto m = reFree.match(line); m.hasMatch()) {
            const QString v = m.captured(1);
            int a = st.env.value(v, 0);
            if (a==0) { st.errors << QString("[%1] free(NULL)").arg(lineNo); continue; }
            if (st.mem.contains(a)) st.mem[a].active = false;
            continue;
        }

        // x = NULL; Modify
        if (auto m = reAssignNull.match(line); m.hasMatch()) {
            const QString v = m.captured(1);
            // если текущий адрес был помечен null=2, запретим обнулять
            int cur = st.env.value(v, 0);
            if (cur!=0 && st.mem.contains(cur) && st.mem[cur].nullFlag==2) {
                st.errors << QString("[%1] попытка присвоить NULL узлу, помеченному non-NULL").arg(lineNo);
            }
            st.env[v] = 0;
            st.constraints << QString("[%1] %2 = NULL").arg(lineNo).arg(v);
            continue;
        }

        // if (x == NULL) / if (x != NULL) — свойства null (CONSTRAINTS)
        if (auto m = reIfEqNull.match(line); m.hasMatch()) {
            int a = st.env.value(m.captured(1), 0);
            if (a!=0) setNullFlag(st, a, 1);
            st.constraints << QString("[%1] %2 == NULL").arg(lineNo).arg(m.captured(1));
            continue;
        }
        if (auto m = reIfNeNull.match(line); m.hasMatch()) {
            int a = st.env.value(m.captured(1), 0);
            if (a!=0) setNullFlag(st, a, 2);
            st.constraints << QString("[%1] %2 != NULL").arg(lineNo).arg(m.captured(1));

            // if (p->f->g == NULL) / if (p->f->g != NULL) — ограничения на NULL для цепочки
            // Важно: это ограничение относится к ПОСЛЕДНЕМУ полю; базовая часть цепочки должна быть разыменована (=> non-NULL).
            if (auto m = reIfChainEqNull.match(line); m.hasMatch()) {
                const QString expr = m.captured(1).trimmed();
                QStringList chain = parseChainExpr(expr);
                if (chain.size() >= 2) {
                    const QString lastField = chain.takeLast();
                    const int base = useChain(st, chain,
                                              /*createIfClassI*/true,
                                              /*createBaseIfMissing*/true,
                                              /*markNonNull*/true);
                    if (base==0 || !st.mem.contains(base) || !st.mem[base].active) {
                        st.errors << QString("[%1] constraint %2 == NULL невозможно").arg(lineNo).arg(expr);
                    } else {
                        // поле указывает на NULL
                        st.mem[base].fields[lastField] = 0;
                        mirrorFieldWriteToSolution(st, base, lastField, 0);
                    }
                } else {
                    // это обычная переменная; пусть обработает reIfEqNull
                }
                st.constraints << QString("[%1] %2 == NULL").arg(lineNo).arg(expr);
                continue;
            }
            if (auto m = reIfChainNeNull.match(line); m.hasMatch()) {
                const QString expr = m.captured(1).trimmed();
                QStringList chain = parseChainExpr(expr);
                if (chain.size() >= 2) {
                    const QString lastField = chain.takeLast();
                    const int base = useChain(st, chain,
                                              /*createIfClassI*/true,
                                              /*createBaseIfMissing*/true,
                                              /*markNonNull*/true);
                    if (base==0 || !st.mem.contains(base) || !st.mem[base].active) {
                        st.errors << QString("[%1] constraint %2 != NULL невозможно").arg(lineNo).arg(expr);
                    } else {
                        int cur = st.mem[base].fields.value(lastField, 0);
                        if (cur == 0) {
                            // если поле не задано или было приравнено к NULL — нужно создать non-NULL адрес
                            int child = 0;
                            if (st.mem[base].classI) {
                                child = ensureField(st, base, lastField, /*nextIsClassI*/true);
                            } else {
                                // для class II допустимо создать новый heap-узел (class II)
                                child = createCell(st, /*classI*/false);
                            }
                            if (child==0) {
                                st.errors << QString("[%1] %2 != NULL: не удалось создать адрес").arg(lineNo).arg(expr);
                            } else {
                                st.mem[base].fields[lastField] = child;
                                mirrorFieldWriteToSolution(st, base, lastField, child);
                                setNullFlag(st, child, 2);
                            }
                        } else {
                            setNullFlag(st, cur, 2);
                        }
                    }
                } else {
                    // это обычная переменная; пусть обработает reIfNeNull
                }
                st.constraints << QString("[%1] %2 != NULL").arg(lineNo).arg(expr);
                continue;
            }

            continue;
        }

        // if (expr != expr) — CONSTRAINT INEQUALITY (не модифицирует память)
        if (auto m = reIfNeAny.match(line); m.hasMatch()) {
            QStringList L = parseChainExpr(m.captured(1));
            QStringList R = parseChainExpr(m.captured(2));
            int a = peekChain(st, L);
            int b = peekChain(st, R);
            if (a!=0 && b!=0) addUnequal(st, a, b);
            st.constraints << QString("[%1] %2 != %3").arg(lineNo).arg(m.captured(1), m.captured(2));
            continue;
        }

        // if (expr == expr) — после препроцессора это только фиксация ограничения
        if (auto m = reIfEqAny.match(line); m.hasMatch()) {
            st.constraints << QString("[%1] %2 == %3").arg(lineNo).arg(m.captured(1), m.captured(2));
            continue;
        }

        // x = y->f;  (1 уровень; для >1 уровня можно расширить аналогично через parseChainExpr)
        if (auto m = reAssignFieldGet1.match(line); m.hasMatch()) {
            const QString x = m.captured(1), y = m.captured(2), f = m.captured(3);
            int to = useChain(st, QStringList{y, f});
            if (to==0) st.errors << QString("[%1] разыменование %2->%3 невозможно").arg(lineNo).arg(y, f);
            st.env[x] = to;
            continue;
        }

        // y->f = z / NULL;
        if (auto m = reFieldAssign1.match(line); m.hasMatch()) {
            const QString y = m.captured(1), f = m.captured(2), z = m.captured(3);

            // 1) USE для базы y: создаём входной адрес, помечаем y как non-NULL
            int base = useChain(st, QStringList{y},
                                /*createIfClassI*/true,
                                /*createBaseIfMissing*/true,
                                /*markNonNull*/true);
            if (base==0 || !st.mem.contains(base) || !st.mem[base].active) {
                st.errors << QString("[%1] запись в %2->%3 невозможна").arg(lineNo).arg(y, f);
                continue;
            }
            if (st.mem[base].nullFlag == 1) {
                st.errors << QString("[%1] разыменование NULL (%2)").arg(lineNo).arg(y);
                continue;
            }

            // 2) целевой адрес
            int target = 0;
            if (!isNullLike(z)) {
                target = ensureVar(st, z); // явный алиас RHS
                if (target==0)
                    st.errors << QString("[%1] RHS %2 == NULL?").arg(lineNo).arg(z);
            }

            // 3) если поле ещё не создано, а база — class I -> создаём child class I
            if (!st.mem[base].fields.contains(f) && st.mem[base].classI) {
                (void)ensureField(st, base, f, /*nextIsClassI*/true);
            }

            // 4) проверка неравенства
            int prev = st.mem[base].fields.value(f, 0);
            if (prev!=0 && target!=0 &&
                st.mem.contains(prev) && st.mem[prev].unequal.contains(target)) {
                st.errors << QString("[%1] %2->%3 := a%4 против p!=q")
                                 .arg(lineNo).arg(y, f).arg(target);
            }

            // 5) запись + синхронизация χ
            st.mem[base].fields[f] = target;
            mirrorFieldWriteToSolution(st, base, f, target);
            continue;
        }

        // x = y;  (явный alias)
        if (auto m = reAssignAlias.match(line); m.hasMatch()) {
            const QString x = m.captured(1), y = m.captured(2);
            if (x != y) {
                int rhs = ensureVar(st, y);
                int lhs = st.env.value(x, 0); // не создаём до присваивания
                if (lhs!=0 && rhs!=0 &&
                    st.mem.contains(lhs) && st.mem[lhs].unequal.contains(rhs)) {
                    st.errors << QString("[%1] %2 := %3 против p!=q").arg(lineNo).arg(x, y);
                }
                st.env[x] = rhs;
            }
            continue;
        }



        // прочее игнорируем
    }

    bool valid = st.errors.isEmpty();
    return dumpReport(st, valid);
}
