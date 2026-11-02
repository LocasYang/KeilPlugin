#include "keilproject.h"
#include "novoproject.h"

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QRegularExpression>
#include <QSet>
#include <QStringConverter>
#include <QStringDecoder>
#include <QStringEncoder>
#include <QXmlStreamReader>

#include "uvprojxhook.h"

namespace {

QString cleanPath(const QString &path)
{
    return QDir::cleanPath(QFileInfo(path).absoluteFilePath());
}

qint64 sumRegionSizes(const QString &cpuText, const char *kind)
{
    static const QRegularExpression numberRe(QStringLiteral("0[xX][0-9A-Fa-f]+|\\d+"));
    const QRegularExpression regionRe(QStringLiteral("%1\\(([^)]*)\\)").arg(kind));

    qint64 total = 0;
    auto it = regionRe.globalMatch(cpuText);
    while (it.hasNext()) {
        const QString inner = it.next().captured(1);
        QList<qint64> values;
        auto numberIt = numberRe.globalMatch(inner);
        while (numberIt.hasNext()) {
            const QString number = numberIt.next().captured(0);
            bool converted = false;
            const qint64 parsed = number.startsWith(QLatin1String("0x"), Qt::CaseInsensitive)
                                          ? number.toLongLong(&converted, 16)
                                          : number.toLongLong(&converted, 10);
            if (converted)
                values.append(parsed);
        }
        for (int i = 0; i + 1 < values.size(); i += 2)
            total += values.at(i + 1);
    }
    return total;
}

QString newestMapInDir(const QString &directory)
{
    QString best;
    QDateTime bestTime;
    const QDir dir(directory);
    const QStringList maps = dir.entryList({ QStringLiteral("*.map") }, QDir::Files);
    for (const QString &entry : maps) {
        const QFileInfo info(dir.absoluteFilePath(entry));
        if (info.lastModified() > bestTime) {
            bestTime = info.lastModified();
            best = info.absoluteFilePath();
        }
    }
    return best;
}

// Extracts an armlink --list= path (the map file override) from linker
// "Misc controls" text. Handles both "--list=path" and "--list path",
// quoted or not; returns an empty string when absent.
QString linkerListPath(const QString &miscControls)
{
    static const QRegularExpression re(
        QStringLiteral("--list\\s*(?:=|\\s)\\s*(\"[^\"]+\"|[^\\s\"]+)"),
        QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch match = re.match(miscControls);
    if (!match.hasMatch())
        return QString();
    QString value = match.captured(1);
    value.remove(QLatin1Char('"'));
    return value.trimmed();
}

// Nearest common ancestor of two absolute paths; empty when they share no
// component (different drives on Windows).
QString commonAncestor(const QString &a, const QString &b)
{
    const QStringList partsA = cleanPath(a).split(QLatin1Char('/'));
    const QStringList partsB = cleanPath(b).split(QLatin1Char('/'));
    QStringList common;
    const int count = qMin(partsA.size(), partsB.size());
    for (int i = 0; i < count; ++i) {
        if (partsA.at(i).compare(partsB.at(i), Qt::CaseInsensitive) != 0)
            break;
        common.append(partsA.at(i));
    }
    if (common.isEmpty())
        return QString();
    if (common.size() == 1 && common.first().endsWith(QLatin1Char(':')))
        return common.first() + QLatin1Char('/');
    return common.join(QLatin1Char('/'));
}

// Bounded recursive search for *.map under root. A file whose name starts with
// the target's output name beats any other map name; newest modification time
// wins inside each tier. Used only as a last-resort layer, so it stays cheap:
// depth-limited, file-count-capped, and it never descends into VCS folders.
QString searchMapTree(const QString &root, const QString &outputName)
{
    struct Frame {
        QString path;
        int depth;
    };

    QString namedBest;
    QDateTime namedTime;
    QString anyBest;
    QDateTime anyTime;
    qint64 visited = 0;

    QVector<Frame> pending{ Frame{ cleanPath(root), 0 } };
    while (!pending.isEmpty()) {
        const Frame frame = pending.takeLast();
        const QDir dir(frame.path);
        const QFileInfoList maps = dir.entryInfoList({ QStringLiteral("*.map") }, QDir::Files,
                                                      QDir::NoSort);
        for (const QFileInfo &info : maps) {
            if (++visited > 20000)
                return namedBest.isEmpty() ? anyBest : namedBest;
            const bool named = !outputName.isEmpty()
                               && info.completeBaseName().startsWith(outputName,
                                                                     Qt::CaseInsensitive);
            if (named) {
                if (info.lastModified() > namedTime) {
                    namedTime = info.lastModified();
                    namedBest = info.absoluteFilePath();
                }
            } else if (info.lastModified() > anyTime) {
                anyTime = info.lastModified();
                anyBest = info.absoluteFilePath();
            }
        }
        if (frame.depth >= 4)
            continue;
        const QFileInfoList subdirs = dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot,
                                                        QDir::NoSort);
        for (const QFileInfo &subdir : subdirs) {
            const QString name = subdir.fileName();
            if (name.compare(QLatin1String(".git"), Qt::CaseInsensitive) == 0
                || name.compare(QLatin1String(".svn"), Qt::CaseInsensitive) == 0)
                continue;
            pending.append(Frame{ subdir.absoluteFilePath(), frame.depth + 1 });
        }
    }
    return namedBest.isEmpty() ? anyBest : namedBest;
}

// Layered .map resolution for a target. The uvprojx is the authority: an
// explicit linker --list= override first, then the Listing/Output folders.
// "Newest .map" in those folders and a bounded recursive search around the
// project only kick in when the configured locations hold no map at all.
QString findMapFile(const QString &projectDir, const KeilTarget &target)
{
    QStringList candidates;
    if (!target.linkerMapFile.isEmpty())
        candidates.append(target.linkerMapFile);
    if (!target.outputName.isEmpty()) {
        if (!target.listingDir.isEmpty())
            candidates.append(QDir(target.listingDir).absoluteFilePath(target.outputName
                                                                       + QStringLiteral(".map")));
        if (!target.outputDir.isEmpty())
            candidates.append(QDir(target.outputDir).absoluteFilePath(target.outputName
                                                                      + QStringLiteral(".map")));
    }
    for (const QString &candidate : candidates) {
        if (QFileInfo::exists(candidate))
            return cleanPath(candidate);
    }

    QSet<QString> directories;
    if (!target.listingDir.isEmpty())
        directories.insert(cleanPath(target.listingDir));
    if (!target.outputDir.isEmpty())
        directories.insert(cleanPath(target.outputDir));
    QString best;
    QDateTime bestTime;
    for (const QString &directory : directories) {
        const QString found = newestMapInDir(directory);
        if (found.isEmpty())
            continue;
        const QFileInfo info(found);
        if (info.lastModified() > bestTime) {
            bestTime = info.lastModified();
            best = found;
        }
    }
    if (!best.isEmpty())
        return best;

    QString root = projectDir;
    for (const QString &directory : { target.listingDir, target.outputDir }) {
        if (directory.isEmpty())
            continue;
        const QString ancestor = commonAncestor(root, directory);
        if (!ancestor.isEmpty())
            root = ancestor;
    }
    return searchMapTree(root, target.outputName);
}

QString joinDirectory(const QString &base, const QString &sub)
{
    QString normalized = sub;
    normalized.replace(QLatin1Char('\\'), QLatin1Char('/'));
    if (normalized.isEmpty() || normalized == QLatin1String("."))
        return base;
    return QDir::cleanPath(QDir(base).absoluteFilePath(normalized));
}

void parseUvprojxTargets(const QString &uvprojx, KeilProjectData &data)
{
    const ProjectText::Decoded decoded = ProjectText::decodeFile(uvprojx);
    if (!decoded.ok) {
        data.error = decoded.error;
        return;
    }

    QXmlStreamReader reader(decoded.text);
    const QString projectDir = QFileInfo(uvprojx).absolutePath();
    const QString projectBase = QFileInfo(uvprojx).completeBaseName();

    bool insideTarget = false;
    bool insideLdads = false;
    KeilTarget current;
    QString userProg1;
    QString userProg2;
    QString cpuText;
    bool armCompiler6 = false;
    bool haveV6Lto = false;
    bool v6Lto = false;
    bool haveLegacyLto = false;
    bool legacyLto = false;
    bool hasTarget = false;

    auto finalize = [&]() {
        if (!hasTarget)
            return;
        current.flashCap = sumRegionSizes(cpuText, "IROM");
        current.ramCap = sumRegionSizes(cpuText, "IRAM");
        if (armCompiler6 && haveV6Lto) {
            current.ltoKnown = true;
            current.ltoEnabled = v6Lto;
        } else if (haveLegacyLto) {
            current.ltoKnown = true;
            current.ltoEnabled = legacyLto;
        }
        current.outputDir = joinDirectory(projectDir, current.outputDir);
        current.listingDir = joinDirectory(projectDir, current.listingDir);
        if (!current.linkerMapFile.isEmpty())
            current.linkerMapFile = joinDirectory(projectDir, current.linkerMapFile);
        current.mapPath = findMapFile(projectDir, current);
        current.mapFound = !current.mapPath.isEmpty();
        current.hookInstalled =
                UvprojxHook::containsToolMarker(userProg1 + QStringLiteral(" ") + userProg2);
        current.reportLanguage = UvprojxHook::reportLanguage(
                userProg1 + QStringLiteral(" ") + userProg2);
        current.legacyMemoryReportInstalled = UvprojxHook::containsLegacyMemoryReport(
                userProg1 + QStringLiteral(" ") + userProg2);
        data.targets.append(current);
        hasTarget = false;
        userProg1.clear();
        userProg2.clear();
        cpuText.clear();
        armCompiler6 = false;
        haveV6Lto = false;
        v6Lto = false;
        haveLegacyLto = false;
        legacyLto = false;
    };

    while (!reader.atEnd()) {
        const auto token = reader.readNext();
        if (token == QXmlStreamReader::StartElement) {
            const QStringView name = reader.name();
            if (name == QLatin1String("Target")) {
                finalize();
                insideTarget = true;
                insideLdads = false;
                hasTarget = true;
                current = KeilTarget{};
                current.project = projectBase;
                current.uvprojx = cleanPath(uvprojx);
                continue;
            }
            if (!insideTarget)
                continue;
            if (name == QLatin1String("TargetName"))
                current.name = reader.readElementText().trimmed();
            else if (name == QLatin1String("Device"))
                current.device = reader.readElementText().trimmed();
            else if (name == QLatin1String("Cpu"))
                cpuText = reader.readElementText();
            else if (name == QLatin1String("uAC6"))
                armCompiler6 = reader.readElementText().trimmed() == QLatin1String("1");
            else if (name == QLatin1String("v6Lto")) {
                haveV6Lto = true;
                v6Lto = reader.readElementText().trimmed() == QLatin1String("1");
            }
            else if (name == QLatin1String("uLtcg")) {
                haveLegacyLto = true;
                legacyLto = reader.readElementText().trimmed() == QLatin1String("1");
            }
            else if (name == QLatin1String("OutputDirectory"))
                current.outputDir = reader.readElementText().trimmed();
            else if (name == QLatin1String("OutputName"))
                current.outputName = reader.readElementText().trimmed();
            else if (name == QLatin1String("ListingPath"))
                current.listingDir = reader.readElementText().trimmed();
            else if (name == QLatin1String("FilePath")) {
                const QString filePath = reader.readElementText().trimmed();
                if (!filePath.isEmpty()) {
                    const QString absolute = joinDirectory(projectDir, filePath);
                    if (!current.sourceFiles.contains(absolute, Qt::CaseInsensitive))
                        current.sourceFiles.append(absolute);
                }
            }
            else if (name == QLatin1String("LDads"))
                insideLdads = true;
            else if (insideLdads && name == QLatin1String("Misc"))
                current.linkerMapFile = linkerListPath(reader.readElementText());
            else if (name == QLatin1String("AdsLmap"))
                current.linkerMapEnabled = reader.readElementText().trimmed()
                                           != QLatin1String("0");
            else if (name == QLatin1String("UserProg1Name"))
                userProg1 = reader.readElementText();
            else if (name == QLatin1String("UserProg2Name"))
                userProg2 = reader.readElementText();
        } else if (token == QXmlStreamReader::EndElement) {
            if (reader.name() == QLatin1String("Target")) {
                finalize();
                insideTarget = false;
            } else if (reader.name() == QLatin1String("LDads")) {
                insideLdads = false;
            }
        }
    }
    finalize();

    if (reader.hasError() && reader.error() != QXmlStreamReader::PrematureEndOfDocumentError) {
        data.error = QStringLiteral("解析 %1 失败: %2").arg(uvprojx, reader.errorString());
    }
}

} // namespace

