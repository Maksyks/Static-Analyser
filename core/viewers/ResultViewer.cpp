// core/viewers/ResultViewer.cpp
#include "core/viewers/ResultViewer.h"
#include "core/model/AnalyseModel.h"
#include "../../app/CodeEditor.h"
#include "GraphTab.h"

ResultViewer::ResultViewer(QWidget* parent)
    : QTabWidget(parent)
{
    tabCustom_  = new CodeEditor(this); tabCustom_->setReadOnly(true);
    tabLlvm_    = new CodeEditor(this); tabLlvm_->setReadOnly(true);
    tabGraph_   = new GraphTab(this);
    tabAddrMap_ = new CodeEditor(this); tabAddrMap_->setReadOnly(true);

    addTab(tabCustom_,  tr("original_custom"));
    addTab(tabLlvm_,    tr("llvm2c"));
    addTab(tabGraph_,   tr("graph"));
    addTab(tabAddrMap_, tr("addrmap"));

    connect(tabGraph_,  &GraphTab::lineActivated, this, &ResultViewer::lineActivated);
    connect(tabCustom_, &CodeEditor::lineActivated, this, &ResultViewer::lineActivated);
}

void ResultViewer::bind(AnalyseModel* model) {
    connect(model, &AnalyseModel::changed, this, &ResultViewer::onChanged);
}

void ResultViewer::onChanged(const AnalyseResult& s) {
    tabCustom_->setPlainText(s.custom);
    tabLlvm_->setPlainText(s.llvm2c);
    tabAddrMap_->setPlainText(s.addrmap);

    const QString dot = !s.dotVisible.isEmpty() ? s.dotVisible : s.dotCfg;
    if(!dot.isEmpty()) tabGraph_->setDot(dot);
    else tabGraph_->setDot(QString());
}
