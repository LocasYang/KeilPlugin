#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QGuiApplication>
#include <QIcon>
#include <QProcess>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QSurfaceFormat>
#include <QTimer>

#include <cstdio>

#include <algorithm>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "keilproject.h"
#include "keysequence.h"
#include "mapparser.h"
#include "pluginmanager.h"
#include "pluginregistry.h"
#include "windowchrome.h"

namespace {

QStringList nativeArguments(int argc, char *argv[])
{
    QStringList arguments;
    for (int i = 0; i < argc; ++i)
        arguments.append(QString::fromLocal8Bit(argv[i]));
    return arguments;
}

void printLine(const QString &text, bool toStderr = false)
{
    const QByteArray encoded = text.toLocal8Bit();
    std::FILE *stream = toStderr ? stderr : stdout;
    std::fwrite(encoded.constData(), 1, encoded.size(), stream);
    std::fputc('\n', stream);
    std::fflush(stream);
}

#ifdef Q_OS_WIN
// Runs the command through cmd.exe, inheriting our standard handles. A raw
// CreateProcess is used instead of QProcess so the wrapped command's output
// goes straight to our stdout (Keil Build Output) without pipe plumbing.
int runWrappedCommand(const QString &command, QString *errorDetail)
{
    QString commandLine = QStringLiteral("cmd.exe /c ") + command;
    STARTUPINFOW startupInfo;
    PROCESS_INFORMATION processInfo;
    ZeroMemory(&startupInfo, sizeof(startupInfo));
    ZeroMemory(&processInfo, sizeof(processInfo));
    startupInfo.cb = sizeof(startupInfo);

    // Keil hands us pipe handles that are not necessarily inheritable. Pass
    // duplicates created with bInheritHandle=TRUE, otherwise CreateProcess
    // fails outright and the wrapped command never runs.
    HANDLE dupIn = nullptr;
    HANDLE dupOut = nullptr;
    HANDLE dupErr = nullptr;
    const HANDLE stdIn = GetStdHandle(STD_INPUT_HANDLE);
    const HANDLE stdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    const HANDLE stdErr = GetStdHandle(STD_ERROR_HANDLE);
    const SECURITY_ATTRIBUTES inheritable = { sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE };
    if (stdOut != nullptr && stdErr != nullptr) {
        if (stdIn != nullptr
            && !DuplicateHandle(GetCurrentProcess(), stdIn, GetCurrentProcess(), &dupIn,
                                0, TRUE, DUPLICATE_SAME_ACCESS))
            dupIn = nullptr;
        if (!DuplicateHandle(GetCurrentProcess(), stdOut, GetCurrentProcess(), &dupOut,
                             0, TRUE, DUPLICATE_SAME_ACCESS)
            || !DuplicateHandle(GetCurrentProcess(), stdErr, GetCurrentProcess(), &dupErr,
                                0, TRUE, DUPLICATE_SAME_ACCESS)) {
            if (dupIn)
                CloseHandle(dupIn);
            if (dupOut)
                CloseHandle(dupOut);
            if (dupErr)
                CloseHandle(dupErr);
            if (errorDetail)
                *errorDetail = QStringLiteral("复制标准句柄失败 (GetLastError=%1)").arg(GetLastError());
            return -1;
        }
        startupInfo.dwFlags = STARTF_USESTDHANDLES;
        startupInfo.hStdInput = dupIn;
        startupInfo.hStdOutput = dupOut;
        startupInfo.hStdError = dupErr;
    }

    const BOOL inheritHandles =
            (startupInfo.dwFlags & STARTF_USESTDHANDLES) ? TRUE : FALSE;
    const BOOL started = CreateProcessW(nullptr, reinterpret_cast<LPWSTR>(commandLine.data()),
                                        nullptr, nullptr, inheritHandles, CREATE_NO_WINDOW,
                                        nullptr, nullptr, &startupInfo, &processInfo);
    if (dupIn)
        CloseHandle(dupIn);
    if (dupOut)
        CloseHandle(dupOut);
    if (dupErr)
        CloseHandle(dupErr);
    if (!started) {
        const DWORD lastError = GetLastError();
        LPWSTR buffer = nullptr;
        const DWORD length = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER
                                                | FORMAT_MESSAGE_FROM_SYSTEM
                                                | FORMAT_MESSAGE_IGNORE_INSERTS,
                                            nullptr, lastError, 0,
                                            reinterpret_cast<LPWSTR>(&buffer), 0, nullptr);
        const QString message = (length > 0 && buffer)
                ? QString::fromWCharArray(buffer, static_cast<int>(length)).trimmed()
                : QString();
        if (buffer)
            LocalFree(buffer);
        if (errorDetail)
            *errorDetail = QStringLiteral("Win32 错误 %1%2")
                               .arg(lastError)
                               .arg(message.isEmpty() ? QString()
                                                      : QStringLiteral(": ") + message);
        return -1;
    }
    WaitForSingleObject(processInfo.hProcess, INFINITE);
    DWORD exitCode = 0;
    GetExitCodeProcess(processInfo.hProcess, &exitCode);
    CloseHandle(processInfo.hThread);
    CloseHandle(processInfo.hProcess);
    return static_cast<int>(exitCode);
}
#else
int runWrappedCommand(const QString &command, QString *errorDetail)
{
    Q_UNUSED(errorDetail)
    QProcess process;
    process.setProgram(QStringLiteral("sh"));
    process.setNativeArguments(QStringLiteral("-c ") + command);
    process.start();
    if (!process.waitForStarted(-1))
        return -1;
    process.waitForFinished(-1);
    return process.exitCode();
}
#endif

