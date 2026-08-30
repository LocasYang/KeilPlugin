#pragma once

#include <QString>
#include <QVector>

struct MapComponent {
    QString name;
    qint64 rom = 0;
    qint64 ram = 0;
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
    QVector<MapComponent> components;
};

namespace MapParser {

// Parses an armlink generated .map file: "Total RO/RW/ROM Size" lines give the
// image footprints, "Grand Totals" gives the Code/RO/RW/ZI breakdown and the
// "Image component sizes" section is aggregated per object file.
MapSummary parse(const QString &mapPath);

// Parses user supplied size strings such as "230K", "0x39800" or "235520".
qint64 parseSizeBytes(const QString &text, bool *ok = nullptr);

QString formatBytes(qint64 bytes);

// ASCII occupancy bar used by the command line report (Keil Build Output).
QString percentBar(qint64 used, qint64 cap, int width = 24);

} // namespace MapParser
