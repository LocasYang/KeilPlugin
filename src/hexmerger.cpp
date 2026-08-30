#include "hexmerger.h"

#include <QFile>
#include <QFileInfo>

namespace {

struct RecordParts {
    quint16 address = 0;
    quint8 type = 0;
    QByteArray data;
};

bool decodeRecord(const QByteArray &line, const QString &source, int lineNumber,
                  RecordParts *parts, QString *error)
{
    const QByteArray trimmed = line.trimmed();
    if (!trimmed.startsWith(':')) {
        *error = QStringLiteral("%1:%2: 无效的 Intel HEX 记录").arg(source).arg(lineNumber);
        return false;
    }
    const QByteArray raw = QByteArray::fromHex(trimmed.mid(1));
    if (raw.size() < 5 || raw.size() != raw.at(0) + 5) {
        *error = QStringLiteral("%1:%2: 记录长度无效").arg(source).arg(lineNumber);
        return false;
    }
    int checksum = 0;
    for (char byte : raw)
        checksum += static_cast<quint8>(byte);
    if ((checksum & 0xFF) != 0) {
        *error = QStringLiteral("%1:%2: 校验和不匹配").arg(source).arg(lineNumber);
        return false;
    }
    parts->address = (static_cast<quint8>(raw.at(1)) << 8) | static_cast<quint8>(raw.at(2));
    parts->type = static_cast<quint8>(raw.at(3));
    parts->data = raw.mid(4, raw.size() - 5);
    return true;
}

QByteArray makeRecord(quint16 address, quint8 type, const QByteArray &data)
{
    QByteArray body;
    body.append(static_cast<char>(data.size()));
    body.append(static_cast<char>((address >> 8) & 0xFF));
    body.append(static_cast<char>(address & 0xFF));
    body.append(static_cast<char>(type));
    body.append(data);
    int checksum = 0;
    for (char byte : body)
        checksum += static_cast<quint8>(byte);
    body.append(static_cast<char>((-checksum) & 0xFF));
    return ':' + body.toHex().toUpper();
}

void writeHex(const QString &path, const std::map<quint32, quint8> &memory,
              const QByteArray &startAddress, QString *error)
{
    QByteArray out;
    quint32 currentUpper = 0xFFFFFFFFu;
    bool haveUpper = false;

    auto address = memory.begin();
    while (address != memory.end()) {
        const quint32 first = address->first;
        const quint32 upper = first >> 16;
        if (!haveUpper || upper != currentUpper) {
            QByteArray upperData;
            upperData.append(static_cast<char>((upper >> 8) & 0xFF));
            upperData.append(static_cast<char>(upper & 0xFF));
            out.append(makeRecord(0, 0x04, upperData));
            out.append("\r\n");
            currentUpper = upper;
            haveUpper = true;
        }

        QByteArray chunk;
        chunk.append(static_cast<char>(address->second));
        ++address;
        while (address != memory.end() && chunk.size() < 16) {
            const quint32 next = address->first;
            if (next != first + static_cast<quint32>(chunk.size())
                || (next >> 16) != upper)
                break;
            chunk.append(static_cast<char>(address->second));
            ++address;
        }
        out.append(makeRecord(static_cast<quint16>(first & 0xFFFF), 0x00, chunk));
        out.append("\r\n");
    }

    if (!startAddress.isEmpty()) {
        out.append(makeRecord(0, 0x05, startAddress));
        out.append("\r\n");
    }
    out.append(makeRecord(0, 0x01, QByteArray()));
    out.append("\r\n");

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        *error = QStringLiteral("无法写入输出文件: %1").arg(path);
        return;
    }
    file.write(out);
    file.close();
}

bool loadImage(const QString &path, std::map<quint32, quint8> *memory,
               QByteArray *startAddress, quint64 *dataBytes, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        *error = QStringLiteral("无法读取 HEX 文件: %1").arg(path);
        return false;
    }
    const QStringList lines = QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'));
    quint32 upperAddress = 0;

    int lineNumber = 0;
    for (const QString &line : lines) {
        ++lineNumber;
        if (line.trimmed().isEmpty())
            continue;
        RecordParts parts;
        if (!decodeRecord(line.toUtf8(), path, lineNumber, &parts, error))
            return false;

        if (parts.type == 0x00) {
            const quint32 absolute = upperAddress + parts.address;
            for (int offset = 0; offset < parts.data.size(); ++offset)
                (*memory)[absolute + static_cast<quint32>(offset)] =
                        static_cast<quint8>(parts.data.at(offset));
            *dataBytes += static_cast<quint64>(parts.data.size());
        } else if (parts.type == 0x01) {
            break;
        } else if (parts.type == 0x02) {
            if (parts.data.size() != 2) {
                *error = QStringLiteral("%1:%2: 段地址记录无效").arg(path).arg(lineNumber);
                return false;
            }
            upperAddress = (static_cast<quint8>(parts.data.at(0)) << 8
                            | static_cast<quint8>(parts.data.at(1)))
                           << 4;
        } else if (parts.type == 0x04) {
            if (parts.data.size() != 2) {
                *error = QStringLiteral("%1:%2: 线性地址记录无效").arg(path).arg(lineNumber);
                return false;
            }
            upperAddress = (static_cast<quint8>(parts.data.at(0)) << 8
                            | static_cast<quint8>(parts.data.at(1)))
                           << 16;
        } else if (parts.type == 0x05) {
            if (parts.data.size() == 4 && startAddress->isEmpty())
                *startAddress = parts.data;
        }
    }
    return true;
}

} // namespace

bool HexMerger::loadHex(const QString &path, std::map<quint32, quint8> &memory,
                        quint64 *dataBytes, QString *error)
{
    QByteArray startAddress;
    return loadImage(path, &memory, &startAddress, dataBytes, error);
}

HexMerger::MergeResult HexMerger::merge(const QString &bootHex, const QString &appHex,
                                        const QString &outputHex)
{
    MergeResult result;
    std::map<quint32, quint8> merged;
    QByteArray startAddress;

    QString error;
    for (const QString &image : { bootHex, appHex }) {
        std::map<quint32, quint8> memory;
        quint64 bytes = 0;
        if (!loadImage(image, &memory, &startAddress, &bytes, &error)) {
            result.error = error;
            return result;
        }
        for (const auto &entry : memory) {
            const auto previous = merged.find(entry.first);
            if (previous != merged.end() && previous->second != entry.second) {
                result.error = QStringLiteral("地址 0x%1 处数据重叠 (合并 %2 时发现)")
                                   .arg(entry.first, 8, 16, QLatin1Char('0'))
                                   .arg(QFileInfo(image).fileName());
                return result;
            }
            merged[entry.first] = entry.second;
        }
    }

    if (merged.empty()) {
        result.error = QStringLiteral("输入镜像中没有任何数据");
        return result;
    }

    writeHex(outputHex, merged, startAddress, &error);
    if (!error.isEmpty()) {
        result.error = error;
        return result;
    }

    result.ok = true;
    result.dataBytes = static_cast<quint64>(merged.size());
    return result;
}
