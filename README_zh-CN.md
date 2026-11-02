# KeilPlugin Studio

**中文** | [English](README.md)

KeilPlugin Studio 是面向 Keil MDK / NovoStudio GNU Arm 工程的 Windows 构建辅助与交付工具，用于统一生成
FLASH/RAM 存储占用报告，并完成 Bootloader 与应用程序 Intel HEX 镜像合并。

软件采用 Qt 6 / Qt Quick / C++17 实现，发布包已包含 Qt 运行库和编译器运行库。
目标计算机无需安装 Qt、Python 或其他脚本环境。

### 文档与界面语言

当前文档为中文版本；可通过顶部的 [English](README.md) 链接切换英文 README。
软件界面首次启动默认英文，在“系统设置 → 语言”中选择 **English** 或 **中文** 后立即生效，
并自动保存到当前 Windows 用户设置。配置 Flash/RAM 存储占用分析时，当前语言会以 `--lang en` 或 `--lang zh-CN` 写入 Keil After Build / NovoStudio Post-build 命令，构建报告随之使用对应语言；已配置工程切换语言后，点击“更新工程集成”即可写入新语言。NovoStudio 中文报告使用 UTF-8 输出。

## 1. 交付信息

| 项目 | 说明 |
| --- | --- |
| 软件版本 | v1.4.0 |
| 交付状态 | Release |
| 目标平台 | Windows x64 |
| 支持工程 | Keil 的 .uvprojx / .uvmpw；NovoStudio 的 .cproject / .project / .novoproject；工程目录 |
| 编译器范围 | Arm Compiler 5 / 6 armlink MAP；GNU Arm 的 MAP + ELF32 小端输出 |
| 运行方式 | 图形界面 + Keil After Build / NovoStudio Post-build 命令行模式 |
| 发布目录 | dist\KeilPluginStudio |
| 主程序 | dist\KeilPluginStudio\KeilPluginStudio.exe |
| 外部依赖 | 无 Python 依赖；发布目录已包含 Qt 运行库 |

交付或复制软件时，应保留 **dist\KeilPluginStudio** 整个目录，不要只复制 EXE。

## 2. 交付范围

本版本包含以下功能：

- **工程功能**：载入 Keil / NovoStudio 工程，识别构建配置、目标器件、输出目录、
  存储容量、MAP 路径和工程集成状态。
- **Flash/RAM 存储占用分析**：解析 armlink MAP 或 GNU Arm MAP + ELF，在 IDE
  构建窗口输出统一报告，并在 Qt 页面显示总体占用、组成明细和独立的 Flash/RAM 排行榜。
- **Intel HEX 镜像合并**：校验并合并 Bootloader 与应用程序 HEX，生成出厂烧录镜像。
- **操作记录**：记录工程载入、配置、移除、状态刷新和工具执行结果。
- **系统设置**：配置浅色/深色主题、界面主色，并查看标准工程使用流程。

本工具不会修改固件源代码、Scatter/LD 文件、链接参数或器件配置。执行
“配置到工程”或“从工程移除”时，只修改 .uvprojx 的 After Build 或 .cproject 的 Post-build 命令。
NovoStudio 的接入步骤如下，统计口径见第 6 节。

### NovoStudio 快速接入（v1.4.0 新增）

1. **关闭 NovoStudio**，避免 IDE 缓存覆盖工程文件。
2. 运行发布目录中的 `KeilPluginStudio.exe`，点击“载入工程”，选择工程根目录的 `.cproject`（也可选 `.project` / `.novoproject`）。
3. 在“工程功能”中选择“Flash/RAM 存储占用分析”，点击“配置到工程”。这会为工程中的 Debug、Release 等配置追加报告命令；已有的 BIN 导出、反汇编等命令会保留。
4. 重新打开 NovoStudio，执行 Build/Rebuild。在构建输出末尾查看与 Keil 相同布局的文件占用表、增量标记、镜像总量、存储分区和 Top-N。
5. 返回本软件，点击“重新检测”或报告页刷新；首次载入时会优先显示已有 MAP 的配置。

