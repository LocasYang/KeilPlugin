# KeilPlugin Studio 设计思路与避坑指南

> 审阅基线：`d751fc2`（2026-08-31）  
> 审阅范围：`src/`、`qml/`、`tests/`、`scripts/`、`CMakeLists.txt`、`README.md` 以及当前构建/发布目录。  
> 文档定位：描述**当前代码真实行为**，并给出后续演进时必须优先规避的风险；不是对 README 的简单复述。

## 1. 一句话定位

KeilPlugin Studio 是一个面向 Windows 与 Keil MDK 工程的 Qt 6 桌面工作台。它把“配置插件”和“执行插件”放进同一个 EXE：用户平时通过 QML 图形界面管理工程；Keil 编译完成后则通过 After Build 命令再次启动同一 EXE 的轻量命令行模式，将报告写回 Keil Build Output。

当前包含两个内置功能：

1. **Flash/RAM 占用报告**：修改 `.uvprojx` 的 `<AfterMake>`，编译后解析 armlink `.map`。
2. **HEX 合并工具**：在 GUI 中合并 Bootloader 与 App 的 Intel HEX。

当前更准确的产品定义是“**带插件式内部抽象的内置工具集合**”，还不是可独立安装、卸载 DLL 或脚本包的动态插件平台。

## 2. 设计目标与边界

### 2.1 当前设计目标

- 无 Python 依赖，部署到目标电脑后仅依赖随程序打包的 Qt 运行时。
- 尽量不破坏工程已有 After Build 命令；两个槽位都占用时包装原命令。
- 通过 `#L` 让 Keil 把当前 Target 的链接输出传给报告程序，避免多 Target 混报。
- 从 `.uvprojx` 自动取得 Target、器件、输出目录、IROM/IRAM 容量与 map 配置。
- 用同一套 C++ 核心同时服务 GUI 报告页与 Keil Build Output 文本报告。
- 通过插件描述符驱动通用列表、能力标签、说明和安装状态。

### 2.2 明确的产品边界

- 主要运行平台是 Windows；标题栏融合、进程句柄继承和发布脚本均明显依赖 Windows。
- 工程模型面向 `.uvprojx`、`.uvmpw` 与包含这些文件的目录。
- 尺寸解析器面向 armlink 英文 `.map` 固定文本格式，不是任意链接器 map 的通用解析器。
- 当前插件在编译期注册，新增插件需要修改源码并重新构建应用。
- GUI 没有直接驱动 Keil Build/Rebuild，只管理钩子、读取既有产物和运行独立工具。

## 3. 总体架构

```text
                         KeilPluginStudio.exe
                                  |
                    +-------------+-------------+
                    |                           |
          普通启动 / GUI 参数             --size-report / --wrap
                    |                           |
          QGuiApplication + QML             QCoreApplication
                    |                           |
              PluginManager                 CLI 参数解析
                    |                           |
        +-----------+-----------+        可选：执行原 After Build 命令
        |           |           |                 |
 KeilProject  PluginRegistry  日志/结果       工程/AXF 上下文匹配
        |           |                             |
  uvprojx/map   KeilPlugin                     MapParser
        |           |                             |
        |     +-----+------+                 stdout/stderr
        |     |            |                     |
        | SizeReport   HexMerge              Keil Build Output
        |     |            |
        +--UvprojxHook   HexMerger
```

### 3.1 分层职责

| 层次 | 主要文件 | 当前职责 |
|---|---|---|
| 进程入口 | `src/main.cpp` | 判断 GUI/CLI 模式；创建 Qt 应用；CLI 报告排版；包装命令执行 |
| QML 门面 | `src/pluginmanager.*` | 向 QML 暴露工程、插件、报告、日志和工具调用 |
| 插件抽象 | `src/keilplugin.*` | 定义描述符、安装、卸载、状态检查、工具执行接口 |
| 插件注册 | `src/pluginregistry.*` | 构造时注册两个内置插件并按 ID 查询 |
| 内置功能 | `src/builtinplugins.*` | 把占用报告、HEX 合并适配到统一插件接口 |
| 工程解析 | `src/keilproject.*` | 展开 uvmpw/目录，解析 Target、容量、钩子和 map 路径 |
| 钩子修改 | `src/uvprojxhook.*` | 安装、升级、包装、解包和卸载 `<AfterMake>` 命令 |
| 报告解析 | `src/mapparser.*` | 解析总 ROM/RW、段明细和目标文件占用 |
| HEX 处理 | `src/hexmerger.*` | 校验记录、构建稀疏地址映射、检查冲突、重新输出 HEX |
| 界面 | `qml/` | 插件中心、报告、日志、设置、确认弹窗和主题令牌 |
| 平台适配 | `src/windowchrome.*` | 用 DWM 属性同步 Windows 标题栏颜色 |

