#include "mapparser.h"
#include "gnumapparser.h"

#include <QFile>
#include <QRegularExpression>

namespace {

const QRegularExpression &roSizeRe()
{
    static const QRegularExpression re(QStringLiteral("Total\\s+RO\\s+Size[^)]*\\)\\s+(\\d+)"));
    return re;
}

const QRegularExpression &rwSizeRe()
{
    static const QRegularExpression re(QStringLiteral("Total\\s+RW\\s+Size[^)]*\\)\\s+(\\d+)"));
    return re;
}

const QRegularExpression &romSizeRe()
{
    static const QRegularExpression re(QStringLiteral("Total\\s+ROM\\s+Size[^)]*\\)\\s+(\\d+)"));
    return re;
}

const QRegularExpression &grandTotalsRe()
{
    static const QRegularExpression re(
        QStringLiteral("^\\s*(\\d+)\\s+\\d+\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+\\d+\\s+Grand Totals\\s*$"),
        QRegularExpression::MultilineOption);
    return re;
}

// One "Image component sizes" row: Code, inc.data, RO, RW, ZI, Debug, object.
const QRegularExpression &componentRowRe()
{
    static const QRegularExpression re(
        QStringLiteral("^\\s*(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+(\\d+)\\s+(\\S.*?)\\s*$"),
        QRegularExpression::MultilineOption);
    return re;
}

const QStringList &nonObjectPrefixes()
{
    static const QStringList prefixes { QStringLiteral("Total"),
                                        QStringLiteral("Grand"),
                                        QStringLiteral("ELF"),
                                        QStringLiteral("Image"),
                                        QStringLiteral("Object") };
    return prefixes;
}

const QRegularExpression &loadRegionRe()
{
    static const QRegularExpression re(
        QStringLiteral("^\\s*Load Region\\s+(\\S+)\\s+\\(Base:\\s*"
                       "(0[xX][0-9A-Fa-f]+),\\s*Size:\\s*"
                       "(0[xX][0-9A-Fa-f]+),\\s*Max:\\s*"
                       "(0[xX][0-9A-Fa-f]+)"));
    return re;
}

const QRegularExpression &executionRegionRe()
{
    static const QRegularExpression re(
        QStringLiteral("^\\s*Execution Region\\s+(\\S+)\\s+\\(Exec base:\\s*"
                       "(0[xX][0-9A-Fa-f]+),\\s*Load base:\\s*[^,]+,\\s*Size:\\s*"
                       "(0[xX][0-9A-Fa-f]+),\\s*Max:\\s*"
                       "(0[xX][0-9A-Fa-f]+)"));
    return re;
}

const QRegularExpression &sectionAttributeRe()
{
    // Memory-map row: Exec Addr, Load Addr, Size, Type, Attr, ...
    static const QRegularExpression re(
        QStringLiteral("^\\s*0[xX][0-9A-Fa-f]+\\s+\\S+\\s+"
                       "0[xX][0-9A-Fa-f]+\\s+\\S+\\s+(RO|RW)\\b"));
    return re;
}

quint64 hexValue(const QString &text)
{
    bool ok = false;
    const quint64 value = text.mid(2).toULongLong(&ok, 16);
    return ok ? value : 0;
}

MemoryRegionType identifyRegionType(const QString &name, bool hasRo, bool hasRw)
{
    if (hasRw && !hasRo)
        return MemoryRegionType::Ram;
    if (hasRo && !hasRw)
        return MemoryRegionType::Rom;

    const QString upper = name.toUpper();
    if (upper.contains(QLatin1String("IRAM"))
        || upper.contains(QLatin1String("RAM"))
        || upper.startsWith(QLatin1String("RW_"))
        || upper.contains(QLatin1String("_RW"))
        || upper.contains(QLatin1String("ZI")))
        return MemoryRegionType::Ram;
    if (upper.contains(QLatin1String("IROM"))
        || upper.contains(QLatin1String("ROM"))
        || upper.contains(QLatin1String("FLASH"))
        || upper.startsWith(QLatin1String("ER_RO"))
        || upper.startsWith(QLatin1String("RO_")))
        return MemoryRegionType::Rom;
    return MemoryRegionType::Unknown;
}

QVector<MapMemoryRegion> parseMemoryRegions(const QString &text)
{
    QVector<MapMemoryRegion> regions;
    const QStringList lines = text.split(QLatin1Char('\n'));
    QString currentLoadRegion;

    for (qsizetype i = 0; i < lines.size(); ++i) {
        const QRegularExpressionMatch load = loadRegionRe().match(lines.at(i));
        if (load.hasMatch()) {
            currentLoadRegion = load.captured(1);
            continue;
        }

        const QRegularExpressionMatch execution = executionRegionRe().match(lines.at(i));
        if (!execution.hasMatch())
            continue;

        bool hasRo = false;
        bool hasRw = false;
        for (qsizetype j = i + 1; j < lines.size(); ++j) {
            if (loadRegionRe().match(lines.at(j)).hasMatch()
                || executionRegionRe().match(lines.at(j)).hasMatch()
                || lines.at(j).contains(QLatin1String("Image component sizes")))
                break;
            const QRegularExpressionMatch attribute = sectionAttributeRe().match(lines.at(j));
            if (!attribute.hasMatch())
                continue;
            if (attribute.captured(1) == QLatin1String("RW"))
                hasRw = true;
            else
                hasRo = true;
        }

        MapMemoryRegion region;
        region.name = execution.captured(1);
        region.loadRegion = currentLoadRegion;
        region.base = hexValue(execution.captured(2));
        region.used = hexValue(execution.captured(3));
        region.total = hexValue(execution.captured(4));
        region.type = identifyRegionType(region.name, hasRo, hasRw);
        regions.append(region);
    }
    return regions;
}

} // namespace

