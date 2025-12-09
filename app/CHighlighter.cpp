// CHighlighter.cpp
#include "CHighlighter.h"

CHighlighter::CHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    // 0) Общие переменные для порядка правил
    // Чтобы обойти проблему переопределения формата, сначала добавляем правило для переменных,
    // а затем для более специфичных шаблонов (типы, ключевые слова).

    // 1) Переменные - белый (VSCode Dark+)
    QTextCharFormat variableFormat;
    variableFormat.setForeground(QColor("#D4D4D4")); // светло-серый как в VSCode
    // Обобщенный идентификатор, но без переопределения ключевых слов и типов
    rules.append({ QRegularExpression(R"(\b[A-Za-z_][A-Za-z0-9_]*\b)"), variableFormat });

    // 2) Типы данных - светло-голубой (#569CD6)
    QTextCharFormat typeFormat;
    typeFormat.setForeground(QColor("#569CD6"));
    const QStringList typePatterns = {
        "\\bchar\\b", "\\bdouble\\b", "\\bfloat\\b", "\\bint\\b",
        "\\blong\\b", "\\bshort\\b", "\\bsigned\\b", "\\bunsigned\\b",
        "\\bvoid\\b", "\\b_Bool\\b", "\\b_Complex\\b", "\\b_Imaginary\\b"
    };
    for (const QString &pattern : typePatterns) {
        HighlightingRule rule;
        rule.pattern = QRegularExpression(pattern);
        rule.format = typeFormat;
        rules.append(rule);
    }

    // 3) Ключевые слова - фиолетовый (#C586C0)
    QTextCharFormat keywordFormat;
    keywordFormat.setForeground(QColor("#C586C0"));
    const QStringList keywordPatterns = {
        "\\bauto\\b", "\\bbreak\\b", "\\bcase\\b", "\\bconst\\b",
        "\\bcontinue\\b", "\\bdefault\\b", "\\bdo\\b", "\\belse\\b",
        "\\benum\\b", "\\bextern\\b", "\\bfor\\b", "\\bgoto\\b",
        "\\bif\\b", "\\bregister\\b", "\\breturn\\b", "\\bsizeof\\b",
        "\\bstatic\\b", "\\bstruct\\b", "\\bswitch\\b", "\\btypedef\\b",
        "\\bunion\\b", "\\bvolatile\\b", "\\bwhile\\b", "\\binline\\b",
        "\\brestrict\\b", "\\b_Alignas\\b", "\\b_Alignof\\b", "\\b_Atomic\\b",
        "\\b_Generic\\b", "\\b_Noreturn\\b", "\\b_Static_assert\\b", "\\b_Thread_local\\b"
    };
    for (const QString &pattern : keywordPatterns) {
        HighlightingRule rule;
        rule.pattern = QRegularExpression(pattern);
        rule.format = keywordFormat;
        rules.append(rule);
    }

    // 3.5) Директивы препроцессора - розовый (#C586C0)
    QTextCharFormat preprocessorFormat;
    preprocessorFormat.setForeground(QColor("#C586C0"));
    HighlightingRule preprocessorRule;
    preprocessorRule.pattern = QRegularExpression(R"(^\s*#include\b)");
    preprocessorRule.format = preprocessorFormat;
    rules.append(preprocessorRule);

    // 3.6) Путь в #include - оранжевый (#CE9178) ТОЛЬКО ЧАСТЬ В СКОБКАХ/КАВЫЧКАХ
    QTextCharFormat includePathFormat;
    includePathFormat.setForeground(QColor("#CE9178"));
    HighlightingRule includePathRule;
    includePathRule.pattern = QRegularExpression(R"(<[^>]+>|\"[^\"]+\")");
    includePathRule.format = includePathFormat;
    rules.append(includePathRule);

    // 4) Строки и символы - зелено-коричневый (#CE9178)
    QTextCharFormat stringFormat;
    stringFormat.setForeground(QColor("#CE9178"));
    // Строковые литералы
    rules.append({ QRegularExpression(R"(".*?")"), stringFormat });
    // Символьные литералы
    rules.append({ QRegularExpression(R"('.*?')"), stringFormat });

    // 5) Числа - тёмно-бирюзовый (#B5CEA8)
    QTextCharFormat numberFormat;
    numberFormat.setForeground(QColor("#B5CEA8"));
    rules.append({ QRegularExpression(R"(\b\d+\.\d*\b|\b\d*\.\d+\b|\b\d+\b)"), numberFormat });

    // 6) Комментарии - зелёный (#6A9955)
    QTextCharFormat commentFormat;
    commentFormat.setForeground(QColor("#6A9955"));
    // Однострочные
    rules.append({ QRegularExpression(R"(//[^\n]*)"), commentFormat });
    multiLineCommentFormat = commentFormat;
    commentStartExpression = QRegularExpression(R"(/\*)");
    commentEndExpression   = QRegularExpression(R"(\*/)");
}

void CHighlighter::highlightBlock(const QString &text)
{
    // Применяем все правила в порядке добавления
    for (const HighlightingRule &rule : rules) {
        auto matchIt = rule.pattern.globalMatch(text);
        while (matchIt.hasNext()) {
            auto m = matchIt.next();
            // Устанавливаем формат для каждого найденного совпадения
            setFormat(m.capturedStart(), m.capturedLength(), rule.format);
        }
    }

    // Многострочные комментарии
    setCurrentBlockState(0);
    int startIndex = 0;
    if (previousBlockState() != 1)
        startIndex = text.indexOf(commentStartExpression);

    while (startIndex >= 0) {
        auto endMatch = commentEndExpression.match(text, startIndex);
        int endIndex = endMatch.hasMatch() ? endMatch.capturedStart() : -1;
        int length = (endIndex == -1)
                         ? text.length() - startIndex
                         : endIndex - startIndex + endMatch.capturedLength();
        setFormat(startIndex, length, multiLineCommentFormat);
        if (endIndex == -1) {
            setCurrentBlockState(1);
            break;
        }
        startIndex = text.indexOf(commentStartExpression, startIndex + length);
    }
}