这个分层的优点是核心解析和文件修改逻辑不依赖 QML，单元测试可以直接覆盖；缺点是 `main.cpp` 仍承担了较多 CLI 编排和报告格式化职责，插件自定义界面也还没有抽象出来。

## 4. 实际运行逻辑

### 4.1 启动分流

入口位于 `src/main.cpp:489`。

程序先读取原生命令行。只要参数中包含 `--size-report`、`--wrap` 或 `--wrap-b64`，就进入 CLI：

- 只创建 `QCoreApplication`，不加载 QML；
- 可先执行被包装的旧 After Build 命令；
- 再解析当前 Target 的 map；
- 将结果写入标准输出/错误输出。

否则进入 GUI：

- 强制 Qt Quick 使用 OpenGL 和 threaded render loop；
- 创建 `QGuiApplication`；
- 把 `PluginManager` 与 `WindowChrome` 注册成 QML 单例；
- 加载 `Main.qml`；
- 支持 `--project=`、`--page=`、`--dark` 和截图环境变量。

这样做能复用一个 EXE，又避免 Keil 每次编译后启动完整 GUI。

### 4.2 GUI 加载工程

调用链如下：

```text
FileDialog / --project=
        -> PluginManager::loadProject()
        -> KeilProject::load()
        -> parseKeilProject()
        -> 解析 uvprojx/uvmpw/目录
        -> 解析每个 Target + 搜索 map + 检测钩子
        -> rebuildPluginStates()
        -> QML 自动刷新列表、状态栏和 Target 表
```

`PluginManager::loadProject()` 位于 `src/pluginmanager.cpp:72`。输入可以是本地 URL 或字符串。底层实际支持单个 `.uvprojx`、`.uvmpw` 和目录；但当前 GUI 的 FileDialog 只能方便地选择文件，不能浏览并选择目录。

#### `.uvmpw` 与目录展开规则

- `.uvmpw`：用正则提取 `<PathAndName>`，解析存在的 `.uvprojx`。
- 目录：只查当前目录第一层的 `*.uvprojx`；没有时取按名称排序后的最后一个 `*.uvmpw`。
- 不会递归寻找任意深度的 `.uvprojx`。

#### Target 解析内容

每个 `<Target>` 读取：

- `TargetName`、`Device`；
- `<Cpu>` 中全部 `IROM(start,size)`、`IRAM(start,size)` 的 size 之和；
- `OutputDirectory`、`OutputName`、`ListingPath`；
- Linker `Misc` 中的 `--list=`；
- `AdsLmap` 是否启用；
- 两个 After Build 用户命令中是否出现本工具标记。

### 4.3 map 发现策略

工程解析阶段按下列优先级找 map：

1. Linker Misc controls 中 `--list=` 指定的文件；
2. `<ListingPath>/<OutputName>.map`；
3. `<OutputDirectory>/<OutputName>.map`；
4. Listing/Output 目录内修改时间最新的任意 `.map`；
5. 工程目录与输出目录的公共上级中递归搜索，最大深度 4；文件名以 OutputName 开头者优先，其次取最新文件。

CLI 额外支持 `--map`，它覆盖所有自动发现结果。

当 Keil 传入 `#L` 展开的 `.axf` 后，`resolveAxfContext()`（`src/keilproject.cpp:315`）按以下规则匹配 Target：

1. `OutputName` 与 AXF 主文件名相同；
2. Target 已解析 map 与 AXF 同目录同名；
3. AXF 目录与 Target 的 Output/Listing 目录相同；
4. 单 Target 工程兜底。

最终若 AXF 旁边存在同名 map，它优先于工程配置推导出的 map。

### 4.4 安装占用报告插件

