#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QTextStream>

#include <cstdio>

#include "hexmerger.h"
#include "keilproject.h"
#include "keysequence.h"
#include "mapparser.h"
#include "pluginregistry.h"
#include "uvprojxhook.h"

namespace {

int failures = 0;

void check(bool condition, const QString &what)
{
    if (!condition) {
        qWarning().noquote() << "FAIL:" << what;
        ++failures;
    }
}

const char *kProjectTemplate = R"(<?xml version="1.0" encoding="UTF-8" standalone="no" ?>
<Project>
  <Target>
    <TargetName>App</TargetName>
    <TargetOption>
      <TargetCommonOption>
        <Device>STM32F103ZE</Device>
        <Vendor>STMicroelectronics</Vendor>
        <Cpu>IROM(0x08000000,0x80000) IRAM(0x20000000,0x10000) CPUTYPE("Cortex-M3") CLOCK(12000000) ELITTLE</Cpu>
        <OutputDirectory>.\Objects\</OutputDirectory>
        <OutputName>demo</OutputName>
        <ListingPath>.\Listings\</ListingPath>
        <AfterMake>
          <UserProg1Name>%1</UserProg1Name>
          <RunUserProg1>%2</RunUserProg1>
          <UserProg2Name>%3</UserProg2Name>
          <RunUserProg2>%4</RunUserProg2>
        </AfterMake>
      </TargetCommonOption>
    </TargetOption>
  </Target>
</Project>
)";

QString mapFixture()
{
    QStringList lines;
    lines << "Section Cross References"
          << ""
          << "    Image component sizes"
          << ""
          << ""
          << "      Code (inc. data)   RO Data    RW Data    ZI Data      Debug   Object Name"
          << ""
          << "       100          12         40          8        320       8448   main.o"
          << "         60           0          0          0       1100       1024   bsp_led.o"
          << "       180          12         48       1120       3136      16896   Total"
          << ""
          << "      6856        1316       4212       1168       4000      17880   Grand Totals"
          << "      6856        1316       4212       1168       4000      17880   ELF Image Totals"
          << "      6856        1316       4212       5168          0      0     ROM Totals"
          << ""
          << "  Total RO  Size (Code + RO Data)           11068 (  10.81kB)"
          << "  Total RW  Size (RW Data + ZI Data)         5168 (   5.05kB)"
          << "  Total ROM Size (Code + RO Data + RW Data)  12236 (  11.95kB)"
          << "";
    return lines.join(QLatin1Char('\n'));
}

void writeText(const QString &path, const QString &text)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qWarning().noquote() << "FAIL: cannot write" << path << file.errorString();
        ++failures;
        return;
    }
    file.write(text.toUtf8());
    file.close();
}

QString readText(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return QString();
    return QString::fromUtf8(file.readAll());
}

void testMapParser()
{
    QTemporaryDir dir;
    const QString mapPath = dir.filePath("demo.map");
    writeText(mapPath, mapFixture());

    const MapSummary summary = MapParser::parse(mapPath);
    check(summary.ok, "map parse ok");
    check(summary.totalRO == 11068, "total RO parsed");
    check(summary.totalRW == 5168, "total RW parsed");
    check(summary.totalROM == 12236, "total ROM parsed");
    check(summary.hasDetail, "grand totals found");
    check(summary.code == 6856, "code column");
    check(summary.roData == 4212, "ro column");
    check(summary.rwData == 1168, "rw column");
    check(summary.ziData == 4000, "zi column");
    check(summary.components.size() == 2, "two aggregated objects");
    for (const MapComponent &component : summary.components) {
        if (component.name == QLatin1String("main.o")) {
            check(component.rom == 148, "main.o rom = code+ro+rw");
            check(component.ram == 328, "main.o ram = rw+zi");
        }
        if (component.name == QLatin1String("bsp_led.o")) {
            check(component.rom == 60, "bsp_led.o rom");
            check(component.ram == 1100, "bsp_led.o ram");
        }
    }

    bool ok = false;
    check(MapParser::parseSizeBytes("230K", &ok) == 230 * 1024 && ok, "size 230K");
    check(MapParser::parseSizeBytes("0x39800", &ok) == 0x39800 && ok, "size 0x39800");
    check(MapParser::parseSizeBytes("235520", &ok) == 235520 && ok, "size plain");
    check(MapParser::percentBar(230686, 235520, 24).count(QLatin1Char('#')) == 24, "bar full");
    check(MapParser::percentBar(0, 235520, 24).count(QLatin1Char('.')) == 24, "bar empty");
}

