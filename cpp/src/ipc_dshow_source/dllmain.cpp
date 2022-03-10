#include <aRibeiroCore/aRibeiroCore.h>
#include <aRibeiroPlatform/aRibeiroPlatform.h>
#include <aRibeiroData/aRibeiroData.h>
using namespace aRibeiro;

#include "ARibeiroSourceDevice.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
//#include <dshow.h>

#include <streams.h>
#include <initguid.h>

#include <comdef.h>

#ifdef _MSC_VER

#ifdef NDEBUG
#        pragma comment(lib, "strmbase")
#else
#        pragma comment(lib, "strmbasd")
#endif

#endif


extern CUnknown * WINAPI ARibeiroSourceDevice_CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);

//#include "virtual-cam.h"
//#include "virtual-audio.h"

STDAPI AMovieSetupRegisterServer(CLSID clsServer, LPCWSTR szDescription, 
	LPCWSTR szFileName, LPCWSTR szThreadingModel = L"Both", 
	LPCWSTR szServerType = L"InprocServer32");
STDAPI AMovieSetupUnregisterServer(CLSID clsServer);

// {8EC85528-072A-4690-9494-2928DC7447EB}
DEFINE_GUID( CLSID_ARIBEIRO_VIDEO_OUTPUT_01,
    0x8ec85528, 0x72a, 0x4690, 0x94, 0x94, 0x29, 0x28, 0xdc, 0x74, 0x47, 0xeb);

//
// constant variables
//

const AMOVIESETUP_MEDIATYPE setupMediaType = {
	&MEDIATYPE_Video,
	&MEDIASUBTYPE_YUY2
};

const AMOVIESETUP_PIN videoPin = {
	L"Output",            
	FALSE,                 
	TRUE,                  
	FALSE,                 
	FALSE,                 
	&CLSID_NULL,           
	NULL,                  
	1,                     
	&setupMediaType
};

const AMOVIESETUP_FILTER filter = {
	&CLSID_ARIBEIRO_VIDEO_OUTPUT_01,
	L"aRibeiro Video 01",
	MERIT_DO_NOT_USE,      
	1,                     
	&videoPin
};

const int num_filters_to_register = 1;

CFactoryTemplate g_Templates[num_filters_to_register] =
{
	{
		L"aRibeiro Cam 01",
		&CLSID_ARIBEIRO_VIDEO_OUTPUT_01,
        ARibeiroSourceDevice_CreateInstance,
		NULL,
		&filter
	}
};

int g_cTemplates = sizeof(g_Templates) / sizeof(g_Templates[0]);



#define ARIBEIRO_ASSERT(bool_exp, ...) \
    if (!(bool_exp)) {\
        console->printf("[%s:%i]\n", __FILE__, __LINE__);\
        console->printf(__VA_ARGS__);\
        delete console;\
        console = NULL;\
        ASSERT((bool_exp));\
    }

CUnknown * WINAPI ARibeiroSourceDevice_CreateInstance(LPUNKNOWN lpunk, HRESULT *phr) {

    DebugConsoleIPC *console = new DebugConsoleIPC();

    //ARIBEIRO_ASSERT(lpunk, "[ARibeiroSourceDevice_CreateInstance] lpunk NULL.\n");
    ARIBEIRO_ASSERT(phr, "[ARibeiroSourceDevice_CreateInstance] phr NULL.\n");
    delete console;
    console = NULL;

	//ASSERT(lpunk);
	//ASSERT(phr);
    /*
	IBaseFilter *m_pFilter;
	HRESULT hr;

	hr = lpunk->QueryInterface(IID_IBaseFilter, (LPVOID *) &m_pFilter);

	
    ARIBEIRO_ASSERT(SUCCEEDED(hr), "[ARibeiroSourceDevice_CreateInstance] Error lpunk->QueryInterface.\n");
    //ASSERT(SUCCEEDED(hr));

	CLSID clsID;
	hr = m_pFilter->GetClassID(&clsID);

    ARIBEIRO_ASSERT(SUCCEEDED(hr), "[ARibeiroSourceDevice_CreateInstance] Error m_pFilter->GetClassID.\n");


	CFactoryTemplate *selected_template = NULL;

	for(int i=0;i<num_filters_to_register;i++) {
		if (  IsEqualGUID(clsID, *(g_Templates[i].m_ClsID)) ) {
			selected_template = &g_Templates[i];
			//g_Templates[i].m_Name
		}
	}

    ARIBEIRO_ASSERT(selected_template != NULL, "[ARibeiroSourceDevice_CreateInstance] Could not found a template.\n");
	//ASSERT(selected_template != NULL);

	//DShowSource* sourceClass = dshow_map[clsID];
	//ASSERT(sourceClass);

	//DShowSource *sourceConfig, LPUNKNOWN lpunk, HRESULT *phr, int mode

    */

    const CFactoryTemplate *selected_template = &g_Templates[0];

	CUnknown *punk = new ARibeiroSourceDevice(selected_template, lpunk, phr);

	return punk;

}