GUI 确认后调用 `PluginManager::installPlugin()`（`src/pluginmanager.cpp:111`）。管理器会遍历当前工作区内的**每一个 `.uvprojx`**，而钩子修改器又会遍历文件内的**每一个 `<AfterMake>`**，所以安装粒度目前是整个工作区，不是单工程或单 Target。

单个 AfterMake 的安装决策：

1. 已包含工具标记：尝试升级旧 Python/旧格式钩子；否则跳过。
2. slot2 命令为空：写入 slot2，并启用 `RunUserProg2`。
3. slot1 命令为空：写入 slot1，并启用 `RunUserProg1`。
4. 两个命令都非空：包装 slot2 原命令，先执行原命令，再执行报告。

新钩子大致为：

```text
"<安装时的绝对 EXE 路径>" --size-report "#L" --project "<绝对 uvprojx 路径>"
```

如果原命令包含双引号，原文本先按 UTF-8 转 Base64，再写为 `--wrap-b64`，避免多层引号在 Keil、当前 EXE 与 `cmd.exe` 之间损坏。

### 4.5 Keil 编译后的执行链

```text
Keil Build/Rebuild 完成
  -> 展开 #L 为当前 Target 的 AXF
  -> 启动安装时记录的 KeilPluginStudio.exe
  -> 如为 wrap：通过 cmd.exe /c 运行旧命令并等待退出
  -> 用 --project 恢复 Target/器件/IROM/IRAM 上下文
  -> 为 AXF 定位 map
  -> MapParser 解析尺寸
  -> 输出文本报告到 Keil Build Output
```

Windows 包装执行使用 `CreateProcessW`，并复制 Keil 传来的标准句柄，使子进程输出继续进入 Build Output。原命令非零退出时，当前实现直接返回该退出码，**不会继续打印占用报告**。

### 4.6 报告口径

`MapParser::parse()` 位于 `src/mapparser.cpp:55`。

- Flash 已用：`Total ROM Size`。
- RAM 已用：`Total RW Size`，即 RW Data + ZI Data。
- 细分：`Grand Totals` 中的 Code、RO Data、RW Data、ZI Data。
- 单目标文件 ROM：Code + RO + RW。
- 单目标文件 RAM：RW + ZI。

CLI 可显示 Top N 和 90% 警告；GUI 在 90% 变黄、97% 变红，并展示进度条与 Top ROM/RAM。

### 4.7 卸载逻辑

卸载同样遍历全部 `.uvprojx` 和全部 AfterMake：

- 普通本工具命令直接清空并关闭对应 Run 标志；
- `--wrap`/`--wrap-b64` 解包后恢复原命令；
- 旧 Python 钩子会被清理；
- 老式 `&&` 命令链会删除含工具标记的片段，保留其他片段。

修改完成后重新加载工程并重建每个插件的 Target 状态。

### 4.8 HEX 合并逻辑

`HexMerger::merge()` 位于 `src/hexmerger.cpp:168`。

1. 逐行检查冒号、长度和校验和；
2. 支持处理 00、01、02、04、05 记录；
3. 把数据记录展开成 `std::map<32 位绝对地址, 字节>`；
4. 合并两个映像；同地址字节不同则失败；
5. 按地址升序、每行最多 16 字节，用 Type 04 线性地址重新输出；
6. 保留遇到的第一个 Type 05 起始地址并写 EOF。

工具通过 `PluginManager::runTool()` 同步执行，结果进入 Toast 与内存日志。

## 5. 当前插件模型：能做什么，不能做什么

### 5.1 已实现的通用化

`PluginDescriptor` 可声明 ID、名称、版本、作者、说明、类型、能力、注意事项和报告菜单名。插件中心可以自动渲染：

- 插件列表与详情；
- 能力标签；
- 通用安装/卸载按钮；
- 每 Target 安装状态；
- 注意事项和基础元数据。

### 5.2 尚未实现的通用化

以下内容仍然硬编码：

- `PluginRegistry` 构造函数直接 `new SizeReportPlugin`、`new HexMergePlugin`；
- `Main.qml:59` 手工维护插件 ID 到 StackLayout 页索引的映射；
- `PluginsPage.qml:692` 写死 `PluginManager.runTool("hex-merge", ...)`；
- 所有 `run` 插件都会看到同一个 HEX 输入面板；
- 插件没有参数 Schema、QML Component URL、图标、权限、兼容版本或独立生命周期；
- 没有 DLL 扫描、Qt 插件 IID、签名、沙箱、热加载和版本隔离。

