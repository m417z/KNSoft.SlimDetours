/*
 * KNSoft.SlimDetours (https://github.com/KNSoft/KNSoft.SlimDetours) Function Table Hook Implementation
 * 
 * Hook function address in a read-only table, used by COM/IAT/EAT/... hooking.
 * 
 * Copyright (c) KNSoft.org (https://github.com/KNSoft). All rights reserved.
 * Licensed under the MIT license.
 */

#include "SlimDetours.inl"

/* Function table hook core implementation */

static
NTSTATUS
detour_hook_table_func(
    _In_ PVOID* pFuncTable,
    _In_ ULONG ulOffset,
    _Out_opt_ PVOID* ppOldFunc,
    _In_ PVOID pNewFunc)
{
    NTSTATUS Status;
    PVOID Base, *Method;
    SIZE_T Size;
    ULONG OldProtect;

    Base = Method = Add2Ptr(pFuncTable, ulOffset);
    Size = sizeof(PVOID);
    Status = NtProtectVirtualMemory(NtCurrentProcess(), &Base, &Size, PAGE_READWRITE, &OldProtect);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }
    if (ppOldFunc != NULL)
    {
        *ppOldFunc = *Method;
    }
    *Method = pNewFunc;
    NtProtectVirtualMemory(NtCurrentProcess(), &Base, &Size, OldProtect, &OldProtect);

    return STATUS_SUCCESS;
}

static
NTSTATUS
detour_hook_table_funcs(
    _In_ BOOL bEnable,
    _In_ PVOID* pFuncTable,
    _In_ ULONG ulCount,
    _Inout_updates_(ulCount) PDETOUR_FUNC_TABLE_HOOK pHooks)
{
    NTSTATUS Status;
    PVOID Base, *Method;
    SIZE_T Size;
    ULONG i, OldProtect, MaxOffset;

    MaxOffset = 0;
    for (i = 0; i < ulCount; i++)
    {
        if (pHooks[i].ulOffset > MaxOffset)
        {
            MaxOffset = pHooks[i].ulOffset;
        }
    }
    Base = pFuncTable;
    Size = MaxOffset + sizeof(PVOID);
    Status = NtProtectVirtualMemory(NtCurrentProcess(), &Base, &Size, PAGE_READWRITE, &OldProtect);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    for (i = 0; i < ulCount; i++)
    {
        Method = Add2Ptr(pFuncTable, pHooks[i].ulOffset);
        if (bEnable)
        {
            *pHooks[i].ppOriginal = *Method;
            *Method = pHooks[i].pDetour;
        } else
        {
            *Method = *pHooks[i].ppOriginal;
        }
    }

    NtProtectVirtualMemory(NtCurrentProcess(), &Base, &Size, OldProtect, &OldProtect);
    return STATUS_SUCCESS;
}

HRESULT
NTAPI
SlimDetoursSetTableHook(
    _In_ PVOID* pFuncTable,
    _In_ ULONG ulOffset,
    _Out_ PVOID* ppOriginal,
    _In_ PVOID pDetour)
{
    return HRESULT_FROM_NT(detour_hook_table_func(pFuncTable, ulOffset, ppOriginal, pDetour));
}

HRESULT
NTAPI
SlimDetoursUnsetTableHook(
    _In_ PVOID* pFuncTable,
    _In_ ULONG ulOffset,
    _In_ PVOID pOriginal)
{
    return HRESULT_FROM_NT(detour_hook_table_func(pFuncTable, ulOffset, NULL, pOriginal));
}

HRESULT
NTAPI
SlimDetoursEnableTableHooks(
    _In_ BOOL bEnable,
    _In_ PVOID* pFuncTable,
    _In_ ULONG ulCount,
    _Inout_updates_(ulCount) PDETOUR_FUNC_TABLE_HOOK pHooks)
{
    return HRESULT_FROM_NT(detour_hook_table_funcs(bEnable, pFuncTable, ulCount, pHooks));
}
