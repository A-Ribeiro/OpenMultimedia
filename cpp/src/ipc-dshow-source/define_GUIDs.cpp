#define WIN32_LEAN_AND_MEAN
#include <windows.h>
//#include <dshow.h>


#include <initguid.h>

// {8EC85528-072A-4690-9494-2928DC7447EB}
DEFINE_GUID( CLSID_ARIBEIRO_VIDEO_OUTPUT_01,
    0x8ec85528, 0x72a, 0x4690, 0x94, 0x94, 0x29, 0x28, 0xdc, 0x74, 0x47, 0xeb);

/*
#define STDAPI_DLL                  EXTERN_C __declspec(dllexport) HRESULT STDAPICALLTYPE

HRESULT register_filter();
HRESULT unregister_filter();

//LPCTSTR
STDAPI_DLL DllInstall(BOOL bInstall, _In_opt_ LPCWSTR pszCmdLine)
{
    if (!bInstall)
        return unregister_filter();
    else
        return register_filter();
}

STDAPI_DLL DllRegisterServer(void)
{
    return register_filter();
}


STDAPI_DLL DllUnregisterServer(void)
{
    return unregister_filter();
}
*/