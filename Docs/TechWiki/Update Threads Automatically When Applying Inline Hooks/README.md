| **English (en-US)** | [简体中文 (zh-CN)](./README.zh-CN.md) |
| --- | --- |

<br>

# Update Threads Automatically When Applying Inline Hooks

## The necessity of updating threads when applying inline hooks

Inline hook has to modify the instructions at the beginning of the function to implement the jump, in order to cope with the possibility that a thread is running on the instruction to be modified, it's necessary to update the thread in this state to avoid executing an illegal combination of old and new instructions.

## Implementations on other hooking library

### Detours

[Detours](https://github.com/microsoft/Detours) provides [`DetourUpdateThread`](https://github.com/microsoft/Detours/wiki/DetourUpdateThread) function to update threads, but it needs the caller pass the handle of the thread that needs to be updated:
```C
LONG WINAPI DetourUpdateThread(_In_ HANDLE hThread);
```
In other words, the caller needs to traverse all threads in the process except itself and pass them to this function, which is complicated and inconvenient to use.

But [Detours](https://github.com/microsoft/Detours) updates threads very precisely, it accurately adjusts the PC (Program Counter) in the thread context to the correct position by using [`GetThreadContext`](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getthreadcontext) and [`SetThreadContext`](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-setthreadcontext), see [Detours/src/detours.cpp at 4b8c659f · microsoft/Detours](https://github.com/microsoft/Detours/blob/4b8c659f549b0ab21cf649377c7a84eb708f5e68/src/detours.cpp#L1840) for implementation.

> [!TIP]
> While its official example "[Using Detours](https://github.com/microsoft/Detours/wiki/Using-Detours)" has code like `DetourUpdateThread(GetCurrentThread())`, such usage is pointless and invalid, and should be used to update all threads in the process except the current thread, see also: [`DetourUpdateThread`](https://github.com/microsoft/Detours/wiki/DetourUpdateThread). But even updating threads in the right way, it also brings a new risk, see [🔗 TechWiki: Avoid Deadlocking on The Heap When Updating Threads](https://github.com/KNSoft/KNSoft.SlimDetours/blob/main/Docs/TechWiki/Avoid%20Deadlocking%20on%20The%20Heap%20When%20Updating%20Threads/README.md).

### mhook

[mhook](https://github.com/martona/mhook) is a well-known Windows API hooking library like [Detours](https://github.com/microsoft/Detours), it updates threads automatically when set (or unset) hooks, the caller doesn't need to be concerned about this problem, see [mhook/mhook-lib/mhook.cpp at e58a58ca · martona/mhook](https://github.com/martona/mhook/blob/e58a58ca31dbe14f202b9b26315bff9f7a32598c/mhook-lib/mhook.cpp#L557) for implementation.

But the way it updates threads is a bit hacky compared to the [Detours](https://github.com/microsoft/Detours) mentioned above, wait 100ms if the thread is exactly in the area where the instruction is about to be modified, try up to 3 times:
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

## SlimDetours implementation

[SlimDetours](https://github.com/KNSoft/KNSoft.SlimDetours) takes all of the above advantages into account, traverse all threads of the process at hook (or unhook) time, and then update the thread context in the same way as [Detours](https://github.com/microsoft/Detours).

Suspend all threads in the current process except the current thread and return their handles:
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

Update threads' context PC (Program Counter) precisely:
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

Resume suspended threads and release handles:
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

Key points:
1. Call `NtQuerySystemInformation` to acquire all threads of the current process
2. Call `NtSuspendThread` to suspend all threads except the current thread
3. Modify the instruction to implement the inline hook
4. Update the threads that were successfully suspended
5. Call `NtResumeThread` to resume the suspended threads

See [KNSoft.SlimDetours/Source/SlimDetours/Thread.c at main · KNSoft/KNSoft.SlimDetours](../../../Source/SlimDetours/Thread.c) for the full implementation.

<br>
<hr>

This work is licensed under [Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0)](http://creativecommons.org/licenses/by-nc-sa/4.0/).  
<br>
**[Ratin](https://github.com/RatinCN) &lt;[<ratin@knsoft.org>](mailto:ratin@knsoft.org)&gt;**  
*China national certified senior system architect*  
*[ReactOS](https://github.com/reactos/reactos) contributor*
