# NesV3 项目架构与开发状态

更新时间：2026-09-10

## 1. 项目目标

NesV3 是一个学习型 NES 模拟器项目。目标不是一次性完成完整商业级模拟器，而是通过逐层实现 NES 硬件，理解以下问题：

- 6502 CPU 的状态机、寻址模式和指令执行
- CPU Bus 与 PPU Bus 的地址空间和设备映射
- Cartridge、Mapper、PRG/CHR 存储和镜像
- PPU 寄存器、VRAM、OAM、扫描线和渲染时序
- CPU、PPU、DMA、NMI 之间的时钟协作
- Qt QGraphicsScene/QGraphicsItem 的 tile 级显示组织
- 设备生命周期、映射冲突、测试和可恢复的系统组装

当前项目优先保证架构清晰和可测试性，不使用旧 MMU 抽象堆叠兼容层，也不为了短期通过测试而假装 CPU 已经完整。

## 2. 工具链与构建

当前开发环境：

- Windows
- Qt 6.11.2
- MinGW 13.1
- CMake
- C++23
- Qt Widgets 用于 GUI
- Qt Core 用于 NES 核心库
- CTest + 普通 C++ `main()` 作为核心测试方式

主要目录：

```text
src/
  MainWindow.*             Qt 主窗口
  NesScene.*               QGraphicsScene 显示层
  NesNametableItem.*       当前整张 nametable 显示 Item
  NesTileItem.*            独立 8x8 tile Item 原型
  Nes/                     NES 核心库
    Bus.*                  通用地址总线和设备映射
    Ram.*                  连续字节存储
    Cpu.*                  6502 CPU，当前仍在重构
    Ppu.*                  PPU Bus 和 PPU 基础状态
    Nes.*                  NES 系统组装
    NesClock.*             CPU/PPU 统一时钟骨架
    Cartridge.*            Cartridge 和 Mapper 连接
    Mapper/                Mapper 抽象及 Mapper000
    Excption/              项目异常类

tests/
  CMakeLists.txt
  NesCoreTests.cpp         Bus/RAM/Cartridge/时钟核心测试

docs/
  PROJECT_ARCHITECTURE.md  本文
  PPU programmer reference - NESdev Wiki.html
                           本地保存的 NESdev PPU 参考文档
  2C02G_U_wiki.pal         PPU 调色板资源
```

配置、构建和测试：

```powershell
D:\Qt\Tools\CMake_64\bin\cmake.exe -S . -B build-qt6 -DBUILD_TESTS=ON
D:\Qt\Tools\CMake_64\bin\cmake.exe --build build-qt6 --parallel 4
$env:PATH = "D:\Qt\6.11.2\mingw_64\bin;D:\Qt\Tools\mingw1310_64\bin;$env:PATH"
ctest --test-dir build-qt6 --output-on-failure
```

## 3. 当前系统组装

`Nes` 当前拥有：

- `Cpu`
- `Ppu`
- `Cartridge`
- `NesClock`

`Cartridge` 连接到 CPU Bus 和 PPU Bus：

```text
CPU Bus $8000-$FFFF -> Cartridge CPU Mapper -> PRG ROM
PPU Bus $0000-$1FFF -> Cartridge PPU Mapper -> CHR ROM
```

CPU Bus 和 PPU Bus 是两个独立的 `Bus` 实例：

- CPU Bus：地址空间 `$0000-$FFFF`
- PPU Bus：地址空间 `$0000-$3FFF`

设备采用非拥有关系：Bus 保存 `BusDevice*`，设备生命周期由上层系统组件管理。

## 4. Bus 设计决策

### 4.1 BusDevice

设备只负责自己的读写语义：

```cpp
virtual bool read(quint16 address, quint8 &value) = 0;
virtual bool write(quint16 address, quint8 value) = 0;
```

Bus 负责：

- 地址范围匹配
- 权限检查
- 地址转换
- open-bus 值
- 访问结果
- mapping 生命周期

### 4.2 Mapping

当前 mapping 使用闭区间：

```text
start <= address <= end
```

普通 `registerMapping()` 的规则是：

- `device` 不能为空
- `start <= end`
- `end` 不得超过 Bus 地址空间
- 任何与已有 mapping 的地址重叠都拒绝
- `priority` 不能让普通设备静默覆盖其他设备

相邻窗口允许注册。例如：

```text
$0000-$07FF 允许
$0800-$0FFF 允许
$07F0-$08FF 拒绝
```

未来如果调试器 overlay 或硬件覆盖确实需要多重命中，应提供命名明确的显式 overlay API，而不是放宽普通注册规则。

### 4.3 访问结果

未映射地址：

```text
返回 Unmapped，并返回当前 open-bus 值
```

权限错误：

```text
返回 PermissionDenied
```

设备主动拒绝：

```text
返回 DeviceRejected
```

