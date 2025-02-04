/*
 * This demo shows COM hook, hooks IOpenControlPanel::GetPath method.
 *
 * Run "Demo.exe -Run COMHook".
 */

#include "Demo.h"

#include <Shobjidl.h>

typedef
HRESULT
STDMETHODCALLTYPE
FN_IOpenControlPanel_GetPath(
    __RPC__in IOpenControlPanel* This,
    /* [string][unique][in] */ __RPC__in_opt_string LPCWSTR pszName,
    /* [size_is][string][out] */ __RPC__out_ecount_full_string(cchPath) LPWSTR pszPath,
    /* [in] */ UINT cchPath);

static FN_IOpenControlPanel_GetPath* g_pfnIOpenControlPanel_GetPath = NULL;
static IOpenControlPanel* g_pThis = NULL;
static LPWSTR g_pszPath = NULL;
static UINT g_cchPath = 0;
static HRESULT g_hr = S_OK;

static
HRESULT
STDMETHODCALLTYPE
Hooked_IOpenControlPanel_GetPath(
    __RPC__in IOpenControlPanel* This,
    /* [string][unique][in] */ __RPC__in_opt_string LPCWSTR pszName,
    /* [size_is][string][out] */ __RPC__out_ecount_full_string(cchPath) LPWSTR pszPath,
    /* [in] */ UINT cchPath)
{
    g_pThis = This;
    g_pszPath = pszPath;
    g_cchPath = cchPath;
    g_hr = g_pfnIOpenControlPanel_GetPath(This, pszName, pszPath, cchPath);
    return g_hr;
}

static
LOGICAL
Test_IOpenControlPanel_GetPath(
    _In_ IOpenControlPanel* pocp)
{
    HRESULT hr;
    WCHAR szPath[MAX_PATH];

    hr = pocp->lpVtbl->GetPath(pocp, NULL, szPath, ARRAYSIZE(szPath));
    return (g_pThis == pocp &&
            g_pszPath == szPath &&
            g_cchPath == ARRAYSIZE(szPath) &&
            g_hr == hr);
}

TEST_FUNC(COMHook)
{
    HRESULT hr;
    IOpenControlPanel* pocp1;
    IOpenControlPanel* pocp2;

    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr))
    {
        TEST_SKIP("CoInitializeEx failed with: 0x%08lX\n", hr);
        return;
    }

    hr = SlimDetoursCOMHook(&CLSID_OpenControlPanel,
                            &IID_IOpenControlPanel,
                            FIELD_OFFSET(IOpenControlPanelVtbl, GetPath),
                            (PVOID*)&g_pfnIOpenControlPanel_GetPath,
                            Hooked_IOpenControlPanel_GetPath);
    if (FAILED(hr))
    {
        TEST_FAIL("SlimDetoursCOMHook failed with: 0x%08lX\n", hr);
        goto _Exit_0;
    }

    hr = CoCreateInstance(&CLSID_OpenControlPanel, NULL, CLSCTX_INPROC_SERVER, &IID_IOpenControlPanel, &pocp1);
    if (FAILED(hr))
    {
        TEST_SKIP("CoCreateInstance failed with: 0x%08lX\n", hr);
        goto _Exit_0;
    }
    TEST_OK(Test_IOpenControlPanel_GetPath(pocp1));

    hr = CoCreateInstance(&CLSID_OpenControlPanel, NULL, CLSCTX_INPROC_SERVER, &IID_IOpenControlPanel, &pocp2);
    if (FAILED(hr))
    {
        TEST_SKIP("CoCreateInstance failed with: 0x%08lX\n", hr);
        goto _Exit_1;
    }
    TEST_OK(Test_IOpenControlPanel_GetPath(pocp2));

    hr = SlimDetoursCOMHook(&CLSID_OpenControlPanel,
                            &IID_IOpenControlPanel,
                            FIELD_OFFSET(IOpenControlPanelVtbl, GetPath),
                            NULL,
                            g_pfnIOpenControlPanel_GetPath);
    if (FAILED(hr))
    {
        TEST_FAIL("SlimDetoursCOMHook failed with: 0x%08lX\n", hr);
        goto _Exit_0;
    }

    pocp2->lpVtbl->Release(pocp2);
_Exit_1:
    pocp1->lpVtbl->Release(pocp1);
_Exit_0:
    CoUninitialize();
}
