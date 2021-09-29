#include "pch.h"
#include "DX.h"


#include <windows.h>
#include <gdiplus.h>
#include <stdio.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

#define RESET_OBJECT(obj) { if(obj) obj->Release(); obj = NULL; }
static BOOL g_bAttach = FALSE;



extern "C" __declspec(dllexport)  BOOL Init()
{

    m_bInit = FALSE;

    m_hDevice = NULL;
    m_hContext = NULL;
    m_hDeskDupl = NULL;

    ZeroMemory(&m_dxgiOutDesc, sizeof(m_dxgiOutDesc));

    HRESULT hr = S_OK;

    if (m_bInit)
    {
        return FALSE;
    }

    // Driver types supported
    D3D_DRIVER_TYPE DriverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT NumDriverTypes = ARRAYSIZE(DriverTypes);

    // Feature levels supported
    D3D_FEATURE_LEVEL FeatureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_1
    };
    UINT NumFeatureLevels = ARRAYSIZE(FeatureLevels);

    D3D_FEATURE_LEVEL FeatureLevel;

    //
    // Create D3D device
    //
    for (UINT DriverTypeIndex = 0; DriverTypeIndex < NumDriverTypes; ++DriverTypeIndex)
    {
        hr = D3D11CreateDevice(NULL, DriverTypes[DriverTypeIndex], NULL, 0, FeatureLevels, NumFeatureLevels, D3D11_SDK_VERSION, &m_hDevice, &FeatureLevel, &m_hContext);
        if (SUCCEEDED(hr))
        {
            break;
        }
    }
    if (FAILED(hr))
    {
        return FALSE;
    }

    //
    // Get DXGI device
    //
    IDXGIDevice* hDxgiDevice = NULL;
    hr = m_hDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&hDxgiDevice));
    if (FAILED(hr))
    {
        return FALSE;
    }

    //
    // Get DXGI adapter
    //
    IDXGIAdapter* hDxgiAdapter = NULL;
    hr = hDxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&hDxgiAdapter));
    RESET_OBJECT(hDxgiDevice);
    if (FAILED(hr))
    {
        return FALSE;
    }

    //
    // Get output
    //
    INT nOutput = 0;
    IDXGIOutput* hDxgiOutput = NULL;
    hr = hDxgiAdapter->EnumOutputs(nOutput, &hDxgiOutput);
    RESET_OBJECT(hDxgiAdapter);
    if (FAILED(hr))
    {
        return FALSE;
    }

    //
    // get output description struct
    //
    hDxgiOutput->GetDesc(&m_dxgiOutDesc);

    //
    // QI for Output 1
    //
    IDXGIOutput1* hDxgiOutput1 = NULL;
    hr = hDxgiOutput->QueryInterface(__uuidof(hDxgiOutput1), reinterpret_cast<void**>(&hDxgiOutput1));
    RESET_OBJECT(hDxgiOutput);
    if (FAILED(hr))
    {
        return FALSE;
    }

    //
    // Create desktop duplication
    //
    hr = hDxgiOutput1->DuplicateOutput(m_hDevice, &m_hDeskDupl);
    RESET_OBJECT(hDxgiOutput1);
    if (FAILED(hr))
    {
        return FALSE;
    }

    // 初始化成功
    m_bInit = TRUE;
    AttatchToThread();
    return TRUE;
    // #else
        // 小于vs2012,此功能不能实现
    return FALSE;
    // #endif
}
extern "C" __declspec(dllexport)  VOID Deinit()
{
    if (!m_bInit)
    {
        return;
    }

    m_bInit = FALSE;

    if (m_hDeskDupl)
    {
        m_hDeskDupl->Release();
        m_hDeskDupl = NULL;
    }

    if (m_hDevice)
    {
        m_hDevice->Release();
        m_hDevice = NULL;
    }

    if (m_hContext)
    {
        m_hContext->Release();
        m_hContext = NULL;
    }
    // #endif
}
BOOL AttatchToThread(VOID)
{
    if (g_bAttach)
    {
        return TRUE;
    }

    HDESK hCurrentDesktop = OpenInputDesktop(0, FALSE, GENERIC_ALL);
    if (!hCurrentDesktop)
    {
        return FALSE;
    }

    // Attach desktop to this thread
    BOOL bDesktopAttached = SetThreadDesktop(hCurrentDesktop);
    if (bDesktopAttached == 0) {
        int a = GetLastError();
        printf("%d", a);
    }
    CloseDesktop(hCurrentDesktop);
    hCurrentDesktop = NULL;

    g_bAttach = TRUE;

    return bDesktopAttached;
}


