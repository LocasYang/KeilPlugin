#include "gnumapparser.h"

#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QHash>
#include <QRegularExpression>
#include <QtEndian>
#include <algorithm>

namespace {
using Span = QPair<quint64, quint64>; // half-open address interval
quint64 unionSize(QVector<Span> spans)
{
    std::sort(spans.begin(), spans.end());
    quint64 size = 0, end = 0;
    for (const auto &s : spans) {
        if (s.second > qMax(end, s.first))
            size += s.second - qMax(end, s.first);
        end = qMax(end, s.second);
    }
    return size;
}
quint64 hex(const QString &s) { return s.toULongLong(nullptr, 16); }
struct ElfSection {
    QString name;
    quint64 address = 0, size = 0, flags = 0, type = 0, load = 0;
};

bool readElf(const QString &path, QVector<ElfSection> &sections, QString &error)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        error = QStringLiteral("GNU MAP requires its matching .elf file: %1").arg(path);
        return false;
    }
    const QByteArray b = f.readAll();
    auto valid = [&](quint64 off, quint64 len) {
        return off <= quint64(b.size()) && len <= quint64(b.size()) - off;
    };
    auto u16 = [&](quint64 p) { return qFromLittleEndian<quint16>(b.constData() + p); };
    auto u32 = [&](quint64 p) { return qFromLittleEndian<quint32>(b.constData() + p); };
    if (!valid(0, 52) || b.left(4) != QByteArray("\x7f" "ELF", 4)
        || b[4] != 1 || b[5] != 1 || u16(18) != 40 || u16(16) != 2) {
        error = QStringLiteral("Expected a linked little-endian ARM ELF32 image: %1").arg(path);
        return false;
    }
    const quint64 sh = u32(32), ph = u32(28);
    const quint64 shSize = u16(46), count = u16(48), names = u16(50);
    const quint64 phSize = u16(42), phCount = u16(44);
    if (shSize < 40 || !count || names >= count || !valid(sh, count * shSize)
        || (phCount && (phSize < 32 || !valid(ph, phCount * phSize)))) {
        error = QStringLiteral("Invalid ELF section/program headers: %1").arg(path);
        return false;
    }
    const quint64 strOff = u32(sh + names * shSize + 16);
    const quint64 strSize = u32(sh + names * shSize + 20);
    if (!valid(strOff, strSize)) {
        error = QStringLiteral("Invalid ELF section names: %1").arg(path);
        return false;
    }
    for (quint64 i = 1; i < count; ++i) {
        const quint64 p = sh + i * shSize, name = u32(p);
        if (name >= strSize) { error = QStringLiteral("Invalid ELF section name offset"); return false; }
        const QByteArray n = b.mid(strOff + name, strSize - name);
        const int nul = n.indexOf('\0');
        if (nul < 0) { error = QStringLiteral("Unterminated ELF section name"); return false; }
        ElfSection s;
        s.name = QString::fromUtf8(n.left(nul));
        s.type = u32(p + 4); s.flags = u32(p + 8);
        s.address = u32(p + 12); s.size = u32(p + 20); s.load = s.address;
        const quint64 offset = u32(p + 16);
        if (s.type != 8 && !valid(offset, s.size)) {
            error = QStringLiteral("Truncated ELF section: %1").arg(s.name); return false;
        }
        if ((s.flags & 2) && s.type != 8 && s.size) {
            for (quint64 j = 0; j < phCount; ++j) {
                const quint64 h = ph + j * phSize;
                const quint64 va = u32(h + 8), bytes = u32(h + 16), off = u32(h + 4);
                if (u32(h) == 1 && s.address >= va && offset >= off
                    && s.address - va == offset - off && offset - off <= bytes
                    && s.size <= bytes - (offset - off)) {
                    s.load = u32(h + 12) + s.address - va;
                    break;
                }
            }
        }
        sections.append(s);
    }
    return true;
}
struct Section {
    QString name;
    quint64 address = 0, size = 0, load = 0;
    QStringList rows;
};
}

