#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include "keilplugin.h"

// Facade between the QML UI and the plugin registry. Owns the loaded Keil
// project, rebuilds per-plugin install states, and records an operation log.
class PluginManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(KeilProject *project READ project NOTIFY projectChanged)
    Q_PROPERTY(bool projectLoaded READ projectLoaded NOTIFY projectChanged)
    Q_PROPERTY(QVariantList plugins READ plugins NOTIFY pluginsChanged)
    Q_PROPERTY(QVariantList logs READ logs NOTIFY logsChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(QVariantMap lastResult READ lastResult NOTIFY lastResultChanged)

public:
    explicit PluginManager(QObject *parent = nullptr);
    ~PluginManager() override;

    KeilProject *project() const { return m_project; }
    bool projectLoaded() const { return m_project && m_project->valid(); }
    QVariantList plugins() const { return m_plugins; }
    QVariantList logs() const { return m_logs; }
    QString statusMessage() const { return m_statusMessage; }
    QVariantMap lastResult() const { return m_lastResult; }

    Q_INVOKABLE bool loadProject(const QVariant &source);
    Q_INVOKABLE void reloadProject();
    Q_INVOKABLE bool installPlugin(const QString &pluginId);
    Q_INVOKABLE bool uninstallPlugin(const QString &pluginId);
    Q_INVOKABLE QVariantMap targetReport(int targetIndex) const;
    Q_INVOKABLE QVariantMap runTool(const QString &pluginId, const QVariantMap &args);
    Q_INVOKABLE void clearLogs();
    Q_INVOKABLE void appendLog(const QString &level, const QString &message);

signals:
    void projectChanged();
    void pluginsChanged();
    void logsChanged();
    void statusMessageChanged();
    void lastResultChanged();

private:
    void rebuildPluginStates();
    void pushResult(bool ok, const QString &text);
    void setStatusMessage(const QString &text);

    KeilProject *m_project = nullptr;
    QVariantList m_plugins;
    QVariantList m_logs;
    QString m_statusMessage;
    QVariantMap m_lastResult;
};
