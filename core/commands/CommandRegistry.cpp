#include "core/commands/CommandRegistry.h"

CommandRegistry::CommandRegistry(QObject* parent) : QObject(parent) {}

void CommandRegistry::registerCommands(const QList<CommandDescriptor>& cmds) {
    all_ += cmds;
}

QList<CommandDescriptor> CommandRegistry::forContext(const EditorContext& ctx) const {
    Q_UNUSED(ctx);
    // MVP:
    return all_;
}