QString sizeOrNA(qint64 cap)
{
    return cap > 0 ? MapParser::formatBytes(cap) : QStringLiteral("n/a");
}

QString percentText(qint64 used, qint64 cap)
{
    if (cap <= 0)
        return QString();
    return QStringLiteral(" (%1%)").arg(100.0 * used / cap, 0, 'f', 1);
}

// Port of the reference tool's report layout so the Build Output keeps the
// same look as before.
QString formatReportTarget(const KeilTarget &target, const MapSummary &summary, int top,
                           qint64 flashOverride, qint64 ramOverride)
{
    const qint64 flashCap = flashOverride > 0 ? flashOverride : target.flashCap;
    const qint64 ramCap = ramOverride > 0 ? ramOverride : target.ramCap;
    const qint64 romUsed = summary.totalROM;
    const qint64 ramUsed = summary.totalRW;

    QStringList lines;
    lines.append(QStringLiteral("Target '%1'  Device: %2  [%3]")
                     .arg(target.name.isEmpty() ? QStringLiteral("-") : target.name,
                          target.device.isEmpty() ? QStringLiteral("?") : target.device,
                          target.project));
    lines.append(QStringLiteral("  Map  : %1").arg(summary.mapPath));
    lines.append(QStringLiteral("  Flash: %1 / %2%3  %4")
                     .arg(MapParser::formatBytes(romUsed), sizeOrNA(flashCap),
                          percentText(romUsed, flashCap), MapParser::percentBar(romUsed, flashCap)));
    lines.append(QStringLiteral("  RAM  : %1 / %2%3  %4")
                     .arg(MapParser::formatBytes(ramUsed), sizeOrNA(ramCap),
                          percentText(ramUsed, ramCap), MapParser::percentBar(ramUsed, ramCap)));

    if (summary.hasDetail) {
        const qint64 uncompressed = summary.code + summary.roData + summary.rwData;
        QString note;
        if (uncompressed > romUsed)
            note = QStringLiteral("  (RW 数据压缩存放, 实占以 Flash 行为准)");
        lines.append(QStringLiteral("  明细 : Code=%1  RO-data=%2  RW-data=%3  ZI-data=%4%5")
                         .arg(summary.code)
                         .arg(summary.roData)
                         .arg(summary.rwData)
                         .arg(summary.ziData)
                         .arg(note));
    }

    if (flashCap > 0 && 100.0 * romUsed / flashCap >= 90.0)
        lines.append(QStringLiteral("  *** WARNING: Flash usage >= 90%, 剩余 %1")
                         .arg(MapParser::formatBytes(flashCap - romUsed)));
    if (ramCap > 0 && 100.0 * ramUsed / ramCap >= 90.0)
        lines.append(QStringLiteral("  *** WARNING: RAM usage >= 90%, 剩余 %1")
                         .arg(MapParser::formatBytes(ramCap - ramUsed)));

    if (top > 0 && !summary.components.isEmpty()) {
        QVector<MapComponent> byRom = summary.components;
        std::sort(byRom.begin(), byRom.end(),
                  [](const MapComponent &a, const MapComponent &b) { return a.rom > b.rom; });
        QVector<MapComponent> byRam = summary.components;
        std::sort(byRam.begin(), byRam.end(),
                  [](const MapComponent &a, const MapComponent &b) { return a.ram > b.ram; });

        lines.append(QStringLiteral("  Top ROM modules:"));
        for (int i = 0; i < byRom.size() && i < top; ++i) {
            lines.append(QStringLiteral("    %1  %2  %3")
                             .arg(byRom.at(i).name.leftJustified(28),
                                  MapParser::formatBytes(byRom.at(i).rom).rightJustified(8),
                                  MapParser::percentBar(byRom.at(i).rom, romUsed)));
        }
        lines.append(QStringLiteral("  Top RAM modules:"));
        for (int i = 0; i < byRam.size() && i < top; ++i) {
            lines.append(QStringLiteral("    %1  %2  %3")
                             .arg(byRam.at(i).name.leftJustified(28),
                                  MapParser::formatBytes(byRam.at(i).ram).rightJustified(8),
                                  MapParser::percentBar(byRam.at(i).ram, ramUsed)));
        }
    }
    return lines.join(QLatin1Char('\n'));
}

