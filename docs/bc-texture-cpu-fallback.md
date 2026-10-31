# BC 纹理压缩的 CPU 回退方案（待办备忘）

> 状态：**未实施**，仅记录方案。后续修改时以本文档为起点。
> 来源：iSebas21/ngbrecomp-android fork 中的 CLAUDE.md 规划指南（Phase 3），结合本仓库代码现状整理。
> **2026-10-30 更新**：补充 SansNope/UnleashedRecomp-Android 的生产级实现参考（见文末"成熟实现参考"）。

## 问题背景

`src/d3d/vk/backend.cpp` 创建 Vulkan 设备时**无条件**要求 BC 纹理压缩特性：

```cpp
// backend.cpp:717
f2.features.textureCompressionBC = VK_TRUE;
```

游戏纹理是 DXT 格式（BC1/BC2/BC3，映射见 `backend.cpp:165-167`）。桌面 GPU 普遍支持，
但移动端 Adreno / Mali 的**官方驱动通常不暴露 `textureCompressionBC`**，此时设备创建直接失败，
游戏无法启动。当前 Android 版靠 DriverStore 安装 Turnip 驱动绕过此问题，但
Mali 设备及无法安装 Turnip 的设备仍是死路。

## 回退方案思路

检测 `vkGetPhysicalDeviceFeatures()` 返回的 `textureCompressionBC`：

- **支持** → 维持现状，纹理以 BC 压缩格式上传 GPU（性能最优）。
- **不支持** → 设备创建时不再要求该特性，纹理上传前在 **CPU 端解码为 RGBA8/BGRA8** 再上传。

### 关键点：解码代码已经存在，可直接复用

`backend.cpp:177` 的 `DecodeBC()` 已经能把 DXT1-5（BC1/BC2/BC3）块解码为 BGRA8，
目前仅用于 mipmap 生成路径（`backend.cpp:2073-2076`：`bc` 纹理在 host 侧先解码出顶层，
再由 texels 生成整条 mipmap 链）。回退时把这条路推广到**所有 mip 层**即可。

上游 macOS 移植（PR #49）也解过类似的移动端特性问题，可交叉参考。

### 大致改动点（实施时再细化）

1. 初始化时查询物理设备特性，保存 `bool bc_supported`。
2. `backend.cpp:717` 改为 `f2.features.textureCompressionBC = bc_supported;`
3. 纹理上传路径：`!bc_supported` 且 `fi.host` 为 BC1/BC2/BC3 时，
   用 `DecodeBC()` 解码为 BGRA8，改用 `VK_FORMAT_B8G8R8A8_UNORM` 上传（内存占用约 ×4，需留意显存）。
4. mipmap 链：回退路径下所有层级都从解码后的 texels 生成（现有 gen==1 逻辑已类似）。
5. 仅 Android/移动端启用回退判断，桌面路径行为不变（遵循本 fork "不破坏既有 target" 的规则，
   改动放在 `#if defined(__ANDROID__)` 或运行时判断之后）。

## 附带风险提示（同属 Phase 3，一并记录）

- `VK_KHR_push_descriptor`（`backend.cpp:725` 附近无条件启用）：移动端驱动支持度不一，考虑可选化。
- depth/stencil 格式（`backend.cpp:479、743` 附近选 `D24_UNORM_S8_UINT` 或 `D32_SFLOAT_S8_UINT`）：
  需确认移动端至少有一个可用，否则回退到存在的格式（如 `D32_SFLOAT` + 独立 stencil）。
- NV2A xemu 模型所需的 `geometryShader` 等特性：macOS 版 `instance.c:580` 附近已做过
  "使用 d3d renderer 时放宽特性要求" 的处理，Android 需要同样的放宽。
- 排查顺序建议：先 Adreno（Vulkan 支持最好），Mali 作为实验性支持。

## 成熟实现参考：SansNope/UnleashedRecomp-Android

该项目的 Mali 支持正是靠 BC 回退实现的（已验证于 Mali-G615 / Dimensity 8300 Ultra），
实现在 `UnleashedRecomp/gpu/video.cpp`。比本方案更精细的两点：

### 1. 两级回退，优先转码而非解码（重要改进）

```cpp
// video.cpp:5922 附近
enum class BCnFallbackMode { None, Decode, Transcode };
```

- **优先 Transcode**：设备不支持 BC 但支持 **ETC2/EAC**（移动端几乎都支持）时，
  用 [etcpak](https://github.com/wolfpld/etcpak) 把 BC 块**转码**为 ETC2/EAC——
  显存占用与 BC 相当（仅带透明通道的 BC1 翻倍），远优于直接解 RGBA8 的 4-8 倍。
- **兜底 Decode**：连 ETC2 都没有时才用 [bcdec](https://github.com/iOrange/bcdec) 解码为 RGBA8/R8/RG8。
- 细节：bcdec 输出 RGBA 而 etcpak 要求 BGRA，中间需交换 R/B（`DecodeBlockSwapRB`）；
  BC4/BC5 → EAC R/RG 有专门的通道摆放（`DecodeBC4BlockForEac` / `DecodeBC5BlockForEac`）；
  sRGB 变体只在转码路径保留（纯解码会丢失）。
- 决策：扫描原始 BC 块判断不透明/透明，选择 ETC2 RGB（与 BC1 同大小）或 ETC2 RGBA。

> **对本项目的含义**：NGB 移动端目标设备基本都支持 ETC2，把本方案的第 3 步
> （解码为 BGRA8）改为"优先转码 ETC2/EAC、兜底 RGBA8"能省约 4 倍纹理显存。
> 可引入 etcpak + bcdec 两个单头/小库。若想省事，先只做 RGBA8 兜底路径也可接受。

### 2. 调试开关（实施时建议照抄）

通过在 `driver_import/` 放标记文件强制切换路径，方便在真机上验证两条路：

- `force_no_bc.txt`：在支持 BC 的 GPU 上强制走回退路径（测试用）。
- `force_bc.txt`：在报告不支持但实际支持的 GPU（如 Xclipse）上强制原生 BC。
  注释明确警告：在真缺 BC 的 Adreno/Turnip 上用会纹理损坏，故藏在 Debug 选项后。
- 启动器 UI 上显示当前模式：Supported / ETC2 Transcode / CPU Decode（`video.cpp:2710`）。
