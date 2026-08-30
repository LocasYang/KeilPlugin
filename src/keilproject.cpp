#include "keilproject.h"

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

QString findMapFile(const KeilTarget &target)
{
    QStringList candidates;
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
    return best;
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
    KeilTarget current;
    QString userProg1;
    QString userProg2;
    QString cpuText;
    bool hasTarget = false;

    auto finalize = [&]() {
        if (!hasTarget)
            return;
        current.flashCap = sumRegionSizes(cpuText, "IROM");
        current.ramCap = sumRegionSizes(cpuText, "IRAM");
        current.outputDir = joinDirectory(projectDir, current.outputDir);
        current.listingDir = joinDirectory(projectDir, current.listingDir);
        current.mapPath = findMapFile(current);
        current.mapFound = !current.mapPath.isEmpty();
        current.hookInstalled =
                UvprojxHook::containsToolMarker(userProg1 + QStringLiteral(" ") + userProg2);
        data.targets.append(current);
        hasTarget = false;
        userProg1.clear();
        userProg2.clear();
        cpuText.clear();
    };

    while (!reader.atEnd()) {
        const auto token = reader.readNext();
        if (token == QXmlStreamReader::StartElement) {
            const QStringView name = reader.name();
            if (name == QLatin1String("Target")) {
                finalize();
                insideTarget = true;
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
            else if (name == QLatin1String("OutputDirectory"))
                current.outputDir = reader.readElementText().trimmed();
            else if (name == QLatin1String("OutputName"))
                current.outputName = reader.readElementText().trimmed();
            else if (name == QLatin1String("ListingPath"))
                current.listingDir = reader.readElementText().trimmed();
            else if (name == QLatin1String("UserProg1Name"))
                userProg1 = reader.readElementText();
            else if (name == QLatin1String("UserProg2Name"))
                userProg2 = reader.readElementText();
        } else if (token == QXmlStreamReader::EndElement
                   && reader.name() == QLatin1String("Target")) {
            finalize();
            insideTarget = false;
        }
    }
    finalize();

    if (reader.hasError() && reader.error() != QXmlStreamReader::PrematureEndOfDocumentError) {
        data.error = QStringLiteral("解析 %1 失败: %2").arg(uvprojx, reader.errorString());
    }
}

} // namespace

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
        map.insert(QStringLiteral("outputDir"), target.outputDir);
        map.insert(QStringLiteral("outputName"), target.outputName);
        map.insert(QStringLiteral("hookInstalled"), target.hookInstalled);
        map.insert(QStringLiteral("mapFound"), target.mapFound);
        map.insert(QStringLiteral("mapPath"), target.mapPath);
        list.append(map);
    }
    return list;
}
