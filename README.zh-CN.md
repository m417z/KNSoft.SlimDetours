| [English (en-US)](https://github.com/KNSoft/KNSoft.SlimDetours/blob/main/README.md) | **简体中文 (zh-CN)** |
| --- | --- |

<br>

# KNSoft.SlimDetours

[![NuGet Downloads](https://img.shields.io/nuget/dt/KNSoft.SlimDetours)](https://www.nuget.org/packages/KNSoft.SlimDetours) [![GitHub Actions Workflow Status](https://img.shields.io/github/actions/workflow/status/KNSoft/KNSoft.SlimDetours/msbuild.yml)](https://github.com/KNSoft/KNSoft.SlimDetours/actions/workflows/msbuild.yml) ![PR Welcome](https://img.shields.io/badge/PR-welcome-0688CB.svg) [![GitHub License](https://img.shields.io/github/license/KNSoft/KNSoft.SlimDetours)](https://github.com/KNSoft/KNSoft.SlimDetours/blob/main/LICENSE)

[SlimDetours](https://github.com/KNSoft/KNSoft.SlimDetours)是一个基于[Microsoft Detours](https://github.com/microsoft/Detours)改进而来的Windows API挂钩库。

## 功能

相比于原版[Detours](https://github.com/microsoft/Detours)，有以下优势：

- 新功能
  - **支持延迟挂钩（目标DLL加载时自动挂钩）** [🔗 技术Wiki：实现延迟挂钩](https://github.com/KNSoft/KNSoft.SlimDetours/blob/main/Docs/TechWiki/Implement%20Delay%20Hook/README.zh-CN.md)
  - **挂钩时自动更新线程** [🔗 技术Wiki：应用内联钩子时自动更新线程](https://github.com/KNSoft/KNSoft.SlimDetours/blob/main/Docs/TechWiki/Update%20Threads%20Automatically%20When%20Applying%20Inline%20Hooks/README.zh-CN.md)
- 经改进
  - **更新线程时避免堆死锁** [🔗 技术Wiki：更新线程时避免堆死锁](https://github.com/KNSoft/KNSoft.SlimDetours/blob/main/Docs/TechWiki/Avoid%20Deadlocking%20on%20The%20Heap%20When%20Updating%20Threads/README.zh-CN.md)
  - 避免占用系统保留的内存区域 [🔗 技术Wiki：分配Trampoline时避免占用系统保留区域](https://github.com/KNSoft/KNSoft.SlimDetours/blob/main/Docs/TechWiki/Avoid%20Occupying%20System%20Reserved%20Region%20When%20Allocating%20Trampoline/README.zh-CN.md)
  - 其它Bug修复与代码改进
- 轻量
  - **仅依赖`Ntdll.dll`**
  - 仅保留API挂钩函数
  - 移除对ARM (ARM32)、IA64、WinXP、GNUC的支持
  - 更小的二进制体积
- 开箱即用
  - NuGet包发布

## 用法

[![NuGet Downloads](https://img.shields.io/nuget/dt/KNSoft.SlimDetours)](https://www.nuget.org/packages/KNSoft.SlimDetours)

### 提要

KNSoft.SlimDetours包体同时含有[SlimDetours](https://github.com/KNSoft/KNSoft.SlimDetours)与原版[Microsoft Detours](https://github.com/microsoft/Detours)。

对于KNSoft.SlimDetours包含[SlimDetours.h](https://github.com/KNSoft/KNSoft.SlimDetours/blob/main/Source/SlimDetours/SlimDetours.h)头文件，或者对于原版[Microsoft Detours](https://github.com/microsoft/Detours)包含[detours.h](https://github.com/KNSoft/KNSoft.SlimDetours/blob/main/Source/Detours/src/detours.h)头文件，然后链接编译出的库`KNSoft.SlimDetours.lib`。

NuGet包[KNSoft.SlimDetours](https://www.nuget.org/packages/KNSoft.SlimDetours)是开箱即用的，只要安装到项目，编译好的库就会自动加入链接。

```C
#include <KNSoft/SlimDetours/SlimDetours.h> // KNSoft.SlimDetours
#include <KNSoft/SlimDetours/detours.h>     // Microsoft Detours
```

如果你项目的配置名称既不是“Release”也不是“Debug”，NuGet包中的[MSBuild表单](https://github.com/KNSoft/KNSoft.SlimDetours/blob/main/Source/KNSoft.SlimDetours.targets)无法自动链接编译好的库，需要手动链接，例如：
```C
#pragma comment(lib, "Debug/KNSoft.SlimDetours.lib")
```

用法与原版[Microsoft Detours](https://github.com/microsoft/Detours)相似，除了：

- 函数名以`"SlimDetours"`开头，大多数返回值是`NTSTATUS`，使用`NT_SUCCESS`宏检查。
- 线程会被自动更新，`DetourUpdateThread`已移除。
```C
Status = SlimDetoursTransactionBegin();
if (!NT_SUCCESS(Status))
{
    return Status;
}
Status = SlimDetoursAttach((PVOID*)&g_pfnXxx, Hooked_Xxx);
if (!NT_SUCCESS(Status))
{
    SlimDetoursTransactionAbort();
    return Status;
}
return SlimDetoursTransactionCommit();
```

### 延迟挂钩

“延迟挂钩”将在目标DLL加载时自动挂钩。

比如，调用`SlimDetoursDelayAttach`来在`a.dll`加载时自动挂勾`a.dll!FuncXxx`：
```C
SlimDetoursDelayAttach((PVOID*)&g_pfnFuncXxx,
                       Hooked_FuncXxx,
                       L"a.dll",
                       L"FuncXxx",
                       NULL,
                       NULL);
```
演示：[DelayHook.c](https://github.com/KNSoft/KNSoft.SlimDetours/blob/main/Source/Demo/DelayHook.c)

## 兼容性

项目构建：仅考虑对最新MSVC生成工具及SDK的支持，但一般也能较广泛地向下兼容。

制品集成：能较广泛地向下兼容MSVC生成工具，以及不同编译配置（如`/MD`、`/MT`）。

运行环境：NT6及以上操作系统，x86/x64/ARM64平台。

> [!CAUTION]
> 处于beta阶段，应小心使用。

## 协议

[![GitHub License](https://img.shields.io/github/license/KNSoft/KNSoft.SlimDetours)](https://github.com/KNSoft/KNSoft.SlimDetours/blob/main/LICENSE)

KNSoft.SlimDetours根据[MPL-2.0](https://github.com/KNSoft/KNSoft.SlimDetours/blob/main/LICENSE)协议进行许可。

源码基于[Microsoft Detours](https://github.com/microsoft/Detours)，其根据[MIT](https://github.com/microsoft/Detours/blob/main/LICENSE)协议进行许可。

同时使用了[KNSoft.NDK](https://github.com/KNSoft/KNSoft.NDK)以访问底层Windows NT API及其中的单元测试框架。