## 5. RAM、Internal RAM 与旧抽象

`Ram` 现在只负责连续存储和边界检查，不负责 Bus 地址镜像。

NES 主板 2 KiB Internal RAM 不属于 CPU 芯片。后续设计是：

- `Nes` 拥有 `InternalRam`
- CPU 通过 CPU Bus 访问
- 注册四个不重叠窗口：

```text
$0000-$07FF
$0800-$0FFF
$1000-$17FF
$1800-$1FFF
```

四个窗口共享一个 `InternalRam` 设备；设备可使用原始 Bus 地址执行 `address & 0x07FF`。

旧的 `Mmu`、`RamBank`、`Rom` 抽象已删除，不恢复为兼容层。

目前 CPU 仍暂时持有旧式 Internal RAM 成员，这是迁移到 `Nes` 所待处理的工作项。

## 6. Cartridge 与 Mapper

当前已实现：

- `Mapper` 抽象
- `Mapper000`
- Cartridge CPU/PPU 设备适配器
- PRG ROM CPU `$8000-$FFFF` 映射
- 16 KiB PRG 的 `$C000-$FFFF` 镜像
- CHR `$0000-$1FFF` 映射
- CHR/PRG 只读权限
- Cartridge 双 Bus 连接和失败回滚
- `disconnect()` 清理 mapping

尚未实现：

- iNES 文件加载
- Cartridge header 解析
- nametable mirroring 元数据
- CHR RAM
- 其他 Mapper
- SRAM、扩展音频和 Mapper IRQ

## 7. CPU 当前状态

CPU 目标架构是：

```text
6502 状态机 + CpuBus 客户端 + opcode dispatcher
```

CPU 不应直接依赖：

- MMU
- Mapper
- Cartridge
- APU
- Nes 进行内存读写

当前已经有：

- CPU Bus
- 基础 RAM/ROM 访问接口
- reset、NMI/IRQ 相关状态接口
- 周期计数接口
- `clock()` 周期入口骨架

当前明确未完成：

- 大量 opcode handler 缺失
- 旧 opcode 表仍会在直接链接/构造 CPU 时触发未定义符号
- `exec()` 尚未成为完整、可靠的按周期 CPU 执行器
- CPU Internal RAM 尚未迁移到 `Nes`

因此当前核心测试避免直接构造完整 CPU，CPU 完整实现应作为独立阶段推进。

## 8. PPU 计划与参考行为

本地 [PPU programmer reference - NESdev Wiki.html](PPU%20programmer%20reference%20-%20NESdev%20Wiki.html) 作为开发参考，不复制整篇文档内容。

PPU 第一阶段目标：

- CPU 可见寄存器 `$2000-$2007`
- `$2000-$2007` 每 8 字节镜像到 `$3FFF`
- `PPUCTRL`
- `PPUMASK`
- `PPUSTATUS`
- `OAMADDR`
- `OAMDATA`
- `PPUSCROLL`
- `PPUADDR`
- `PPUDATA`
- 内部 `v/t/x/w` 状态
- PPUDATA 读缓冲
- OAM 256 字节
- nametable VRAM
- palette RAM
- vblank 和 NMI 基础

关键寄存器副作用必须由 PPU register device 处理，不能用普通 RAM mapping 替代：

- `PPUSTATUS` 读取清除 vblank，并重置 `w`
- `PPUSCROLL` 和 `PPUADDR` 使用双写翻转
- `PPUDATA` 具有延迟读取和地址递增行为
- `PPUCTRL` 的 NMI enable 与 vblank 状态有关
- `$4014` 是 CPU Bus 上的 OAM DMA 入口，不是普通 PPU RAM

PPU Bus 规划：

```text
$0000-$1FFF  Cartridge CHR
$2000-$2FFF  nametable VRAM
$3000-$3EFF  nametable 镜像
$3F00-$3FFF  palette RAM 镜像
```

## 9. 统一时钟决策

CPU 和 PPU 不各自运行独立墙钟。当前实现加入了 `NesClock`：

```text
一个 NesClock tick
  -> PPU tick
每 3 个 PPU tick
  -> CPU cycle callback
```

`NesClock` 使用 callback 驱动 CPU/PPU，而不是每个 tick 发送 Qt queued signal。这样可以保证 CPU/PPU 在同一推进点执行，测试也不依赖 Qt EventLoop。

未来推荐的运行模型：

```text
EmulationThread
  -> NesClock
      -> PPU
      -> 每 3 tick CPU
```

墙钟只负责决定应该追赶多少模拟 tick，不能决定硬件行为。系统资源不足时：

- 不跳过 CPU 指令
- 不跳过 PPU tick
- 不跳过 DMA/NMI/寄存器副作用
- 可以丢弃过时显示快照
- 可以合并脏 tile 更新
- 可以限制追赶预算，避免无限追赶导致 GUI 卡死