首次写入会创建 `.cproject.keilpluginstudio.bak`。移除集成只移除本软件追加的命令，保留原有步骤。
切换报告语言或移动软件目录后，关闭 IDE，再“更新工程集成”。

**构建产物要求：** GNU Arm 项目应生成同目录、同名 `.map` 和 `.elf`。ELF 用于识别段属性，缺失或段信息不匹配时会报错，不猜测用量。若工程没有 MAP，可在 NovoStudio 链接器设置中启用 Map 输出；本软件不会替你改变链接参数。自定义 MAP/ELF 路径可用下面的命令行显式指定。

```bat
KeilPluginStudio.exe --size-report "Release\firmware.elf" --project ".cproject" --target Release --lang en
KeilPluginStudio.exe --size-report "custom.map" --elf "firmware.elf" --flash 256K --ram 128K
KeilPluginStudio.exe --install-novo ".cproject" --lang zh-CN
KeilPluginStudio.exe --remove-novo ".cproject"
```

安装到 Post-build 的命令使用 `${BuildArtifactFileName}` 和 `${ConfigName}`，只报告当前构建配置。
Debug/Release 的增量历史分开保存为 `.keilpluginstudio-memory-<配置>-<产物名>.json`，可加入工程的忽略列表。
独立 MAP 模式不保存工程增量记录。GUI 默认识别标准 GNU Arm managed-build 配置和产物命名；未展开的自定义路径宏会报错。

**报告显示：** 构建输出保留文件占用、增量标记、镜像总量、组成明细、分区和热点榜单。界面统一使用“RAM 运行占用”标题。v1.4.0 已精简额外的统计口径提示文字，计算结果保持一致。

**本机验证（2026-11-02）：** 使用 `PV_Inverter_Code_RT7P65` 的独立副本，经 `E:\NovoStudio` 自带 CDT 构建器和 GNU Arm 工具链完成 Release 全量重编译，Post-build 自动输出报告。构建 0 错误、1 条固件原有 `main` 返回类型警告。Flash 为 **139,312 / 262,143 B（53.1%）**，RAM 为 **131,292 / 397,312 B（33.0%）**。原始工程的 `.cproject` 与源码未修改。