void testHookInstallUninstall()
{
    QTemporaryDir dir;
    const QString projectPath = dir.filePath("demo.uvprojx");

    // 1. Fresh project: hook goes into slot 2 first (same order as the
    //    reference tool).
    writeText(projectPath, QString(kProjectTemplate).arg("", "0", "", "0"));
    auto outcome = UvprojxHook::installInFile(projectPath);
    check(outcome.error.isEmpty(), "install no error");
    check(outcome.changed, "install changed file");
    QString installed = readText(projectPath);
    check(UvprojxHook::containsToolMarker(installed), "marker present after install");
    check(installed.contains(QStringLiteral("--size-report")), "report flag written");
    check(installed.contains("<RunUserProg2>1</RunUserProg2>"), "slot2 enabled");
    check(installed.contains("<RunUserProg1>0</RunUserProg1>"), "slot1 untouched");

    // Re-install is a no-op with a note.
    outcome = UvprojxHook::installInFile(projectPath);
    check(!outcome.changed, "re-install unchanged");
    check(outcome.notes.join(QLatin1Char(';')).contains(QStringLiteral("已经安装")),
          "re-install note recorded");

    // 2. Uninstall restores the empty slots.
    outcome = UvprojxHook::uninstallInFile(projectPath);
    check(outcome.error.isEmpty() && outcome.changed, "uninstall changed file");
    QString restored = readText(projectPath);
    check(!UvprojxHook::containsToolMarker(restored), "marker gone after uninstall");
    check(restored.contains("<UserProg2Name></UserProg2Name>"), "slot2 cleared");
    check(restored.contains("<RunUserProg2>0</RunUserProg2>"), "slot2 disabled");
    check(restored.contains(QLatin1String("STM32F103ZE")), "other content preserved");

    // 3. Both slots busy -> slot 2 gets wrapped; uninstall unwraps.
    writeText(projectPath, QString(kProjectTemplate)
                               .arg("fromelf --bin .\\Objects\\demo.axf", "1",
                                    "copy_output.bat", "1"));
    outcome = UvprojxHook::installInFile(projectPath);
    check(outcome.error.isEmpty() && outcome.changed, "wrap install changed file");
    installed = readText(projectPath);
    check(installed.contains(QStringLiteral("--wrap")), "wrap flag written");
    check(installed.contains(QStringLiteral("copy_output.bat")),
          "original slot2 command preserved inside wrap");
    check(installed.contains("<UserProg1Name>fromelf --bin .\\Objects\\demo.axf</UserProg1Name>"),
          "slot1 command untouched");

    outcome = UvprojxHook::uninstallInFile(projectPath);
    restored = readText(projectPath);
    if (!outcome.changed || UvprojxHook::containsToolMarker(restored))
        qWarning().noquote() << "unwrap debug: changed=" << outcome.changed << " error="
                             << outcome.error << " notes=" << outcome.notes
                             << " slot2=" << restored.section("<UserProg2Name>", 1, 1);
    check(outcome.changed && !UvprojxHook::containsToolMarker(restored),
          "wrap removed after uninstall");
    check(restored.contains("<UserProg2Name>copy_output.bat</UserProg2Name>"),
          "original slot2 command restored verbatim");

    // 4. Legacy python hook is also cleaned up.
    writeText(projectPath,
              QString(kProjectTemplate)
                  .arg("", "0",
                       "python \"C:\\tools\\keil_size_report.py\" \"demo.uvprojx\"", "1"));
    outcome = UvprojxHook::uninstallInFile(projectPath);
    restored = readText(projectPath);
    check(outcome.changed && !restored.contains(QStringLiteral("keil_size_report.py")),
          "legacy python hook removed");

    // 5. Both slots busy with quoted commands -> base64 wrap (no skip).
    writeText(projectPath, QString(kProjectTemplate)
                               .arg("fromelf --bin -o \"$L@L.bin\" \"#L\"", "1",
                                    "..\\..\\tool\\convert.exe \"$L@L.bin\" \"$L@L.app\"", "1"));
    outcome = UvprojxHook::installInFile(projectPath);
    check(outcome.error.isEmpty() && outcome.changed, "quoted wrap install changed file");
    installed = readText(projectPath);
    check(installed.contains(QStringLiteral("--wrap-b64")), "base64 wrap flag written");
    check(!installed.contains(QStringLiteral("无法包装")),
          "quoted command not skipped");
    check(installed.contains(QStringLiteral("<UserProg1Name>fromelf --bin -o \"$L@L.bin\" \"#L\"</UserProg1Name>")),
          "slot1 quoted command untouched");

    // 5b. A base64 hook written by an older build (without --project) is
    //     upgraded in place; the wrapped original stays unchanged.
    const QString legacyOriginal = QStringLiteral("..\\..\\tool\\convert.exe \"$L@L.bin\" \"$L@L.app\"");
    const QString legacyB64 = QString::fromLatin1(legacyOriginal.toUtf8().toBase64());
    writeText(projectPath,
              QString(kProjectTemplate)
                  .arg("fromelf --bin -o \"$L@L.bin\" \"#L\"", "1",
                       "\"C:\\old\\KeilPluginStudio.exe\" --wrap-b64 " + legacyB64 + " \"#L\"",
                       "1"));
    outcome = UvprojxHook::installInFile(projectPath);
    check(outcome.changed, "b64 hook upgrade changed file");
    installed = readText(projectPath);
    check(installed.contains(QStringLiteral("--project")), "b64 hook upgraded with --project");
    check(installed.contains(QStringLiteral("--wrap-b64")), "b64 hook still wrapped");

    outcome = UvprojxHook::uninstallInFile(projectPath);
    restored = readText(projectPath);
    check(!UvprojxHook::containsToolMarker(restored) && !restored.contains(QStringLiteral("--project")),
          "upgraded b64 hook uninstalls cleanly");
    check(restored.contains(QStringLiteral("<UserProg2Name>") + legacyOriginal + QStringLiteral("</UserProg2Name>")),
          "legacy b64 original restored verbatim");

    // 6. Old-format plain hooks are upgraded to the "#L" per-target format.
    writeText(projectPath, QString(kProjectTemplate)
                               .arg("", "0",
                                    "\"C:\\ somewhere\\KeilPluginStudio.exe\" --size-report \"demo.uvprojx\"",
                                    "1"));
    outcome = UvprojxHook::installInFile(projectPath);
    check(outcome.changed, "upgrade install changed file");
    installed = readText(projectPath);
    check(installed.contains(QStringLiteral("--size-report \"#L\"")),
          "upgraded hook uses #L");
    check(!installed.contains(QStringLiteral("somewhere")), "old path replaced");

    outcome = UvprojxHook::uninstallInFile(projectPath);
    restored = readText(projectPath);
    check(!UvprojxHook::containsToolMarker(restored), "upgraded hook uninstalls cleanly");
}

