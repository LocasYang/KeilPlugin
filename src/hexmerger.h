#pragma once

#include <QString>
#include <map>

namespace HexMerger {

struct MergeResult {
    bool ok = false;
    QString error;
    quint64 dataBytes = 0;
};

// Loads an Intel HEX image into a sparse byte map.
// Record types: 00 data, 01 EOF, 02 segment address, 04 linear address,
// 05 start linear address.
bool loadHex(const QString &path, std::map<quint32, quint8> &memory, quint64 *dataBytes,
             QString *error);

// Merges two non-overlapping Intel HEX images (typically bootloader + app)
// into one factory image.
MergeResult merge(const QString &bootHex, const QString &appHex, const QString &outputHex);

} // namespace HexMerger