AxfContextMatch resolveAxfContext(const KeilProjectData &data, const QString &axfPath)
{
    AxfContextMatch result;
    if (data.uvprojxFiles.isEmpty() || data.targets.isEmpty())
        return result;

    // Keil expands #L relative to the uvprojx directory, so resolve a relative
    // axf path against that instead of whatever CWD the hook happens to run in.
    QFileInfo info(axfPath);
    if (info.isRelative())
        info.setFile(QDir(QFileInfo(data.uvprojxFiles.first()).absolutePath()), axfPath);
    const QString axfDir = QDir::cleanPath(info.absolutePath());
    const QString axfBase = info.completeBaseName();
    const QString adjacentMap = QDir(axfDir).absoluteFilePath(axfBase
                                                              + QStringLiteral(".map"));

    // Pick the owning target: the OutputName matching the axf file name wins,
    // then a target whose resolved map sits next to the axf, then a target
    // sharing the axf's output/listing directory. A single-target project
    // always matches so the hook context survives unusual layouts.
    const KeilTarget *best = nullptr;
    int bestTier = 4;
    for (const KeilTarget &target : data.targets) {
        int tier = 4;
        if (!target.outputName.isEmpty()
            && target.outputName.compare(axfBase, Qt::CaseInsensitive) == 0)
            tier = 0;
        else if (!target.mapPath.isEmpty()
                 && cleanPath(target.mapPath).compare(cleanPath(adjacentMap),
                                                      Qt::CaseInsensitive) == 0)
            tier = 1;
        else if ((!target.outputDir.isEmpty()
                  && cleanPath(target.outputDir).compare(axfDir, Qt::CaseInsensitive) == 0)
                 || (!target.listingDir.isEmpty()
                     && cleanPath(target.listingDir).compare(axfDir, Qt::CaseInsensitive)
                            == 0))
            tier = 2;
        else if (data.targets.size() == 1)
            tier = 3;
        if (tier < bestTier) {
            bestTier = tier;
            best = &target;
        }
    }

    if (best) {
        result.matched = true;
        result.target = *best;
    }
    // The map next to the linker output is Keil's own strongest hint and wins
    // even over the project-resolved map; otherwise reuse the latter (which
    // already went through the layered findMapFile lookup).
    if (QFileInfo::exists(adjacentMap))
        result.mapPath = cleanPath(adjacentMap);
    else if (result.matched)
        result.mapPath = result.target.mapPath;
    return result;
}

