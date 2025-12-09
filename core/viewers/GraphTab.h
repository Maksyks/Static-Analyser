#pragma once
#include <QWidget>

class QWebEngineView;
class QWebChannel;
class GraphBridge;

class GraphTab : public QWidget {
    Q_OBJECT
public:
    explicit GraphTab(QWidget* parent = nullptr);
    void setDot(const QString& dotText);

signals:
    void lineActivated(int line1based);

private:
    // поля только для webengine-варианта
    QWebEngineView* view_ = nullptr;
    QWebChannel*    chan_ = nullptr;
    GraphBridge*    bridge_ = nullptr;
};
