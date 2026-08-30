#include "pluginmanager.h"

#include <QDateTime>
#include <QFileInfo>
#include <QUrl>

#include <algorithm>

#include "mapparser.h"
#include "pluginregistry.h"

namespace {
constexpr int kMaxLogEntries = 600;
constexpr int kMaxReportComponents = 12;

QVariantList sortedComponents(const QVector<MapComponent> &components, bool byRom)
{
    QVariantList sorted;
    for (const MapComponent &component : components) {
        QVariantMap entry;
        entry.insert(QStringLiteral("name"), component.name);
        entry.insert(QStringLiteral("rom"), component.rom);
        entry.insert(QStringLiteral("ram"), component.ram);
        sorted.append(entry);
    }
    std::sort(sorted.begin(), sorted.end(),
              [byRom](const QVariant &a, const QVariant &b) {
                  const QVariantMap ma = a.toMap();
                  const QVariantMap mb = b.toMap();
                  const qint64 keyA = ma.value(byRom ? "rom" : "ram").toLongLong();
                  const qint64 keyB = mb.value(byRom ? "rom" : "ram").toLongLong();
                  return keyA > keyB;
              });
    if (sorted.size() > kMaxReportComponents)
        sorted.erase(sorted.begin() + kMaxReportComponents, sorted.end());
    return sorted;
}

QString localPathOf(const QVariant &source)
{
    if (!source.isValid())
        return QString();
    if (source.metaType().id() == QMetaType::QUrl) {
        const QUrl url = source.toUrl();
        if (url.isLocalFile())
            return url.toLocalFile();
        if (url.scheme().isEmpty())
            return url.path();
        return QString();
    }
    const QString text = source.toString();
    if (text.startsWith(QLatin1String("file:///")))
        return QUrl(text).toLocalFile();
    return text;
}

} // namespace

PluginManager::PluginManager(QObject *parent)
    : QObject(parent)
{
    rebuildPluginStates();
    setStatusMessage(QStringLiteral("就绪 · 内置 %1 个插件").arg(m_plugins.size()));
}

PluginManager::~PluginManager()
{
    if (m_project)
        m_project->deleteLater();
}

bool PluginManager::loadProject(const QVariant &source)
{
    const QString path = localPathOf(source);
    if (path.isEmpty()) {
        pushResult(false, QStringLiteral("无效的工程路径"));
        return false;
    }
    if (!m_project)
        m_project = new KeilProject(this);
    const bool loaded = m_project->load(path);
    rebuildPluginStates();
    emit projectChanged();

    if (loaded) {
        appendLog(QStringLiteral("success"),
                  QStringLiteral("已加载 Keil 工程: %1 (%2 个目标)")
                      .arg(path)
                      .arg(m_project->targetList().size()));
        setStatusMessage(QStringLiteral("工程: %1 · 目标 %2 个")
                             .arg(QFileInfo(path).fileName())
                             .arg(m_project->targetList().size()));
    } else {
        appendLog(QStringLiteral("error"),
                  QStringLiteral("加载工程失败: %1 (%2)").arg(path, m_project->errorString()));
        setStatusMessage(QStringLiteral("工程加载失败"));
        pushResult(false, m_project->errorString());
    }
    return loaded;
}

void PluginManager::reloadProject()
{
    if (!m_project)
        return;
    m_project->reload();
    rebuildPluginStates();
    emit projectChanged();
}