因此“新增插件不改任何 UI 代码”只对**复用现有通用安装界面的无自定义视图 hook 插件**近似成立。新增自定义报告页或不同参数的工具插件，仍需修改 QML、CMake QML 文件列表并重新编译。

## 6. 必须优先避开的坑

| 优先级 | 含义 | 当前最典型问题 |
|---|---|---|
| P0 | 可能破坏工程、改变既有构建行为或导致不可恢复状态 | 非原子写回、无备份/回滚、Run 状态丢失、旧 EXE 路径失效 |
| P1 | 可能给出错误结果、误操作钩子或让构建卡死 | marker 误判、模糊 map、无限等待、Top RAM 漏项、HEX 规则不严格 |
| P2 | 主要影响扩展、性能、体验和长期维护 | UI 硬编码、同步卡顿、状态不持久、构建产物入库 |

### 6.1 P0：工程文件与用户命令安全

#### 1. `.uvprojx` 不是原子写回，也没有自动备份

当前代码在 XML 内存校验后，用 `WriteOnly | Truncate` 直接覆盖原文件，并且没有检查 `write()` 是否完整成功。断电、磁盘写满、杀进程或杀毒软件干预，都可能留下空文件或半文件。

**建议：**使用 `QSaveFile` 写临时文件并 `commit()` 原子替换；修改前生成带时间戳的 `.bak`；提交后再次读取、解码和 XML 校验。

#### 2. 多工程安装/卸载没有事务

工作区有多个 `.uvprojx` 时逐个修改。若前两个成功、第三个失败，前两个不会回滚，UI 只提示“部分工程写入失败”。

**建议：**先对全部文件做预检和内存 patch，全部可写、全部 XML 合法后再进入提交阶段；任一提交失败时用备份回滚已提交文件。

#### 3. Keil 打开工程时存在双写覆盖

工具无法锁定 Keil 内存中的工程配置。即使本程序成功写盘，Keil 退出时仍可能用旧配置覆盖文件。

**建议：**安装前检测 `UV4.exe`/`UV5.exe` 进程并强提示；至少记录修改前后文件哈希，刷新时发现回退要明确告警，不能只显示“未安装”。

#### 4. 原命令的启用状态没有完整保存

当前只看 `UserProgNName` 是否为空，不把原 `RunUserProgN` 状态当作包装状态的一部分。两个槽位都有文本但 slot2 原本禁用时，安装会包装并强制启用它；卸载包装后也固定把 Run 标志设为 true。这可能意外执行用户刻意禁用的命令。

**建议：**包装数据中同时保存 `slot`、`command`、`runEnabled`、插件 ID、格式版本和校验值，卸载时原样恢复。

#### 5. 当前格式的旧 EXE 路径无法靠“重新安装”刷新

钩子写的是安装时 EXE 的绝对路径。程序移动、切换 build/dist 或升级目录后，旧钩子会失效。更隐蔽的是：如果旧钩子已经是当前 `#L + --project` 格式，再点“安装”只会判断为已安装，不会把 EXE 路径换成当前程序。

**当前可行操作：**先卸载，再从最终固定位置的 EXE 安装。README 中“在 dist 的 exe 上重新安装一次”不足以保证刷新。

**长期建议：**安装稳定的 launcher 到固定目录，或让升级逻辑比较规范化 EXE 路径与插件版本，发现不一致就重写。

### 6.2 P1：钩子识别与执行正确性

#### 6. 标记匹配过宽

`containsToolMarker()` 只要看到 `keilpluginstudio.exe`、`--size-report`、`--wrap` 或 `keil_size_report.py` 任一子串就认为是本插件。其他工具若恰好使用 `--wrap`，可能被误判、升级或卸载。

**建议：**使用唯一、带版本的标记，例如 `--kps-plugin-id keil-size-report --hook-format 2`，识别时校验完整命令结构与可执行文件名。

#### 7. XML 修改依赖正则和固定标签形态