void testKeySequenceExpansion()
{
    const QString linkerOutput = QStringLiteral("D:\\proj\\release_output\\ycomm_wifi.axf");
    const QString expanded = expandKeilKeySequences(
        QStringLiteral("..\\..\\tool\\convert.exe \"$L@L.bin\" \"$L@L.app\" \"#L\""),
        linkerOutput);
    check(expanded.contains(QLatin1String("D:\\proj\\release_output\\ycomm_wifi.bin")),
          "key $L@L expands to dir + basename");
    check(expanded.contains(QLatin1String("ycomm_wifi.app")), "second $L@L expands too");
    check(expanded.contains(QLatin1String("D:\\proj\\release_output\\ycomm_wifi.axf")),
          "key #L expands to absolute linker output");
    check(!expanded.contains(QLatin1String("$L")) && !expanded.contains(QLatin1String("@L"))
              && !expanded.contains(QLatin1String("#L")),
          "no key sequence left unexpanded");

    // Empty linker output leaves the command untouched.
    check(expandKeilKeySequences(QStringLiteral("echo $L"), QString())
              == QStringLiteral("echo $L"),
          "empty linker output is a no-op");
}

void testProjectParse()
{
    QTemporaryDir dir;
    const QString projectPath = dir.filePath("demo.uvprojx");
    writeText(projectPath, QString(kProjectTemplate).arg("", "0", "", "0"));

    const KeilProjectData data = parseKeilProject(projectPath);
    check(data.kind == QLatin1String("uvprojx"), "kind uvprojx");
    check(data.uvprojxFiles.size() == 1, "one uvprojx");
    check(data.targets.size() == 1, "one target parsed");
    if (data.targets.size() == 1) {
        const KeilTarget &target = data.targets.first();
        check(target.name == QLatin1String("App"), "target name");
        check(target.device == QLatin1String("STM32F103ZE"), "device");
        check(target.flashCap == 0x80000, "flash capacity from IROM");
        check(target.ramCap == 0x10000, "ram capacity from IRAM");
        check(target.outputName == QLatin1String("demo"), "output name");
        check(target.outputDir == QDir::cleanPath(dir.path() + "/Objects"), "output dir joined");
        check(target.listingDir == QDir::cleanPath(dir.path() + "/Listings"), "listing dir joined");
        check(!target.hookInstalled, "hook not installed initially");

        const QString mapPath = QDir::cleanPath(dir.path() + "/Listings/demo.map");
        writeText(mapPath, mapFixture());
        KeilProjectData withMap = parseKeilProject(projectPath);
        check(withMap.targets.first().mapFound, "map discovered");
        check(withMap.targets.first().mapPath == mapPath, "map path matches");
    }
}

