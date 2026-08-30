#pragma once

#include <QVector>

#include "keilplugin.h"

// Global plugin registry. Built-ins self-register on first use; future plugins
// only need one line: PluginRegistry::instance().registerPlugin(new MyPlugin).
class PluginRegistry
{
public:
    static PluginRegistry &instance();

    // Takes ownership of the plugin.
    void registerPlugin(KeilPlugin *plugin);

    const QVector<KeilPlugin *> &plugins() const { return m_plugins; }
    KeilPlugin *find(const QString &id) const;

private:
    PluginRegistry();
    Q_DISABLE_COPY(PluginRegistry)

    QVector<KeilPlugin *> m_plugins;
};
