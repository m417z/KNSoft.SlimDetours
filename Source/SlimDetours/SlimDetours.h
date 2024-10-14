/*
 * KNSoft.SlimDetours (https://github.com/KNSoft/KNSoft.SlimDetours)
 * Copyright (c) KNSoft.org (https://github.com/KNSoft). All rights reserved.
 * Licensed under the MIT license.
 *
 * Source base on Microsoft Detours:
 *
 * Microsoft Research Detours Package, Version 4.0.1
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT license.
 */

#pragma once

#include <Windows.h>

#if !defined(_X86_) && !defined(_AMD64_) && !defined(_ARM64_)
#error Unsupported architecture (x86, amd64, arm64)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Instruction Target Macros */

#define DETOUR_INSTRUCTION_TARGET_NONE ((PVOID)0)
#define DETOUR_INSTRUCTION_TARGET_DYNAMIC ((PVOID)(LONG_PTR)-1)

/* API from Detours */

HRESULT
NTAPI
SlimDetoursTransactionBegin(VOID);

HRESULT
NTAPI
SlimDetoursTransactionAbort(VOID);

HRESULT
NTAPI
SlimDetoursTransactionCommit(VOID);

HRESULT
NTAPI
SlimDetoursAttach(
    _Inout_ PVOID* ppPointer,
    _In_ PVOID pDetour);

HRESULT
NTAPI
SlimDetoursDetach(
    _Inout_ PVOID* ppPointer,
    _In_ PVOID pDetour);

PVOID
NTAPI
SlimDetoursCodeFromPointer(
    _In_ PVOID pPointer);

PVOID
NTAPI
SlimDetoursCopyInstruction(
    _In_opt_ PVOID pDst,
    _In_ PVOID pSrc,
    _Out_opt_ PVOID* ppTarget,
    _Out_opt_ LONG* plExtra);

/* Wrapper API by SlimDetours */

HRESULT
NTAPI
SlimDetoursEnableHook(
    _In_ BOOL Enable,
    _Inout_ PVOID* ppPointer,
    _In_ PVOID pDetour);

HRESULT
NTAPI
SlimDetoursSetHook(
    _Inout_ PVOID* ppPointer,
    _In_ PVOID pDetour);

HRESULT
NTAPI
SlimDetoursUnsetHook(
    _Inout_ PVOID* ppPointer,
    _In_ PVOID pDetour);

HRESULT
NTAPI
SlimDetoursEnableHooksV(
    _In_ BOOL Enable,
    _In_ ULONG Count,
    _In_ va_list ArgPtr);

HRESULT
WINAPIV
SlimDetoursEnableHooks(
    _In_ BOOL Enable,
    _In_ ULONG Count,
    ...);

HRESULT
WINAPIV
SlimDetoursSetHooks(
    _In_ ULONG Count,
    ...);

HRESULT
WINAPIV
SlimDetoursUnsetHooks(
    _In_ ULONG Count,
    ...);

#if (NTDDI_VERSION >= NTDDI_WIN6)

typedef
VOID(CALLBACK* DETOUR_DELAY_ATTACH_CALLBACK)(
    _In_ HRESULT Result,
    _In_ PVOID* ppPointer,
    _In_ PCWSTR DllName,
    _In_ PCSTR Function,
    _In_opt_ PVOID Context);

/// <summary>
/// Delay hook, set hooks automatically when target DLL loaded.
/// </summary>
/// <param name="ppPointer">Variable to receive the address of the trampoline when hook apply.</param>
/// <param name="pDetour">Pointer to the detour function.</param>
/// <param name="DllName">Name of target DLL.</param>
/// <param name="Function">Name of target function.</param>
/// <param name="Callback">Optional. Callback to receive delay hook notification.</param>
/// <param name="Context">Optional. A parameter to be passed to the callback function.</param>
/// <returns>
/// Returns HRESULT.
/// HRESULT_FROM_NT(STATUS_PENDING): Delay hook register successfully.
/// Other success status: Hook is succeeded right now, delay hook won't execute.
/// Otherwise, returns an error HRESULT from NTSTATUS.
/// </returns>
HRESULT
NTAPI
SlimDetoursDelayAttach(
    _In_ PVOID* ppPointer,
    _In_ PVOID pDetour,
    _In_ PCWSTR DllName,
    _In_ PCSTR Function,
    _In_opt_ __callback DETOUR_DELAY_ATTACH_CALLBACK Callback,
    _In_opt_ PVOID Context);

#endif /* (NTDDI_VERSION >= NTDDI_WIN6) */

#ifdef __cplusplus
}
#endif
