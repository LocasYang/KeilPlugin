#pragma once

#include <QVariantMap>
#include <QString>
#include <QStringList>

#include "keilproject.h"
#include "uvprojxhook.h"

// Capability strings understood by the UI:
//   install / uninstall - the plugin writes hooks into a Keil project
//   report              - the plugin can produce a per-target report
//   run                 - the plugin is a standalone tool invoked with arguments
struct PluginDescriptor {
    QString id;
    QString name;
    QString version;
    QString author;
    QString summary;
    QString description;
    QString category;        // "hook" or "tool"
    QStringList capabilities;
    QStringList notes;
    // Optional sidebar sub-menu label: when set, the plugin gets its own
    // report/view page listed under "插件报告" in the workspace navigation.
    QString viewLabel;
};

// Base class for every Keil Plugin Studio plugin. The UI is fully driven by
// PluginDescriptor, so adding a new plugin never requires UI changes: derive
// from KeilPlugin and register it in PluginRegistry::instance().
class KeilPlugin
{
public:
    virtual ~KeilPlugin();

    virtual PluginDescriptor descriptor() const = 0;

    // Hook plugins: whether the target's After Build slots contain the plugin.
    virtual bool targetInstalled(const KeilTarget &target) const;

    // Hook plugins: install/uninstall the build hook in one uvprojx file.
    virtual UvprojxHook::PatchOutcome installProject(const QString &uvprojxPath) const;
    virtual UvprojxHook::PatchOutcome uninstallProject(const QString &uvprojxPath) const;

    // Tool plugins: run the tool with caller supplied arguments.
    virtual QVariantMap runTool(const QVariantMap &args) const;

protected:
    KeilPlugin() = default;

private:
    Q_DISABLE_COPY(KeilPlugin)
};