HRESULT register_filter() {

    DebugConsoleIPC *console = new DebugConsoleIPC();

    console->printf("[register_filter] Begin.\n");

	HRESULT hr = NOERROR;

	//
	// Get current DLL filename
	//
	WCHAR achFileName[MAX_PATH];
	char achTemp[MAX_PATH];


    ARIBEIRO_ASSERT(g_hInst != 0, "[register_filter] Error get module handle instance.\n");
	//ASSERT(g_hInst != 0);

    console->printf("[register_filter] Retrieving module file name.\n");
    if (0 == GetModuleFileNameA(g_hInst, achTemp, sizeof(achTemp))) {
        console->printf("[register_filter] GetModuleFileNameA.\n");
        delete console;
        console = NULL;
        return AmHresultFromWin32(GetLastError());
    }

    console->printf("[register_filter] Converting char to wchar.\n");
	MultiByteToWideChar(CP_ACP, 0L, achTemp, lstrlenA(achTemp) + 1, achFileName, NUMELMS(achFileName));

    console->printf("[register_filter] Module Name: %s\n", aRibeiro::StringUtil::toString(achFileName).c_str() );


    console->printf("[register_filter] Initialize COM.\n");
	hr = CoInitialize(0);
    if (FAILED(hr)) {
        _com_error err(hr);
        LPCTSTR errMsg = err.ErrorMessage();
        console->printf("[register_filter]     FAILED, message: %s\n", errMsg);
        delete console;
        console = NULL;
        return hr;
    }
	
	for (int i = 0; i < num_filters_to_register; i++) {
        console->printf("[register_filter] AMovie Registering filter: %i\n", i);
		hr |= AMovieSetupRegisterServer(*(g_Templates[i].m_ClsID), g_Templates[i].m_Name, achFileName);
        

        console->printf("[register_filter]     Filter Name: %s\n", aRibeiro::StringUtil::toString(g_Templates[i].m_Name).c_str());
        if (SUCCEEDED(hr))
            console->printf("[register_filter]     OK\n");
        else {
            _com_error err(hr);
            LPCTSTR errMsg = err.ErrorMessage();
            console->printf("[register_filter]     FAILED, message: %s\n", errMsg);
        }


	}

	if (SUCCEEDED(hr)) {

        console->printf("[register_filter] Creating filter mapping\n");

		IFilterMapper2 *fm = 0;
		hr = CoCreateInstance(CLSID_FilterMapper2, 0, CLSCTX_INPROC_SERVER, IID_IFilterMapper2, (void **)&fm);

		if (SUCCEEDED(hr)) {

			REGFILTER2 rf2;

			rf2.dwVersion = 1;
			rf2.dwMerit = MERIT_DO_NOT_USE;
			rf2.cPins = 1;
			rf2.rgPins = &videoPin;
			
			for (int i = 0; i < num_filters_to_register; i++) {
                console->printf("[register_filter] FilterMapper2 Registering filter: %i\n", i);
				IMoniker *moniker_video = 0;
				hr |= fm->RegisterFilter(*(g_Templates[i].m_ClsID), g_Templates[i].m_Name, &moniker_video, &CLSID_VideoInputDeviceCategory, NULL, &rf2);
                if (SUCCEEDED(hr))
                    console->printf("[register_filter]     OK\n");
                else {
                    _com_error err(hr);
                    LPCTSTR errMsg = err.ErrorMessage();
                    console->printf("[register_filter]     FAILED, message: %s\n", errMsg);
                }
			}

        }
        else {
            console->printf("[register_filter] Filter FAILED!\n");
        }

		if (fm)
			fm->Release();
    }
    /*
    else {
        _com_error err(hr);
        LPCTSTR errMsg = err.ErrorMessage();
        console->printf("[register_filter]     FAILED, message: %s\n", errMsg);
    }
    */
	
    console->printf("[register_filter] free COM objects!\n");

	CoFreeUnusedLibraries();
	CoUninitialize();

    console->printf("[register_filter] All done.\n");

    delete console;
    console = NULL;
	return hr;
}

