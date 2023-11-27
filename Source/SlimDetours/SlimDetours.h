/*
 * KNSoft SlimDetours (https://github.com/KNSoft/SlimDetours)
 * Copyright (c) KNSoft.org (https://github.com/KNSoft). All rights reserved.
 * Licensed under the MPL-2.0 license.
 *
 * Source base on Microsoft Detours:
 *
 * Microsoft Research Detours Package, Version 4.0.1
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT license.
 */

#pragma once

#if !defined(_M_IX86) && !defined(_M_X64) && !defined(_M_ARM64)
#error Unknown architecture (x86, amd64, arm64)
#endif

#include <Windows.h>

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

EXTERN_C_START

/* Instruction Target Macros */

#define DETOUR_INSTRUCTION_TARGET_NONE ((PVOID)0)
#define DETOUR_INSTRUCTION_TARGET_DYNAMIC ((PVOID)(LONG_PTR)-1)

typedef
VOID(CALLBACK* DETOUR_DELAY_ATTACH_CALLBACK)(
    _In_ NTSTATUS Status,
    _In_ PVOID* ppPointer,
    _In_ PCWSTR DllName,
    _In_ PCSTR Function,
    _In_opt_ PVOID Context);

#pragma region APIs

NTSTATUS NTAPI SlimDetoursTransactionBegin(VOID);

NTSTATUS NTAPI SlimDetoursTransactionAbort(VOID);

NTSTATUS NTAPI SlimDetoursTransactionCommit(VOID);

NTSTATUS NTAPI SlimDetoursAttach(
    _Inout_ PVOID* ppPointer,
    _In_ PVOID pDetour);

NTSTATUS NTAPI SlimDetoursDetach(
    _Inout_ PVOID* ppPointer,
    _In_ PVOID pDetour);

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
/// Returns NTSTATUS.
/// STATUS_PENDING: Delay hook register successfully.
/// Other success status: Hook is succeeded right now, delay hook won't execute.
/// Otherwise, returns an error status.
/// </returns>
NTSTATUS NTAPI SlimDetoursDelayAttach(
    _In_ PVOID* ppPointer,
    _In_ PVOID pDetour,
    _In_ PCWSTR DllName,
    _In_ PCSTR Function,
    _In_opt_ __callback DETOUR_DELAY_ATTACH_CALLBACK Callback,
    _In_opt_ PVOID Context);

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

EXTERN_C_END

#pragma endregion

#pragma region Type - safe overloads for C++

#if __cplusplus >= 201103L || _MSVC_LANG >= 201103L
#include <type_traits>

template<typename T>
struct SlimDetoursIsFunctionPointer : std::false_type
{
};

template<typename T>
struct SlimDetoursIsFunctionPointer<T*> : std::is_function<typename std::remove_pointer<T>::type>
{
};

template<typename T, typename std::enable_if<SlimDetoursIsFunctionPointer<T>::value, int>::type = 0>
NTSTATUS
SlimDetoursAttach(
    _Inout_ T* ppPointer,
    _In_ T pDetour) noexcept
{
    return SlimDetoursAttach(reinterpret_cast<void**>(ppPointer), reinterpret_cast<void*>(pDetour));
}

template<typename T, typename std::enable_if<SlimDetoursIsFunctionPointer<T>::value, int>::type = 0>
NTSTATUS
SlimDetoursDetach(
    _Inout_ T* ppPointer,
    _In_ T pDetour) noexcept
{
    return SlimDetoursDetach(reinterpret_cast<void**>(ppPointer), reinterpret_cast<void*>(pDetour));
}

template<typename T, typename std::enable_if<SlimDetoursIsFunctionPointer<T>::value, int>::type = 0>
NTSTATUS
SlimDetoursDelayAttach(
    _In_ T* ppPointer,
    _In_ T pDetour,
    _In_ PCWSTR DllName,
    _In_ PCSTR Function,
    _In_opt_ __callback DETOUR_DELAY_ATTACH_CALLBACK Callback,
    _In_opt_ PVOID Context)
{
    return SlimDetoursDelayAttach(reinterpret_cast<void**>(ppPointer),
                                  reinterpret_cast<void*>(pDetour),
                                  DllName,
                                  Function,
                                  Callback,
                                  Context);
}

#endif // __cplusplus >= 201103L || _MSVC_LANG >= 201103L

#pragma endregion