// Flexible Target block for map-resolution tests: output/listing paths,
// linker Misc controls and the AdsLmap (Linker Listing) checkbox per target.
const char *kTargetBlock = R"(
  <Target>
    <TargetName>%1</TargetName>
    <TargetOption>
      <TargetCommonOption>
        <Device>STM32F103ZE</Device>
        <Vendor>STMicroelectronics</Vendor>
        <Cpu>IROM(0x08000000,0x80000) IRAM(0x20000000,0x10000) CPUTYPE("Cortex-M3") CLOCK(12000000) ELITTLE</Cpu>
        <OutputDirectory>%2</OutputDirectory>
        <OutputName>%3</OutputName>
        <ListingPath>%4</ListingPath>
      </TargetCommonOption>
      <TargetArmAds>
        <ArmAdsMisc>
          <AdsLmap>%6</AdsLmap>
        </ArmAdsMisc>
        <LDads>
          <Misc>%5</Misc>
        </LDads>
      </TargetArmAds>
    </TargetOption>
  </Target>
)";

void appendTarget(QString &xml, const QString &name, const QString &outputDir,
                  const QString &outputName, const QString &listingPath,
                  const QString &misc, const QString &adsLmap)
{
    xml += QString(kTargetBlock).arg(name, outputDir, outputName, listingPath, misc, adsLmap);
}

QString layeredProjectHeader()
{
    return QStringLiteral("<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\" ?>\n"
                          "<Project>");
}

void testMapResolutionLayers()
{
    // 1) Map in a third custom directory: neither the Listing nor the Output
    //    folder holds it, so only the bounded project-tree search finds it.
    {
        QTemporaryDir dir;
        QDir().mkpath(dir.path() + "/Project/uv5");
        const QString projectPath = dir.path() + "/Project/uv5/app.uvprojx";
        QString xml = layeredProjectHeader();
        appendTarget(xml, "App", "../../Output", "app", "../../Listings", "", "1");
        xml += QStringLiteral("\n</Project>\n");
        writeText(projectPath, xml);
        QDir().mkpath(dir.path() + "/Output");
        QDir().mkpath(dir.path() + "/Listings");
        QDir().mkpath(dir.path() + "/Maps");
        const QString mapPath = QDir::cleanPath(dir.path() + "/Maps/app.map");
        writeText(mapPath, mapFixture());

        const KeilProjectData data = parseKeilProject(projectPath);
        check(data.targets.size() == 1, "custom dir: one target");
        if (data.targets.size() == 1) {
            check(data.targets.first().mapFound, "map in custom dir found recursively");
            check(data.targets.first().mapPath == mapPath, "custom dir map path");
        }
    }

    // 2) Linker "Misc controls" --list= override: wins even when the Listing
    //    folder also holds a map, and may rename it anywhere.
    {
        QTemporaryDir dir;
        const QString projectPath = dir.filePath("app.uvprojx");
        QString xml = layeredProjectHeader();
        appendTarget(xml, "App", "./Objects", "app", "./Listings",
                     QStringLiteral("--list=\"Renamed/build.map\""), "1");
        xml += QStringLiteral("\n</Project>\n");
        writeText(projectPath, xml);
        QDir().mkpath(dir.path() + "/Listings");
        writeText(dir.path() + "/Listings/app.map", mapFixture());
        QDir().mkpath(dir.path() + "/Renamed");
        const QString overrideMap = QDir::cleanPath(dir.path() + "/Renamed/build.map");
        writeText(overrideMap, mapFixture());

        const KeilProjectData data = parseKeilProject(projectPath);
        check(data.targets.size() == 1, "--list override: one target");
        if (data.targets.size() == 1) {
            check(data.targets.first().mapFound, "--list override map found");
            check(data.targets.first().mapPath == overrideMap, "--list override wins");
        }
    }

    // 3) AdsLmap=0 (Linker Listing unchecked) is surfaced so a missing map can
    //    be explained instead of just asking the user to rebuild.
    {
        QTemporaryDir dir;
        const QString projectPath = dir.filePath("app.uvprojx");
        QString xml = layeredProjectHeader();
        appendTarget(xml, "App", "./Objects", "app", "./Listings", "", "0");
        xml += QStringLiteral("\n</Project>\n");
        writeText(projectPath, xml);

        const KeilProjectData data = parseKeilProject(projectPath);
        check(data.targets.size() == 1, "AdsLmap=0: one target");
        if (data.targets.size() == 1)
            check(!data.targets.first().linkerMapEnabled, "AdsLmap=0 recorded");
    }
}