MapSummary parseGnuMap(const QString &mapPath, const QString &text, const QString &elfPath)
{
    MapSummary result;
    result.mapPath = mapPath;
    result.gnu = true;
    QVector<ElfSection> elf;
    const QFileInfo mapInfo(mapPath);
    const QString image = elfPath.isEmpty()
            ? QDir(mapInfo.absolutePath()).filePath(mapInfo.completeBaseName() + ".elf") : elfPath;
    if (!readElf(image, elf, result.error)) return result;
    const int mem = text.indexOf("Memory Configuration");
    const int body = text.indexOf("Linker script and memory map");
    if (mem < 0 || body <= mem) {
        result.error = QStringLiteral("GNU MAP has no MEMORY configuration: %1").arg(mapPath);
        return result;
    }
    static const QRegularExpression bankRe(
        R"(^\s*(\S+)\s+(0x[\da-fA-F]+)\s+(0x[\da-fA-F]+)\s*([rwx!ail]*)\s*$)");
    QVector<Span> flashBanks, ramBanks;
    for (const auto &line : text.mid(mem, body - mem).split('\n')) {
        const auto m = bankRe.match(line);
        if (!m.hasMatch() || m.captured(1) == "*default*") continue;
        MapMemoryRegion r;
        r.name = m.captured(1); r.base = hex(m.captured(2)); r.total = hex(m.captured(3));
        // MEMORY permissions are authoritative here: RAM code can be read-only
        // in ELF even though its physical execution bank is writable.
        const QString attr = m.captured(4);
        const QString positive = attr.section('!', 0, 0);
        r.type = positive.contains('w') ? MemoryRegionType::Ram
                : positive.contains('r') ? MemoryRegionType::Rom : MemoryRegionType::Unknown;
        result.regions.append(r);
        if (r.type == MemoryRegionType::Ram) ramBanks.append({r.base, r.base + r.total});
        if (r.type == MemoryRegionType::Rom) flashBanks.append({r.base, r.base + r.total});
    }
    if (result.regions.isEmpty()) {
        result.error = QStringLiteral("GNU MAP has no usable named MEMORY regions"); return result;
    }
    result.flashCapacity = unionSize(flashBanks);
    result.ramCapacity = unionSize(ramBanks);
    auto inBank = [&](quint64 start, quint64 size, MemoryRegionType type) {
        for (const auto &r : result.regions)
            if (r.type == type && start >= r.base && start - r.base <= r.total
                && size <= r.total - (start - r.base)) return true;
        return false;
    };

    static const QRegularExpression header(R"(^([^\s]+)\s+(0x[\da-fA-F]+)\s+(0x[\da-fA-F]+)(?:\s+load address\s+(0x[\da-fA-F]+))?\s*$)");
    static const QRegularExpression continuation(R"(^\s+(0x[\da-fA-F]+)\s+(0x[\da-fA-F]+)(?:\s+load address\s+(0x[\da-fA-F]+))?\s*$)");
    static const QRegularExpression nameOnly(R"(^([^\s]+)\s*$)");
    QVector<Section> sections;
    QString pending;
    for (const QString &line : text.mid(body).split('\n')) {
        auto m = header.match(line);
        if (m.hasMatch()) {
            sections.append({m.captured(1), hex(m.captured(2)), hex(m.captured(3)),
                             m.captured(4).isEmpty() ? hex(m.captured(2)) : hex(m.captured(4)), {}});
            pending.clear();
            continue;
        }
        if (!pending.isEmpty()) {
            m = continuation.match(line);
            if (m.hasMatch()) {
                sections.append({pending, hex(m.captured(1)), hex(m.captured(2)),
                                 m.captured(3).isEmpty() ? hex(m.captured(1)) : hex(m.captured(3)), {}});
                pending.clear();
                continue;
            }
        }
        m = nameOnly.match(line);
        if (m.hasMatch()) pending = m.captured(1);
        else if (!line.trimmed().isEmpty()) pending.clear();
        if (!sections.isEmpty()) sections.last().rows.append(line);
    }
    QVector<Span> flashUsed, ramUsed;
    QHash<QString, MapComponent> objects;
    QStringList order;
    auto addObject = [&](const QString &name, quint64 bytes, bool flash, bool ram) {
        if (!bytes || (!flash && !ram)) return;
        if (!objects.contains(name)) { order.append(name); objects[name].name = name; }
        if (flash) objects[name].rom += bytes;
        if (ram) objects[name].ram += bytes;
    };
    static const QRegularExpression inputRow(
        R"(^\s+(?:\S+\s+)?(0x[\da-fA-F]+)\s+(0x[\da-fA-F]+)\s+(.+\.(?:o|obj)(?:\))?)\s*$)");
    static const QRegularExpression reservedName(R"(^\.(?:heap|stack|_user_heap_stack)(?:[._].*)?$)", QRegularExpression::CaseInsensitiveOption);
    int matched = 0;
    for (const auto &s : sections) {
        if (!s.size) continue;
        const ElfSection *e = nullptr;
        for (const auto &candidate : elf)
            if (candidate.name == s.name && candidate.address == s.address && candidate.size == s.size) {
                e = &candidate; break;
            }
        if (!e) {
            result.error = QStringLiteral("MAP/ELF section mismatch: %1. Rebuild both files.").arg(s.name);
            return result;
        }
        const bool reserved = !(e->flags & 2) && reservedName.match(s.name).hasMatch()
                && inBank(s.address, s.size, MemoryRegionType::Ram);
        if (!(e->flags & 2) && !reserved) continue;
        ++matched;
        if (!reserved && e->type != 8 && e->load != s.load) {
            result.error = QStringLiteral("MAP/ELF load address mismatch: %1. Rebuild both files.").arg(s.name);
            return result;
        }
        const bool ram = inBank(s.address, s.size, MemoryRegionType::Ram);
        const bool flash = !reserved && e->type != 8 && inBank(s.load, s.size, MemoryRegionType::Rom);
        if (!ram && !flash) {
            result.error = QStringLiteral("Cannot classify allocated section %1 in GNU MEMORY regions").arg(s.name);
            return result;
        }
        if (flash) flashUsed.append({s.load, s.load + s.size});
        if (ram) ramUsed.append({s.address, s.address + s.size});
        if (reserved) result.reservedRam += s.size;
        if (reserved || e->type == 8) result.ziData += s.size;
        else if (e->flags & 4) result.code += s.size;
        else if (e->flags & 1) result.rwData += s.size;
        else result.roData += s.size;

        quint64 attributed = 0;
        for (const auto &line : s.rows) {
            const auto m = inputRow.match(line);
            if (!m.hasMatch()) continue;
            const quint64 addr = hex(m.captured(1)), size = hex(m.captured(2));
            if (addr < s.address || addr - s.address > s.size || size > s.size - (addr - s.address)) continue;
            QString object = QDir::fromNativeSeparators(m.captured(3).trimmed());
            if (object.startsWith("./")) object.remove(0, 2);
            addObject(object, size, flash, ram);
            attributed += size;
        }
        if (attributed > s.size) {
            result.error = QStringLiteral("Overlapping GNU input sections: %1").arg(s.name); return result;
        }
        // Keep linker tables, alignment and reservations visible instead of
        // silently assigning them to an arbitrary source file.
        addObject(QStringLiteral("[linker: %1]").arg(s.name), s.size - attributed, flash, ram);
    }
    // A stale/truncated map must not silently omit allocated ELF sections.
    for (const auto &e : elf) {
        if (!(e.flags & 2) || !e.size) continue;
        const auto found = std::find_if(sections.begin(), sections.end(), [&](const Section &s) {
            return s.name == e.name && s.address == e.address && s.size == e.size;
        });
        if (found == sections.end()) {
            result.error = QStringLiteral("Allocated ELF section missing from MAP: %1").arg(e.name); return result;
        }
    }
    if (!matched) { result.error = QStringLiteral("No allocated GNU image sections found"); return result; }
    result.totalROM = unionSize(flashUsed);
    result.totalRW = unionSize(ramUsed);
    result.totalRO = result.code + result.roData;
    result.hasDetail = true;
    for (auto &r : result.regions) {
        QVector<Span> intersections;
        const auto &spans = r.type == MemoryRegionType::Ram ? ramUsed : flashUsed;
        for (const auto &s : spans) {
            const quint64 begin = qMax(r.base, s.first), end = qMin(r.base + r.total, s.second);
            if (end > begin) intersections.append({begin, end});
        }
        r.used = unionSize(intersections);
    }
    for (const QString &name : order) {
        result.components.append(objects[name]);
        result.objectComponents.append(objects[name]);
    }
    result.ok = true;
    return result;
}
