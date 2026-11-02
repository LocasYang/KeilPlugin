#pragma once
#include "keilproject.h"
#include "uvprojxhook.h"

namespace NovoProject {
KeilProjectData parse(const QString &path);
UvprojxHook::PatchOutcome patchText(const QString &text, const QString &projectPath,
                                   bool remove, const QString &language = QStringLiteral("en"));
UvprojxHook::PatchOutcome patchFile(const QString &path, bool remove,
                                   const QString &language = QStringLiteral("en"));
}