extern "C" __declspec(dllexport) BOOL CaptureImage(void* pData, INT& nLen, INT& Pitch)
{
    return QueryFrame(pData, nLen, Pitch);
}
extern "C" __declspec(dllexport) BOOL ResetDevice()
{
    Deinit();
    return Init();
}
BOOL QueryFrame(void* pImgData, INT& nImgSize, INT& Pitch)
{
    if (!m_bInit || !AttatchToThread())
    {
        if (!m_bInit)
            MessageBoxA(0, "e11", "", 0);
        if (!AttatchToThread())
            MessageBoxA(0, "e12", "", 0);
        return FALSE;
    }

    nImgSize = 0;

    IDXGIResource* hDesktopResource = NULL;
    DXGI_OUTDUPL_FRAME_INFO FrameInfo;
    HRESULT hr = m_hDeskDupl->AcquireNextFrame(0, &FrameInfo, &hDesktopResource);



    if (FAILED(hr))
    {
        //
        // 在一些win10的系统上,如果桌面没有变化的情况下，;
        // 这里会发生超时现象，但是这并不是发生了错误，而是系统优化了刷新动作导致的。;
        // 所以，这里没必要返回FALSE，返回不带任何数据的TRUE即可;
        //
        return TRUE;
    }

    //
    // query next frame staging buffer
    //
    ID3D11Texture2D* hAcquiredDesktopImage = NULL;
    hr = hDesktopResource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&hAcquiredDesktopImage));
    RESET_OBJECT(hDesktopResource);
    if (FAILED(hr))
    {
        MessageBoxA(0, "e2", "", 0);
        return FALSE;
    }

    //
    // copy old description
    //
    D3D11_TEXTURE2D_DESC frameDescriptor;
    hAcquiredDesktopImage->GetDesc(&frameDescriptor);

    //
    // create a new staging buffer for fill frame image
    //
    ID3D11Texture2D* hNewDesktopImage = NULL;
    frameDescriptor.Usage = D3D11_USAGE_STAGING;
    frameDescriptor.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    frameDescriptor.BindFlags = 0;
    frameDescriptor.MiscFlags = 0;
    frameDescriptor.MipLevels = 1;
    frameDescriptor.ArraySize = 1;
    frameDescriptor.SampleDesc.Count = 1;
    hr = m_hDevice->CreateTexture2D(&frameDescriptor, NULL, &hNewDesktopImage);
    if (FAILED(hr))
    {
        RESET_OBJECT(hAcquiredDesktopImage);
        m_hDeskDupl->ReleaseFrame();
        MessageBoxA(0, "e3", "", 0);
        return FALSE;
    }

    //
    // copy next staging buffer to new staging buffer
    //
    m_hContext->CopyResource(hNewDesktopImage, hAcquiredDesktopImage);

    RESET_OBJECT(hAcquiredDesktopImage);
    m_hDeskDupl->ReleaseFrame();

    //
    // create staging buffer for map bits
    //
    IDXGISurface* hStagingSurf = NULL;
    hr = hNewDesktopImage->QueryInterface(__uuidof(IDXGISurface), (void**)(&hStagingSurf));
    RESET_OBJECT(hNewDesktopImage);
    if (FAILED(hr))
    {
        MessageBoxA(0, "e4", "", 0);
        return FALSE;
    }

    //
    // copy bits to user space
    //
    DXGI_MAPPED_RECT mappedRect;
    hr = hStagingSurf->Map(&mappedRect, DXGI_MAP_READ);
    if (SUCCEEDED(hr))
    {
        // nImgSize = GetWidth() * GetHeight() * 3;
        // PrepareBGR24From32(mappedRect.pBits, (BYTE*)pImgData, m_dxgiOutDesc.DesktopCoordinates);
        // mappedRect.pBits;
        // am_dxgiOutDesc.DesktopCoordinates;
        nImgSize = mappedRect.Pitch * m_dxgiOutDesc.DesktopCoordinates.bottom;
        Pitch = mappedRect.Pitch;
        memcpy((BYTE*)pImgData, mappedRect.pBits, m_dxgiOutDesc.DesktopCoordinates.right * m_dxgiOutDesc.DesktopCoordinates.bottom * 4);
        hStagingSurf->Unmap();
    }

    RESET_OBJECT(hStagingSurf);
    if (!SUCCEEDED(hr)) MessageBoxA(0, "e5", "", 0);
    return SUCCEEDED(hr);
}