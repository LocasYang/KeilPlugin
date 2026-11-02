#pragma once
#include "mapparser.h"

// GNU ld maps omit section flags. Use the matching ARM ELF to distinguish
// allocated sections, NOBITS and debug data, including code copied into RAM.
MapSummary parseGnuMap(const QString &mapPath, const QString &text,
                       const QString &elfPath);
