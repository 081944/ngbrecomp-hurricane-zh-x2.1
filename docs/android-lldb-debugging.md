# Android 真机 LLDB 断点调试流程

本文记录 2026-11-07 在 AYN Thor 上实际走通的流程，包括函数断点、源码符号、硬件写入监视点和换装异常现场。所有命令使用 PowerShell 7；LLDB 提示符下的命令单独标注。地址、PID、断点编号只对当次进程有效。

## 1. 前提与文件

- 项目：`D:\CN\NGPort\zh-work`。
- 设备序列号：`c53b4da`；包名：`io.github.gamboozino.ngbrecomp`。
- 当前测试 APK 的 manifest 设置 `android:debuggable="true"`，使 `run-as` 能以应用 UID 启动调试服务器。
- 本地符号必须来自设备实际运行的同一次编译：`build/android-release/build/libmain.so`。不要拿重新编译后的文件解释旧进程。
- 使用 Android NDK `29.0.14206865` 中配套的 LLDB 和 AArch64 lldb-server。
- 调试过程不需要重新打包。只有代码修改后的真机验证才需要构建、打包、安装。

## 2. 启动设备端服务

在一个 PowerShell 7 终端执行：

```powershell
$ErrorActionPreference = 'Stop'
$debugSerial = 'c53b4da'
$debugPackage = 'io.github.gamboozino.ngbrecomp'
$debugNdk = Join-Path $env:ANDROID_HOME 'ndk/29.0.14206865'
$debugServer = Join-Path $debugNdk 'toolchains/llvm/prebuilt/windows-x86_64/lib/clang/21/lib/linux/aarch64/lldb-server'
adb -s $debugSerial shell pidof $debugPackage
adb -s $debugSerial push $debugServer /data/local/tmp/ngb-lldb21
adb -s $debugSerial shell chmod 755 /data/local/tmp/ngb-lldb21
adb -s $debugSerial forward tcp:5043 localabstract:/ngb-debug21
adb -s $debugSerial shell run-as $debugPackage /data/local/tmp/ngb-lldb21 gdbserver unix-abstract:///ngb-debug21
```

最后一条持续运行；保留终端。执行 adb 等原生命令后应检查 `$LASTEXITCODE`，非零就停止。`$ErrorActionPreference` 本身不能保证所有 PowerShell 版本都会因原生命令非零退出。

此设备上，以应用 UID 监听 TCP 返回 `Operation not permitted`。使用 abstract Unix socket 成功。这里 URI 对应的 socket 名包含起始 `/`，因此转发必须是 **`localabstract:/ngb-debug21`**。少这个斜杠会出现连接建立但握手失败。先启动空服务、连接后再 attach 成功；不要沿用此前挂住的启动即 `--attach` 方式。

## 3. 启动主机 LLDB 并附加

在另一个 PowerShell 7 终端执行：

```powershell
$ErrorActionPreference = 'Stop'
$debugNdk = Join-Path $env:ANDROID_HOME 'ndk/29.0.14206865'
$debugHost = Join-Path $debugNdk 'toolchains/llvm/prebuilt/windows-x86_64'
$env:PATH = (Join-Path $debugHost 'python3') + ';' + $env:PATH
& (Join-Path $debugHost 'bin/lldb.exe')
```

必须把配套 `python3` 目录放进 PATH，否则本机 lldb.exe 曾以 `-1073741515` 退出，且没有正常错误提示。

以下在 LLDB 提示符执行，把 PID 换成 `pidof` 的实际输出：

```text
gdb-remote 127.0.0.1:5043
process attach --pid 6091
process status
```

成功标志是进程出现 `stopped`，能执行 `register read pc sp lr`、`thread list`、`bt`。仅 TCP 端口连接成功不代表 attach 成功。

## 4. 配置运行时内部信号

本项目运行时使用 SIGSEGV/SIGBUS 完成内存访问陷阱处理，附加时可能先停在这种信号上。在 LLDB 执行：

