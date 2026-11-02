#pragma once

#include <QString>
#include <QVector>

struct MapComponent {
    QString name;
    qint64 rom = 0;
    qint64 ram = 0;
};

enum class MemoryRegionType {
    Unknown,
    Rom,
    Ram,
};

// An armlink execution region or GNU MEMORY address range. The address range and
// Size/Max values are the most useful view for multi-bank devices where a
// single aggregate IROM/IRAM percentage hides individual bank pressure.
struct MapMemoryRegion {
    QString name;
    QString loadRegion;
    MemoryRegionType type = MemoryRegionType::Unknown;
    quint64 base = 0;
    quint64 used = 0;
    quint64 total = 0;
};

struct MapSummary {
    QString mapPath;
    bool ok = false;
    QString error;
    qint64 totalRO = 0;
    qint64 totalRW = 0;
    qint64 totalROM = 0;
    bool hasDetail = false;
    qint64 code = 0;
    qint64 roData = 0;
    qint64 rwData = 0;
    qint64 ziData = 0;
    bool gnu = false;
    qint64 flashCapacity = 0;
    qint64 ramCapacity = 0;
    qint64 reservedRam = 0; // linker-reserved heap/stack, included in totalRW
    // Object rows from the first "Object Name" table, in linker order. This
    // excludes library members and library summary rows and drives the full
    // source-file table in the unified report.
    // GNU reports also include archive members and explicit linker-generated rows.
    QVector<MapComponent> objectComponents;
    // All object/library rows aggregated by name. This broader set drives the
    // Top-N hotspot list, where library footprints are still useful.
    QVector<MapComponent> components;
    QVector<MapMemoryRegion> regions;
};

namespace MapParser {

// Parses an armlink generated .map file: "Total RO/RW/ROM Size" lines give the
// image footprints, each "Execution Region" supplies a bank-level usage range,
// "Grand Totals" gives the Code/RO/RW/ZI breakdown and "Image component sizes"
// is aggregated per object file.
// GNU maps are detected automatically and require a matching ARM ELF32 image;
// when elfPath is omitted, a same-stem .elf next to the map is used.
MapSummary parse(const QString &mapPath, const QString &elfPath = QString());

// Parses user supplied size strings such as "230K", "0x39800" or "235520".
qint64 parseSizeBytes(const QString &text, bool *ok = nullptr);

QString formatBytes(qint64 bytes);

// ASCII occupancy bar used by the command line report (Keil Build Output).
QString percentBar(qint64 used, qint64 cap, int width = 24);

} // namespace MapParser
