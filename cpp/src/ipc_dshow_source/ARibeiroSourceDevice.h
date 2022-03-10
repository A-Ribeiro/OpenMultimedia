#ifndef _aribeiro_source_device_h__
#define _aribeiro_source_device_h__

#include <aRibeiroCore/aRibeiroCore.h>
#include <aRibeiroPlatform/aRibeiroPlatform.h>
#include <aRibeiroData/aRibeiroData.h>
using namespace aRibeiro;

//#include <dshow.h>
#include <streams.h>

struct DShowSourceResolution
{
	DShowSourceResolution(int width_, int height_, REFERENCE_TIME time_per_frame_) {
		width = width_;
		height = height_;
		time_per_frame = time_per_frame_;
	}
	int width;
	int height;
	REFERENCE_TIME time_per_frame;
};


static void Double2Fract_inverse(double f, uint32_t *numerator, uint32_t *denominator) {
    uint32_t lUpperPart = 1;
    uint32_t lLowerPart = 1;

    double df = (double)lUpperPart / (double)lLowerPart;

    while (fabs(df - f) > 2e-8) {
        if (df < f) {
            lUpperPart = lUpperPart + 1;
        }
        else {
            lLowerPart = lLowerPart + 1;
            lUpperPart = (uint32_t)(f * (double)lLowerPart);
        }
        df = (double)lUpperPart / (double)lLowerPart;
    }

    //inverse, because the dshow supply FPS
    *numerator = lLowerPart;
    *denominator = lUpperPart;
}

class ARibeiroOutputStream;
//class DShowSource;

class ARibeiroSourceDevice : public CSource
{
public:
	DECLARE_IUNKNOWN;
	
	STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void **ppv);
	IFilterGraph *GetGraph() { return m_pGraph; }
	FILTER_STATE GetState(){ return m_State; }
	ARibeiroSourceDevice(
		//DShowSource *sourceConfig,
        //const char *name_str,
		const CFactoryTemplate *base_template,
		LPUNKNOWN lpunk, HRESULT *phr);

	virtual ~ARibeiroSourceDevice();


    ARibeiroOutputStream *getStream(int id);


    const CFactoryTemplate *base_template;

    DebugConsoleIPC *debug;
    PlatformQueueIPC *input_queue;

    //DShowSource *sourceConfig;

};

class ARibeiroOutputStream : public CSourceStream, public IAMStreamConfig, public IKsPropertySet
{
public:

	//  IUnknown
	STDMETHODIMP QueryInterface(REFIID riid, void **ppv){
        if (riid == _uuidof(IAMStreamConfig))
            *ppv = (IAMStreamConfig*)this;
        else if (riid == _uuidof(IKsPropertySet))
            *ppv = (IKsPropertySet*)this;
        else
            return CSourceStream::QueryInterface(riid, ppv);

        AddRef();
        return S_OK;
    }
	STDMETHODIMP_(ULONG) AddRef() { return GetOwner()->AddRef(); }
	STDMETHODIMP_(ULONG) Release() { return GetOwner()->Release(); }

	//  IQualityControl
	STDMETHODIMP Notify(IBaseFilter * pSender, Quality q){
        return E_NOTIMPL;
    }

	//  IAMStreamConfig
	HRESULT STDMETHODCALLTYPE SetFormat(AM_MEDIA_TYPE *ptrMediaType);
	HRESULT STDMETHODCALLTYPE GetFormat(AM_MEDIA_TYPE **outMediaType);
	HRESULT STDMETHODCALLTYPE GetNumberOfCapabilities(int *piCount, int *piSize);
	HRESULT STDMETHODCALLTYPE GetStreamCaps(int iIndex, AM_MEDIA_TYPE **outMediaType, BYTE *ptrStreamConfigCaps);

	//  IKsPropertySet
	HRESULT STDMETHODCALLTYPE Set(REFGUID guidPropSet, DWORD dwID, 
		void *pInstanceData, DWORD cbInstanceData, void *pPropData, DWORD cbPropData){
		return E_NOTIMPL;
	}

	HRESULT STDMETHODCALLTYPE Get(REFGUID guidPropSet, DWORD dwPropID, 
		void *pInstanceData, DWORD cbInstanceData, void *pPropData, 
		DWORD cbPropData, DWORD *pcbReturned){
		if (guidPropSet != AMPROPSETID_Pin)             return E_PROP_SET_UNSUPPORTED;
		if (dwPropID != AMPROPERTY_PIN_CATEGORY)        return E_PROP_ID_UNSUPPORTED;
		if (pPropData == NULL && pcbReturned == NULL)   return E_POINTER;

		if (pcbReturned) *pcbReturned = sizeof(GUID);
		if (pPropData == NULL)          return S_OK; 
		if (cbPropData < sizeof(GUID))  return E_UNEXPECTED;

		*(GUID *)pPropData = PIN_CATEGORY_CAPTURE;
		return S_OK;
	}

	HRESULT STDMETHODCALLTYPE QuerySupported(REFGUID guidPropSet, 
		DWORD dwPropID, DWORD *pTypeSupport){
		if (guidPropSet != AMPROPSETID_Pin) return E_PROP_SET_UNSUPPORTED;
		if (dwPropID != AMPROPERTY_PIN_CATEGORY) return E_PROP_ID_UNSUPPORTED;
		if (pTypeSupport) *pTypeSupport = KSPROPERTY_SUPPORT_GET;
		return S_OK;
	}

	//  CSourceStream
	ARibeiroOutputStream(HRESULT *phr, ARibeiroSourceDevice *pParent, LPCWSTR pPinName);
	~ARibeiroOutputStream();

	HRESULT FillBuffer(IMediaSample *pms);
	HRESULT DecideBufferSize(IMemAllocator *ptrAllocator, ALLOCATOR_PROPERTIES *ptrProperties);
	HRESULT CheckMediaType(const CMediaType *ptrMediaType);
	HRESULT GetMediaType(int iPosition,CMediaType *ptrMediaType);
	HRESULT SetMediaType(const CMediaType *ptrMediaType);
	HRESULT OnThreadCreate(){
		first = true;
		return NOERROR;
	}
	HRESULT OnThreadDestroy(){
		return NOERROR;
	}
	
private:

    std::vector<DShowSourceResolution> format_list;

    void CheckCreatedFormatList();


	// ActiveMovie units (100 nS)
	void sleepToSync(REFERENCE_TIME *current, const REFERENCE_TIME &target);


	REFERENCE_TIME next_time;
	REFERENCE_TIME current_time;

	PlatformTime timer;
	bool first;

    PlatformLowLatencyQueueIPC *data_queue;
    ObjectBuffer data_buffer;

	PlatformProcess* process;

};



#endif