```text
process handle SIGSEGV --stop false --notify false --pass true
process handle SIGBUS --stop false --notify false --pass true
process handle 45 --stop false --notify false --pass true
```

保留 `--pass true`，否则运行时收不到自己的信号。这是此运行时的配置，不是对所有应用通用的忽略崩溃方式。需要检查真正崩溃时可重新开启 stop。

Android 的虚拟 CPU 调度还使用 `SIGRTMIN + 4`（本机为信号 45，见 `xboxrecomp/src/platform/vcpu.c`）。必须一并透传。本次蓄力问题调查中补齐了此项。无线调试不要在每帧函数上设置条件断点：即使条件为假，仍会产生每帧调试往返。优先在异常专用基本块设置地址断点，或监视已确认的状态地址。调试器暂停期间的耗时不能作为游戏性能证据。

## 5. 符号加载和 ASLR 地址

先在 PowerShell 7 查询映射和本地符号：

```powershell
$ErrorActionPreference = 'Stop'
adb -s c53b4da shell run-as io.github.gamboozino.ngbrecomp cat /proc/6091/maps
$debugElf = 'D:/CN/NGPort/zh-work/build/android-release/build/libmain.so'
$debugBin = Join-Path $env:ANDROID_HOME 'ndk/29.0.14206865/toolchains/llvm/prebuilt/windows-x86_64/bin'
& (Join-Path $debugBin 'llvm-readelf.exe') -l $debugElf
& (Join-Path $debugBin 'llvm-nm.exe') -n $debugElf | Select-String 'ngb_costume_frame_tick|g_memory_base|g_xbox_mem_offset'
Copy-Item -LiteralPath $debugElf -Destination 'D:/CN/NGPort/zh-work/build/android-release/ngb-debug-symbols.so'
```

本次 libmain.so 的 executable 映射起点 `0x7703e74000`、文件 offset 为 0；ELF 对应 LOAD 的 virtual address 为 0，因此 load slide 是 `0x7703e74000`。每次重新启动都要重算，不能硬编码复用。一般情况应将 maps 的文件 offset 与 ELF LOAD 对齐后确定 slide。

本次直接添加原名 `libmain.so` 出现 `Failed to call GetCachedSharedModule`。复制为不同 basename 后加载成功；这是本次会话实际有效的绕过方式，未断言 LLDB 内部失败原因。

在 LLDB 执行：

```text
target modules add D:/CN/NGPort/zh-work/build/android-release/ngb-debug-symbols.so
target modules load --file ngb-debug-symbols.so --slide 0x7703e74000
image list
bt 12
```

验收标准：栈帧显示 `ngb_costume_frame_tick` 等函数名及 `costume_switch.c` / `recomp_0013.c` 等源码行；函数断点拥有 resolved location。

## 6. 实际验证函数断点命中

先选每帧必经函数，排除需要人工换装才能验证的问题：

```text
breakpoint set --name ngb_costume_frame_tick
continue
```

若尚未加载符号，也可使用 `slide + llvm-nm 的函数值` 设置绝对地址断点。本次函数值 `0x19fee30`，实际使用：

```text
breakpoint set --address 0x7705872e30
continue
```

实际结果：进程 6091 的 `Thread-4` 停止原因是 **`breakpoint 2.1`**，PC `0x7705872e30`。加载符号后调用栈：

```text
ngb_costume_frame_tick
ngb_frame_tick
d3d_hook
sub_003CC160
sub_0030CDB0
sub_0030E140
```

记录 `breakpoint list` 的 hit count；仅创建 pending breakpoint 或只有“已附加”不能当作命中。

## 7. 换装入口和位置写入监视点

```text
breakpoint disable 1 2
breakpoint set --name sub_002FC6F0
```

编号根据 `breakpoint list` 调整。用户切换一次后，实际命中 `sub_002FC6F0`、`costume_switch.c:123`，停止原因 `breakpoint 3.1`。检查后 `continue` 继续。

