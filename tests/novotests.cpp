#include "mapparser.h"
#include "novoproject.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QXmlStreamReader>
#include <QtEndian>

namespace {
void write(const QString &path, const QByteArray &bytes)
{
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly) || f.write(bytes) != bytes.size())
        qFatal("Cannot write test fixture");
}
QByteArray read(const QString &path)
{
    QFile f(path); f.open(QIODevice::ReadOnly); return f.readAll();
}
QByteArray elfFixture()
{
    struct S { const char *name; quint32 addr, size, flags, type, load; };
    const QList<S> sections {
        {".text", 0x08000000, 32, 6, 1, 0x08000000},
        {".ramcode", 0, 16, 6, 1, 0x08000020},
        {".data", 0x20000000, 8, 3, 1, 0x08000030},
        {".bss", 0x20000008, 16, 3, 8, 0x20000008},
        {".heap", 0x20000018, 8, 0, 1, 0x20000018},
        {".debug_info", 0, 6, 0, 1, 0}
    };
    QByteArray b(4096, '\0'), names(1, '\0');
    auto put16 = [&](int p, quint16 v) { qToLittleEndian(v, b.data() + p); };
    auto put32 = [&](int p, quint32 v) { qToLittleEndian(v, b.data() + p); };
    b.replace(0, 7, QByteArray("\x7f" "ELF\x01\x01\x01", 7));
    put16(16, 2); put16(18, 40); put32(20, 1);
    put32(28, 52); put32(32, 2048); put16(40, 52);
    put16(42, 32); put16(44, 3); put16(46, 40); put16(48, 8); put16(50, 7);
    int segment = 0;
    for (int i = 0; i < sections.size(); ++i) {
        const auto &s = sections[i];
        const int p = 2048 + (i + 1) * 40, offset = 512 + i * 64;
        put32(p, names.size()); names.append(s.name).append('\0');
        put32(p + 4, s.type); put32(p + 8, s.flags); put32(p + 12, s.addr);
        put32(p + 16, offset); put32(p + 20, s.size); put32(p + 32, 4);
        if ((s.flags & 2) && s.type != 8) {
            const int h = 52 + segment++ * 32;
            put32(h, 1); put32(h + 4, offset); put32(h + 8, s.addr);
            put32(h + 12, s.load); put32(h + 16, s.size); put32(h + 20, s.size);
        }
    }
    const int str = 2048 + 7 * 40;
    put32(str, names.size()); names.append(".shstrtab").append('\0');
    put32(str + 4, 3); put32(str + 16, 1024); put32(str + 20, names.size());
    b.replace(1024, names.size(), names);
    return b;
}
QByteArray mapFixture()
{
    return R"(Discarded input sections
 .text          0x00000000 0xffff discarded.o
Memory Configuration
Name Origin Length Attributes
ALIAS 0x08000000 0x10 xr
FLASH 0x08000000 0x100 xr
ITCM 0x00000000 0x100 xrw
RAM 0x20000000 0x100 rw
*default* 0x00000000 0xffffffff
Linker script and memory map
.text 0x08000000 0x20
 .text.main 0x08000000 0x10 ./src/main.o
 .text.lib
                0x08000010 0x4 C:/Tool Chain/lib.a(foo.o)
 *fill*         0x08000014 0xc
.ramcode
                0x00000000 0x10 load address 0x08000020
 .fast          0x00000000 0x10 ./src/main.o
.data 0x20000000 0x8 load address 0x08000030
 .data          0x20000000 0x8 ./src/main.o
.bss 0x20000008 0x10
 COMMON         0x20000008 0x10 ./src/state.o
.heap 0x20000018 0x8
 *fill*         0x20000018 0x8
.debug_info 0x00000000 0x6
 .debug_info    0x00000000 0x6 ./src/main.o
)";
}
}