HRESULT unregister_filter() {
    DebugConsoleIPC *console = new DebugConsoleIPC();

    console->printf("[unregister_filter] Begin.\n");

	HRESULT hr = NOERROR;

    console->printf("[unregister_filter] Initialize COM.\n");

	hr = CoInitialize(0);
    if (FAILED(hr)) {
        console->printf("[unregister_filter] FAILED(hr).\n");
        delete console;
        console = NULL;
        return hr;
    }
	
    console->printf("[unregister_filter] Creating filter mapping\n");

	IFilterMapper2 *fm = 0;
	hr = CoCreateInstance(CLSID_FilterMapper2, 0, CLSCTX_INPROC_SERVER, IID_IFilterMapper2, (void **)&fm);

	if (SUCCEEDED(hr)) {
		for (int i = 0; i < num_filters_to_register; i++) {
            console->printf("[unregister_filter] FilterMapper2 UnRegistering filter: %i\n", i);
			hr |= fm->UnregisterFilter(&CLSID_VideoInputDeviceCategory, 0, *(g_Templates[i].m_ClsID));
            if (SUCCEEDED(hr))
                console->printf("[unregister_filter]     OK\n");
            else {
                _com_error err(hr);
                LPCTSTR errMsg = err.ErrorMessage();
                console->printf("[register_filter]     FAILED, message: %s\n", errMsg);
            }
		}
    }
    else {
        console->printf("[unregister_filter] Filter FAILED!\n");
    }

	if (fm)
		fm->Release();
	
	if (SUCCEEDED(hr)) {
		for (int i = 0; i < num_filters_to_register; i++) {
            console->printf("[unregister_filter] AMovie UnRegistering filter: %i\n", i);
			hr |= AMovieSetupUnregisterServer(*(g_Templates[i].m_ClsID));
            console->printf("[unregister_filter]     Filter Name: %s\n", aRibeiro::StringUtil::toString(g_Templates[i].m_Name).c_str());
            if (SUCCEEDED(hr))
                console->printf("[unregister_filter]     OK\n");
            else {
                _com_error err(hr);
                LPCTSTR errMsg = err.ErrorMessage();
                console->printf("[register_filter]     FAILED, message: %s\n", errMsg);
            }
		}
	}

    console->printf("[unregister_filter] free COM objects!\n");

	CoFreeUnusedLibraries();
	CoUninitialize();

    console->printf("[unregister_filter] All done.\n");

    delete console;
    console = NULL;
	return hr;
}

#define STDAPI_DLL                  EXTERN_C __declspec(dllexport) HRESULT STDAPICALLTYPE

//LPCTSTR

STDAPI DllInstall(BOOL bInstall, _In_opt_ LPCWSTR pszCmdLine)
{
	if (!bInstall)
		return unregister_filter();
	else
		return register_filter();
}

STDAPI DllRegisterServer(void)
{
	return register_filter();
}

STDAPI DllUnregisterServer(void)
{
	return unregister_filter();
}


extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

HINSTANCE hDllModuleDll = NULL;
BOOL APIENTRY DllMain(HANDLE hModule, DWORD  dwReason, LPVOID lpReserved)
{
    hDllModuleDll = (HINSTANCE)hModule;
	return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}