QString formatQuietLine(const KeilTarget &target, const MapSummary &summary)
{
    const QString flashCap = target.flashCap > 0 ? QString::number(target.flashCap)
                                                 : QStringLiteral("?");
    const QString ramCap = target.ramCap > 0 ? QString::number(target.ramCap)
                                             : QStringLiteral("?");
    return QStringLiteral("%1/%2: Flash %3/%4 B%5, RAM %6/%7 B%8")
        .arg(target.project, target.name)
        .arg(summary.totalROM)
        .arg(flashCap)
        .arg(percentText(summary.totalROM, target.flashCap))
        .arg(summary.totalRW)
        .arg(ramCap)
        .arg(percentText(summary.totalRW, target.ramCap));
}

struct CliConfig {
    QString path;
    QString projectPath; // uvprojx context for .axf inputs (#L): caps + metadata
    QString mapOverride; // explicit .map path (--map), beats every auto guess
    QString wrap;        // verbatim wrapped command (--wrap)
    QString wrapB64;     // base64-wrapped command (--wrap-b64)
    int top = 0;
    qint64 flash = 0;
    qint64 ram = 0;
    bool quiet = false;
};

int runSizeReportCli(const QStringList &arguments)
{
    CliConfig config;
    bool haveMode = false;
    bool havePath = false;

    for (int i = 0; i < arguments.size(); ++i) {
        const QString argument = arguments.at(i);
        if (argument == QLatin1String("--size-report")) {
            haveMode = true;
        } else if (argument == QLatin1String("--wrap")) {
            haveMode = true;
            if (i + 1 < arguments.size())
                config.wrap = arguments.at(++i);
        } else if (argument == QLatin1String("--wrap-b64")) {
            haveMode = true;
            if (i + 1 < arguments.size())
                config.wrapB64 = arguments.at(++i);
        } else if (argument == QLatin1String("--project") && i + 1 < arguments.size()) {
            config.projectPath = arguments.at(++i);
        } else if (argument == QLatin1String("--map") && i + 1 < arguments.size()) {
            config.mapOverride = arguments.at(++i);
        } else if (argument == QLatin1String("--top") && i + 1 < arguments.size()) {
            config.top = arguments.at(++i).toInt();
        } else if (argument == QLatin1String("--flash") && i + 1 < arguments.size()) {
            config.flash = MapParser::parseSizeBytes(arguments.at(++i));
        } else if (argument == QLatin1String("--ram") && i + 1 < arguments.size()) {
            config.ram = MapParser::parseSizeBytes(arguments.at(++i));
        } else if (argument == QLatin1String("--quiet")) {
            config.quiet = true;
        } else if (!argument.startsWith(QLatin1Char('-')) && !havePath) {
            config.path = argument;
            havePath = true;
        }
    }

    if (!haveMode || !havePath) {
        printLine(QStringLiteral("用法: KeilPluginStudio.exe --size-report <uvprojx|uvmpw|.map|.axf|目录>"
                                 " [--project <uvprojx>] [--map <map文件>] [--top N] [--flash 230K]"
                                 " [--ram 64K] [--quiet]"),
                  true);
        return 2;
    }

    QString wrappedCommand;
    if (!config.wrap.isEmpty())
        wrappedCommand = config.wrap;
    if (!config.wrapB64.isEmpty()) {
        wrappedCommand = QString::fromUtf8(QByteArray::fromBase64(config.wrapB64.toLatin1()));
        if (wrappedCommand.isEmpty()) {
            printLine(QStringLiteral("无效的 --wrap-b64 参数"), true);
            return 1;
        }
    }

    if (!wrappedCommand.isEmpty()) {
        // Run the wrapped original After Build command first (shell semantics,
        // same behaviour as the reference tool's shell=True). The child shares
        // our stdout so its output lands in Keil's Build Output window.
        //
        // Keil cannot expand key sequences hidden inside the base64 blob, so
        // expand $L/@L/#L ourselves using the linker output (#L) we received.
        const QFileInfo pathInfo(config.path);
        if (pathInfo.suffix().compare(QLatin1String("axf"), Qt::CaseInsensitive) == 0)
            wrappedCommand = expandKeilKeySequences(wrappedCommand, config.path);

        QString startError;
        const int wrappedExit = runWrappedCommand(wrappedCommand, &startError);
        if (wrappedExit < 0) {
            printLine(QStringLiteral("无法启动被包装的命令: %1%2")
                          .arg(wrappedCommand,
                               startError.isEmpty() ? QString()
                                                    : QStringLiteral(" (%1)").arg(startError)),
                      true);
            return 1;
        }
        if (wrappedExit != 0) {
            printLine(QStringLiteral("wrapped command failed, exit=%1").arg(wrappedExit), true);
            return wrappedExit;
        }
    }

    struct Entry {
        KeilTarget target;
        bool hasTarget = false;
        bool hasMap = false;
    };

    KeilProjectData data = parseKeilProject(config.path);
    QVector<Entry> entries;

    if ((data.kind == QLatin1String("uvprojx") || data.kind == QLatin1String("uvmpw"))) {
        for (const KeilTarget &target : data.targets) {
            Entry entry;
            entry.target = target;
            entry.hasTarget = true;
            entry.hasMap = target.mapFound;
            entries.append(entry);
        }
    } else {
        const QFileInfo info(config.path);
        if (info.isFile()) {
            const QString suffix = info.suffix().toLower();
            QString mapPath;
            if (suffix == QLatin1String("map")) {
                mapPath = config.path;
            } else if (suffix == QLatin1String("axf")) {
                // "#L" from Keil lands here: report exactly this target. The
                // optional --project context restores the target's name, device
                // and IROM/IRAM capacities (the axf alone cannot provide them)
                // and resolves the map even when it does not sit next to the
                // axf (custom Listing folder, linker --list override, ...).
                Entry entry;
                entry.target.project = info.completeBaseName();
                entry.target.name = QStringLiteral("-");
                entry.target.device = QStringLiteral("?");
                entry.hasTarget = false;

                if (!config.projectPath.isEmpty()) {
                    const AxfContextMatch match =
                            resolveAxfContext(parseKeilProject(config.projectPath),
                                              config.path);
                    if (match.matched) {
                        entry.target = match.target;
                        entry.hasTarget = true;
                    }
                    entry.target.mapPath = match.mapPath;
                } else {
                    const QString candidate =
                            QDir(info.absolutePath()).absoluteFilePath(
                                    info.completeBaseName() + QStringLiteral(".map"));
                    if (QFileInfo::exists(candidate))
                        entry.target.mapPath = QDir::cleanPath(candidate);
                }
                entry.hasMap = !entry.target.mapPath.isEmpty();
                entries.append(entry);
            }
            if (suffix == QLatin1String("map") && !mapPath.isEmpty()) {
                Entry entry;
                entry.target.project = info.completeBaseName();
                entry.target.name = QStringLiteral("-");
                entry.target.device = QStringLiteral("?");
                entry.hasTarget = false;
                entry.hasMap = true;
                entry.target.mapPath = mapPath;
                entries.append(entry);
            }
        } else if (info.isDir()) {
            QString newest;
            QDateTime newestTime;
            QDirIterator it(config.path, { QStringLiteral("*.map") }, QDir::Files,
                            QDirIterator::Subdirectories);
            while (it.hasNext()) {
                const QFileInfo found(it.next());
                if (found.lastModified() > newestTime) {
                    newestTime = found.lastModified();
                    newest = found.absoluteFilePath();
                }
            }
            if (!newest.isEmpty()) {
                Entry entry;
                entry.target.project = QFileInfo(newest).completeBaseName();
                entry.target.name = QStringLiteral("-");
                entry.target.device = QStringLiteral("?");
                entry.hasTarget = false;
                entry.hasMap = true;
                entry.target.mapPath = newest;
                entries.append(entry);
            }
        }
    }

    if (entries.isEmpty()) {
        printLine(QStringLiteral("no uvprojx/uvmpw/map found at: %1").arg(config.path), true);
        return 2;
    }

    // Explicit user override: wins over every automatic resolution.
    if (!config.mapOverride.isEmpty()) {
        if (!QFileInfo::exists(config.mapOverride)) {
            printLine(QStringLiteral("--map 指定的文件不存在: %1").arg(config.mapOverride), true);
            return 2;
        }
        const QString overrideMap =
                QDir::cleanPath(QFileInfo(config.mapOverride).absoluteFilePath());
        for (Entry &entry : entries) {
            entry.target.mapPath = overrideMap;
            entry.hasMap = true;
        }
    }

    bool ok = false;
    for (const Entry &entry : entries) {
        if (!entry.hasMap) {
            printLine(QStringLiteral("Target '%1': 未找到 .map 文件")
                          .arg(entry.hasTarget ? entry.target.name : entry.target.project));
            QStringList searched;
            if (!entry.target.listingDir.isEmpty())
                searched.append(entry.target.listingDir);
            if (!entry.target.outputDir.isEmpty())
                searched.append(entry.target.outputDir);
            if (!searched.isEmpty())
                printLine(QStringLiteral("  已查找: %1, 以及它与工程目录的公共上级(递归, 限深 4 层)")
                              .arg(searched.join(QStringLiteral(", "))));
            if (entry.hasTarget && !entry.target.linkerMapEnabled)
                printLine(QStringLiteral(
                        "  注意: 该 Target 未勾选 Linker Listing (AdsLmap=0), Keil 不会生成 map"));
            printLine(QStringLiteral(
                    "  若从未编译请先编译一次; 否则请检查 map 输出目录设置, 或用 --map <map文件> 显式指定"));
            continue;
        }
        const MapSummary summary = MapParser::parse(entry.target.mapPath);
        if (!summary.ok) {
            printLine(QStringLiteral("WARN: %1").arg(summary.error), true);
            continue;
        }
        ok = true;
        if (config.quiet) {
            printLine(formatQuietLine(entry.target, summary));
        } else {
            printLine(formatReportTarget(entry.target, summary, config.top, config.flash,
                                         config.ram));
            printLine(QStringLiteral("-").repeated(72));
        }
    }
    return ok ? 0 : 2;
}

} // namespace