bool PluginManager::installPlugin(const QString &pluginId)
{
    KeilPlugin *plugin = PluginRegistry::instance().find(pluginId);
    if (!plugin) {
        pushResult(false, QStringLiteral("未找到插件: %1").arg(pluginId));
        return false;
    }
    if (!projectLoaded()) {
        pushResult(false, QStringLiteral("请先打开一个 Keil 工程"));
        return false;
    }

    const PluginDescriptor descriptor = plugin->descriptor();
    appendLog(QStringLiteral("info"),
              QStringLiteral("开始安装插件「%1」到工程").arg(descriptor.name));

    bool allOk = true;
    bool anyChanged = false;
    for (const QString &uvprojx : m_project->data().uvprojxFiles) {
        const UvprojxHook::PatchOutcome outcome = plugin->installProject(uvprojx);
        for (const QString &note : outcome.notes)
            appendLog(QStringLiteral("info"), note);
        if (!outcome.error.isEmpty()) {
            appendLog(QStringLiteral("error"), outcome.error);
            allOk = false;
        } else if (outcome.changed) {
            appendLog(QStringLiteral("success"),
                      QStringLiteral("构建钩子已写入 -> %1").arg(uvprojx));
            anyChanged = true;
        }
    }

    m_project->reload();
    rebuildPluginStates();
    emit projectChanged();

    if (!allOk) {
        appendLog(QStringLiteral("error"), QStringLiteral("插件安装: 部分工程写入失败"));
        pushResult(false, QStringLiteral("部分工程写入失败，详情见运行日志"));
        return false;
    }
    const QString message = anyChanged
            ? QStringLiteral("「%1」已安装到工程").arg(descriptor.name)
            : QStringLiteral("「%1」已处于安装状态，无需重复安装").arg(descriptor.name);
    appendLog(QStringLiteral("success"), message);
    pushResult(true, message);
    return true;
}

bool PluginManager::uninstallPlugin(const QString &pluginId)
{
    KeilPlugin *plugin = PluginRegistry::instance().find(pluginId);
    if (!plugin) {
        pushResult(false, QStringLiteral("未找到插件: %1").arg(pluginId));
        return false;
    }
    if (!projectLoaded()) {
        pushResult(false, QStringLiteral("请先打开一个 Keil 工程"));
        return false;
    }

    const PluginDescriptor descriptor = plugin->descriptor();
    appendLog(QStringLiteral("info"),
              QStringLiteral("开始从工程卸载插件「%1」").arg(descriptor.name));

    bool allOk = true;
    bool anyChanged = false;
    for (const QString &uvprojx : m_project->data().uvprojxFiles) {
        const UvprojxHook::PatchOutcome outcome = plugin->uninstallProject(uvprojx);
        for (const QString &note : outcome.notes)
            appendLog(QStringLiteral("info"), note);
        if (!outcome.error.isEmpty()) {
            appendLog(QStringLiteral("error"), outcome.error);
            allOk = false;
        } else if (outcome.changed) {
            appendLog(QStringLiteral("success"),
                      QStringLiteral("构建钩子已移除 -> %1").arg(uvprojx));
            anyChanged = true;
        }
    }

    m_project->reload();
    rebuildPluginStates();
    emit projectChanged();

    if (!allOk) {
        appendLog(QStringLiteral("error"), QStringLiteral("插件卸载: 部分工程写入失败"));
        pushResult(false, QStringLiteral("部分工程写入失败，详情见运行日志"));
        return false;
    }
    const QString message = anyChanged
            ? QStringLiteral("「%1」已从工程卸载").arg(descriptor.name)
            : QStringLiteral("工程中未发现「%1」的钩子").arg(descriptor.name);
    appendLog(anyChanged ? QStringLiteral("success") : QStringLiteral("warning"), message);
    pushResult(true, message);
    return true;
}

QVariantMap PluginManager::targetReport(int targetIndex) const
{
    QVariantMap report;
    report.insert(QStringLiteral("ok"), false);
    if (!m_project || targetIndex < 0 || targetIndex >= m_project->targetList().size()) {
        report.insert(QStringLiteral("reason"), QStringLiteral("目标不存在"));
        return report;
    }
    const KeilTarget &target = m_project->targetList().at(targetIndex);

    report.insert(QStringLiteral("name"), target.name);
    report.insert(QStringLiteral("device"), target.device);
    report.insert(QStringLiteral("project"), target.project);
    report.insert(QStringLiteral("uvprojx"), target.uvprojx);
    report.insert(QStringLiteral("flashCap"), target.flashCap);
    report.insert(QStringLiteral("ramCap"), target.ramCap);

    if (!target.mapFound) {
        report.insert(QStringLiteral("reason"),
                      QStringLiteral(".map 文件未找到，请先在 Keil 中编译一次"));
        return report;
    }

    const MapSummary summary = MapParser::parse(target.mapPath);
    if (!summary.ok) {
        report.insert(QStringLiteral("reason"), summary.error);
        return report;
    }

    report.insert(QStringLiteral("ok"), true);
    report.insert(QStringLiteral("mapPath"), summary.mapPath);
    report.insert(QStringLiteral("romUsed"), summary.totalROM);
    report.insert(QStringLiteral("ramUsed"), summary.totalRW);
    report.insert(QStringLiteral("hasDetail"), summary.hasDetail);
    report.insert(QStringLiteral("code"), summary.code);
    report.insert(QStringLiteral("roData"), summary.roData);
    report.insert(QStringLiteral("rwData"), summary.rwData);
    report.insert(QStringLiteral("ziData"), summary.ziData);
    report.insert(QStringLiteral("components"), sortedComponents(summary.components, true));
    return report;
}