当前 `Cpu::clock()` 只增加周期计数，还没有执行真实 opcode；这是有意保留的时钟基础边界。

## 10. Qt 显示架构决策

项目希望保留 tile 级 QGraphicsItem，而不是退化成只绘制整张 QImage 的普通模拟器。

当前状态：

- `NesNametableItem` 是整张 256x240 图像 Item，内部按 tile 更新
- `NesTileItem` 是独立 8x8 tile Item 原型
- `NesScene` 当前使用四张 nametable 图像做滚动演示

目标显示模型：

```text
PPU 逻辑层
  -> 按 dot/scanline 决定真实像素

显示投影层
  -> 用 tile/sprite QGraphicsItem 表达已确定的显示结果
```

PPU 不应让 GraphicsItem 自己重新模拟取数规则。

扫描线和滚动的关键约束：

- 已完成扫描线不能因为后续滚动而回溯移动
- tile 边界滚动需要更新 Item 位置
- tile 内像素偏移可能需要扩展 Item 的绘制区域
- 扩展区域应使用透明边缘
- 动态改变 `boundingRect()` 前调用 `prepareGeometryChange()`
- 必须明确 scene clipping、z-value、透明覆盖、缓存失效和旧区域清理

用户倾向于保留 tile 级 Item，并允许通过扩展宽度或扫描线片段解决 tile 内偏移，例如 1 像素偏移时生成 9x8 的显示区域。正式实现前需要基准测试 QGraphicsScene 在约 960 个背景 tile、精灵 Item 和 nametable 副本下的表现。

## 11. 内存与刷新策略

不要在 bank 切换或每帧刷新时频繁创建/销毁 Qt 对象。

计划采用：

- 固定背景 tile Item 池
- 固定 sprite Item 池
- 复用 tile 像素缓存和 QImage
- bank 切换只改变 tile cache key 或版本号
- 只标记受影响 tile 为 dirty
- 模拟线程批量产生显示提交
- GUI 线程一轮批量应用 dirty tile
- 不为每个像素或每个 tick 发送 Qt signal

长期可以提供调试模式和性能模式，但默认目标仍是 tile 级 Item 显示。

## 12. 测试状态

当前 `NesCoreTests` 已覆盖：

- CPU/PPU Bus 隔离
- Internal RAM 镜像行为（测试映射）
- Bus 地址重叠拒绝
- 相邻 mapping 注册
- PRG ROM 镜像
- PRG 写保护
- CHR 读保护
- Cartridge connect/disconnect
- `NesClock` 的 3:1 CPU/PPU 推进比例

最近验证结果：

```text
Build: success
CTest: 1/1 passed
Diagnostics: no errors in touched files
```

后续测试重点：

- 四个独立 InternalRam mapping
- PPU register mirror
- PPUSTATUS/PPUSCROLL/PPUADDR/PPUDATA 副作用
- OAMDATA 回绕
- PPU 地址递增模式
- vblank/NMI
- OAM DMA
- nametable mirroring
- tile Item 的几何变化、透明边缘和 z-order

## 13. 当前未完成事项

按优先级：

1. 将 Internal RAM 从 Cpu 移交给 Nes，并注册四个镜像窗口。
2. 实现 PPU register device 和 CPU `$2000-$3FFF` 映射。
3. 实现 PPU 内部 OAM、nametable、palette 存储。
4. 完善 `NesClock` 与 PPU dot/scanline/vblank 状态。
5. 接入 NMI 和 OAM DMA。
6. 重构 CPU 为可工作的按周期 6502 状态机，补齐 opcode 策略。
7. 增加 iNES 加载、mirroring 和更多 Mapper。
8. 将显示层从整张 nametable Item 逐步迁移到 tile/sprite Item 池。
9. 实现背景渲染、滚动、精灵评估、sprite 0 hit 和 overflow。
10. 接入 APU、Controller 和完整运行控制。

## 14. Git 状态与恢复方式

重要历史提交：

- `c349fd8 refactor-NES-memory-bus-cartridge-mapping`
- `3e1178e add-nes-core-tests`

远程仓库：

```text
https://github.com/Ruilx/NesV3.git
```

恢复开发时建议顺序：

1. 阅读本文。
2. 阅读 `src/Nes/Bus.*`、`Cartridge.*`、`NesClock.*`。
3. 运行 CMake 构建和 CTest。
4. 阅读本地 NESdev PPU HTML 的寄存器和 Internal registers 章节。
5. 从 Internal RAM 迁移和 PPU register device 开始，不恢复 MMU。

提交前验证：

```powershell
git status
git diff --check
D:\Qt\Tools\CMake_64\bin\cmake.exe --build build-qt6 --parallel 4
ctest --test-dir build-qt6 --output-on-failure
git diff --stat
```