int main(int argc, char *argv[])
{
    const QStringList rawArguments = nativeArguments(argc, argv);

    // Command line mode: invoked by the Keil After Build hook. Runs headless
    // (no GUI, no QML engine) so it stays fast inside the build.
    if (rawArguments.contains(QLatin1String("--size-report"))
        || rawArguments.contains(QLatin1String("--wrap"))
        || rawArguments.contains(QLatin1String("--wrap-b64"))) {
        QCoreApplication app(argc, argv);
        QCoreApplication::setApplicationName(QStringLiteral("KeilPluginStudio"));
        return runSizeReportCli(rawArguments.mid(1));
    }

    if (qEnvironmentVariableIsEmpty("QSG_RENDER_LOOP"))
        qputenv("QSG_RENDER_LOOP", QByteArrayLiteral("threaded"));

    QSurfaceFormat surfaceFormat = QSurfaceFormat::defaultFormat();
    surfaceFormat.setSamples(4);
    QSurfaceFormat::setDefaultFormat(surfaceFormat);
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    QGuiApplication app(argc, argv);
    QGuiApplication::setOrganizationName(QStringLiteral("TBEA"));
    QGuiApplication::setApplicationName(QStringLiteral("KeilPluginStudio"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("KeilPlugin Studio"));
    QGuiApplication::setWindowIcon(
        QIcon(QStringLiteral(":/qt/qml/KeilPluginStudio/qml/assets/keilplugin-mark.svg")));

    QQuickStyle::setStyle(QStringLiteral("Basic"));
    qmlRegisterSingletonInstance<PluginManager>("KeilPluginStudio.Backend", 1, 0,
                                                "PluginManager",
                                                new PluginManager(&app));
    qmlRegisterSingletonInstance<WindowChrome>("KeilPluginStudio.Backend", 1, 0,
                                               "WindowChrome", new WindowChrome(&app));

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.loadFromModule("KeilPluginStudio", "Main");

    const QByteArray screenshotPath = qgetenv("KEILPLUGIN_SCREENSHOT");
    if (!screenshotPath.isEmpty()) {
        const QByteArray screenshotSize = qgetenv("KEILPLUGIN_SCREENSHOT_SIZE");
        if (!engine.rootObjects().isEmpty()) {
            if (auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst())) {
                const QList<QByteArray> parts = screenshotSize.toLower().split('x');
                if (parts.size() == 2) {
                    bool widthOk = false;
                    bool heightOk = false;
                    const int width = parts.at(0).toInt(&widthOk);
                    const int height = parts.at(1).toInt(&heightOk);
                    if (widthOk && heightOk)
                        window->resize(width, height);
                }
            }
        }
        bool delayOk = false;
        const int configuredDelay = qgetenv("KEILPLUGIN_SCREENSHOT_DELAY").toInt(&delayOk);
        const int screenshotDelay = delayOk ? qMax(250, configuredDelay) : 1800;
        QTimer::singleShot(screenshotDelay, &app, [&app, &engine, screenshotPath] {
            if (!engine.rootObjects().isEmpty()) {
                if (auto *window = qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst()))
                    window->grabWindow().save(QString::fromLocal8Bit(screenshotPath));
            }
            app.quit();
        });
    }

    return app.exec();
}
