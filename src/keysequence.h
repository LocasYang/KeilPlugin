#pragma once

#include <QDir>
#include <QFileInfo>
#include <QString>

// Keil key-sequence expansion for wrapped After Build commands.
//
// Keil expands key sequences ($L, @L, #L, ...) in the user command text
// BEFORE running it - but a base64-wrapped command hides them, so we have to
// expand them ourselves. The linker output path arrives via the "#L" argument
// the hook passes us, from which the other forms are derived (this matches
// the expansion Keil itself applies, e.g. fromelf -o "$L@L.bin" "#L"):
//   #L -> absolute path + name + extension of the linker output (.axf)
//   $L -> directory of the linker output, with trailing separator
//   @L -> linker output name without extension
//   %L -> linker output name with extension, no directory
inline QString expandKeilKeySequences(const QString &command, const QString &linkerOutput)
{
    if (linkerOutput.isEmpty())
        return command;
    const QFileInfo info(linkerOutput);
    QString dir = info.absolutePath();
    if (!dir.endsWith(QLatin1Char('/')) && !dir.endsWith(QLatin1Char('\\')))
        dir += QLatin1Char('\\');

    QString out = command;
    out.replace(QLatin1String("#L"),
                QDir::toNativeSeparators(info.absoluteFilePath()), Qt::CaseInsensitive);
    out.replace(QLatin1String("$L"), QDir::toNativeSeparators(dir), Qt::CaseInsensitive);
    out.replace(QLatin1String("%L"), info.fileName(), Qt::CaseInsensitive);
    out.replace(QLatin1String("@L"), info.completeBaseName(), Qt::CaseInsensitive);
    return out;
}
