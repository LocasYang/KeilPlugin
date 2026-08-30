#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVector>

struct KeilTarget {
    QString project;       // uvprojx basename without extension
    QString uvprojx;       // absolute path
    QString name;          // TargetName
    QString device;        // Device
    qint64 flashCap = 0;   // sum of IROM region sizes (bytes)
    qint64 ramCap = 0;     // sum of IRAM region sizes (bytes)
    QString outputDir;
    QString listingDir;
    QString outputName;
    // Map path from a Linker "Misc controls" --list= override, resolved
    // against the uvprojx directory. Empty when no override is configured.
    QString linkerMapFile;
    // uvprojx <AdsLmap>: the Listing tab "Linker Listing" map checkbox. When
    // false Keil never writes a .map, so not finding one is expected.
    bool linkerMapEnabled = true;
    bool hookInstalled = false;
    QString mapPath;
    bool mapFound = false;
};

// Result of matching an After Build linker output (#L, a .axf path) back to a
// project target. target is only meaningful when matched; mapPath is the best
// map file for the axf (map next to the axf first, then the target's own
// project-resolved map).
struct AxfContextMatch {
    KeilTarget target;
    bool matched = false;
    QString mapPath;
};

struct KeilProjectData {
    QString sourcePath;
    QString kind;             // uvprojx / uvmpw / dir / other / missing
    QString error;
    QStringList uvprojxFiles; // absolute paths, de-duplicated
    QVector<KeilTarget> targets;
};

namespace ProjectText {

struct Decoded {
    QString text;
    bool isUtf8 = true;
    bool hadBom = false;
    bool ok = false;
    QString error;
};

// Keil project files are UTF-8 or ANSI (GBK on zh-CN systems). Mirrors the
// reference tool: try strict UTF-8 first, fall back to the system codepage.
Decoded decodeFile(const QString &path);
QByteArray encodeText(const QString &text, bool isUtf8, bool hadBom);

} // namespace ProjectText

// Expands a uvprojx / uvmpw / directory input and parses every target of every
// project file, including hook marker detection and .map discovery.
KeilProjectData parseKeilProject(const QString &path);

// Maps an After Build .axf path (exactly as Keil expands #L, i.e. possibly
// relative to the uvprojx directory) to its target and map file.
AxfContextMatch resolveAxfContext(const KeilProjectData &data, const QString &axfPath);

class KeilProject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString sourcePath READ sourcePath NOTIFY dataChanged)
    Q_PROPERTY(QString kind READ kind NOTIFY dataChanged)
    Q_PROPERTY(bool valid READ valid NOTIFY dataChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY dataChanged)
    Q_PROPERTY(QVariantList targets READ targets NOTIFY dataChanged)

public:
    explicit KeilProject(QObject *parent = nullptr);

    bool load(const QString &path);
    void reload();

    const KeilProjectData &data() const { return m_data; }
    const QVector<KeilTarget> &targetList() const { return m_data.targets; }

    QString sourcePath() const { return m_data.sourcePath; }
    QString kind() const { return m_data.kind; }
    bool valid() const { return !m_data.uvprojxFiles.isEmpty(); }
    QString errorString() const { return m_data.error; }
    QVariantList targets() const;

signals:
    void dataChanged();

private:
    KeilProjectData m_data;
};