void testResolveAxfContext()
{
    // Regression for the ISO-STM32 layout: uvprojx nested two folders deep,
    // outputs hoisted to <root>/Output and <root>/Listing, #L handed over
    // relative to the uvprojx directory. The map lives in Listing, not next
    // to the axf — this must still resolve.
    {
        QTemporaryDir dir;
        QDir().mkpath(dir.path() + "/Project/uv5");
        const QString projectPath = dir.path() + "/Project/uv5/iso.uvprojx";
        QString xml = layeredProjectHeader();
        appendTarget(xml, "Template", "../../Output", "Template", "../../Listing", "", "1");
        xml += QStringLiteral("\n</Project>\n");
        writeText(projectPath, xml);
        QDir().mkpath(dir.path() + "/Output");
        QDir().mkpath(dir.path() + "/Listing");
        writeText(dir.path() + "/Listing/Template.map", mapFixture());

        const KeilProjectData data = parseKeilProject(projectPath);
        check(data.targets.size() == 1, "nested layout: one target");
        check(data.targets.first().mapFound, "nested layout map found");
        const AxfContextMatch match =
                resolveAxfContext(data, QStringLiteral("../../Output/Template.axf"));
        check(match.matched, "axf matched to its target");
        check(match.target.name == QLatin1String("Template"), "axf target name");
        check(match.target.device == QLatin1String("STM32F103ZE"), "axf target device");
        check(match.target.flashCap == 0x80000, "axf target flash cap");
        check(match.mapPath == QDir::cleanPath(dir.path() + "/Listing/Template.map"),
              "map from Listing dir when axf has no sibling map");
    }

    // A map sitting next to the axf is the strongest hint and wins over the
    // project-resolved one.
    {
        QTemporaryDir dir;
        const QString projectPath = dir.filePath("flat.uvprojx");
        QString xml = layeredProjectHeader();
        appendTarget(xml, "Flat", "./Objects", "flat", "./Listings", "", "1");
        xml += QStringLiteral("\n</Project>\n");
        writeText(projectPath, xml);
        QDir().mkpath(dir.path() + "/Listings");
        writeText(dir.path() + "/Listings/flat.map", mapFixture());
        QDir().mkpath(dir.path() + "/Objects");
        writeText(dir.path() + "/Objects/flat.map", mapFixture());

        const KeilProjectData data = parseKeilProject(projectPath);
        const AxfContextMatch match = resolveAxfContext(data, QStringLiteral("Objects/flat.axf"));
        check(match.matched, "flat layout axf matched");
        check(match.mapPath == QDir::cleanPath(dir.path() + "/Objects/flat.map"),
              "map next to axf wins");
    }

    // Multi-target projects: the OutputName disambiguates when all targets
    // share one output folder.
    {
        QTemporaryDir dir;
        const QString projectPath = dir.filePath("multi.uvprojx");
        QString xml = layeredProjectHeader();
        appendTarget(xml, "Boot", "./Objects", "boot", "./Listings", "", "1");
        appendTarget(xml, "App", "./Objects", "app", "./Listings", "", "1");
        xml += QStringLiteral("\n</Project>\n");
        writeText(projectPath, xml);
        QDir().mkpath(dir.path() + "/Objects");
        writeText(dir.path() + "/Objects/boot.map", mapFixture());

        const KeilProjectData data = parseKeilProject(projectPath);
        check(data.targets.size() == 2, "multi-target project parsed");
        const AxfContextMatch match = resolveAxfContext(data, QStringLiteral("Objects/app.axf"));
        check(match.matched && match.target.name == QLatin1String("App"),
              "outputName disambiguates shared output dir");
    }
}

