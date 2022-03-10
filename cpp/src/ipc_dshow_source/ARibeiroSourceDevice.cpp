#include "ARibeiroSourceDevice.h"

#include "resource.h"

extern HINSTANCE hDllModuleDll;

#define ARIBEIRO_ASSERT(cls_obj, bool_exp, ...) \
    if (!(bool_exp)) {\
        cls_obj->printf("[%s:%i]\n", __FILE__, __LINE__);\
        cls_obj->printf(__VA_ARGS__);\
        delete cls_obj;\
        cls_obj = NULL;\
        ASSERT((bool_exp));\
    }


ARibeiroSourceDevice::ARibeiroSourceDevice(
    //const char *name_str, 
    const CFactoryTemplate *base_template, LPUNKNOWN lpunk, HRESULT *phr) :
CSource(
    aRibeiro::StringUtil::toString(base_template->m_Name).c_str(),//base_template->m_Name,
    lpunk, *base_template->m_ClsID)
{
    input_queue = NULL;
    debug = NULL;

    debug = new DebugConsoleIPC();

    ARIBEIRO_ASSERT(debug, phr, "[ARibeiroSourceDevice_CreateInstance] phr NULL.\n");
	//ASSERT(phr);

    debug->printf("[ARibeiroSourceDevice] Creating Source From Template: %s.\n", aRibeiro::StringUtil::toString(base_template->m_Name).c_str());
    debug->printf("[ARibeiroSourceDevice] creating outputstream\n");

    this->base_template = base_template;

    CAutoLock cAutoLock(&m_cStateLock);
	
	m_paStreams = (CSourceStream **) new ARibeiroOutputStream*[1];
	m_paStreams[0] = new ARibeiroOutputStream(phr, this, L"Video");

    debug->printf("[ARibeiroSourceDevice] initialization done\n");

    //sourceConfig = _sourceConfig;
}

ARibeiroSourceDevice::~ARibeiroSourceDevice() {
	CAutoLock cAutoLock(&m_cStateLock);

    if (debug != NULL) {
        debug->printf("[ARibeiroSourceDevice] destroying Object!!!\n");
    }

    if (input_queue != NULL) {
        delete input_queue;
        input_queue = NULL;
    }

    if (m_paStreams != NULL) {

        if (m_paStreams[0] != NULL) {
            delete m_paStreams[0];
        }
        delete[] m_paStreams;

        m_paStreams = NULL;
    }

    if (debug != NULL) {
        delete debug;
        debug = NULL;
    }
}

HRESULT ARibeiroSourceDevice::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
	if (riid == _uuidof(IAMStreamConfig) || riid == _uuidof(IKsPropertySet))
		return m_paStreams[0]->QueryInterface(riid, ppv);
	else
		return CSource::NonDelegatingQueryInterface(riid, ppv);
}


ARibeiroOutputStream *ARibeiroSourceDevice::getStream(int id) {
    debug->printf("[ARibeiroSourceDevice] getStream(%i)\n", id);

    return (ARibeiroOutputStream *)m_paStreams[0];
}



