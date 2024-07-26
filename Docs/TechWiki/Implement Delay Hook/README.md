| **English (en-US)** | [简体中文 (zh-CN)](./README.zh-CN.md) |
| --- | --- |

<br>

# Implement Delay Hook

## What's "delay hook" and the benefits it brings?

The usual way to hook a function in a DLL has to load the corresponding DLL into the process space and locate its address (for example, use [`LoadLibraryW`](https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-loadlibraryw) + [`GetProcAddress`](https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getprocaddress)) at first.

For the hooks designed for a specific program, usually theirs target function will be called sooner or later, and the DLL is also required by the process, so loading the corresponding DLL early is fair enough. But for hooks are injected into different processes (especially global hooks), they don't know this DLL is required by each process or not, so they usually still load the DLL into each process space and hook the function, even if the process itself doesn't want this DLL.

Imagine a global hook with many dependencies trying to hook functions in various system DLLs, and then bring all the DLLs involved into all processes for loading and initialization, which the cost is enormous.

"Delay hook" is a good solution to this problem. That is, the hook is set immediately if the target DLL is already loaded, otherwise set the hook when the target DLL is loaded into the process.

## Technical solution

Obviously, the key to implementing "delay hook" is to acquire DLL load notification in the first place. "[DLL Load Notification](https://learn.microsoft.com/en-us/windows/win32/devnotes/dll-load-notification)" mechanism is introduced since NT6, this is what we need.

See [LdrRegisterDllNotification](https://learn.microsoft.com/en-us/windows/win32/devnotes/ldrregisterdllnotification) function, DLL load (and unload) notification will be sent to the callback registered by it, and the memory space mapped from the DLL will be available at that point while we can set hook.

Although Microsoft Learning prompts that related APIs may be changed or removed, their usage has not changed, just changed the held lock from `LdrpLoaderLock` to a dedicated `LdrpDllNotificationLock` since NT6.1. However, please keep the callback as simple as possible.

> [!TIP]
> If you want to know the internal implementation of "[DLL Load Notification](https://learn.microsoft.com/en-us/windows/win32/devnotes/dll-load-notification)" on Windows, see [ReactOS PR #6795](https://github.com/reactos/reactos/pull/6795) which I contributed to [ReactOS](https://github.com/reactos/reactos). Don't see [the WINE implementation](https://gitlab.winehq.org/wine/wine/-/commit/4c13e1765f559b322d8c071b2e23add914981db7), as it has mistakes as of this writing, for example, its `LdrUnregisterDllNotification` removes the node without checking it is in the list or not.

## Using "delay hook" in SlimDetours

[SlimDetours](https://github.com/KNSoft/KNSoft.SlimDetours) provides `SlimDetoursDelayAttach` function to register delay hooks, see comments above function declaration and [Demo: DelayHook](../../../Source/Demo/DelayHook.c) for more information.

In this example, `SlimDetoursDelayAttach` is called to register a delay hook for `User32.dll!EqualRect` API, and confirm that the `User32.dll` is not loaded at this point by checking the return values from it and `LdrGetDllHandle`:
```C
/* Register SlimDetours delay hook */
hr = SlimDetoursDelayAttach((PVOID*)&g_pfnEqualRect,
                             Hooked_EqualRect,
                             g_usUser32.Buffer,
                             g_asEqualRect.Buffer,
                             DelayAttachCallback,
                             NULL);
if (FAILED(hr))
{
    TEST_FAIL("SlimDetoursDelayAttach failed with 0x%08lX\n", hr);
    return;
} else if (hr != HRESULT_FROM_NT(STATUS_PENDING))
{
    TEST_FAIL("SlimDetoursDelayAttach succeeded with 0x%08lX, which is not using delay attach\n", hr);
    return;
}

/* Make sure user32.dll is not loaded yet */
Status = LdrGetDllHandle(NULL, NULL, &g_usUser32, &hUser32);
if (NT_SUCCESS(Status))
{
    TEST_SKIP("user32.dll is loaded, test cannot continue\n");
    return;
} else if (Status != STATUS_DLL_NOT_FOUND)
{
    TEST_SKIP("LdrGetDllHandle failed with 0x%08lX\n", Status);
    return;
}
```

Then call `LdrLoadDll` to load `User32.dll`:
```C
/* Load user32.dll now */
Status = LdrLoadDll(NULL, NULL, &g_usUser32, &hUser32);
if (!NT_SUCCESS(Status))
{
    TEST_SKIP("LdrLoadDll failed with 0x%08lX\n", Status);
    return;
}
```

If `User32.dll` is loaded successfully, the previously registered delay hook should have been hooked, and then verify that the delay hook callback was called correctly and the `User32.dll!EqualRect` function was successfully hooked:
```C
/* Delay attach callback should be called and EqualRect is hooked successfully */
TEST_OK(g_bDelayAttach);
Status = LdrGetProcedureAddress(hUser32, &g_asEqualRect, 0, (PVOID*)&pfnEqualRect);
if (NT_SUCCESS(Status))
{
    TEST_OK(pfnEqualRect(&rc1, &rc2) == TRUE);
    TEST_OK(g_lEqualRectCount == 1);
} else
{
    TEST_SKIP("LdrGetProcedureAddress failed with 0x%08lX\n", Status);
}
```

<br>
<hr>

This work is licensed under [Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0)](http://creativecommons.org/licenses/by-nc-sa/4.0/).  
<br>
**[Ratin](https://github.com/RatinCN) &lt;[<ratin@knsoft.org>](mailto:ratin@knsoft.org)&gt;**  
*China national certified senior system architect*  
*[ReactOS](https://github.com/reactos/reactos) contributor*