`<AfterMake>.*?</AfterMake>`、`<UserProgNName>...</...>`、单数字 Run 标志均要求现有结构符合预期。若 Keil 版本、第三方生成器或手工工程使用自闭合标签、缺标签、大小写差异或不同命名空间，patch 可能零命中。

零命中时安装器没有报错；上层可能给出“已处于安装状态”的成功提示，实际上没有写入任何钩子。

**建议：**用 XML DOM/流式重写定位 Target 和 AfterMake；至少统计期望 Target 数、命中块数、修改块数，`命中数 == 0` 必须失败。

#### 8. 包装命令无超时，且失败后不再报告

`WaitForSingleObject(..., INFINITE)` 会无限等待。旧命令挂住会把 Keil Build 一起挂住；旧命令返回非零时，本程序提前退出，不再输出尺寸报告。

**建议：**引入可配置超时与进程树终止策略；明确策略是“旧命令失败仍报告但继承失败码”，还是“立即终止”，并在输出中区分两阶段结果。

#### 9. `cmd.exe /c` 保留了完整 Shell 语义

这是兼容批处理、重定向和命令链所需，但也意味着命令文本中的 `& | < > % !` 都有 Shell 语义。工程来源不可信时，应把 After Build 命令当作可执行代码，而不是普通配置文本。

**建议：**在安装确认框展示将被包装的原命令；日志避免吞掉命令来源；不要从未知来源的工程直接安装并执行钩子。

#### 10. Keil 关键序列只覆盖一部分

Base64 包装后自行展开 `#L/$L/@L/%L`，其他 Keil key sequence 不会展开。替换也只是字符串替换，不理解引号、转义或上下文。

**建议：**建立受支持序列表并做真实 Keil 集成测试；遇到未支持的 `$x/@x/#x/%x` 时拒绝包装或明确告警。

### 6.3 P1：工程与 map 推导正确性

#### 11. “最新 map”兜底可能拿到错误或陈旧 Target

当精确文件不存在时，会选目录内最新的任意 map，递归搜索时也允许选择任意名称的最新 map。多工程、多 Target 共用输出根目录时，GUI 可能展示其他 Target 的尺寸，却没有显式告诉用户这是模糊匹配。

**建议：**给 map 结果增加 `resolutionKind`（explicit/exact/adjacent/fuzzy）与时间戳；模糊结果只作为候选提示，不应静默当作成功。还应比较 AXF 与 map 修改时间。

#### 12. 递归搜索的“文件数上限”并未真正限制目录遍历

代码中的 20000 计数只在发现 `.map` 时累加，不统计访问的目录与其他文件。若公共上级退化到盘符根目录，即使深度只有 4，也可能在 GUI 线程扫描大量目录。

**建议：**限制总目录数、总耗时和允许的搜索根；公共上级是盘符根时直接停止模糊搜索。

#### 13. `--list=` 与路径宏解析能力有限

当前正则只取一个带引号或无空格的值，不展开环境变量、Keil 宏，也不处理更复杂的转义形式。目录查找也不递归展开工作区深层工程。

**建议：**把路径解析独立成可测试模块，记录原始值、展开值和失败原因；支持策略必须显式，不要默默退化到“最新 map”。

#### 14. IROM/IRAM 总和不一定等于业务允许容量

代码把 `<Cpu>` 中所有 IROM/IRAM 区域 size 相加。对于多 Bank、保留区、外部存储、不可同时使用区域或 Boot/App 分区，这可能高估 Target 真正可用容量。

**建议：**报告中标注“物理区域合计”；增加 per-Target 容量覆盖配置，并允许选择具体 region，而不只提供 CLI 全局 `--flash/--ram`。

#### 15. map 解析格式过于固定

正则依赖英文 `Total RO/RW/ROM Size`、`Grand Totals` 和列顺序。链接器版本、国际化、格式空格变化或非 armlink 输出都可能失败；map 始终按 UTF-8 读入，也没有显式记录工具链版本。

**建议：**用多套真实 AC5/AC6 fixture 建兼容矩阵；解析结果携带 parser/version；无法确认口径时失败，不做猜测。

#### 16. GUI 的 Top RAM 候选集可能不完整

后端先按 ROM 排序并截断到 12 个组件（`src/pluginmanager.cpp:14,247`），QML 再从这 12 个中排序 Top RAM。一个 ROM 很小但 ZI 很大的对象可能完全不在 GUI Top RAM 中；CLI 不受此问题影响。

