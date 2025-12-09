#pragma once
#include <QObject>
#include <QList>
#include "core/plugins/CommandDescriptor.h"
#include "core/EditorContext.h"

class CommandRegistry : public QObject {
    Q_OBJECT
public:
    explicit CommandRegistry(QObject* parent=nullptr);
    void registerCommands(const QList<CommandDescriptor>& cmds);
    QList<CommandDescriptor> forContext(const EditorContext& ctx) const;
private:
    QList<CommandDescriptor> all_;
};