HRESULT STDMETHODCALLTYPE ARibeiroOutputStream::SetFormat(AM_MEDIA_TYPE *ptrMediaType)
{
	if (ptrMediaType == nullptr)
		return E_FAIL;


    ARibeiroSourceDevice *parent = (ARibeiroSourceDevice *)m_pFilter;

	if (parent->GetState() != State_Stopped)
		return E_FAIL;

	if (CheckMediaType((CMediaType *)ptrMediaType) != S_OK)
		return E_FAIL;

	//VIDEOINFOHEADER *videoInfoHeader = (VIDEOINFOHEADER *)(ptrMediaType->pbFormat);

	m_mt.SetFormat(m_mt.Format(), sizeof(VIDEOINFOHEADER));

    CheckCreatedFormatList();

    /*
	format_list.insert(
        format_list.begin(),
        DShowSourceResolution(
            videoInfoHeader->bmiHeader.biWidth, 
		    videoInfoHeader->bmiHeader.biHeight, 
            videoInfoHeader->AvgTimePerFrame
        )
    );
    */

	IPin* pin;
	ConnectedTo(&pin);
	if (pin){
		IFilterGraph *pGraph = parent->GetGraph();
		pGraph->Reconnect(this);
	}
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ARibeiroOutputStream::GetFormat(AM_MEDIA_TYPE **outMediaType)
{
	*outMediaType = CreateMediaType(&m_mt);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ARibeiroOutputStream::GetNumberOfCapabilities(int *piCount, 
	int *piSize)
{
	CheckCreatedFormatList();
	
	*piCount = (int)format_list.size();
	*piSize = sizeof(VIDEO_STREAM_CONFIG_CAPS);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE ARibeiroOutputStream::GetStreamCaps(int iIndex, 
	AM_MEDIA_TYPE **outMediaType, BYTE *ptrStreamConfigCaps)
{
    CheckCreatedFormatList();

	if (iIndex < 0 || iIndex > format_list.size() - 1)
		return E_INVALIDARG;

    ARibeiroSourceDevice *parent = (ARibeiroSourceDevice *)m_pFilter;

	*outMediaType = CreateMediaType(&m_mt);
    VIDEOINFOHEADER* videoInfoHeader = (VIDEOINFOHEADER*)((*outMediaType)->pbFormat);


	videoInfoHeader->bmiHeader.biWidth = format_list[iIndex].width;
	videoInfoHeader->bmiHeader.biHeight = format_list[iIndex].height;
	videoInfoHeader->AvgTimePerFrame = format_list[iIndex].time_per_frame;
	//pvi->AvgTimePerFrame = 333333;
    //if (parent->sourceConfig->type == DShowSourceFormat_Yuy2) {
	    videoInfoHeader->bmiHeader.biCompression = MAKEFOURCC('Y', 'U', 'Y', '2');
	    videoInfoHeader->bmiHeader.biBitCount = 16;
    //}

	videoInfoHeader->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	videoInfoHeader->bmiHeader.biPlanes = 1;
	//if (parent->sourceConfig->type == DShowSourceFormat_Yuy2)
		videoInfoHeader->bmiHeader.biSizeImage = videoInfoHeader->bmiHeader.biWidth * videoInfoHeader->bmiHeader.biHeight * 2;
	videoInfoHeader->bmiHeader.biClrImportant = 0;

	SetRectEmpty(&(videoInfoHeader->rcSource)); 
	SetRectEmpty(&(videoInfoHeader->rcTarget)); 

	(*outMediaType)->majortype = MEDIATYPE_Video;
    //if (parent->sourceConfig->type == DShowSourceFormat_Yuy2)
	    (*outMediaType)->subtype = MEDIASUBTYPE_YUY2;
	(*outMediaType)->formattype = FORMAT_VideoInfo;
	(*outMediaType)->bTemporalCompression = FALSE;
	(*outMediaType)->bFixedSizeSamples = FALSE;
	(*outMediaType)->lSampleSize = videoInfoHeader->bmiHeader.biSizeImage;
	(*outMediaType)->cbFormat = sizeof(VIDEOINFOHEADER);

    VIDEO_STREAM_CONFIG_CAPS* videoStreamConfigCaps = (VIDEO_STREAM_CONFIG_CAPS*)(ptrStreamConfigCaps);

	memset(videoStreamConfigCaps, 0, sizeof(VIDEO_STREAM_CONFIG_CAPS));

	SIZE size = {
		videoInfoHeader->bmiHeader.biWidth,
		videoInfoHeader->bmiHeader.biHeight
	};

	videoStreamConfigCaps->guid = FORMAT_VideoInfo;
	videoStreamConfigCaps->VideoStandard = AnalogVideo_None;
	videoStreamConfigCaps->InputSize = size;
	videoStreamConfigCaps->MinCroppingSize = size;
	videoStreamConfigCaps->MaxCroppingSize = size;
	videoStreamConfigCaps->CropGranularityX = videoInfoHeader->bmiHeader.biWidth;
	videoStreamConfigCaps->CropGranularityY = videoInfoHeader->bmiHeader.biHeight;
	videoStreamConfigCaps->MinOutputSize = size;
	videoStreamConfigCaps->MaxOutputSize = size;

	videoStreamConfigCaps->MinFrameInterval = videoInfoHeader->AvgTimePerFrame;   
	videoStreamConfigCaps->MaxFrameInterval = videoInfoHeader->AvgTimePerFrame; 

	LONG bps;
	//if (parent->sourceConfig->type == DShowSourceFormat_Yuy2)
		bps = videoInfoHeader->bmiHeader.biWidth * videoInfoHeader->bmiHeader.biHeight * 2 * 8 * (10000000 / videoInfoHeader->AvgTimePerFrame);
	videoStreamConfigCaps->MinBitsPerSecond = bps;
	videoStreamConfigCaps->MaxBitsPerSecond = bps;

	return S_OK;
}


void ARibeiroOutputStream::CheckCreatedFormatList()
{
	if (format_list.size() > 0)
        return;
    
    ARibeiroSourceDevice *parent = (ARibeiroSourceDevice *)m_pFilter;



    int width = 1920;
    int height = 1080;
    double framerate = 30.0f;

    /*
    uint32_t numerator;
    uint32_t denominator;
    Double2Fract_inverse(framerate, &numerator, &denominator);
    */

    double timePerFrame = floor(10000000.0 / static_cast<double>(framerate));
    REFERENCE_TIME AvgTimePerFrame = (REFERENCE_TIME)(timePerFrame + 0.5);

    format_list.push_back(DShowSourceResolution(
        width,
        height,
        AvgTimePerFrame
    ));

    /*
	
    format_list.push_back(DShowSourceResolution(
        parent->sourceConfig->width, 
        parent->sourceConfig->height, 
        parent->sourceConfig->AvgTimePerFrame
    ));
    */

}


ARibeiroOutputStream::ARibeiroOutputStream(HRESULT *phr, ARibeiroSourceDevice *pParent, LPCWSTR pPinName):
CSourceStream(NAME("Video"), phr, pParent, pPinName)
{
    process = NULL;

    ARibeiroSourceDevice *parent = (ARibeiroSourceDevice *)m_pFilter;
    parent->debug->printf("  [ARibeiroOutputStream] creating Object!!!\n");

    parent->debug->printf("  [ARibeiroOutputStream] hDllModuleDll: %x\n", hDllModuleDll);

    // unzip the splash image from rc file
    HRSRC hRes = ::FindResourceA(hDllModuleDll, MAKEINTRESOURCE(IDR_SPLASH), RT_RCDATA);//"splash.bin"
    if (hRes != NULL) {
        parent->debug->printf("  [ARibeiroOutputStream] resource found.\n");

        unsigned int myResourceSize = ::SizeofResource(hDllModuleDll, hRes);
        HGLOBAL hResLoad = ::LoadResource(hDllModuleDll, hRes);
        if (hResLoad != NULL) {
            parent->debug->printf("  [ARibeiroOutputStream]   Resource loaded.\n");
            void* pMyBinaryData = ::LockResource(hResLoad);
            if (pMyBinaryData != NULL) {
                
                BinaryReader reader;
                reader.readFromBuffer((uint8_t*)pMyBinaryData, myResourceSize);

                uint8_t *buffer;
                uint32_t size;

                reader.readBuffer( &buffer, &size);
                data_buffer.setSize(size);
                memcpy(data_buffer.data, buffer, size);
            }
            else {
                parent->debug->printf("  [ARibeiroOutputStream]   resource pointer is null...\n");
            }
            //::UnlockResource(myResourceData);
            ::FreeResource(hResLoad);
        }
        else {
            parent->debug->printf("  [ARibeiroOutputStream]   Error to load resource.\n");
        }
    }
    else {
        parent->debug->printf("  [ARibeiroOutputStream] resource not found.\n");
    }

    std::string queueName = aRibeiro::StringUtil::toString(parent->base_template->m_Name);

    // 8 buffers of 1920x1080x2 (yuyv2)
    data_queue = new PlatformLowLatencyQueueIPC(queueName.c_str(), PlatformQueueIPC_READ, 8, 1920*1080*2, false);

	CheckCreatedFormatList();
	GetMediaType(0, &m_mt);

	first = true;
}

ARibeiroOutputStream::~ARibeiroOutputStream()
{
    aRibeiro::setNullAndDelete(process);

    ARibeiroSourceDevice *parent = (ARibeiroSourceDevice *)m_pFilter;
    if (parent != NULL && parent->debug != NULL)
        parent->debug->printf("  [ARibeiroOutputStream] destroying Object!!!\n");

    if (data_queue != NULL) {
        delete data_queue;
        data_queue = NULL;
    }
}


HRESULT ARibeiroOutputStream::FillBuffer(IMediaSample *ptrMediaSample)
{
    ARibeiroSourceDevice *parent = (ARibeiroSourceDevice *)m_pFilter;

    if (process == NULL) {
        char achTemp[MAX_PATH];
        if (GetModuleFileNameA(hDllModuleDll, achTemp, sizeof(achTemp)))
        {
            parent->debug->printf("  [ARibeiroOutputStream] dll file: %s\n", achTemp);
            std::string dll_path = PlatformPath::getExecutablePath(achTemp);
            parent->debug->printf("  [ARibeiroOutputStream] dll path: %s\n", dll_path.c_str());
            process = new PlatformProcess(dll_path + "/network-to-ipc/network-to-ipc", "-noconsole");
            parent->debug->printf("  [ARibeiroOutputStream] process created: %i\n", process->isCreated());
        }
    }

	VIDEOINFOHEADER* videoInfoHeader = (VIDEOINFOHEADER*)m_mt.pbFormat;

	uint8_t* buffer;
	HRESULT hr = ptrMediaSample->GetPointer((BYTE**)&buffer);
	int bufferSize = ptrMediaSample->GetActualDataLength();
	REFERENCE_TIME duration = videoInfoHeader->AvgTimePerFrame;

    bool read_content = false;
    /*
    //parent->debug->printf(".");
    if (data_queue->readHasElement(true)) {
        //parent->debug->printf("  [ARibeiroOutputStream] Has content\n");
        data_queue->read(&data_buffer, false, true);

        //clear queue... low latency strategy
        while (data_queue->readHasElement(true))
            data_queue->read(&data_buffer, false, true);

        if (bufferSize == data_buffer.size) {
            //parent->debug->printf("  [ARibeiroOutputStream]     Valid Buffer Size\n");
            memcpy(buffer, data_buffer.data, bufferSize);
            read_content = true;
        }
    }
    */
    if (data_queue->read(&data_buffer)) {

        //clear queue... low latency strategy
        while (data_queue->read(&data_buffer));

        if (bufferSize == data_buffer.size) {
            //parent->debug->printf("  [ARibeiroOutputStream]     Valid Buffer Size\n");
            memcpy(buffer, data_buffer.data, bufferSize);
            read_content = true;
        }
    }
    else {
        data_buffer.setSize(bufferSize);

        if (bufferSize == data_buffer.size) {
            //parent->debug->printf("  [ARibeiroOutputStream]     Valid Buffer Size\n");
            memcpy(buffer, data_buffer.data, bufferSize);
            read_content = true;
        }
    }

    timer.update();
	if (first){
		first = false;
		timer.reset();
        current_time = 0;
        next_time = duration;
	} else {
        // 1micro = 10 ActiveMovie units (100 nS)
        current_time += (REFERENCE_TIME)(timer.deltaTimeMicro * 10);// next_time;
        //if (read_content) {
            sleepToSync(&current_time, next_time);
            //update next just in case of having new content...
            next_time = current_time + duration;
        //} else 
            // return E_PENDING;
	}

	ptrMediaSample->SetTime(&current_time, &next_time);
	ptrMediaSample->SetSyncPoint(TRUE);

	return NOERROR;
}

void ARibeiroOutputStream::sleepToSync(REFERENCE_TIME *current, const REFERENCE_TIME &target) {

    ARibeiroSourceDevice *parent = (ARibeiroSourceDevice *)m_pFilter;

	// 1ms = 1000000ns
	// 1micro = 1000ns
	// ActiveMovie units (100 nS)
	REFERENCE_TIME delta = target - (*current);


    //parent->debug->printf("  [ARibeiroOutputStream] SLEEP TO SYNC from: %" PRIi64 " to %" PRIi64 "(delta: %" PRIi64 ")\n", (*current), target, delta);
    if (delta <= 0) {
        //parent->debug->printf("  [ARibeiroOutputStream]    No need to sync...\n", (*current), target);
        return;
    }

    // 1ms = 10000 ActiveMovie units (100 nS)
    uint64_t delta_ms = delta / 10000;
    if (delta % 10000 > 0)
        delta_ms += 1;
    PlatformSleep::sleepMillis(delta_ms);
    
    // 1micro = 10 ActiveMovie units (100 nS)
    //uint64_t delta_micro = delta / 10;
	//PlatformSleep::busySleepMicro(delta_micro);

    timer.update();

    (*current) += (REFERENCE_TIME)(timer.deltaTimeMicro * 10);
    delta = target - (*current);
    //parent->debug->printf("  [ARibeiroOutputStream]    After SYNC: %" PRIi64 " -> %" PRIi64 "(delta: %" PRIi64 ")\n", (*current), target, delta);
}

HRESULT ARibeiroOutputStream::DecideBufferSize(IMemAllocator *ptrAllocator, 
	ALLOCATOR_PROPERTIES *ptrProperties)
{
	CAutoLock cAutoLock(m_pFilter->pStateLock());
	HRESULT hr = NOERROR;

	VIDEOINFOHEADER *videoInfoHeader = (VIDEOINFOHEADER *)m_mt.Format();
	ptrProperties->cBuffers = 1;
	ptrProperties->cbBuffer = videoInfoHeader->bmiHeader.biSizeImage;

	ALLOCATOR_PROPERTIES Actual;
	hr = ptrAllocator->SetProperties(ptrProperties, &Actual);

	if (FAILED(hr)) return hr;
	if (Actual.cbBuffer < ptrProperties->cbBuffer) return E_FAIL;

	return NOERROR;
} 

HRESULT ARibeiroOutputStream::CheckMediaType(const CMediaType *ptrMediaType)
{
	if (ptrMediaType == nullptr)
		return E_FAIL;

	VIDEOINFOHEADER *videoInforHeader = (VIDEOINFOHEADER *)(ptrMediaType->Format());

	const GUID* type = ptrMediaType->Type();
	const GUID* info = ptrMediaType->FormatType();
	const GUID* subtype = ptrMediaType->Subtype();

	ARibeiroSourceDevice *parent = (ARibeiroSourceDevice *)m_pFilter;

	if (*type != MEDIATYPE_Video)
		return E_INVALIDARG;

	if (*info != FORMAT_VideoInfo)
		return E_INVALIDARG;

	//if (parent->sourceConfig->type == DShowSourceFormat_Yuy2)
	if (*subtype != MEDIASUBTYPE_YUY2)
		return E_INVALIDARG;

    for (int i = 0; i < format_list.size(); i++)
    {
        if (videoInforHeader->AvgTimePerFrame == format_list[i].time_per_frame &&
            videoInforHeader->bmiHeader.biWidth == format_list[i].width &&
            videoInforHeader->bmiHeader.biHeight == format_list[i].height)
            return S_OK;
    }

    /*
	if (videoInforHeader->AvgTimePerFrame != parent->sourceConfig->AvgTimePerFrame)
		return E_INVALIDARG;

	if (videoInforHeader->bmiHeader.biWidth == parent->sourceConfig->width ||
	    videoInforHeader->bmiHeader.biHeight == parent->sourceConfig->height)
		return S_OK;
    */

	return E_INVALIDARG;
} 

HRESULT ARibeiroOutputStream::GetMediaType(int iPosition,CMediaType *ptrMediaType)
{
	CheckCreatedFormatList();

	if (iPosition < 0 || iPosition > format_list.size()-1)
		return E_INVALIDARG;

	VIDEOINFOHEADER* videoInfoHeader = (VIDEOINFOHEADER*)(ptrMediaType->AllocFormatBuffer(sizeof(VIDEOINFOHEADER)));

	ZeroMemory(videoInfoHeader, sizeof(VIDEOINFOHEADER));

	ARibeiroSourceDevice *parent = (ARibeiroSourceDevice *)m_pFilter;

	videoInfoHeader->bmiHeader.biWidth = format_list[iPosition].width;
	videoInfoHeader->bmiHeader.biHeight = format_list[iPosition].height;
	videoInfoHeader->AvgTimePerFrame = format_list[iPosition].time_per_frame;
	//if (parent->sourceConfig->type == DShowSourceFormat_Yuy2) 
    {
		videoInfoHeader->bmiHeader.biCompression = MAKEFOURCC('Y', 'U', 'Y', '2');
		videoInfoHeader->bmiHeader.biBitCount = 16;
	}
	videoInfoHeader->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	videoInfoHeader->bmiHeader.biPlanes = 1;

	//if (parent->sourceConfig->type == DShowSourceFormat_Yuy2) 
    {
		videoInfoHeader->bmiHeader.biSizeImage = videoInfoHeader->bmiHeader.biWidth * videoInfoHeader->bmiHeader.biHeight * 2;
	}

	videoInfoHeader->bmiHeader.biClrImportant = 0;

	SetRectEmpty(&(videoInfoHeader->rcSource)); 
	SetRectEmpty(&(videoInfoHeader->rcTarget)); 

	ptrMediaType->SetType(&MEDIATYPE_Video);
	ptrMediaType->SetFormatType(&FORMAT_VideoInfo);
	ptrMediaType->SetTemporalCompression(FALSE);
	//if (parent->sourceConfig->type == DShowSourceFormat_Yuy2)
		ptrMediaType->SetSubtype(&MEDIASUBTYPE_YUY2);
	ptrMediaType->SetSampleSize(videoInfoHeader->bmiHeader.biSizeImage);
	return NOERROR;

}

HRESULT ARibeiroOutputStream::SetMediaType(const CMediaType *ptrMediaType)
{
	VIDEOINFOHEADER* videoInfoHeader = (VIDEOINFOHEADER*)(ptrMediaType->Format());
	HRESULT hr = CSourceStream::SetMediaType(ptrMediaType);
	return hr;
}