**建议：**后端分别返回 `topRom` 和 `topRam`，或返回完整组件列表并只在 UI 最终截断。

### 6.4 P1：HEX 合并正确性

#### 17. 文案称“任何地址重叠即失败”，代码只拒绝不同字节

相同地址、相同数据会被接受；同一个输入 HEX 内的重复地址也会直接覆盖。当前测试只验证不同字节冲突。

**建议：**明确产品策略。若产线要求镜像分区绝不相交，应对任何跨镜像地址重复失败；若允许相同字节重叠，UI 和文档必须准确表述并报告重叠字节数。

#### 18. 起始地址与未知记录处理不严格

两个输入均有 Type 05 时只保留第一个，不检查冲突；未知记录类型被静默忽略；没有强制验证 EOF；Type 03 没有处理。

**建议：**未知类型默认失败；验证 EOF 与记录顺序；两个 start address 不同必须报错；明确支持 Type 03/05 的转换策略。

#### 19. 输出同样不是原子文件

HEX 用 Truncate 直接写出，`file.write(out)` 返回值未检查。“不会写出半成品”只对地址冲突成立，不覆盖磁盘 I/O 失败。

**建议：**同样改为 `QSaveFile`，提交后重新加载并逐字节比对稀疏地址映射。

#### 20. 整个镜像逐字节存入 `std::map`

普通 MCU 镜像可以接受，但大镜像会产生显著内存和 CPU 开销。

**建议：**若未来覆盖大容量外部 Flash，改用排序区段模型或流式归并，而不是每字节一个红黑树节点。

### 6.5 P2：插件扩展与 API 边界

#### 21. 插件 ID、能力和参数都缺少约束

注册表不检查重复 ID；管理器没有再次校验调用能力；`runTool(QVariantMap)` 没有类型 Schema；插件对象也没有显式释放，但当前静态进程生命周期下影响较小。

**建议：**注册时校验唯一 ID、API 版本和能力组合；为工具参数定义 Schema；返回统一的错误码、用户消息和诊断详情。

#### 22. 自定义视图仍依赖 QML 硬编码

报告页映射、HEX 面板和插件色彩存在 ID 分支。继续堆插件会让 `PluginsPage.qml` 变成大型条件分支页面。

**建议：**描述符增加 `detailComponent`、`viewComponent`、`icon`、`accentRole`、`argumentSchema`。QML 用 Loader 按描述符加载组件；若追求真正第三方插件，再在此之上选择 Qt Plugin 或进程外插件协议。

#### 23. 同步调用会阻塞 GUI

工程递归搜索、map 解析、批量 XML 修改和 HEX 合并都在 GUI 线程同步执行。大工程或网络盘上会表现为窗口卡死，Toast 也只能在操作结束后出现。

**建议：**把解析和工具执行放入工作线程，公开 busy/progress/cancel；文件提交阶段串行化，避免并发写同一个工程。

#### 24. 状态和日志不持久化

主题色、深浅模式、最近工程、页面、日志都只在内存中。日志最多 600 条，退出即丢失。

**建议：**用 `QSettings` 保存用户偏好与最近工程；操作日志写滚动文件，并记录工程哈希、插件版本、修改文件和备份路径。

### 6.6 P1/P2：构建与发布维护

#### 25. 构建目录和发布运行时被提交进 Git

当前仓库没有 `.gitignore`；基线实测 `build/` 有 674 个已跟踪文件、约 102.9 MB，`dist/` 有 538 个已跟踪文件、约 95.3 MB。CMake 缓存、目标文件、DLL 和生成 QML 都进入版本库。

后果包括：仓库膨胀、二进制冲突、跨机器绝对路径失效、源码改动被生成文件噪声淹没。

**建议：**Git 只保留源码、脚本和必要资源；忽略 `build*/`、`dist/`、CMake/Ninja 生成物。发布包改由 CI artifact、压缩包或 Release 托管。

#### 26. build 脚本只在目录不存在时配置 CMake

当前 `build/CMakeCache.txt` 仍记录旧路径 `J:/mine/code/QT/keil_-plugin` 和旧工具盘符。`scripts/build.ps1` 只要发现 build 目录存在就跳过配置，因此工程移动、Qt 路径变化或 CMakeLists 修改后可能继续使用失效缓存。

