#pragma once
#include <QTabWidget>
#include "core/model/AnalyseResult.h"

class AnalyseModel;
class CodeEditor; // предварительное объявление типа для private

class ResultViewer : public QTabWidget {
    Q_OBJECT
public:
    explicit ResultViewer(QWidget* parent=nullptr);
    void bind(AnalyseModel* model);
signals:
    void lineActivated(int line1based);
private slots:
    void onChanged(const AnalyseResult& s);

private:
    CodeEditor* tabCustom_ = nullptr;
    CodeEditor* tabLlvm_   = nullptr;
    class GraphTab* tabGraph_=nullptr;
    CodeEditor* tabAddrMap_ = nullptr;
};