namespace ProjectText {

Decoded decodeFile(const QString &path)
{
    Decoded result;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        result.error = QStringLiteral("无法读取文件: %1").arg(path);
        return result;
    }
    const QByteArray raw = file.readAll();

    result.hadBom = raw.startsWith(QByteArrayLiteral("\xEF\xBB\xBF"));

    QStringDecoder utf8(QStringConverter::Utf8);
    QString text = utf8(raw);
    if (!utf8.hasError()) {
        result.text = text;
        result.isUtf8 = true;
        result.ok = true;
        return result;
    }

    QStringDecoder systemDecoder(QStringConverter::System);
    text = systemDecoder(raw);
    if (systemDecoder.hasError()) {
        result.error = QStringLiteral("无法识别文件编码: %1").arg(path);
        return result;
    }
    result.text = text;
    result.isUtf8 = false;
    result.ok = true;
    return result;
}

QByteArray encodeText(const QString &text, bool isUtf8, bool hadBom)
{
    if (isUtf8) {
        QStringEncoder encoder(QStringConverter::Utf8, QStringConverter::Flag::Stateless);
        QByteArray encoded = encoder(text);
        if (hadBom)
            encoded.prepend(QByteArrayLiteral("\xEF\xBB\xBF"));
        return encoded;
    }
    QStringEncoder encoder(QStringConverter::System, QStringConverter::Flag::Stateless);
    return encoder(text);
}

} // namespace ProjectText