**建议：**每次都执行幂等 `cmake -S -B`；使用 `CMakePresets.json`；不要分发或提交 CMakeCache。

#### 27. 发布包明显过度部署

当前 dist 包含 qmltooling、虚拟键盘、Pdf、Network、多套 Controls Style 等大量当前功能未直接使用的模块，增加体积和攻击面。

**建议：**以干净发布目录做依赖审计和启动烟测；关闭不需要的 QML debug tooling；核对 Qt 许可与第三方运行库清单。

#### 28. 版本号存在多处手工重复

CMake 项目版本、QML 关于页/侧栏、插件版本分别维护，容易出现界面、文件属性和插件描述不一致。

**建议：**由 CMake 生成统一版本头和 QML singleton；插件版本独立，但应用版本只保留一个来源。

## 7. 推荐的目标设计

建议保留当前“双入口、共享核心”的总体方向，但把文件安全、执行编排和插件视图三处重构为稳定边界。

```text
GUI / CLI
   |
ApplicationController
   |
   +-- ProjectService -------- 只读解析、文件监视、Target/产物模型
   +-- HookTransactionService - 预检、备份、原子提交、回滚、审计
   +-- ReportService ---------- 工具链识别、map 定位、解析、口径校验
   +-- ToolExecutionService --- 异步、进度、取消、统一 Result
   +-- PluginCatalog ---------- ID/API 校验、能力、参数 Schema、视图组件
   |
Core library（不依赖 QML）
```

### 7.1 建议的数据模型

- `ProjectWorkspace`：来源、工程列表、解析告警、文件哈希。
- `TargetModel`：Target ID、输出、容量 regions、map candidates、hook states。
- `HookState`：插件 ID、格式版本、slot、原命令、原 Run 状态、EXE 路径、安装时间。
- `MapResolution`：最终路径、匹配级别、候选列表、AXF/map 时间关系。
- `OperationResult`：`ok`、稳定错误码、用户消息、诊断详情、变更文件、备份路径。
- `PluginDescriptorV2`：API 版本、参数 Schema、视图组件、平台/应用兼容范围。

### 7.2 钩子持久化建议

不要只靠可读命令文本猜测状态。至少让命令包含稳定标记和格式版本，包装原状态可以放入 Base64 编码的 JSON：

```json
{
  "format": 2,
  "pluginId": "keil-size-report",
  "slot": 2,
  "originalCommand": "...",
  "originalRunEnabled": false
}
```

同时应考虑命令长度上限。若 JSON 过长，可把状态放到工程旁的 `.kps/` 文件中，钩子只保存短 ID；但需要处理文件被删除、工程复制和相对路径问题。

### 7.3 动态插件的选择

后续先决定产品是否真的需要第三方插件：

- **只由本团队开发：**继续静态内置最稳妥，但应改称“功能模块”，并完成视图/参数 Schema 解耦。
- **受信任第三方 C++ 插件：**可用 Qt Plugin + IID，但必须冻结 ABI/API、处理 Qt/编译器版本一致性。
- **不受信任或跨语言插件：**优先进程外 JSON-RPC/本地管道，主程序控制权限、超时与崩溃隔离；不要直接把任意 DLL 加载进主进程。

## 8. 建议实施顺序

### 第一阶段：先保数据安全

1. `QSaveFile + 自动备份 + 回读校验`。
2. 多 `.uvprojx` 两阶段提交与失败回滚。
3. 保存/恢复原 Run 状态。
4. 修复零命中假成功、旧 EXE 路径不刷新和过宽 marker。
5. 安装前检测 Keil 进程，日志记录哈希与备份。

### 第二阶段：修正报告与 HEX 口径

1. map 解析结果标记精确/模糊来源，禁止静默使用可疑 map。
2. Top ROM/Top RAM 分别计算。
3. 建立 AC5/AC6 真实 map fixture 矩阵。
4. 明确 HEX 重叠、start address、未知记录和 EOF 策略。
5. HEX 原子写回并回读验证。

### 第三阶段：消除卡顿与扩展硬编码

1. 工程扫描、报告、HEX 合并异步化。
2. 插件参数 Schema 和动态 QML Component Loader。
3. 统一 Result、错误码、进度和取消模型。
4. QSettings 与持久日志。