void testHexMerger()
{
    QTemporaryDir dir;
    const QString bootPath = dir.filePath("boot.hex");
    const QString appPath = dir.filePath("app.hex");
    const QString outPath = dir.filePath("merged.hex");

    // Boot: extended linear address 0x08000000, two data bytes.
    auto makeRecord = [](quint16 address, quint8 type, const QByteArray &data) {
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
        return ":" + body.toHex().toUpper();
    };

    QByteArray boot;
    boot.append(makeRecord(0, 0x04, QByteArray::fromHex("0800"))).append("\n");
    boot.append(makeRecord(0x0000, 0x00, QByteArray::fromHex("A5A6"))).append("\n");
    boot.append(makeRecord(0, 0x05, QByteArray::fromHex("080000C1"))).append("\n");
    boot.append(makeRecord(0, 0x01, QByteArray())).append("\n");
    writeText(bootPath, QString::fromLatin1(boot));

    QByteArray app;
    app.append(makeRecord(0, 0x04, QByteArray::fromHex("0801"))).append("\n");
    app.append(makeRecord(0x0000, 0x00, QByteArray::fromHex("12345678"))).append("\n");
    app.append(makeRecord(0, 0x01, QByteArray())).append("\n");
    writeText(appPath, QString::fromLatin1(app));

    const HexMerger::MergeResult result = HexMerger::merge(bootPath, appPath, outPath);
    check(result.ok, "merge ok");
    check(result.dataBytes == 6, "six data bytes merged");

    std::map<quint32, quint8> memory;
    quint64 bytes = 0;
    QString error;
    check(HexMerger::loadHex(outPath, memory, &bytes, &error), "merged hex loads");
    check(error.isEmpty(), "no load error");
    check(memory[0x08000000] == 0xA5 && memory[0x08000001] == 0xA6, "boot bytes preserved");
    check(memory[0x08010000] == 0x12 && memory[0x08010003] == 0x78, "app bytes preserved");

    // Overlapping images must fail.
    writeText(appPath, QString::fromLatin1(
                           makeRecord(0, 0x04, QByteArray::fromHex("0800")) + "\n"
                           + makeRecord(0x0000, 0x00, QByteArray::fromHex("FF")) + "\n"
                           + makeRecord(0, 0x01, QByteArray()) + "\n"));
    const HexMerger::MergeResult conflict = HexMerger::merge(bootPath, appPath, outPath);
    check(!conflict.ok, "overlap rejected");
    if (!conflict.error.contains(QStringLiteral("重叠")))
        qWarning().noquote() << "overlap error was:" << conflict.error;
    check(conflict.error.contains(QStringLiteral("重叠")), "overlap error message");
}

void testRegistry()
{
    const PluginRegistry &registry = PluginRegistry::instance();
    check(registry.plugins().size() >= 2, "built-in plugins registered");
    KeilPlugin *sizeReport = registry.find(QLatin1String("keil-size-report"));
    check(sizeReport != nullptr, "size report plugin found");
    if (sizeReport) {
        const PluginDescriptor descriptor = sizeReport->descriptor();
        check(descriptor.capabilities.contains(QLatin1String("install")),
              "size report can install");
        check(descriptor.capabilities.contains(QLatin1String("report")),
              "size report can report");

        KeilTarget target;
        target.hookInstalled = true;
        check(sizeReport->targetInstalled(target), "targetInstalled follows hook flag");
    }
    KeilPlugin *hexMerge = registry.find(QLatin1String("hex-merge"));
    check(hexMerge != nullptr, "hex merge plugin found");
    if (hexMerge) {
        const PluginDescriptor descriptor = hexMerge->descriptor();
        check(!descriptor.capabilities.contains(QLatin1String("install")),
              "hex merge has no install capability");
        QVariantMap toolResult = hexMerge->runTool({});
        check(!toolResult.value("ok").toBool(), "hex merge without args fails gracefully");
    }
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("KeilPluginCoreTest"));

    testMapParser();
    testHookInstallUninstall();
    testKeySequenceExpansion();
    testProjectParse();
    testMapResolutionLayers();
    testResolveAxfContext();
    testHexMerger();
    testRegistry();

    if (failures == 0) {
        qInfo().noquote() << "KeilPluginCoreTest: all checks passed";
        return 0;
    }
    qWarning().noquote() << "KeilPluginCoreTest:" << failures << "check(s) failed";
    return 1;
}