KeilProjectData parseKeilProject(const QString &rawPath)
{
    KeilProjectData data;
    const QString path = cleanPath(rawPath);
    data.sourcePath = path;

    const QFileInfo info(path);
    if ((info.isFile() && (info.fileName() == QLatin1String(".cproject")
                          || info.fileName() == QLatin1String(".project")
                          || info.fileName() == QLatin1String(".novoproject")))
        || (info.isDir() && QFileInfo::exists(QDir(path).filePath(".cproject"))))
        return NovoProject::parse(path);
    if (info.isFile()) {
        const QString suffix = info.suffix().toLower();
        if (suffix == QLatin1String("uvprojx")) {
            data.kind = QStringLiteral("uvprojx");
            data.uvprojxFiles.append(path);
        } else if (suffix == QLatin1String("uvmpw")) {
            data.kind = QStringLiteral("uvmpw");
            const ProjectText::Decoded decoded = ProjectText::decodeFile(path);
            if (!decoded.ok) {
                data.error = decoded.error;
                return data;
            }
            static const QRegularExpression pathAndNameRe(
                QStringLiteral("<PathAndName>(.*?)</PathAndName>"),
                QRegularExpression::DotMatchesEverythingOption);
            auto it = pathAndNameRe.globalMatch(decoded.text);
            while (it.hasNext()) {
                QString entry = it.next().captured(1).trimmed();
                if (!entry.endsWith(QLatin1String(".uvprojx"), Qt::CaseInsensitive))
                    continue;
                entry = QDir::fromNativeSeparators(entry);
                QFileInfo entryInfo(entry);
                if (entryInfo.isRelative())
                    entryInfo.setFile(QDir(QFileInfo(path).absolutePath()), entry);
                if (entryInfo.isFile())
                    data.uvprojxFiles.append(cleanPath(entryInfo.absoluteFilePath()));
            }
            if (data.uvprojxFiles.isEmpty())
                data.error = QStringLiteral("工作区中没有可用的 .uvprojx 工程");
        } else {
            data.kind = QStringLiteral("other");
        }
    } else if (info.isDir()) {
        data.kind = QStringLiteral("dir");
        const QStringList projects = QDir(path).entryList({ QStringLiteral("*.uvprojx") },
                                                          QDir::Files, QDir::Name);
        for (const QString &entry : projects)
            data.uvprojxFiles.append(cleanPath(QDir(path).absoluteFilePath(entry)));
        if (data.uvprojxFiles.isEmpty()) {
            const QStringList workspaces = QDir(path).entryList({ QStringLiteral("*.uvmpw") },
                                                                QDir::Files, QDir::Name);
            if (!workspaces.isEmpty())
                return parseKeilProject(QDir(path).absoluteFilePath(workspaces.last()));
        }
    } else {
        data.kind = QStringLiteral("missing");
        data.error = QStringLiteral("路径不存在: %1").arg(path);
        return data;
    }

    if (data.uvprojxFiles.isEmpty()) {
        if (data.error.isEmpty())
            data.error = QStringLiteral("没有找到可解析的 .uvprojx 工程: %1").arg(path);
        return data;
    }

    for (const QString &uvprojx : data.uvprojxFiles)
        parseUvprojxTargets(uvprojx, data);

    return data;
}

