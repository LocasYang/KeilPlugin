#include "builtinplugins.h"

#include "hexmerger.h"

#include <QFileInfo>

PluginDescriptor SizeReportPlugin::descriptor() const
{
    PluginDescriptor descriptor;
    descriptor.id = QStringLiteral("keil-size-report");
    descriptor.name = QStringLiteral("Flash/RAM 占用报告");
    descriptor.version = QStringLiteral("1.2.0");
    descriptor.author = QStringLiteral("Fayne Yang");
    descriptor.summary =
            QStringLiteral("编译完成后自动解析 armlink .map 文件，在 Keil Build Output 窗口打印 Flash/RAM 占用率报告");
    descriptor.description =
            QStringLiteral("安装后向工程写入 After Build 用户命令（调用本应用的命令行模式），"
                           "每次 Build/Rebuild 结束时解析生成的 .map 文件："
                           "占用值取自 Total RO/RW/ROM Size 三行，容量上限自动取 uvprojx 中 "
                           "<Cpu> 字段的 IROM()/IRAM() 区域大小，因此任何 MDK 工程（AC5/AC6、任意器件）"
                           "都能直接使用，无需器件表。当两个 After Build 槽位都已占用时，"
                           "会把原命令包装为“先执行、再打印报告”，卸载时自动还原。");
    descriptor.category = QStringLiteral("hook");
    descriptor.capabilities = QStringList { QStringLiteral("install"),
                                            QStringLiteral("uninstall"),
                                            QStringLiteral("report") };
    descriptor.viewLabel = QStringLiteral("占用报告");
    descriptor.notes = QStringList {
        QStringLiteral("安装/卸载前请先关闭 Keil，否则 Keil 退出时会用内存中的旧配置覆盖外部修改"),
        QStringLiteral("需要 Target Options → Listing → Memory Map 处于开启状态（默认已开启）"),
        QStringLiteral("工程 PATH 无需 Python，钩子直接调用本应用的可执行文件"),
        QStringLiteral("若工程没有 IROM/IRAM 信息，可在命令行用 --flash/--ram 手工指定容量"),
    };
    return descriptor;
}

bool SizeReportPlugin::targetInstalled(const KeilTarget &target) const
{
    return target.hookInstalled;
}

UvprojxHook::PatchOutcome SizeReportPlugin::installProject(const QString &uvprojxPath) const
{
    return UvprojxHook::installInFile(uvprojxPath);
}

UvprojxHook::PatchOutcome SizeReportPlugin::uninstallProject(const QString &uvprojxPath) const
{
    return UvprojxHook::uninstallInFile(uvprojxPath);
}

PluginDescriptor HexMergePlugin::descriptor() const
{
    PluginDescriptor descriptor;
    descriptor.id = QStringLiteral("hex-merge");
    descriptor.name = QStringLiteral("HEX 合并工具");
    descriptor.version = QStringLiteral("1.1.0");
    descriptor.author = QStringLiteral("Fayne Yang");
    descriptor.summary =
            QStringLiteral("将 Bootloader 与应用程序两段不重叠的 Intel HEX 合并为一个出厂镜像");
    descriptor.description =
            QStringLiteral("内置的独立工具：读取两份 Intel HEX（支持 Type 00/01/02/04/05 记录），"
                           "逐字节校验地址重叠后按线性地址（Type 04/05）输出合并后的 HEX 文件。"
                           "任一地址出现冲突时立即报错中止，不会写出半成品文件。");
    descriptor.category = QStringLiteral("tool");
    descriptor.capabilities = QStringList { QStringLiteral("run") };
    descriptor.notes = QStringList {
        QStringLiteral("合并前会校验地址重叠，发现重叠即报错中止"),
        QStringLiteral("输出 HEX 使用线性地址记录（Type 04）与起始地址记录（Type 05）"),
    };
    return descriptor;
}

QVariantMap HexMergePlugin::runTool(const QVariantMap &args) const
{
    QVariantMap result;
    const QString boot = args.value(QStringLiteral("boot")).toString();
    const QString app = args.value(QStringLiteral("app")).toString();
    const QString output = args.value(QStringLiteral("output")).toString();

    if (boot.isEmpty() || app.isEmpty() || output.isEmpty()) {
        result.insert(QStringLiteral("ok"), false);
        result.insert(QStringLiteral("message"),
                      QStringLiteral("请先选择 Boot HEX、App HEX 和输出路径"));
        return result;
    }

    const HexMerger::MergeResult merged = HexMerger::merge(boot, app, output);
    result.insert(QStringLiteral("ok"), merged.ok);
    result.insert(QStringLiteral("message"),
                  merged.ok ? QStringLiteral("合并完成: %1 + %2 → %3 (%4 字节)")
                                      .arg(QFileInfo(boot).fileName(),
                                           QFileInfo(app).fileName(),
                                           QFileInfo(output).fileName())
                                      .arg(merged.dataBytes)
                            : merged.error);
    return result;
}