QVariantMap PluginManager::runTool(const QString &pluginId, const QVariantMap &args)
{
    KeilPlugin *plugin = PluginRegistry::instance().find(pluginId);
    if (!plugin) {
        pushResult(false, QStringLiteral("未找到插件: %1").arg(pluginId));
        return m_lastResult;
    }
    QVariantMap normalized;
    for (auto it = args.constBegin(); it != args.constEnd(); ++it)
        normalized.insert(it.key(), localPathOf(it.value()));
    const QVariantMap result = plugin->runTool(normalized);
    appendLog(result.value(QStringLiteral("ok")).toBool() ? QStringLiteral("success")
                                                          : QStringLiteral("error"),
              QStringLiteral("工具执行: %1").arg(result.value(QStringLiteral("message")).toString()));
    pushResult(result.value(QStringLiteral("ok")).toBool(),
               result.value(QStringLiteral("message")).toString());
    return result;
}

void PluginManager::clearLogs()
{
    m_logs.clear();
    emit logsChanged();
}

void PluginManager::appendLog(const QString &level, const QString &message)
{
    QVariantMap entry;
    entry.insert(QStringLiteral("time"),
                 QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss")));
    entry.insert(QStringLiteral("level"), level);
    entry.insert(QStringLiteral("message"), message);
    m_logs.append(entry);
    while (m_logs.size() > kMaxLogEntries)
        m_logs.removeFirst();
    emit logsChanged();
}

void PluginManager::rebuildPluginStates()
{
    m_plugins.clear();
    const QVector<KeilTarget> targets =
            m_project ? m_project->targetList() : QVector<KeilTarget>();

    for (KeilPlugin *plugin : PluginRegistry::instance().plugins()) {
        const PluginDescriptor descriptor = plugin->descriptor();
        QVariantMap entry;
        entry.insert(QStringLiteral("id"), descriptor.id);
        entry.insert(QStringLiteral("name"), descriptor.name);
        entry.insert(QStringLiteral("version"), descriptor.version);
        entry.insert(QStringLiteral("author"), descriptor.author);
        entry.insert(QStringLiteral("summary"), descriptor.summary);
        entry.insert(QStringLiteral("description"), descriptor.description);
        entry.insert(QStringLiteral("category"), descriptor.category);
        entry.insert(QStringLiteral("capabilities"), descriptor.capabilities);
        entry.insert(QStringLiteral("notes"), descriptor.notes);
        entry.insert(QStringLiteral("viewLabel"), descriptor.viewLabel);

        const bool hookPlugin = descriptor.capabilities.contains(QStringLiteral("install"));
        int installedCount = 0;
        QVariantList perTarget;
        for (const KeilTarget &target : targets) {
            const bool installed = plugin->targetInstalled(target);
            perTarget.append(installed);
            if (installed)
                ++installedCount;
        }
        entry.insert(QStringLiteral("installedCount"), hookPlugin ? installedCount : -1);
        entry.insert(QStringLiteral("targetCount"), targets.size());
        entry.insert(QStringLiteral("perTarget"), perTarget);
        m_plugins.append(entry);
    }
    emit pluginsChanged();
}

void PluginManager::pushResult(bool ok, const QString &text)
{
    m_lastResult = QVariantMap {
        { "ok", ok },
        { "text", text }
    };
    emit lastResultChanged();
}

void PluginManager::setStatusMessage(const QString &text)
{
    if (m_statusMessage == text)
        return;
    m_statusMessage = text;
    emit statusMessageChanged();
}