KeilProject::KeilProject(QObject *parent)
    : QObject(parent)
{
}

bool KeilProject::load(const QString &path)
{
    m_data = parseKeilProject(path);
    emit dataChanged();
    return valid();
}

void KeilProject::reload()
{
    if (m_data.sourcePath.isEmpty())
        return;
    m_data = parseKeilProject(m_data.sourcePath);
    emit dataChanged();
}

QVariantList KeilProject::targets() const
{
    QVariantList list;
    for (const KeilTarget &target : m_data.targets) {
        QVariantMap map;
        map.insert(QStringLiteral("project"), target.project);
        map.insert(QStringLiteral("uvprojx"), target.uvprojx);
        map.insert(QStringLiteral("name"), target.name);
        map.insert(QStringLiteral("device"), target.device);
        map.insert(QStringLiteral("flashCap"), target.flashCap);
        map.insert(QStringLiteral("ramCap"), target.ramCap);
        map.insert(QStringLiteral("ltoKnown"), target.ltoKnown);
        map.insert(QStringLiteral("ltoEnabled"), target.ltoEnabled);
        map.insert(QStringLiteral("outputDir"), target.outputDir);
        map.insert(QStringLiteral("outputName"), target.outputName);
        map.insert(QStringLiteral("linkerMapEnabled"), target.linkerMapEnabled);
        map.insert(QStringLiteral("hookInstalled"), target.hookInstalled);
        map.insert(QStringLiteral("reportLanguage"), target.reportLanguage);
        map.insert(QStringLiteral("legacyMemoryReportInstalled"),
                   target.legacyMemoryReportInstalled);
        map.insert(QStringLiteral("mapFound"), target.mapFound);
        map.insert(QStringLiteral("mapPath"), target.mapPath);
        list.append(map);
    }
    return list;
}
