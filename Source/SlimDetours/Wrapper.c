/*
 * KNSoft.SlimDetours (https://github.com/KNSoft/KNSoft.SlimDetours) Wrapper API
 * Copyright (c) KNSoft.org (https://github.com/KNSoft). All rights reserved.
 * Licensed under the MIT license.
 */

#include "SlimDetours.inl"

HRESULT
NTAPI
SlimDetoursEnableHook(
    _In_ BOOL Enable,
    _Inout_ PVOID* ppPointer,
    _In_ PVOID pDetour)
{
    HRESULT hr;

    hr = SlimDetoursTransactionBegin();
    if (FAILED(hr))
    {
        return hr;
    }
    hr = Enable ? SlimDetoursAttach(ppPointer, pDetour) : SlimDetoursDetach(ppPointer, pDetour);
    if (FAILED(hr))
    {
        SlimDetoursTransactionAbort();
        return hr;
    }
    return SlimDetoursTransactionCommit();
}

HRESULT
NTAPI
SlimDetoursSetHook(
    _Inout_ PVOID* ppPointer,
    _In_ PVOID pDetour)
{
    return SlimDetoursEnableHook(TRUE, ppPointer, pDetour);
}

HRESULT
NTAPI
SlimDetoursUnsetHook(
    _Inout_ PVOID* ppPointer,
    _In_ PVOID pDetour)
{
    return SlimDetoursEnableHook(FALSE, ppPointer, pDetour);
}

HRESULT
NTAPI
SlimDetoursEnableHooksV(
    _In_ BOOL Enable,
    _In_ ULONG Count,
    _In_ va_list ArgPtr)
{
    HRESULT hr;
    ULONG i;
    PVOID* ppPointer;
    PVOID pDetour;

    hr = SlimDetoursTransactionBegin();
    if (FAILED(hr))
    {
        return hr;
    }
    for (i = 0; i < Count; i++)
    {
        ppPointer = va_arg(ArgPtr, PVOID*);
        pDetour = va_arg(ArgPtr, PVOID);
        hr = Enable ? SlimDetoursAttach(ppPointer, pDetour) : SlimDetoursDetach(ppPointer, pDetour);
        if (FAILED(hr))
        {
            SlimDetoursTransactionAbort();
            return hr;
        }
    }
    return SlimDetoursTransactionCommit();
}

HRESULT
WINAPIV
SlimDetoursEnableHooks(
    _In_ BOOL Enable,
    _In_ ULONG Count,
    ...)
{
    HRESULT hr;
    va_list ap;

    va_start(ap, Count);
    hr = SlimDetoursEnableHooksV(Enable, Count, ap);
    va_end(ap);
    return hr;
}

HRESULT
WINAPIV
SlimDetoursSetHooks(
    _In_ ULONG Count,
    ...)
{
    HRESULT hr;
    va_list ap;

    va_start(ap, Count);
    hr = SlimDetoursEnableHooksV(TRUE, Count, ap);
    va_end(ap);
    return hr;
}

HRESULT
WINAPIV
SlimDetoursUnsetHooks(
    _In_ ULONG Count,
    ...)
{
    HRESULT hr;
    va_list ap;

    va_start(ap, Count);
    hr = SlimDetoursEnableHooksV(FALSE, Count, ap);
    va_end(ap);
    return hr;
}
