#include "keilplugin.h"

KeilPlugin::~KeilPlugin() = default;

bool KeilPlugin::targetInstalled(const KeilTarget &target) const
{
    Q_UNUSED(target);
    return false;
}

UvprojxHook::PatchOutcome KeilPlugin::installProject(const QString &uvprojxPath) const
{
    UvprojxHook::PatchOutcome outcome;
    outcome.error = QStringLiteral("该功能组件不支持写入工程配置");
    Q_UNUSED(uvprojxPath);
    return outcome;
}

UvprojxHook::PatchOutcome KeilPlugin::uninstallProject(const QString &uvprojxPath) const
{
    UvprojxHook::PatchOutcome outcome;
    outcome.error = QStringLiteral("该功能组件不支持移除工程配置");
    Q_UNUSED(uvprojxPath);
    return outcome;
}

QVariantMap KeilPlugin::runTool(const QVariantMap &args) const
{
    QVariantMap result;
    result.insert(QStringLiteral("ok"), false);
    result.insert(QStringLiteral("message"), QStringLiteral("该功能组件不提供本地执行入口"));
    Q_UNUSED(args);
    return result;
}
