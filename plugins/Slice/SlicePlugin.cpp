// plugins/Slice/SlicePlugin.cpp
#include "SlicePlugin.h"
#include <QProcess>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonParseError>
#include <QDebug>

static QString toWslPath(const QString& winPath) {
    QProcess p; p.start("wsl", {"wslpath", "-u", winPath});
    p.waitForFinished(5000);
    return QString::fromUtf8(p.readAllStandardOutput()).trimmed();
}

QVariant SlicePlugin::runAnalysis(QString kind, const EditorContext& ctx, const QVariantMap& params) {
    Q_UNUSED(kind);
    QString criterion = params.value("criterion").toString();

    if (criterion.isEmpty()) {
        const QString var = params.value("variable").toString().trimmed();
        if (!var.isEmpty() && ctx.line1based > 0) {
            criterion = QString::number(ctx.line1based) + ":" + var;   // "21:sum"
        }
    }
    if (criterion.isEmpty())
        throw std::runtime_error("Empty slice criterion (need 'line:var' or 'func:line:var').");

    SliceResult r = runSlice(ctx.documentText, criterion);
    return QVariant::fromValue(r);
}

QList<CommandDescriptor> SlicePlugin::commands() const {
    CommandDescriptor d;
    d.id = "slice.run";
    d.title = "Сделать срез…";
    d.menuPath = "Editor/Context/Slice";
    d.shortcut = "Ctrl+Alt+S";
    d.kind = "slice.v1";
    // d.defaultParams["something"] = "..."; // если надо
    d.paramSpecs = {
        { ParamSpec{ "variable", "Переменная", ParamSpec::Identifier, /*choices*/{}, /*editable*/true, /*optional*/false } }
    };
    return { d };
}

SliceResult SlicePlugin::runSlice(const QString& input, const QString& criterion) const {
    QTemporaryDir tmpd;
    QFile outF(tmpd.path()+"/input.c"); outF.open(QIODevice::WriteOnly|QIODevice::Text);
    QTextStream ts(&outF); ts << input; outF.close();
    qDebug() << "Файл открыт";
    qDebug() << "Критерий";
    qDebug() << criterion;
    const QString appDir = QCoreApplication::applicationDirPath();
    const QString winScript = QDir(appDir).filePath("plugins/slice.sh");
    const QString wslScript = toWslPath(winScript);
    if (!QFile::exists(winScript)) {
        qCritical() << "❌ Скрипт не найден:" << winScript;
        qCritical() << "Текущая директория приложения:" << appDir;
        qCritical() << "Содержимое plugins directory:" << QDir(appDir + "/plugins").entryList();
        throw std::runtime_error("Скрипт slice.sh не найден в директории plugins");
    }
    else
        qDebug()<<"скрипт найден";
    const QString wslInput  = toWslPath(tmpd.path()+"/input.c");
    qDebug() << "Скрипт Запущен";
    QProcess proc;
    proc.setProcessChannelMode(QProcess::MergedChannels);     // stdout+stderr вместе
    QStringList args = {
        "-e", "/usr/bin/env", "bash",  // гарантируем, что скрипт интерпретирует bash
        wslScript,                     // /mnt/c/…/plugins/slice.sh
        wslInput,                      // /mnt/c/…/input.c
        criterion                      // "21:sum" или "main:21:sum"
    };
    proc.start("wsl", args);

    if (!proc.waitForStarted()) {
        qCritical() << "WSL start error:" << proc.errorString();
        throw std::runtime_error("Не удалось запустить WSL");
    }

    // дайте больше времени; можно 5–10 минут, или безлимит:
    if (!proc.waitForFinished(10 * 60 * 1000)) {
        proc.kill();
        qCritical() << "slice.sh timeout; output so far:\n" << proc.readAll();
        throw std::runtime_error("slice.sh превысил таймаут");
    }

    // читаем единым куском (stdout+stderr слиты в MergedChannels):
    const QByteArray all = proc.readAll();
    if (proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0) {
        qCritical() << "slice.sh failed, code:" << proc.exitCode() << "\n" << all;
        throw std::runtime_error("slice.sh завершился с ошибкой");
    }
    const QString bundle = QString::fromUtf8(all);

    // Универсальный helper
    auto between = [](const QString& s, const char* a, const char* b) -> QString {
        int i = s.indexOf(QLatin1String(a)); if (i < 0) return {};
        i += int(strlen(a)); int j = s.indexOf(QLatin1String(b), i); if (j < 0) return {};
        return s.mid(i, j - i);
    };

    SliceResult r;
    r.custom     = between(bundle, "/*__BEGIN_ORIGINAL_CUSTOM__*/", "/*__END_ORIGINAL_CUSTOM__*/");
    r.llvm2c     = between(bundle, "/*__BEGIN_LLVM2C__*/",          "/*__END_LLVM2C__*/");
    r.dotVisible = between(bundle, "/*__BEGIN_DOT_VISIBLE__*/",     "/*__END_DOT_VISIBLE__*/");
    r.dotCfg     = between(bundle, "/*__BEGIN_DOT_CFG__*/",         "/*__END_DOT_CFG__*/");

    // lineMap из JSON-блока:
    const QString meta = between(bundle, "/*__IDE_METADATA__", "__IDE_METADATA_END__*/");
    QJsonParseError e{}; auto doc = QJsonDocument::fromJson(meta.toUtf8(), &e);
    if (e.error == QJsonParseError::NoError && doc.isObject()) {
        const auto lm = doc.object().value("lineMap").toObject();
        for (auto it = lm.begin(); it != lm.end(); ++it) {
            int outLine = it.key().toInt();
            const auto obj = it.value().toObject();
            r.lineMap[outLine] = qMakePair(obj.value("file").toString(), obj.value("line").toInt());
        }
    }

    return r;
}