位置是 guest actor `0x4B4E50 + 0x8C0`，Y 为 guest `0x4B5714`。必须先读取本次运行的 `g_xbox_mem_offset`；不要直接把 guest 地址交给 host watchpoint。本次 offset 为 `0x75ea650000`，Y 的 host 地址为 `0x75eab05714`：

```text
memory read -f f -s 4 -c 4 0x75eab05710
watchpoint set expression -w write -s 4 -- 0x75eab05714
continue
bt 12
```

设备报告支持 4 个硬件监视点。实际已出现 `stop reason = watchpoint 1`。硬件粒度可能使相邻 X 写入也触发，因此停下后应查看反汇编和完整四维向量；不要只看 LLDB 自动打印的整型 old/new 值。读取浮点值用 `memory read -f f -s 4`。

为了避开换装内部的预期恢复写入，先禁用监视点，在最后一次 pose restore 后断住、记录基线，再启用它。当前源码对应 `costume_switch.c:347`；代码改动后重新确认位置，避免复制旧行号。

调试回调可使用 `SBProcess.ReadMemory` 读取向量，记录实际变化、线程和调用栈，仅在较大位移时保持暂停。当前会话回调为 `build/android-release/lldb_costume_probe.py`；它含本次固定地址，复用前必须更新。高频写入监视明显拖慢真机速度，不能以调试帧率判断最终游戏性能。

## 8. 本次异常的实际证据

2026-11-07，用户切换后：

- 换装入口和最终恢复位置均为 `(42.559856, -50.901962, -98.164421)`。
- 随后监视点在 `sub_00148770` 的 `XMM_STORE` 命中，源码 `recomp_0010.c:22448`。
- 同一次向量更新先使 X 增加 `4.948677`，再使 Y 从 `-50.901962` 变为 `-4.796417`，上移 `46.105545`。
- guest stack 位移向量为 `(4.948676, 46.105545, 13.034207, 0)`。
- Y 写入后的 PC 为 `0x7704baf320`。当时暂停在 Z 写入之前，因此内存中的向量仍处于分量更新中。

调用链：

```text
sub_00148770
sub_00148590
sub_001482B0
sub_00147DF0
sub_00151150
sub_0030C8B0
sub_0032A1AF
fiber_host_entry
```

这证明了异常位移的具体写入路径；尚不能仅凭该栈断言上游哪项骨骼或碰撞状态错误。

本次原始证据保留在 ignored 的 `build/android-release/`：

- `lldb-costume-evidence.jsonl`：停点时间、浮点位置、线程、函数和源码行。
- `lldb-actor.bin`、`lldb-cache.bin`：角色和动画缓存。
- `lldb-guest-stack.bin`、`lldb-collision-matrix.bin`、`lldb-collision-history.bin`：异常位移输入现场。
- `lldb-session-20261107.md`：连接记录和现场说明。

上述原始内存及生成源码不提交到仓库。

## 9. 继续、暂停和退出

```text
process interrupt
process status
watchpoint disable 1
continue
```

需要结束调试但保留游戏时，使用 `process detach`；不要误用 `process kill`。按 PID 结束自己启动的服务器，清理自己的 `tcp:5043` 转发。下一轮重新连接时确认进程是否仍是原 PID，并重新计算映射、offset 与所有监视点地址。

故障排查时不要启用持续的 `log enable lldb object`：运行时高频信号会产生海量输出。若误启用，执行 `log disable lldb all`。交互命令尽量短；长 Python 操作保存成文件再 `command script import`，避免终端输入截断。

## 10. 换装问题的进一步定位与修复（2026-11-07）

在同一进程中，调试器通过写入与 `ngb_costume_request` 相同的请求状态触发换装，无需测试者反复操作。写入前检查 actor 可用、trigger 为 `0xFF`、没有 pending 请求；记录明确标注为 debugger request，不与用户实际按键混淆。

逐帧检查进一步确定：换装初次重建后骨骼正常，下一次动画采样就异常。一次记录中，采样根向量从接近零变成 `(17.939999, -5.041000, 8.128200)`，骨骼矩阵随之偏离；这发生在碰撞造成角色起飞之前。

问题出在手写 `rebuild_skeleton()` 的调用顺序：

