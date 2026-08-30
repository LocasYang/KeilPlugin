#include "mapparser.h"

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

} // namespace

MapSummary MapParser::parse(const QString &mapPath)
{
    MapSummary summary;
    summary.mapPath = mapPath;

    QFile file(mapPath);
    if (!file.open(QIODevice::ReadOnly)) {
        summary.error = QStringLiteral("无法读取 map 文件: %1").arg(mapPath);
        return summary;
    }
    const QString text = QString::fromUtf8(file.readAll());

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

        struct Aggregate { qint64 code = 0; qint64 ro = 0; qint64 rw = 0; qint64 zi = 0; };
        QHash<QString, Aggregate> aggregated;

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

            Aggregate &slot = aggregated[base];
            slot.code += match.captured(1).toLongLong();
            slot.ro += match.captured(3).toLongLong();
            slot.rw += match.captured(4).toLongLong();
            slot.zi += match.captured(5).toLongLong();
        }

        for (auto it2 = aggregated.constBegin(); it2 != aggregated.constEnd(); ++it2) {
            MapComponent component;
            component.name = it2.key();
            component.rom = it2->code + it2->ro + it2->rw;
            component.ram = it2->rw + it2->zi;
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