MapSummary MapParser::parse(const QString &mapPath, const QString &elfPath)
{
    MapSummary summary;
    summary.mapPath = mapPath;

    QFile file(mapPath);
    if (!file.open(QIODevice::ReadOnly)) {
        summary.error = QStringLiteral("无法读取 map 文件: %1").arg(mapPath);
        return summary;
    }
    const QString text = QString::fromUtf8(file.readAll());

    if (text.contains(QStringLiteral("Linker script and memory map")))
        return parseGnuMap(mapPath, text, elfPath);

    summary.regions = parseMemoryRegions(text);

    const auto ro = roSizeRe().match(text);
    const auto rw = rwSizeRe().match(text);
    const auto rom = romSizeRe().match(text);
    if (!ro.hasMatch() || !rw.hasMatch() || !rom.hasMatch()) {
        summary.error = QStringLiteral("map 文件中没有 armlink 尺寸汇总: %1").arg(mapPath);
        return summary;
    }
    summary.totalRO = ro.captured(1).toLongLong();
    summary.totalRW = rw.captured(1).toLongLong();
    summary.totalROM = rom.captured(1).toLongLong();

    const auto grand = grandTotalsRe().match(text);
    if (grand.hasMatch()) {
        summary.hasDetail = true;
        summary.code = grand.captured(1).toLongLong();
        summary.roData = grand.captured(2).toLongLong();
        summary.rwData = grand.captured(3).toLongLong();
        summary.ziData = grand.captured(4).toLongLong();
    }

    const int sectionStart = text.indexOf(QStringLiteral("Image component sizes"));
    if (sectionStart >= 0) {
        int sectionEnd = text.indexOf(QStringLiteral("Total RO"), sectionStart);
        if (sectionEnd < 0)
            sectionEnd = text.size();
        const QString section = text.mid(sectionStart, sectionEnd - sectionStart);

        // The first component table contains only application object files.
        // Preserve linker order for the detailed source table and stop before
        // "Object Totals" so library members/summaries are not mixed into it.
        const int objectHeader = section.indexOf(QStringLiteral("Object Name"));
        const int objectTotals = section.indexOf(QStringLiteral("Object Totals"), objectHeader);
        if (objectHeader >= 0 && objectTotals > objectHeader) {
            const QString objectSection = section.mid(objectHeader,
                                                      objectTotals - objectHeader);
            auto objectIt = componentRowRe().globalMatch(objectSection);
            while (objectIt.hasNext()) {
                const auto match = objectIt.next();
                MapComponent component;
                component.name = match.captured(7).simplified();
                component.rom = match.captured(1).toLongLong()
                                + match.captured(3).toLongLong()
                                + match.captured(4).toLongLong();
                component.ram = match.captured(4).toLongLong()
                                + match.captured(5).toLongLong();
                summary.objectComponents.append(component);
            }
        }

        struct Aggregate { qint64 code = 0; qint64 ro = 0; qint64 rw = 0; qint64 zi = 0; };
        QHash<QString, Aggregate> aggregated;
        QStringList aggregateOrder;

        auto it = componentRowRe().globalMatch(section);
        while (it.hasNext()) {
            const auto match = it.next();
            QString base = match.captured(7).simplified();
            const QString head = base.section(QLatin1Char(' '), 0, 0);
            bool prefixed = false;
            for (const QString &prefix : nonObjectPrefixes()) {
                if (head.startsWith(prefix)) {
                    prefixed = true;
                    break;
                }
            }
            if (prefixed || !head.contains(QLatin1Char('.')))
                continue;
            // Drop a trailing section qualifier such as "foo.o(i.SPI_Init)".
            QRegularExpression trailingParen(QStringLiteral("\\([^()]*\\)$"));
            const QString stripped = base;
            base.remove(trailingParen);
            if (base.isEmpty())
                base = stripped;

            if (!aggregated.contains(base))
                aggregateOrder.append(base);
            Aggregate &slot = aggregated[base];
            slot.code += match.captured(1).toLongLong();
            slot.ro += match.captured(3).toLongLong();
            slot.rw += match.captured(4).toLongLong();
            slot.zi += match.captured(5).toLongLong();
        }

        for (const QString &name : aggregateOrder) {
            const Aggregate &aggregate = aggregated[name];
            MapComponent component;
            component.name = name;
            component.rom = aggregate.code + aggregate.ro + aggregate.rw;
            component.ram = aggregate.rw + aggregate.zi;
            summary.components.append(component);
        }
    }

    summary.ok = true;
    return summary;
}

