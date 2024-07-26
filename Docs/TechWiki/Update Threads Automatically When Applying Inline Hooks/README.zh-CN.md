| [English (en-US)](./README.md) | **简体中文 (zh-CN)** |
| --- | --- |

<br>

# 应用内联钩子时自动更新线程

## 内联挂钩时更新线程的必要性

内联挂钩需要修改函数开头的指令实现跳转，为应对有线程正好运行在要修改的指令上的可能，需要更新处于此状态的线程避免其在修改指令时执行非法的新老共存的指令。

## 其它挂钩库中的实现

### Detours

[Detours](https://github.com/microsoft/Detours)提供了[`DetourUpdateThread`](https://github.com/microsoft/Detours/wiki/DetourUpdateThread)函数更新线程，但需要由调用方传入需要进行更新线程的句柄：
```C
LONG WINAPI DetourUpdateThread(_In_ HANDLE hThread);
```
也就是说，需要由调用方遍历进程中除自己以外的所有线程并传入给此函数，用起来比较复杂且不方便。

但[Detours](https://github.com/microsoft/Detours)更新线程非常精细，它通过使用[`GetThreadContext`](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getthreadcontext)与[`SetThreadContext`](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-setthreadcontext)准确地调整线程上下文中的PC（程序计数器）到正确位置，实现参考[Detours/src/detours.cpp于4b8c659f · microsoft/Detours](https://github.com/microsoft/Detours/blob/4b8c659f549b0ab21cf649377c7a84eb708f5e68/src/detours.cpp#L1840)。

> [!TIP]
> 虽然它的官方示例“[Using Detours](https://github.com/microsoft/Detours/wiki/Using-Detours)”中有`DetourUpdateThread(GetCurrentThread())`这样的代码，但这用法无意义且无效，应使用其更新进程中除当前线程外的所有线程，详见[`DetourUpdateThread`](https://github.com/microsoft/Detours/wiki/DetourUpdateThread)。但即便以正确的方式更新线程，也会带来一个新的风险，见[🔗 技术Wiki：更新线程时避免堆死锁](https://github.com/KNSoft/KNSoft.SlimDetours/blob/main/Docs/TechWiki/Avoid%20Deadlocking%20on%20The%20Heap%20When%20Updating%20Threads/README.zh-CN.md)。

### MinHook

[MinHook](https://github.com/TsudaKageyu/minhook)做的比较好，它在挂钩（和脱钩）时自动更新线程，并且像[Detours](https://github.com/microsoft/Detours)一样准确地更新线程上下文中的PC（程序计数器）。

### mhook

[mhook](https://github.com/martona/mhook)在挂钩（和脱钩）时自动更新线程，实现参考[mhook/mhook-lib/mhook.cpp于e58a58ca · martona/mhook](https://github.com/martona/mhook/blob/e58a58ca31dbe14f202b9b26315bff9f7a32598c/mhook-lib/mhook.cpp#L557)。

但它更新线程的方式比起上述几个则有点笨拙，若线程正好位于要修改指令的区域则等待100毫秒，最多尝试3次：
```C
while (GetThreadContext(hThread, &ctx))
{
    ...
    if (nTries < 3)
    {
        // oops - we should try to get the instruction pointer out of here. 
        ODPRINTF((L"mhooks: SuspendOneThread: suspended thread %d - IP is at %p - IS COLLIDING WITH CODE", dwThreadId, pIp));
        ResumeThread(hThread);
        Sleep(100);
        SuspendThread(hThread);
        nTries++;
    }
    ...
}
```

## SlimDetours中的实现

[SlimDetours](https://github.com/KNSoft/KNSoft.SlimDetours)兼顾了以上优点，在挂钩（或脱钩）时遍历进程的所有线程，然后沿用[Detours](https://github.com/microsoft/Detours)的方式更新线程上下文。

挂起当前进程中除当前线程外的所有线程，并返回它们的句柄：
```C
NTSTATUS
detour_thread_suspend(
    _Outptr_result_maybenull_ PHANDLE* SuspendedHandles,
    _Out_ PULONG SuspendedHandleCount)
{
    NTSTATUS Status;
    ULONG i, ThreadCount, SuspendedCount;
    PSYSTEM_PROCESS_INFORMATION pSPI, pCurrentSPI;
    PSYSTEM_THREAD_INFORMATION pSTI;
    PHANDLE Buffer;
    HANDLE ThreadHandle, CurrentPID, CurrentTID;
    OBJECT_ATTRIBUTES ObjectAttributes = RTL_CONSTANT_OBJECT_ATTRIBUTES(NULL, 0);

    /* Get system process and thread information */
    i = _1MB;
_Try_alloc:
    pSPI = (PSYSTEM_PROCESS_INFORMATION)detour_memory_alloc(i);
    if (pSPI == NULL)
    {
        return STATUS_NO_MEMORY;
    }
    Status = NtQuerySystemInformation(SystemProcessInformation, pSPI, i, &i);
    if (!NT_SUCCESS(Status))
    {
        detour_memory_free(pSPI);
        if (Status == STATUS_INFO_LENGTH_MISMATCH)
        {
            goto _Try_alloc;
        }
        return Status;
    }

    /* Find current process and threads */
    CurrentPID = NtGetCurrentProcessId();
    pCurrentSPI = pSPI;
    while (pCurrentSPI->UniqueProcessId != CurrentPID)
    {
        if (pCurrentSPI->NextEntryOffset == 0)
        {
            Status = STATUS_NOT_FOUND;
            goto _Exit;
        }
        pCurrentSPI = (PSYSTEM_PROCESS_INFORMATION)Add2Ptr(pCurrentSPI, pCurrentSPI->NextEntryOffset);
    }
    pSTI = (PSYSTEM_THREAD_INFORMATION)Add2Ptr(pCurrentSPI, sizeof(*pCurrentSPI));

    /* Skip if no other threads */
    ThreadCount = pCurrentSPI->NumberOfThreads - 1;
    if (ThreadCount == 0)
    {
        *SuspendedHandles = NULL;
        *SuspendedHandleCount = 0;
        Status = STATUS_SUCCESS;
        goto _Exit;
    }

    /* Create handle array */
    Buffer = (PHANDLE)detour_memory_alloc(ThreadCount * sizeof(HANDLE));
    if (Buffer == NULL)
    {
        Status = STATUS_NO_MEMORY;
        goto _Exit;
    }

    /* Suspend threads */
    SuspendedCount = 0;
    CurrentTID = NtGetCurrentThreadId();
    for (i = 0; i < pCurrentSPI->NumberOfThreads; i++)
    {
        if (pSTI[i].ClientId.UniqueThread == CurrentTID ||
            !NT_SUCCESS(NtOpenThread(&ThreadHandle,
                                     THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_SET_CONTEXT,
                                     &ObjectAttributes,
                                     &pSTI[i].ClientId)))
        {
            continue;
        }
        if (NT_SUCCESS(NtSuspendThread(ThreadHandle, NULL)))
        {
            _Analysis_assume_(SuspendedCount < ThreadCount);
            Buffer[SuspendedCount++] = ThreadHandle;
        } else
        {
            NtClose(ThreadHandle);
        }
    }

    /* Return suspended thread handles */
    if (SuspendedCount == 0)
    {
        detour_memory_free(Buffer);
        *SuspendedHandles = NULL;
    } else
    {
        *SuspendedHandles = Buffer;
    }
    *SuspendedHandleCount = SuspendedCount;
    Status = STATUS_SUCCESS;

_Exit:
    detour_memory_free(pSPI);
    return Status;
}
```

精准更新线程上下文PC（程序计数器）：
```C
NTSTATUS
detour_thread_update(
    _In_ HANDLE ThreadHandle,
    _In_ PDETOUR_OPERATION PendingOperations)
{
    NTSTATUS Status;
    PDETOUR_OPERATION o;
    CONTEXT cxt;
    BOOL bUpdateContext;

    cxt.ContextFlags = CONTEXT_CONTROL;
    Status = NtGetContextThread(ThreadHandle, &cxt);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    for (o = PendingOperations; o != NULL; o = o->pNext)
    {
        bUpdateContext = FALSE;
        if (o->fIsRemove)
        {
            if (cxt.CONTEXT_PC >= (ULONG_PTR)o->pTrampoline &&
                cxt.CONTEXT_PC < ((ULONG_PTR)o->pTrampoline + sizeof(o->pTrampoline)))
            {
                cxt.CONTEXT_PC = (ULONG_PTR)o->pbTarget +
                    detour_align_from_trampoline(o->pTrampoline, (BYTE)(cxt.CONTEXT_PC - (ULONG_PTR)o->pTrampoline));
                bUpdateContext = TRUE;
            }
        } else
        {
            if (cxt.CONTEXT_PC >= (ULONG_PTR)o->pbTarget &&
                cxt.CONTEXT_PC < ((ULONG_PTR)o->pbTarget + o->pTrampoline->cbRestore))
            {
                cxt.CONTEXT_PC = (ULONG_PTR)o->pTrampoline +
                    detour_align_from_target(o->pTrampoline, (BYTE)(cxt.CONTEXT_PC - (ULONG_PTR)o->pbTarget));
                bUpdateContext = TRUE;
            }
        }
        if (bUpdateContext)
        {
            Status = NtSetContextThread(ThreadHandle, &cxt);
            break;
        }
    }

    return Status;
}
```

恢复挂起的线程和释放句柄：
```C
VOID
detour_thread_resume(
    _In_reads_(SuspendedHandleCount) _Frees_ptr_ PHANDLE SuspendedHandles,
    _In_ ULONG SuspendedHandleCount)
{
    ULONG i;

    for (i = 0; i < SuspendedHandleCount; i++)
    {
        NtResumeThread(SuspendedHandles[i], NULL);
        NtClose(SuspendedHandles[i]);
    }
    detour_memory_free(SuspendedHandles);
}
```

要点：
1. 调用`NtQuerySystemInformation`以获取当前进程所有线程
2. 调用`NtSuspendThread`挂起除当前线程外的所有线程
3. 修改指令实现内联挂钩
4. 更新被成功挂起的线程
5. 调用`NtResumeThread`恢复挂起的线程

完整实现参考[KNSoft.SlimDetours/Source/SlimDetours/Thread.c于main · KNSoft/KNSoft.SlimDetours](../../../Source/SlimDetours/Thread.c)。

<br>
<hr>

本作品采用 [知识共享署名-非商业性使用-相同方式共享 4.0 国际许可协议 (CC BY-NC-SA 4.0)](http://creativecommons.org/licenses/by-nc-sa/4.0/) 进行许可。  
<br>
**[Ratin](https://github.com/RatinCN) &lt;[<ratin@knsoft.org>](mailto:ratin@knsoft.org)&gt;**  
*中国国家认证系统架构设计师*  
*[ReactOS](https://github.com/reactos/reactos)贡献者*
