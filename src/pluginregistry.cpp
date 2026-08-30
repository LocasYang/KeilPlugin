#include "pluginregistry.h"

#include "builtinplugins.h"

PluginRegistry &PluginRegistry::instance()
{
    static PluginRegistry registry;
    return registry;
}

PluginRegistry::PluginRegistry()
{
    registerPlugin(new SizeReportPlugin);
    registerPlugin(new HexMergePlugin);
}

void PluginRegistry::registerPlugin(KeilPlugin *plugin)
{
    if (!plugin)
        return;
    m_plugins.append(plugin);
}

KeilPlugin *PluginRegistry::find(const QString &id) const
{
    for (KeilPlugin *plugin : m_plugins) {
        if (plugin->descriptor().id == id)
            return plugin;
    }
    return nullptr;
}