### 第四阶段：整理工程化发布

1. 清理 Git 中 build/dist，补 `.gitignore`。
2. 引入 CMake Presets 与干净构建 CI。
3. 发布包依赖裁剪、签名、版本信息与干净机器烟测。
4. 若需要，再引入真正的动态插件机制。

## 9. 测试现状与建议补充

现有 `KeilPluginCoreTest` 覆盖了：

- map 总量、Grand Totals、组件聚合、容量字符串和进度条；
- 空槽安装、重复安装、卸载、双槽包装、Base64、旧 Python 钩子；
- Keil `#L/$L/@L/%L` 展开；
- 工程与容量解析、map 分层发现、AXF 到 Target 匹配；
- HEX 基本合并与不同字节冲突；
- 两个内置插件注册。

本次审阅在全新的临时目录完成了 CMake 配置、`KeilPluginCoreTest` 构建与运行（返回 0），并成功构建了包含 QML 编译步骤的 `KeilPluginStudio` 完整目标。仓库内已跟踪的旧 `build/CMakeCache.txt` 仍指向旧工作区，因此不能把仓库自带 build 目录视为可移植构建环境；现有测试通过也不等价于真实 Keil、异常写盘或 GUI 交互验证。

建议新增：

- ANSI/GBK、UTF-8 BOM、写入失败、磁盘满和原子回滚测试；
- 缺失/自闭合/变体 AfterMake、禁用但非空槽、marker 误命中测试；
- 工作区部分提交失败的事务测试；
- 当前格式旧 EXE 路径升级测试；
- 包装超时、非零退出仍报告、标准句柄继承的真实 Keil 集成测试；
- 多 Target 同目录、模糊 map、旧 map、盘符根搜索停止测试；
- AC5/AC6 多版本真实 map fixture；
- Top RAM 不在 Top ROM 候选集的回归测试；
- HEX 相同字节重叠、输入内重复、缺 EOF、未知类型、Type 03/05 冲突、写回失败测试；
- PluginManager/QML 交互测试、发布目录启动与 CLI 输出烟测。

## 10. 日常使用检查表

### 安装插件前

- 关闭 Keil，并确认没有后台 UV4/UV5 进程持有工程。
- 将 KeilPluginStudio 放到最终固定目录，不要安装后再移动 EXE。
- 备份或提交 `.uvprojx`。
- 检查两个 After Build 槽位及其 Run 勾选状态。
- 确认 Target 开启 Linker Listing，并先成功 Build 一次。

### 安装后

- 用文本 diff 检查 `.uvprojx` 只改了预期 AfterMake。
- 分别 Build 每个 Target，确认 `#L` 对应正确 AXF/map。
- 验证原 After Build 命令仍执行且退出码行为符合预期。
- 检查报告显示的 map 绝对路径和修改时间，而不只看百分比。

### 升级或移动软件

- 先从旧位置卸载，再从新位置安装；不要只点一次“安装”期待路径自动刷新。
- 若旧 EXE 已删除，手工核对 `.uvprojx` 中是否仍引用旧路径。
- 保留一个不依赖 GUI 的 CLI 烟测工程，验证部署的 Qt DLL 与 Build Output 重定向。

### 合并 HEX 前

- 明确 Boot/App 是否允许相同字节地址重叠；当前代码会接受相同字节重叠。
- 确认两个镜像的起始地址策略。
- 输出不要直接覆盖唯一原件。
- 合并后用独立工具或烧录链再次校验地址范围、数据字节和入口地址。

## 11. 结论

当前工程最值得保留的是“单 EXE 双模式、核心逻辑脱离 QML、`#L + --project` 恢复当前 Target 上下文、尽量保护原 After Build 命令”这条主线。它已经形成可用的 MVP，也有覆盖核心正常路径的单元测试。

下一步不宜先扩充更多插件，而应先把 `.uvprojx` 修改提升为可备份、可验证、可回滚的事务，把钩子状态从模糊字符串识别升级为带版本的结构化状态，再修正 map 模糊匹配、Top RAM 和 HEX 严格性。完成这些基础后，再做异步化和真正的插件视图解耦，产品才适合长期维护和规模化扩展。
