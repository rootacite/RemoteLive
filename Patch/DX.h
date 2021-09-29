#pragma once


#include <d3d11.h>
#include <dxgi1_2.h>


BOOL  AttatchToThread(VOID);
BOOL  QueryFrame(void* pImgData, INT& nImgSize, INT& Pitch);

IDXGIResource* zhDesktopResource;
DXGI_OUTDUPL_FRAME_INFO zFrameInfo;
ID3D11Texture2D* zhAcquiredDesktopImage;
IDXGISurface* zhStagingSurf;

BOOL                    m_bInit;
int                     m_iWidth, m_iHeight;

ID3D11Device* m_hDevice;
ID3D11DeviceContext* m_hContext;

IDXGIOutputDuplication* m_hDeskDupl;
DXGI_OUTPUT_DESC        m_dxgiOutDesc; 



extern "C" __declspec(dllexport) BOOL Init();
extern "C" __declspec(dllexport) VOID Deinit();

extern "C" __declspec(dllexport) BOOL CaptureImage(void* pData, INT& nLen, INT& Pitch);
extern "C" __declspec(dllexport) BOOL ResetDevice();