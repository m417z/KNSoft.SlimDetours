/*
 * This demo shows delay hook, user32.dll!EqualRect will be hooked automatically when user32.dll loaded.
 * 
 * Run "Demo.exe -Run DelayHook".
 * 
 * See also https://github.com/KNSoft/KNSoft.SlimDetours/tree/main/Docs/TechWiki/Implement%20Delay%20Hook
 */

#include "Demo.h"

static BOOL g_bDelayAttach = FALSE;

static
VOID
CALLBACK
DelayAttachCallback(
    _In_ HRESULT Result,
    _In_ PVOID* ppPointer,
    _In_ PCWSTR DllName,
    _In_ PCSTR Function,
    _In_opt_ PVOID Context)
{
    UnitTest_FormatMessage("Delay hook callback: Result = 0x%08lX, DllName = %ls, Function = %hs, Context = 0x%p\n",
                           Result,
                           DllName,
                           Function,
                           Context);
    g_bDelayAttach = SUCCEEDED(Result) &&
        _wcsicmp(DllName, g_usUser32.Buffer) == 0 &&
        _stricmp(Function, g_asEqualRect.Buffer) == 0;
}

TEST_FUNC(DelayHook)
{
    NTSTATUS Status;
    HRESULT hr;
    PVOID hUser32;
    FN_EqualRect* pfnEqualRect;
    RECT rc1 = { 0 }, rc2 = { 0 };

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

    /* Load user32.dll now */
    Status = LdrLoadDll(NULL, NULL, &g_usUser32, &hUser32);
    if (!NT_SUCCESS(Status))
    {
        TEST_SKIP("LdrLoadDll failed with 0x%08lX\n", Status);
        return;
    }

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
    
    LdrUnloadDll(hUser32);
}
