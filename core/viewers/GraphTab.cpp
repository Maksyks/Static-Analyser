#include "GraphTab.h"

#include <QVBoxLayout>
#include <QUrl>
#include <QObject>
#include <QWebEngineView>
#include <QWebChannel>
#include <QWebEnginePage>

class GraphBridge : public QObject {
    Q_OBJECT
public:
    explicit GraphBridge(GraphTab* owner) : QObject(owner), owner_(owner) {}

public slots:
    void onNodeClicked(int line) { emit owner_->lineActivated(line); }

    // JS вызывает это, когда страница готова к приёму сигнала render()
    void pageReady() {
        ready_ = true;
        if (!pendingDot_.isEmpty()) emit render(pendingDot_);
    }

signals:
    void render(const QString& dot);

public:
    QString pendingDot_;
    bool    ready_ = false;

private:
    GraphTab* owner_;
};

GraphTab::GraphTab(QWidget* parent) : QWidget(parent) {
    auto* lay = new QVBoxLayout(this);
    lay->setContentsMargins(0,0,0,0);

    view_   = new QWebEngineView(this);
    chan_   = new QWebChannel(this);
    bridge_ = new GraphBridge(this);

    chan_->registerObject(QStringLiteral("bridge"), bridge_);
    view_->page()->setWebChannel(chan_);
    view_->setUrl(QUrl(QStringLiteral("qrc:/graph/web/graph.html")));

    lay->addWidget(view_);
}

void GraphTab::setDot(const QString& dot) {
    if (!bridge_) return;
    bridge_->pendingDot_ = dot;
    // пробуем отправить сразу; если страница ещё не готова, она получит dot в pageReady()
    emit bridge_->render(dot);
}

//подключаем moc для класса, объявленного В ЭТОМ .cpp
#include "GraphTab.moc"