1. `sub_00188E70` 解释新动画脚本，将 cache `+0x22BC` 置为 1，表示需要绑定动画片段。
2. 旧实现直接清掉该标志并将 `+0x2344` 设为 0，然后调用 `sub_0018EDF0` 采样。
3. 关键帧解码器在 cache `+0x168` 等处保存的游标没有重新绑定，仍引用旧服装资源。清除输出缓存的帧号不能刷新这些输入游标。
4. 首次看似正常的缓存结果过后，解码器从旧指针读入错误位置/旋转；错误骨骼矩阵再进入 `sub_00148770` 的碰撞位置修正，产生异常位移。

修复调用顺序：保留脚本的绑定请求，通过 `sub_0018E8B0` 消费它，再采样。该函数在 `0x22BC != 0` 时调用 `sub_0018E920`，从新动画资源重建关键帧游标、失效采样缓存，并设置脚本指定的起始帧。不要在之后强制覆盖为 frame 0。

现场验证在首次采样前调用了这条原生绑定路径，观察到游标由旧资源指针改为新的 `0x84756f50` 等地址。240 帧验证中位置最大变化约 `0.000004`；之前同一条写入路径曾造成 Y 增加 `46.105545`。这轮验证属于 LLDB 中的调用顺序验证，后续安装版本的验证需要单独记录。

主机回归测试在模拟脚本选择新片段时故意留下 `0xBAD000` 旧游标，要求原生绑定步骤先将其更新，并保留 frame 1，再允许采样；避免只检查输出帧缓存是否清零。

### Windows 源码文件被 LLDB 锁定

LLDB 的源码缓存可能持有映射，导致编辑时报“文件正被另一进程使用”，原子替换也可能失败。已实际验证在 LLDB 执行 `source cache clear` 后可编辑。改完源文件但旧进程仍运行时，源码行显示可能与旧二进制不一致；必须明确当前符号对应哪个构建，不能据新的行号解释旧地址。

## 11. 安装版本验证与调试脚本校验

2026-11-07 14:08 构建的 APK 已通过 `adb install -r` 安装，SHA256 为 `FD9D310A0EE72ECC8E10033EE8894C7693B5F90C78079411E6E0A010D4FD6483`。这是包含代码修改的版本，验证时不再注入原生绑定函数。

自动请求必须验证实际走进面板重建路径。一次脚本把新进程 `s_costume` 地址手算偏了 `0x10000`，只写对 guest trigger、没有写对 pending 状态，请求因此走原生剧情换装路径；该轮出现骨骼大旋转与位移，不能当作修复路径的回归结论。错误地址的写入需通过重启进程清除，不依赖猜测恢复。保留失败记录并单独标注，不混入有效通过统计。

防止重复此类错误：

- 用整数表达式 `SLIDE + symbol_offset` 计算地址，禁止手算后抄写最终地址。
- 使用 `frame variable s_costume` 与内存读取交叉检查 pending、target 和 area。
- 在最终 `ngb_costume_pose_restore` 后设置断点，只有该断点实际命中后才开始统计换装帧。
- 同款服装请求可能直接返回，不应等待一个不会执行的重建断点。
- 各轮检查 15 根骨骼的矩阵有限性、平移、旋转正交误差、相邻帧旋转角，以及角色根位置；仅检查 bone0 平移不足以排除上半身扭曲。

有效运行的原始逐帧数据保存在 ignored 的 `lldb-installed-validation.jsonl`。其中 `post-switch-investigation` 是上述错误请求的调查记录；正式通过结果应按对应的新运行阶段及 PASS 记录筛选，最终需以干净重启后的结果为准。


### 当前验证记录（14:30 设备断线时）

