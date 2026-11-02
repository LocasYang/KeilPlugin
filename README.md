# KeilPlugin Studio

[中文文档](README_zh-CN.md) | **English**

KeilPlugin Studio is a Windows build-assistance and delivery tool for Keil MDK and NovoStudio GNU Arm projects. It produces unified FLASH/RAM reports from armlink MAP or GNU MAP + ELF files and merges Bootloader and application Intel HEX images.

Built with Qt 6 / Qt Quick / C++17, the release package contains the required Qt and compiler runtime libraries. The target PC does not need Qt, Python, or an external script environment.

## 1. Delivery Information

| Item | Description |
| --- | --- |
| Version | v1.4.0 |
| Delivery status | Release |
| Platform | Windows x64 |
| Supported inputs | Keil `.uvprojx` / `.uvmpw`; NovoStudio `.cproject` / `.project` / `.novoproject`; project directories |
| Compiler output | Arm Compiler 5 / 6 armlink MAP; GNU Arm MAP + little-endian ELF32 |
| Operation | GUI, Keil After Build and NovoStudio Post-build CLI |
| Distribution directory | `dist\\KeilPluginStudio` |
| Application | `dist\\KeilPluginStudio\\KeilPluginStudio.exe` |
| Dependencies | No Python dependency; Qt runtime is bundled |

When copying or delivering the application, keep the whole `dist\\KeilPluginStudio` directory. Do not copy only the EXE.

## 2. Included Components

- **Project Tools** — loads Keil projects and detects project files, Targets, devices, output directories, IROM/IRAM capacities, MAP paths, and integration state.
- **Flash/RAM Memory Analysis** — parses armlink MAP files, prints a unified Keil Build Output report, and displays totals, linker composition, and memory hotspots in the GUI.
- **Intel HEX Image Merge** — validates and merges Bootloader and application HEX images into a production programming image.
- **Activity Log** — records project loading, integration, removal, refresh, and tool results.
- **Settings** — selects light/dark appearance, accent color, and English/Chinese UI language.

The tool does not modify firmware source, Scatter/LD files, linker options, or device settings. Integration changes only After Build commands in `.uvprojx` or Post-build commands in `.cproject` files.

### NovoStudio integration (new in v1.4.0)

1. Close NovoStudio so cached settings cannot overwrite the change.
2. Run the distributed EXE and load the project's `.cproject` (or `.project` / `.novoproject`).
3. Configure **Flash/RAM Memory Analysis**. A report command is appended to each build configuration, preserving existing BIN export and disassembly steps.
4. Reopen NovoStudio and Build/Rebuild. The console shows the same file table, deltas, totals, region bars and Top-N report as Keil.
5. Refresh the GUI to review the output. A configuration with an existing MAP is selected initially.

The first write creates `.cproject.keilpluginstudio.bak`. Removal deletes only the generated report command. Update integration after changing language or moving the executable. The hook uses `${BuildArtifactFileName}` and `${ConfigName}`, so each build reports its own configuration. Debug/Release delta histories are stored separately in `.keilpluginstudio-memory-<configuration>-<artifact>.json`.

GNU reports require matching, adjacent `.map` and `.elf` files. Missing ELF or inconsistent section information produces an error. Enable MAP generation in the IDE if needed; this application does not change linker options. For custom paths use:

```bat
KeilPluginStudio.exe --size-report "Release\firmware.elf" --project ".cproject" --target Release --lang en
KeilPluginStudio.exe --size-report "custom.map" --elf "firmware.elf" --flash 256K --ram 128K
KeilPluginStudio.exe --install-novo ".cproject" --lang en
KeilPluginStudio.exe --remove-novo ".cproject"
```

The GUI supports standard GNU Arm managed-build configurations and artifact names. Unresolved custom path macros are rejected. Standalone MAP reports do not store project history.