qint64 MapParser::parseSizeBytes(const QString &text, bool *ok)
{
    if (ok)
        *ok = false;
    QString value = text.trimmed();

    static const QRegularExpression hexRe(QStringLiteral("^0[xX][0-9A-Fa-f]+$"));
    if (hexRe.match(value).hasMatch()) {
        bool converted = false;
        const qint64 parsed = value.toLongLong(&converted, 16);
        if (!converted)
            return 0;
        if (ok)
            *ok = true;
        return parsed;
    }

    static const QRegularExpression decRe(QStringLiteral("^(\\d+(?:\\.\\d+)?)\\s*([KkMm]?)[Bb]?$"));
    const auto match = decRe.match(value);
    if (!match.hasMatch())
        return 0;
    bool converted = false;
    double number = match.captured(1).toDouble(&converted);
    if (!converted)
        return 0;
    const QString unit = match.captured(2).toUpper();
    if (unit == QLatin1String("K"))
        number *= 1024.0;
    else if (unit == QLatin1String("M"))
        number *= 1024.0 * 1024.0;
    if (ok)
        *ok = true;
    return static_cast<qint64>(number);
}

QString MapParser::formatBytes(qint64 bytes)
{
    return QStringLiteral("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
}

QString MapParser::percentBar(qint64 used, qint64 cap, int width)
{
    if (cap <= 0)
        return QString();
    double fraction = static_cast<double>(used) / static_cast<double>(cap);
    fraction = qBound(0.0, fraction, 1.0);
    const int filled = static_cast<int>(qRound(fraction * width));
    return QStringLiteral("[%1%2]").arg(QString(filled, QLatin1Char('#')),
                                        QString(width - filled, QLatin1Char('.')));
}
