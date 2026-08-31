#include "builtinplugins.h"

#include "hexmerger.h"

#include <QFileInfo>

PluginDescriptor SizeReportPlugin::descriptor() const
{
    PluginDescriptor descriptor;
    descriptor.id = QStringLiteral("keil-size-report");
    descriptor.name = QStringLiteral("Flash/RAM 存储占用分析");
    descriptor.version = QStringLiteral("1.3.0");
    descriptor.author = QStringLiteral("Fayne Yang");
    descriptor.summary =
            QStringLiteral("编译完成后解析 armlink .map 文件，并在 Keil Build Output 中输出统一的存储占用报告");
    descriptor.description =
            QStringLiteral("配置到工程后写入 After Build 用户命令（调用本软件的命令行模式），"
                           "每次 Build/Rebuild 结束时解析生成的 .map 文件，统一输出带增量标记的完整源文件表、"
                           "镜像总量、Execution Region 状态条和 Top-N 热点榜单："
                           "占用值取自 Total RO/RW/ROM Size 三行，容量上限自动取 uvprojx 中 "
                           "<Cpu> 字段的 IROM()/IRAM() 区域大小，因此任何 MDK 工程（AC5/AC6、任意器件）"
                           "都能直接使用，无需器件表。当两个 After Build 槽位都已占用时，"
                           "会把原命令包装为“先执行、再打印报告”，卸载时自动还原。若发现旧版 "
                           "keil-build-viewer，会将两个重复报告钩子合并为一个，并保留可逆恢复信息。");
    descriptor.category = QStringLiteral("hook");
    descriptor.capabilities = QStringList { QStringLiteral("install"),
                                            QStringLiteral("uninstall"),
                                            QStringLiteral("report") };
    descriptor.viewLabel = QStringLiteral("存储占用分析");
    descriptor.notes = QStringList {
        QStringLiteral("配置或移除工程集成前请先关闭 Keil，避免其退出时以旧配置覆盖本次变更"),
        QStringLiteral("需确认 Target Options → Listing → Memory Map 已启用（Keil 默认启用）"),
        QStringLiteral("工程无需配置 Python 环境，构建后处理直接调用本软件"),
        QStringLiteral("若工程未声明 IROM/IRAM 容量，可通过 --flash/--ram 参数补充"),
        QStringLiteral("检测到 keil-build-viewer 时请执行“更新工程集成”，避免同一次构建重复输出"),
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
    descriptor.name = QStringLiteral("Intel HEX 镜像合并");
    descriptor.version = QStringLiteral("1.1.0");
    descriptor.author = QStringLiteral("Fayne Yang");
    descriptor.summary =
            QStringLiteral("校验并合并 Bootloader 与应用程序 Intel HEX，生成可交付的烧录镜像");
    descriptor.description =
            QStringLiteral("本地读取两份 Intel HEX（支持 Type 00/01/02/04/05 记录），"
                           "逐字节校验地址重叠后按线性地址（Type 04/05）输出合并后的 HEX 文件。"
                           "任一地址出现冲突时立即报错中止，不会写出半成品文件。");
    descriptor.category = QStringLiteral("tool");
    descriptor.capabilities = QStringList { QStringLiteral("run") };
    descriptor.notes = QStringList {
        QStringLiteral("生成前会校验地址范围，发现重叠后立即中止且不写出结果文件"),
        QStringLiteral("输出镜像使用线性地址记录（Type 04）与起始地址记录（Type 05）"),
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
                      QStringLiteral("请完整选择 Bootloader 镜像、应用镜像和输出路径"));
        return result;
    }

    const HexMerger::MergeResult merged = HexMerger::merge(boot, app, output);
    result.insert(QStringLiteral("ok"), merged.ok);
    result.insert(QStringLiteral("message"),
                  merged.ok ? QStringLiteral("合并镜像已生成：%1 + %2 → %3（%4 字节）")
                                      .arg(QFileInfo(boot).fileName(),
                                           QFileInfo(app).fileName(),
                                           QFileInfo(output).fileName())
                                      .arg(merged.dataBytes)
                            : merged.error);
    return result;
}