Flash usage follows load addresses (LMA); RAM follows execution addresses (VMA), including code copied to RAM, BSS and `.heap` / `.stack` / `._user_heap_stack` reservations. Debug and discarded sections are excluded. Code/RO/RW follow ELF section attributes; ZI includes reservations. RAM therefore differs from GNU size's `data+bss`. Capacities come from linker MEMORY ranges, with overlapping addresses counted once in totals. Region rows show occupied addresses within each range and cannot be summed when ranges overlap. `[linker: ...]` rows expose padding, generated tables and reservations. Capacity means the project's allocated address space, not the chip's marketing capacity.

Validated on 2026-11-02 using a separate copy of `PV_Inverter_Code_RT7P65`, NovoStudio's CDT builder and bundled GNU Arm toolchain. A full Release rebuild printed the report automatically: **139,312 / 262,143 B Flash (53.1%)**, **131,292 / 397,312 B RAM (33.0%)**, including 7,168 B heap/stack reservations. Build result: zero errors and one existing firmware warning about `main`'s return type. The original project configuration and sources were unchanged.

References: [GNU ld LMA/VMA](https://www.sourceware.org/binutils/docs/ld/Output-Section-LMA.html), [Eclipse CDT Build Steps](https://help.eclipse.org/latest/topic/org.eclipse.cdt.doc.user/reference/cdt_u_prop_build_settings_steps.htm). The Keil-specific procedures and screenshots below remain applicable to Keil projects.

### 2.1 User Interface

The Project Tools page shows installed components, requirements, and project integration state before a project is loaded.

![KeilPlugin Studio overview](shots/1_software_overview.png)

After a `.uvprojx`, `.uvmpw`, or project directory is loaded, the application shows the workspace Targets, devices, and the integration state of each target.

![KeilPlugin Studio project integration](shots/2_project_integration.png)

The Memory Analysis page shows FLASH/RAM totals, linker composition, and the largest object files and libraries for the selected Target.

![KeilPlugin Studio memory analysis](shots/3_memory_analysis.png)

The project paths, Target names, and usage values in these screenshots are examples only; actual values come from the loaded project and its most recent MAP file.

## 3. Language Selection

### 3.1 README language

This file is the English README. Use the link at the top to switch to [中文文档](README_zh-CN.md). Both documents are kept in the repository so Gitee and GitHub can render them directly.

### 3.2 Application language

The GUI defaults to English. Open **Settings → Language** and select **English** or **中文**. The choice takes effect immediately and is saved locally for the next launch.

When Flash/RAM Memory Analysis is configured, the selected language is stored in its After Build command (`--lang en` or `--lang zh-CN`). Therefore the Keil Build Output report follows the language selected when integration is applied, even on another machine. After changing the language for an already integrated project, choose **Update Project Integration** to update the stored command.

## 4. Requirements Before Use

1. Use Windows x64; Windows 10 or Windows 11 is recommended.
2. The Keil project directory and `.uvprojx` files must be writable.
3. Enable **Target Options → Listing → Memory Map** in Keil so a MAP file is generated.
4. Build at least once before opening the memory report page.
5. Close Keil before configuring or removing project integration.

Keil keeps project settings in memory while a project is open. Editing `.uvprojx` externally while Keil is open can be overwritten when Keil exits.

## 5. Memory Report

Version 1.3.0 prints one unified report after Keil displays the After Build command. It includes:

1. Project, Target, device, and LTO state;
2. a complete FILE(s) RAM/FLASH table;
3. Image totals;
4. Memory execution regions;
5. File/object memory usage (Top-N).

### 5.1 Complete file usage table

The report first lists linked objects in linker Object Name order and shows their RAM and FLASH bytes. When an object can be uniquely matched to a source file in `.uvprojx`, the project-relative source path is shown.

![Unified memory report: complete file usage](shots/4_memory_report_files.png)

The saved build snapshot also enables change markers:

| Marker | Meaning |
| --- | --- |
| `[NEW]` | New object in the current build |
| `[+N]` | Increased by N bytes since the last build |
| `[-N]` | Decreased by N bytes since the last build |

The history file is stored next to the project as `.keilpluginstudio-memory-<OutputName>.json`. Deleting it does not affect compilation, but removes the delta baseline.

### 5.2 Image totals and execution regions

The unified report then prints image totals, execution regions, and the Top-N hotspot table. This avoids the duplicate RAM/FLASH summary that often appears when multiple independent After Build tools are used.

![Unified memory report: totals, execution regions, and Top-N](shots/5_memory_report_summary.png)

`RW-data` is stored in the FLASH image and copied to RAM during startup. Use the FLASH total to judge final image capacity and the RAM total to judge runtime data capacity.

Execution regions are grouped as **ROM / FLASH**, **RAM**, or **UNKNOWN**. Custom regions such as ITCM, DTCM, CCM, external SRAM, and custom Scatter regions are retained; regions whose attributes cannot be classified safely are shown under **UNKNOWN** rather than being omitted.

## 6. Project Integration and Safety

For a new project, the tool prefers the second After Build slot. A typical command is:

```text
"C:\\...\\KeilPluginStudio.exe" --size-report "#L" --project "demo.uvprojx" --lang en
```

Integration behavior:

- A free After Build slot is used directly.
- If both slots are occupied, the original command runs first and the memory report runs afterward.
- Complex original commands are stored with Base64 metadata for safe restoration.
- Removing integration restores the original command.
- Legacy `keil_size_report.py` hooks are cleaned up.
- Existing `keil-build-viewer` hooks can be consolidated into one reversible report hook.

Before an `.uvprojx` file is written, XML validity is checked and UTF-8/ANSI encoding plus BOM state are preserved. For important projects, commit or back up the `.uvprojx` file before first integration.

The configured After Build command contains the absolute EXE path. If the distribution directory is moved or renamed, run the EXE from its new location and configure integration again.

## 7. MAP File Discovery

MAP files may be placed in custom Listing or Output paths. Discovery priority is:

1. explicit `--map` command-line argument;
2. `--list=` path in uvprojx linker Misc controls;
3. `ListingPath/OutputName.map`;
4. `OutputDirectory/OutputName.map`;
5. the newest MAP in those directories;
6. bounded recursive search under the common project/output parent.

Recursive search is limited to four levels. Files matching `OutputName` are preferred, then the most recently modified MAP is chosen.

## 8. Command Line Interface

### 8.1 Memory report

```text
KeilPluginStudio.exe --size-report "#L" --project "<uvprojx>" --lang en|zh-CN [options]
```

| Option | Description |
| --- | --- |
| `--project <path>` | uvprojx context for Target, device, and IROM/IRAM values |
| `--map <path>` | Explicit MAP path; highest priority |
| `--top N` | Number of hotspot entries; default 20, 0 disables only this table |
| `--flash 230K` | Manual FLASH capacity when the project does not declare it |
| `--ram 64K` | Manual RAM capacity when the project does not declare it |
| `--lang en\|zh-CN` | Build Output report language; integration writes this automatically |
| `--quiet` | Parses data without printing the complete report |

### 8.2 Internal wrapper commands

These commands are generated by project integration and normally do not need to be called manually:

```text
KeilPluginStudio.exe --wrap "<original command>" "#L" --project "<uvprojx>"
KeilPluginStudio.exe --wrap-b64 <base64> "#L" --project "<uvprojx>"
```

## 9. Intel HEX Merge

The image tool supports Intel HEX record types 00, 01, 02, 04, and 05. It validates inputs, checks record checksums and address overlap, then writes the requested output file. If it detects invalid data or an address conflict, the operation stops and no partial output file is produced.

## 10. Building from Source

Required environment:

- Qt 6.6.3 mingw_64;
- CMake 3.21 or later;
- Ninja;
- MinGW 11.2 64-bit;
- PowerShell.

Build and test:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\\build.ps1 -RunTests
```

Create the self-contained release directory:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts\\package.ps1
```

## 11. Acknowledgements

Special thanks to [MoodDream / keil-memory-bar](https://gitee.com/MoodDream/keil-memory-bar). Its public implementation informed the Execution Region parsing, ROM/RAM classification, and Build Output status-bar presentation used here.

KeilPlugin Studio extends those ideas with `.uvprojx` source-file association, complete RAM/FLASH object statistics, build deltas, multi-Target capacity context, Qt project configuration/status views, and reversible migration.
