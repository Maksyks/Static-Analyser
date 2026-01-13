#pragma once
#include <QTabWidget>
#include "core/model/AnalyseResult.h"

class SliceModel;
class CodeEditor; // forward

class SliceResultViewer : public QTabWidget {
    Q_OBJECT
public:
    explicit SliceResultViewer(QWidget* parent=nullptr);
    void bind(SliceModel* model);
signals:
    void lineActivated(int line1based);   // проброс клика из вложенного редактора
private slots:
    void onChanged(const SliceResult& s);

private:
    CodeEditor* tabCustom_ = nullptr;
    CodeEditor* tabLlvm_   = nullptr;
    class GraphTab* tabGraph_=nullptr;
    CodeEditor* tabAddrMap_ = nullptr;
};
