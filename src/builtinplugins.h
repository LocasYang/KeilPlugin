#pragma once

#include "keilplugin.h"

// Built-in plugin: Flash/RAM usage report. Native C++ port of the reference
// Python tool. Installs an After Build hook that invokes this application's
// CLI mode after every Keil build; the report is printed into the Keil
// Build Output window.
class SizeReportPlugin final : public KeilPlugin
{
public:
    PluginDescriptor descriptor() const override;
    bool targetInstalled(const KeilTarget &target) const override;
    UvprojxHook::PatchOutcome installProject(const QString &uvprojxPath) const override;
    UvprojxHook::PatchOutcome uninstallProject(const QString &uvprojxPath) const override;
};

// Built-in plugin: merges two non-overlapping Intel HEX images (bootloader +
// application) into a single factory image. Standalone tool, no project hook.
class HexMergePlugin final : public KeilPlugin
{
public:
    PluginDescriptor descriptor() const override;
    QVariantMap runTool(const QVariantMap &args) const override;
};
