#include "MainWindow.h"
#include <QHBoxLayout>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QFileDialog>
#include <QFile>
#include <QDirIterator>
#include <QPluginLoader>
#include <QCoreApplication>
#include <QInputDialog>
#include <QLineEdit>
#include <climits>
#include <qDebug>
#include "core/model/AnalyseResult.h"

static QVariantMap collectParams(const CommandDescriptor& c,
                                 const EditorContext& ctx,
                                 QWidget* parent)
{
    QVariantMap out = c.defaultParams;
    for (const auto& p : c.paramSpecs) {
        if (out.contains(p.key)) continue; // уже задано по умолчанию

        switch (p.kind) {
        case ParamSpec::Identifier: {
            QStringList ids = QStringList(ctx.identifiers.begin(), ctx.identifiers.end());
            ids.sort(Qt::CaseInsensitive);
            bool ok = false;
            const QString picked = QInputDialog::getItem(
                                       parent,
                                       p.label, p.label,
                                       ids,
                                       0,
                                       /*editable*/ p.editable,
                                       &ok
                                       ).trimmed();
            if (!ok || (!p.optional && picked.isEmpty()))
                return {}; // отмена/пусто — прервать команду
            if (!picked.isEmpty()) {
                qDebug() << p.key;
                qDebug() << picked;
                out.insert(p.key, picked);
            }
            break;
        }
        case ParamSpec::Text: {
            bool ok = false;
            const QString v = QInputDialog::getText(
                                  parent, p.label, p.label, QLineEdit::Normal, {}, &ok
                                  ).trimmed();
            if (!ok || (!p.optional && v.isEmpty()))
                return {};
            if (!v.isEmpty())
                out.insert(p.key, v);
            break;
        }
        case ParamSpec::Choice: {
            bool ok = false;
            const QString v = QInputDialog::getItem(
                                  parent, p.label, p.label, p.choices, 0, p.editable, &ok
                                  ).trimmed();
            if (!ok || (!p.optional && v.isEmpty()))
                return {};
            if (!v.isEmpty())
                out.insert(p.key, v);
            break;
        }
        case ParamSpec::LineNumber: {
            bool ok = false;
            const int line = QInputDialog::getInt(
                parent, p.label, p.label, /*value*/ ctx.line1based, 1, INT_MAX, 1, &ok
                );
            if (!ok) return {};
            out.insert(p.key, line);
            break;
        }
        }
    }
    return out;
}


IDE::IDE(QWidget *parent) : QMainWindow(parent)
{
    // UI
    QWidget *central = new QWidget(this);
    auto *lay = new QHBoxLayout(central);
    leftEditor = new CodeEditor;
    rightTabs  = new ResultViewer;
    lay->addWidget(leftEditor);
    lay->addWidget(rightTabs);
    setCentralWidget(central);
    setMinimumSize(1280, 720);
    // Меню
    fileMenu    = menuBar()->addMenu(tr("Файл"));
    pluginsMenu = menuBar()->addMenu(tr("Инструменты"));
    fileMenu->addAction(tr("Открыть..."), this, SLOT(openSourceFile()));

    // Ядро
    bus        = new DomainEventBus(this);
    cmdReg     = new CommandRegistry(this);
    analyseModel = new AnalyseModel(this);
    rightTabs->bind(analyseModel);//подписка табов на эмит от плагина
    connect(rightTabs, &ResultViewer::lineActivated,
            this,      &IDE::onRightLineActivated);
    connect(bus, &DomainEventBus::analysisCompleted, analyseModel, &AnalyseModel::apply);

    connect(bus, &DomainEventBus::analysisCompleted,
            this,
            [this](const AnalysisResultEnvelope& env, const EditorContext& ctx) {
                if (env.kind != QLatin1String("slice.v1")) return;

                // Извлекаем текст среза (original_custom) и запускаем addrmap
                if (!env.payload.canConvert<AnalyseResult>()) return;
                const AnalyseResult sr = env.payload.value<AnalyseResult>();

                EditorContext postCtx = ctx;
                postCtx.documentText = sr.custom; // раздаём плагинам именно текст original_custom
                emit bus->analysisRequested(QStringLiteral("addrmap.v1"), QVariantMap{}, postCtx);
            });

    //Контекстные команды из реестра
    connect(leftEditor, &CodeEditor::contextActionsRequested,
            this, [this](const EditorContext& ctx, QMenu* menu){
                for (const auto& c : cmdReg->forContext(ctx)) {
                    QAction* a = menu->addAction(c.title);
                    if (!c.shortcut.isEmpty()) a->setShortcut(QKeySequence(c.shortcut));
                    connect(a, &QAction::triggered, this, [this, c, ctx](){
                        auto params = collectParams(c, ctx, this);
                        if (params.isEmpty()) return; // отменили диалог
                        emit bus->analysisRequested(c.kind, params, ctx);
                    });
                }
            });

    // Загрузка плагинов
    QList<IPlugin*> loaded;
    const QString base = QCoreApplication::applicationDirPath();
    QStringList roots; roots << base + "/plugins";
    if (qEnvironmentVariableIsSet("MYIDE_PLUGINS")) roots << qEnvironmentVariable("MYIDE_PLUGINS");
    for (const QString& root : roots) {
        QDirIterator it(root, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString path = it.next();
#if defined(Q_OS_WIN)
            if (!path.endsWith(".dll", Qt::CaseInsensitive)) continue;
#elif defined(Q_OS_MAC)
            if (!path.endsWith(".dylib", Qt::CaseInsensitive)) continue;
#else
            if (!path.endsWith(".so", Qt::CaseInsensitive)) continue;
#endif
            QPluginLoader *loader = new QPluginLoader(path, this);
            if (QObject* inst = loader->instance()) {
                if (auto* p = qobject_cast<IPlugin*>(inst)) {
                    loaded << p;

                     cmdReg->registerCommands(p->commands());

                    // дублируем команды в верхнее меню «Инструменты»
                    for (const auto& c : p->commands()) {
                        QAction* a = pluginsMenu->addAction(c.title);
                        if (!c.shortcut.isEmpty()) a->setShortcut(QKeySequence(c.shortcut));
                        connect(a, &QAction::triggered, this, [this, c](){
                            EditorContext ctx;
                            ctx.documentText = leftEditor->toPlainText();
                            auto params = collectParams(c, ctx, this);
                            if (params.isEmpty()) return;
                            emit bus->analysisRequested(c.kind, params, ctx);
                        });
                    }
                } else {
                    loader->unload();
                }
            }
        }
    }
    runner = new PluginRunner(loaded, bus, this);
}

void IDE::openSourceFile() {
    const QString fn = QFileDialog::getOpenFileName(this, tr("Открыть исходник"), {}, "C/C++ (*.c *.cpp *.h)");
    if (fn.isEmpty()) return;
    QFile f(fn);
    if (f.open(QIODevice::ReadOnly|QIODevice::Text))
        leftEditor->setPlainText(QString::fromUtf8(f.readAll()));
}

void IDE::onRightLineActivated(int outLine1) {
    const auto& lm = analyseModel->state().lineMap;
    if (!lm.contains(outLine1)) return;
    int srcLine = lm[outLine1].second;
    if (srcLine > 0) leftEditor->highlightLine(srcLine - 1);
}