| 阶段 | 实际结果 | 结论范围 |
| --- | --- | --- |
| 旧进程，首次采样前通过 LLDB 调用原生绑定 | 8 次换装，每次 180 帧；根位置分量最大变化约 0.000004 | 证明调用顺序可消除已复现的坏游标位移；当时只量化 bone0 平移 |
| 安装修复 APK，正确请求地址，服装 3 | 240 帧；15 根骨骼最大单帧旋转约 0.989°，最大平移分量约 1.4253，正交误差约 0.000000535，根位置不变 | 该次面板换装未复现骨骼突变与位移 |
| 干净重启，用户回到楼梯 | PID 10211；符号地址与计算地址一致；换装前采到 52 帧基线，然后 ADB 设备消失、LLDB lost connection | 未执行完换装，不能记为通过；需恢复设备后继续 |

主机 quick-panel C/Java 回归测试、Android native 编译、APK 签名及对齐检查已通过。没有完成全部楼梯服装回归之前，不将问题标记为完全修复。

### 无线 ADB 接续

USB 断开后，用户提供无线调试端点 `192.168.2.183:35903`，使用 `adb connect` 成功，之后所有设备命令的 `-s` 都改成该端点。无线调试的端口由设备提供，会变化，不把本次值当作固定设置。

```powershell
$ErrorActionPreference = 'Stop'
adb connect 192.168.2.183:35903
adb -s 192.168.2.183:35903 shell pidof io.github.gamboozino.ngbrecomp
adb -s 192.168.2.183:35903 forward tcp:5047 localabstract:/ngb-debug-wireless2
```

端口转发后 LLDB 仍连接本机 `gdb-remote 127.0.0.1:5047`，其余服务器流程相同。设备重新启动游戏后 PID 和 slide 均已改变，需重新 attach、重新加载模块地址，并清除旧断点；本轮 PID 11607、slide `0x7705863000`，`g_xbox_mem_offset` 为 `0x75f26d0000`。实际解析的 `s_costume` 地址 `0x7708be1498` 与 `slide + 0x337e498` 一致。

LLDB 的全删断点命令 `breakpoint delete` 会交互确认，批量输入时应显式给编号，避免后续命令被吞为确认回答。长 `script` 行也可能被终端截断；每行保持简短，复杂逻辑放 Python 文件后导入。

## 12. 最终楼梯回归结果（2026-11-07 14:39 完成）

已在安装的修复 APK、干净重启的 PID 11607 中完成整轮验证。用户将角色置于此前复现的楼梯并站稳；通过等效面板请求自动轮换服装 `2 → 3 → 4 → 5 → 6 → 7 → 0 → 1`。调试器仅触发请求和读数，没有注入绑定函数，没有写入角色位置或骨骼矩阵。每次均在最后一次 pose restore 后命中断点才统计换装帧。

| 检查项 | 最终结果 |
| --- | --- |
| 覆盖服装 | 0～7 全部 8 套 |
| 换装后采样 | 每套 240 帧，共 1920 帧，零 FAIL |
| 根位置最大分量变化 | 0；保持 `(44.0570678711, -52.2760658264, -93.3510208130)` |
| 检查的骨骼 | actor `+0xC0` 起的 15 个矩阵，每帧读取 |
| 相邻帧最大旋转角 | 约 1.786294° |
| 骨骼平移分量最大绝对值 | 约 1.394501 |
| 旋转矩阵最大正交误差 | 约 0.000000536 |
| 非有限矩阵、突变、起飞、滑行 | 本轮均未出现 |

`lldb-wireless-clean-validation.jsonl` 保存完整逐帧矩阵和各轮 PASS。换装前另执行 240 帧基线；输出文件在基线第 70 帧改为无线专用文件，故前 69 帧位于先前的 `lldb-clean-validation.jsonl`。8 套换装后的 1920 帧完整保存在无线专用文件。`ngb-costume-validated.png` 是本轮楼梯现场截图，已人工检查角色站姿；捕获时的低帧率来自逐帧断点采集，不作为正常运行性能结论。

最终修复是在首次采样前消费片段绑定请求，更新旧服装资源中的关键帧游标，并保留脚本指定起始帧。已通过上述楼梯静止换装复现场景；不把该场景扩展表述为所有战斗动作和所有地形的穷举验证。验证后执行 `process detach` 恢复游戏运行。