int testNovoSupport()
{
    int failed = 0;
    auto check = [&](bool ok, const char *label) {
        if (!ok) { qWarning("FAIL Novo: %s", label); ++failed; }
    };
    QTemporaryDir dir;
    const QString map = dir.filePath("sample.map"), elf = dir.filePath("sample.elf");
    write(map, mapFixture()); write(elf, elfFixture());
    auto s = MapParser::parse(map);
    check(s.ok && s.gnu, "GNU map + ARM ELF accepted");
    if (!s.ok) qWarning().noquote() << s.error;
    check(s.totalROM == 56 && s.totalRW == 48, "RAM execution counts code, data, bss and reservations");
    check(s.flashCapacity == 256 && s.ramCapacity == 512, "overlapping bank capacity counted once");
    check(s.code == 48 && s.rwData == 8 && s.ziData == 24 && s.reservedRam == 8,
          "ELF attributes determine composition and heap reservation");
    qint64 rom = 0, ram = 0;
    bool mainFound = false, libFound = false;
    for (const auto &c : s.components) {
        rom += c.rom; ram += c.ram;
        check(!c.name.contains("discarded"), "discarded sections excluded");
        if (c.name == "src/main.o") mainFound = c.rom == 40 && c.ram == 24;
        if (c.name == "C:/Tool Chain/lib.a(foo.o)") libFound = c.rom == 4;
    }
    check(mainFound && libFound, "wrapped input rows and library paths with spaces");
    check(rom == s.totalROM && ram == s.totalRW, "object plus linker rows reconcile totals");
    QByteArray mismatch = mapFixture(); mismatch.replace(".text 0x08000000 0x20", ".text 0x08000000 0x21");
    write(map, mismatch); check(!MapParser::parse(map).ok, "stale MAP/ELF rejected");
    write(map, mapFixture()); write(elf, elfFixture().left(80));
    check(!MapParser::parse(map).ok, "truncated ELF rejected");
    QFile::remove(elf); check(!MapParser::parse(map).ok, "missing ELF rejected without guessed sizes");

    const QString text = QStringLiteral(R"(<?xml version="1.0"?>
<cproject><storageModule moduleId="ilg.gnuarmeclipse.managedbuild.cross">
<configuration name="Debug" artifactName="${ProjName}" postbuildStep="objdump &quot;a.elf&quot; &gt; &quot;a.dis&quot;; objcopy -O binary a.elf a.bin"><folderInfo/></configuration>
<configuration name="Release" artifactName="${ProjName}"><folderInfo/></configuration>
</storageModule></cproject>)");
    const QString project = dir.filePath(".cproject");
    const auto installed = NovoProject::patchText(text, project, false, "zh-CN");
    check(installed.error.isEmpty() && installed.changed, "postbuild install");
    check(installed.newText.count("--novo-hook=") == 2, "exactly one hook per configuration");
    check(installed.newText.contains("objcopy -O binary a.elf a.bin"), "original export preserved");
    const auto repeat = NovoProject::patchText(installed.newText, project, false, "zh-CN");
    check(repeat.error.isEmpty() && !repeat.changed, "install idempotence");
    const auto updated = NovoProject::patchText(installed.newText, project, false, "en");
    check(updated.newText.count("--lang en") == 2 && updated.newText.count("--novo-hook=") == 2,
          "language update does not duplicate hooks");
    const auto removed = NovoProject::patchText(updated.newText, project, true);
    check(removed.error.isEmpty() && removed.newText == text, "uninstall restores exact original attributes");
    check(!NovoProject::patchText("<invalid>", project, false).error.isEmpty(), "invalid XML rejected");
    write(project, text.toUtf8());
    write(dir.filePath(".project"), "<projectDescription><name>Demo</name></projectDescription>");
    const auto parsed = parseKeilProject(dir.path());
    check(parsed.kind == "cproject" && parsed.targets.size() == 2, "directory loads Debug and Release");
    if (parsed.targets.size() == 2)
        check(parsed.targets[1].elfPath == dir.filePath("Release/Demo.elf"), "configuration-specific ELF path");
    const auto fileInstall = NovoProject::patchFile(project, false);
    check(fileInstall.error.isEmpty() && fileInstall.changed, "atomic install");
    check(read(project + ".keilpluginstudio.bak") == text.toUtf8(), "original backup created");
    check(parseKeilProject(project).targets.first().hookInstalled, "hook status parsed");
    const auto fileRemove = NovoProject::patchFile(project, true);
    check(fileRemove.error.isEmpty() && read(project) == text.toUtf8(), "file uninstall round trip");
    return failed;
}
