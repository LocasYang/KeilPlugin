#pragma once

#include <QString>
#include <QStringList>

namespace UvprojxHook {

struct PatchOutcome {
    bool changed = false;
    QString newText;
    QStringList notes;
    QString error;
};

// Substrings that identify a hook command written by this application (or by
// the legacy Python reference tool, which uninstall still cleans up).
QStringList toolMarkers();
bool containsToolMarker(const QString &text);

// After Build command written into the project. The "#L" key sequence is
// expanded by Keil to the CURRENT target's linker output (.axf), so each
// target reports only itself. When wrap is not empty the original user
// command is wrapped: run it first, then print the size report. Commands
// containing double quotes cannot be passed verbatim (Keil runs the command
// line directly), so they are base64-wrapped with wrapBase64=true.
QString hookCommand(const QString &uvprojxPath, const QString &wrap = QString(),
                    bool wrapBase64 = false);

// Patches every <AfterMake> block inside the raw (still XML-escaped) text.
PatchOutcome patchProjectText(const QString &text, const QString &uvprojxPath, bool remove);

PatchOutcome installInFile(const QString &uvprojxPath);
PatchOutcome uninstallInFile(const QString &uvprojxPath);

} // namespace UvprojxHook