语义依据：[GNU ld 的 LMA/VMA](https://www.sourceware.org/binutils/docs/ld/Output-Section-LMA.html)、[Eclipse CDT 构建后步骤](https://help.eclipse.org/latest/topic/org.eclipse.cdt.doc.user/reference/cdt_u_prop_build_settings_steps.htm)。以下 Keil 专用操作和示意截图仍适用于 Keil 工程。

### 2.1 软件界面示意

未载入工程时，“工程功能”页面用于查看交付组件、功能说明、实施要求和工程配置状态。

![KeilPlugin Studio 工程功能总览](shots/1_software_overview.png)

载入 Keil / NovoStudio 工程后，软件会识别工程中的构建目标、目标器件和
Flash/RAM 存储占用分析集成状态。配置或移除集成会处理所载入工程中的全部构建配置。

![KeilPlugin Studio 工程集成状态](shots/2_project_integration.png)

“存储占用分析”页面展示当前 Target 的 FLASH/RAM 总量、链接器镜像组成以及
高占用目标文件和库；页面数据来自最近一次有效的 armlink MAP 或 GNU Arm MAP + ELF。

![KeilPlugin Studio 存储占用分析](shots/3_memory_analysis.png)

以上界面中的工程路径、Target 名称和占用数据仅用于功能示意，实际内容以现场载入的
工程及其最新构建结果为准。

## 3. 发布包结构

~~~text
dist/KeilPluginStudio/
├─ KeilPluginStudio.exe    主程序
├─ README.md               工程交付与使用说明
├─ Qt6*.dll                Qt 运行库
├─ lib*.dll                MinGW 运行库
├─ platforms/              Windows 平台插件
├─ qml/                    Qt Quick 运行组件
├─ imageformats/           图像格式插件
├─ translations/           Qt 翻译资源
└─ shots/                  功能效果截图
~~~

## 4. 使用前条件

使用前请确认：

1. 目标计算机为 Windows x64，建议使用 Windows 10 或 Windows 11。
2. 工程目录和对应的 .uvprojx / .cproject 文件具有写权限。
3. Keil 已启用 Target Options → Listing → Memory Map；NovoStudio 已生成对应配置的 MAP 和 ELF。
4. 工程至少完成过一次 Build/Rebuild，便于界面直接读取最近一次 MAP。
5. 配置或移除工程集成前，关闭对应的 Keil / NovoStudio。

IDE 会缓存打开的工程配置，退出时可能用缓存覆盖外部写入的内容。

## 5. 标准使用流程

### 5.1 配置 Keil 存储占用分析

1. 运行 **dist\KeilPluginStudio\KeilPluginStudio.exe**。
2. 点击“载入工程”，选择 .uvprojx 或 .uvmpw 文件。
3. 在“工程功能”页面核对工程文件、构建目标和目标器件。
4. 选择“Flash/RAM 存储占用分析”。
5. 确认 Keil 已关闭，点击“配置到工程”。
6. 重新打开 Keil，执行 Build 或 Rebuild。
7. 在 Keil Build Output 中检查统一存储报告。
8. 在本软件“存储占用分析”页面查看图形化结果。

如果检测到旧版 keil-build-viewer 与本软件同时存在，状态会显示“待更新”。
点击“更新工程集成”后，旧工具的重复输出会被停用，并由本软件一次生成统一报告。

### 5.2 从工程移除

1. 关闭工程对应的 Keil / NovoStudio。
2. 重新载入目标工程。
3. 选择“Flash/RAM 存储占用分析”。
4. 点击“从工程移除”并确认。
5. 检查操作记录，确认原有构建后命令已保留或恢复。

若工程曾迁移自 keil-build-viewer，本软件会使用保存的恢复信息还原原命令。

### 5.3 生成合并镜像

1. 在“工程功能”中选择“Intel HEX 镜像合并”。
2. 选择 Bootloader 镜像。
3. 选择应用程序镜像。
4. 指定输出镜像路径。
5. 点击“生成合并镜像”。
6. 检查操作记录和输出文件。

工具支持 Intel HEX Type 00、01、02、04、05 记录。检测到地址重叠、输入无效
或记录校验失败时会立即中止，不生成半成品输出文件。

## 6. 存储报告说明

当前 v1.4.0 在 Keil / NovoStudio 构建后输出一份统一报告，包含以下五部分：

1. 工程、Target、器件和 LTO 状态；
2. 完整 FILE(s) RAM/FLASH 占用表；
3. Image totals 镜像总量；
4. Memory execution regions；
5. File / object memory usage (top N)。

### 6.1 报告布局示例

以下截图保留自 v1.3.0 的真实 Keil Build Output，用于展示延续至 v1.4.0 的报告布局。报告首先输出工程、Target、器件和
LTO 状态，随后按链接器 Object Name 顺序输出完整 FILE(s) RAM/FLASH 占用表。

![统一存储报告：完整文件占用表](shots/4_memory_report_files.png)

完整文件表之后统一输出 Image totals、Memory execution regions 和 Top-N 文件/对象热点，
避免旧方案重复显示两套 RAM/FLASH 占用结果。

![统一存储报告：镜像总量、执行区域与 Top-N](shots/5_memory_report_summary.png)

说明：After Build 命令末尾可能包含 **--restore-b64**。这是从 keil-build-viewer
迁移时保存的可逆恢复信息，不会形成第二份报告，也不影响正常编译。

### 6.2 完整 FILE(s) 表与增量标记

第一张 FILE(s) 表按链接器 Object Name 顺序完整列出当前参与链接的工程目标文件，
并同时显示 RAM 和 FLASH 字节数。源文件能够与 .uvprojx 的 FilePath 唯一对应时，
优先显示工程相对路径；无法唯一关联时显示目标文件名。

NovoStudio 按 GNU MAP 中的链接顺序汇总目标文件和库成员；`[linker: ...]` 行表示链接器生成的表格、填充或预留区域。

完整文件表会与上一次构建快照比较：

| 标记 | 含义 |
| --- | --- |
| [NEW] | 本次构建新增的文件或目标对象 |
| [+N] | 相比上一次构建增加 N 字节 |
| [-N] | 相比上一次构建减少 N 字节 |

没有变化时不追加标记，因此上面的实际输出中各行只显示当前 RAM/FLASH 数值。

历史快照保存在工程旁，按工程类型区分：

~~~text
Keil:       .keilpluginstudio-memory-<OutputName>.json
NovoStudio: .keilpluginstudio-memory-<配置>-<产物名>.json
~~~

删除该文件不会影响编译，但会丢失下一次报告的增量比较基线。首次迁移时，如果工程目录
存在 keil-build-viewer-record.txt，本软件会尝试使用其数据建立初始比较基线。

### 6.3 Image totals

Image totals 只汇总一次完整镜像：FLASH 行表示最终镜像占用，RAM 行表示运行期存储占用。
RW-data 会存放在 FLASH 镜像中，并在启动后复制到 RAM，因此 FLASH 最终占用应以
FLASH 汇总行和链接器总量为准。

GNU Arm 使用 MAP + ELF 区分加载地址和运行地址。运行于 RAM 的代码、数据、BSS 及堆栈预留均计入相应占用；调试信息和已丢弃段不计入。Code/RO/RW 按 ELF 属性分类，ZI 计入零初始化区域及堆栈预留。容量以工程链接器分配的地址范围为准。

### 6.4 Memory execution regions

执行区域表逐行显示起止地址、状态条、使用率、已用容量、总容量和区域名称。
区域按当前解析结果分为 **ROM / FLASH** 与 **RAM**；无法可靠分类时才增加
**OTHER** 分组。

当 MAP 中存在 **ER_ITCM**、CCM 或其他自定义 Execution Region 时，软件会保留该区域；
能够识别其加载/执行属性时归入 **ROM / FLASH** 或 **RAM**，无法可靠分类时归入
**OTHER**，不会因区域名称不在常见列表中而遗漏。

NovoStudio 的分区来自 GNU MAP 的 MEMORY 表。总容量和总占用按地址去重；各分区行显示自身范围内的占用，重叠分区不能直接相加。

### 6.5 Top-N 文件/对象热点

最后一张表按文件或对象占用排序，RAM 与 FLASH 数据在同一行显示。默认输出前 20 项，
其余数量使用“... N more object(s)”提示；可通过 **--top N** 调整，使用 **--top 0**
时只关闭该热点表，不影响完整 FILE(s) 表、Image totals 和 Execution Region。

图形界面中的 Flash、RAM 榜单分别排序，各显示前 6 项。

## 7. 工程变更与安全边界

| 操作 | 修改对象 | 可恢复性 |
| --- | --- | --- |
| 载入工程 | 只读解析工程、Target、MAP 和所需 ELF | 不产生工程变更 |
| 刷新分析 | 只读重新解析工程与构建产物 | 不产生工程变更 |
| 配置到工程 | 修改 .uvprojx 的 AfterMake 或 .cproject 的 postbuildStep | 可通过“从工程移除”恢复 |
| 更新工程集成 | 更新报告语言和程序路径；Keil 可合并旧报告命令 | 保留原有命令及恢复信息 |
| 从工程移除 | 移除本软件命令并恢复被包装命令 | 恢复至配置前命令 |
| 合并镜像 | 写入用户指定的输出 HEX | 不修改工程配置 |

写回 .uvprojx 前会进行 XML 合法性检查，并保留原文件的 UTF-8/ANSI 编码和 BOM 状态。
由于工程文件属于关键配置，仍建议在首次配置前提交版本控制或单独备份 .uvprojx。

After Build 命令记录的是配置时的 EXE 绝对路径。移动或重命名发布目录后，应使用新位置
的 KeilPluginStudio.exe 重新配置工程集成。

NovoStudio 的 Post-build 同样记录 EXE 绝对路径；首次写入创建 `.cproject.keilpluginstudio.bak`，写回前校验 XML，并通过原子写入保存。

## 8. Keil After Build 集成原理

新工程优先使用 AfterMake slot2，典型命令如下：

~~~text
"C:\...\KeilPluginStudio.exe" --size-report "#L" --project "demo.uvprojx" --lang zh-CN
~~~

处理规则：

- 空闲槽位存在时，直接写入本软件命令。
- 两个槽位都已使用时，包装原命令，先执行原命令，再输出存储报告。
- 原命令包含复杂引号时，使用 Base64 元数据保存和恢复，避免命令行转义破坏。
- 移除集成时解开包装并恢复原命令。
- 支持清理旧版 keil_size_report.py 钩子。
- 检测到 keil-build-viewer 时合并重复报告，并保留可逆恢复信息。

## 9. MAP 文件定位规则

Keil MAP 文件允许位于自定义 Listing 或 Output 目录。软件按以下优先级查找：

1. 命令行 **--map** 明确指定的 MAP；
2. .uvprojx Linker Misc controls 中的 **--list=** 路径；
3. **ListingPath/OutputName.map**；
4. **OutputDirectory/OutputName.map**；
5. 上述目录中修改时间最新的 MAP；
6. 工程目录与输出目录公共上级内的有界递归搜索。

递归搜索最大深度为 4 层，优先匹配 OutputName，其次选择最新修改文件。

NovoStudio 按当前配置的构建目录和产物名称定位同名 MAP / ELF，不跨配置选择最新文件。自定义路径通过 `--map` / `--elf` 显式指定。

## 10. 命令行接口

### 10.1 存储报告

~~~text
KeilPluginStudio.exe --size-report "#L" --project "<uvprojx>" --lang en|zh-CN [options]
KeilPluginStudio.exe --size-report "Release\firmware.elf" --project ".cproject" --target Release --lang en|zh-CN [options]
~~~

| 参数 | 说明 |
| --- | --- |
| --project <path> | 为 AXF / ELF 输入指定 .uvprojx / .cproject，提供构建目标和器件上下文 |
| --map <path> | 明确指定 MAP 文件，优先级最高 |
| --elf <path> | 为 GNU MAP 显式指定配套 ELF 文件 |
| --target <name> | 选择构建配置，例如 Debug / Release |
| --top N | 设置热点榜单行数，默认 20；0 表示关闭热点榜单 |
| --flash 230K | 工程未声明容量时手工指定 FLASH 容量 |
| --ram 64K | 工程未声明容量时手工指定 RAM 容量 |
| --lang en\|zh-CN | Build Output 报告语言；工程集成会自动写入 |
| --quiet | 每个目标仅输出一行 Flash/RAM 摘要 |

**--top** 只控制末尾热点榜单，不影响完整文件增量表和 Execution Region 表。

### 10.2 内部包装入口

以下入口由工程集成自动生成，一般无需手工调用：

~~~text
KeilPluginStudio.exe --wrap "<原命令>" "#L" --project "<uvprojx>"
KeilPluginStudio.exe --wrap-b64 <base64> "#L" --project "<uvprojx>"
~~~

### 10.3 GUI 启动参数

| 参数 | 说明 |
| --- | --- |
| --project=<path> | 启动后自动载入工程 |
| --page=0..3 | 启动后打开指定页面 |
| --dark | 使用深色主题启动 |

### 10.4 NovoStudio 工程集成

关闭 NovoStudio 后，可使用命令行配置或移除集成：

~~~text
KeilPluginStudio.exe --install-novo ".cproject" --lang en|zh-CN
KeilPluginStudio.exe --remove-novo ".cproject"
~~~

## 11. 常见问题

| 现象 | 检查与处理 |
| --- | --- |
| 找不到 MAP 文件 | 先在 Keil 编译一次，确认 Listing → Memory Map 已启用，并检查 Listing/Output 路径 |
| NovoStudio 提示缺少 ELF 或段信息不一致 | 重新构建当前配置，并确认 MAP / ELF 来自同一次链接 |
| Build Output 没有报告 | 检查工程配置状态，并确认 After Build / Post-build 中记录的 EXE 路径仍然有效 |
| 一次构建出现两份占用报告 | 关闭 Keil，在本软件中执行“更新工程集成” |
| 配置后重新打开工程发现修改丢失 | 关闭 Keil / NovoStudio 后重新配置，避免 IDE 缓存覆盖 |
| FLASH/RAM 总容量为空 | 检查 .uvprojx 的 IROM/IRAM 配置，或使用 --flash/--ram |
| 增量标记消失 | 检查工程旁的 .keilpluginstudio-memory-*.json 是否被删除 |
| HEX 合并失败 | 检查记录校验、地址范围、输入文件完整性以及输出目录写权限 |
| 移动软件后报告无法执行 | 从新目录运行 EXE，并重新配置工程集成 |

## 12. 交付验收

v1.4.0 已完成 Release 构建和核心回归，并增加以下 NovoStudio 验证：

| 验收项 | 结果 |
| --- | --- |
| RT7P65 工程副本 Release 全量构建及 Post-build 自动报告 | 0 错误，1 条原有固件警告 |
| GNU MAP + ELF 解析、RAM 运行代码、重叠区域容量去重 | 通过 |
| 缺失/截断 ELF、MAP/ELF 段不匹配检查 | 通过 |
| 工程重复配置、语言更新及移除后原命令恢复 | 通过 |
| 中文 UTF-8 输出及中英文报告提示精简 | 通过 |
| Release 选择显示、Flash/RAM 独立排行 | 已检查 |
| 发布 EXE 与构建产物一致，中英文 README 同步 | 已检查 |

此前 v1.3.0 的验收记录保留如下：

| 验收项 | 结果 |
| --- | --- |
| Windows x64 Release 编译 | 通过 |
| QML 四个常驻页面启动检查 | 通过 |
| KeilPluginCoreTest | 1/1 通过 |
| 真实 GD32 AC6 MAP 回归 | 通过 |
| 完整文件增量、Image totals、Execution Region、Top-N | 通过 |
| ER_ITCM 及自定义 Execution Region 保留 | 通过 |
| keil-build-viewer 合并及移除恢复 | 通过 |
| Intel HEX 地址重叠保护 | 通过 |
| 自包含发布目录打包 | 通过 |

建议现场验收至少完成：

1. 载入实际工程并核对全部 Target。
2. 配置工程集成并执行两次连续 Build。
3. 在两次构建之间修改一项实际占用，确认对应增量；无变化时应无增量标记。
4. 对照链接器产物核对 FLASH/RAM 总量；GNU Arm 的 RAM 口径包含 RAM 运行代码及预留区域。
5. 检查各 Execution Region，特别是 ITCM、CCM 或自定义区域。
6. 从工程移除并确认原构建后命令保留或恢复。

## 13. 源码构建与发布

### 13.1 构建环境

- Qt 6.6.3 mingw_64；
- CMake 3.21 或更高；
- Ninja；
- MinGW 11.2 64-bit；
- PowerShell。

### 13.2 Release 构建与测试

~~~powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\build.ps1 -RunTests
~~~

构建脚本会：

1. 定位本机 Qt、CMake、Ninja 和 MinGW；
2. 生成 Release；
3. 部署构建目录运行库；
4. 按需执行核心测试；
5. 同步 EXE、README 和截图到发布目录。

若 **build/CMakeCache.txt** 来自工程搬迁前的位置，脚本不会删除旧缓存，而是自动使用
**build-local** 完成当前目录构建。

### 13.3 自包含发布

~~~powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\package.ps1
~~~

发布脚本会先构建最新源码，再向 **dist\KeilPluginStudio** 部署 Qt 和编译器运行库。

## 14. 源码目录

~~~text
src/
  main.cpp           GUI/CLI 入口、统一报告输出、历史快照
  keilproject.*      uvprojx/uvmpw、Target、容量和 MAP 定位
  novoproject.*      NovoStudio 配置解析、Post-build 追加与移除
  uvprojxhook.*      After Build 配置、迁移、包装与恢复
  mapparser.*        armlink MAP、对象表和 Execution Region 解析
  gnumapparser.*     GNU MAP + ARM ELF32、对象占用和 MEMORY 分区
  hexmerger.*        Intel HEX 解析、重叠校验与输出
  keilplugin.*       功能组件抽象接口
  builtinplugins.*   存储分析与 HEX 合并功能定义
  pluginregistry.*   功能组件注册
  pluginmanager.*    工程、报告、工具和操作记录 QML 接口
  windowchrome.*     Windows DWM 标题栏适配
qml/
  Main.qml           主窗口、导航、状态栏、主题和 Toast
  Theme.qml          浅色/深色主题令牌
  components/        通用按钮、卡片、下拉框、导航和状态组件
  dialogs/           工程变更确认弹窗
  pages/             工程功能、存储占用分析、操作记录、系统设置
scripts/             构建、打包和 Qt 工具链定位脚本
tests/               KeilPluginCoreTest
shots/               README 功能截图
dist/                自包含发布目录
~~~

## 15. 二次开发接口

界面由功能组件元数据驱动。新增功能时：

1. 派生 **KeilPlugin**。
2. 在 **PluginDescriptor** 中声明名称、版本、能力、说明和实施要求。
3. 工程集成功能实现 targetInstalled、installProject、uninstallProject。
4. 本地工具实现 runTool。
5. 在 PluginRegistry 中注册。

~~~cpp
PluginRegistry::instance().registerPlugin(new MyPlugin);
~~~

新增功能应继续遵守现有工程变更确认、可恢复性、操作记录和深浅主题规范。

## 16. 版本更新摘要

v1.4.0 主要更新：

- 新增 NovoStudio GNU Arm 工程接入，支持 Debug / Release 独立报告和历史快照。
- 使用 MAP + ELF 统计文件、镜像和存储分区，并保留已有构建后步骤。
- 新增 `--target`、`--elf`、`--install-novo`、`--remove-novo` 命令行选项。
- NovoStudio 中文报告采用 UTF-8 输出。
- 精简报告及界面的额外说明文字，RAM 卡片统一为“RAM 运行占用”。
- 修正自动选择配置时的下拉框同步，并让 Flash / RAM 榜单独立排名。

v1.3.0 主要更新：

- 将 Qt 页面统一为工程交付语言：工程功能、工程集成、存储占用分析、操作记录和系统设置。
- 将结果通知居中显示，避免遮挡页面底部操作按钮。
- 统一 Build Output 报告，避免文件占用和内存区域重复输出。
- 增加完整文件增量、Image totals、Execution Region 和 Top-N 组合报告。
- 保留 ER_ITCM 等执行区域，仅在无法可靠分类时输出 UNKNOWN 分组。
- 支持从 keil-build-viewer 迁移并可逆恢复。
- 加固工程搬迁后的构建和发布流程。

## 17. 特别鸣谢

特别感谢 [MoodDream / keil-memory-bar](https://gitee.com/MoodDream/keil-memory-bar)。

本项目的 Execution Region 解析、ROM/RAM 分类及 Build Output 状态条改进，
参考了该项目公开实现提供的思路。在此基础上，KeilPlugin Studio 进一步实现了：

- .uvprojx 源文件路径关联；
- 完整 RAM/FLASH 文件统计与构建增量；
- 多 Target 容量上下文；
- 统一 Image totals、Execution Region 和 Top-N 输出；
- Qt 工程配置、状态查看和可逆迁移流程。
