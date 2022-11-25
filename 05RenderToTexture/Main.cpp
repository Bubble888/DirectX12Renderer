#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <wrl.h>	
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <d3d12.h>     
#include <d3dcompiler.h>
#if defined(_DEBUG)
#include <dxgidebug.h>
#endif
#include <wincodec.h>
//使用到容器Vector
#include <vector>
//为了实用一些微软提供的结构体的helper形式，加了下面的文件
#include"d3dx12.h"

using namespace Microsoft;
using namespace Microsoft::WRL;
using namespace DirectX;

#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")

#define GRS_WND_CLASS_NAME _T("GRS Game Window Class")
#define GRS_WND_TITLE	_T("致我们永不熄灭的游戏开发梦想~")

#define GRS_UPPER_DIV(A,B) ((UINT)(((A)+((B)-1))/(B)))

#define GRS_UPPER(A,B) ((UINT)(((A)+((B)-1))&~(B - 1)))

#define GRS_THROW_IF_FAILED(hr) {HRESULT _hr = (hr);if (FAILED(_hr)){ throw CGRSCOMException(_hr); }}

void printNumber(int num) {
	TCHAR print[MAX_PATH] = {};
	StringCchPrintf(print
		, MAX_PATH
		, _T("%d")
		, num);
	OutputDebugString(print);
}
class CGRSCOMException
{
public:
	CGRSCOMException(HRESULT hr) : m_hrError(hr)
	{
	}
	HRESULT Error() const
	{
		return m_hrError;
	}
private:
	const HRESULT m_hrError;
};

struct WICTranslate
{
	GUID wic;
	DXGI_FORMAT format;
};

static WICTranslate g_WICFormats[] =
{
	{ GUID_WICPixelFormat128bppRGBAFloat,       DXGI_FORMAT_R32G32B32A32_FLOAT },

	{ GUID_WICPixelFormat64bppRGBAHalf,         DXGI_FORMAT_R16G16B16A16_FLOAT },
	{ GUID_WICPixelFormat64bppRGBA,             DXGI_FORMAT_R16G16B16A16_UNORM },

	{ GUID_WICPixelFormat32bppRGBA,             DXGI_FORMAT_R8G8B8A8_UNORM },
	{ GUID_WICPixelFormat32bppBGRA,             DXGI_FORMAT_B8G8R8A8_UNORM }, // DXGI 1.1
	{ GUID_WICPixelFormat32bppBGR,              DXGI_FORMAT_B8G8R8X8_UNORM }, // DXGI 1.1

	{ GUID_WICPixelFormat32bppRGBA1010102XR,    DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM }, // DXGI 1.1
	{ GUID_WICPixelFormat32bppRGBA1010102,      DXGI_FORMAT_R10G10B10A2_UNORM },

	{ GUID_WICPixelFormat16bppBGRA5551,         DXGI_FORMAT_B5G5R5A1_UNORM },
	{ GUID_WICPixelFormat16bppBGR565,           DXGI_FORMAT_B5G6R5_UNORM },

	{ GUID_WICPixelFormat32bppGrayFloat,        DXGI_FORMAT_R32_FLOAT },
	{ GUID_WICPixelFormat16bppGrayHalf,         DXGI_FORMAT_R16_FLOAT },
	{ GUID_WICPixelFormat16bppGray,             DXGI_FORMAT_R16_UNORM },
	{ GUID_WICPixelFormat8bppGray,              DXGI_FORMAT_R8_UNORM },

	{ GUID_WICPixelFormat8bppAlpha,             DXGI_FORMAT_A8_UNORM },
};

struct WICConvert
{
	GUID source;
	GUID target;
};

static WICConvert g_WICConvert[] =
{
	{ GUID_WICPixelFormatBlackWhite,            GUID_WICPixelFormat8bppGray }, // DXGI_FORMAT_R8_UNORM

	{ GUID_WICPixelFormat1bppIndexed,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
	{ GUID_WICPixelFormat2bppIndexed,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
	{ GUID_WICPixelFormat4bppIndexed,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
	{ GUID_WICPixelFormat8bppIndexed,           GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM

	{ GUID_WICPixelFormat2bppGray,              GUID_WICPixelFormat8bppGray }, // DXGI_FORMAT_R8_UNORM
	{ GUID_WICPixelFormat4bppGray,              GUID_WICPixelFormat8bppGray }, // DXGI_FORMAT_R8_UNORM

	{ GUID_WICPixelFormat16bppGrayFixedPoint,   GUID_WICPixelFormat16bppGrayHalf }, // DXGI_FORMAT_R16_FLOAT
	{ GUID_WICPixelFormat32bppGrayFixedPoint,   GUID_WICPixelFormat32bppGrayFloat }, // DXGI_FORMAT_R32_FLOAT

	{ GUID_WICPixelFormat16bppBGR555,           GUID_WICPixelFormat16bppBGRA5551 }, // DXGI_FORMAT_B5G5R5A1_UNORM

	{ GUID_WICPixelFormat32bppBGR101010,        GUID_WICPixelFormat32bppRGBA1010102 }, // DXGI_FORMAT_R10G10B10A2_UNORM

	{ GUID_WICPixelFormat24bppBGR,              GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
	{ GUID_WICPixelFormat24bppRGB,              GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
	{ GUID_WICPixelFormat32bppPBGRA,            GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
	{ GUID_WICPixelFormat32bppPRGBA,            GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM

	{ GUID_WICPixelFormat48bppRGB,              GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
	{ GUID_WICPixelFormat48bppBGR,              GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
	{ GUID_WICPixelFormat64bppBGRA,             GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
	{ GUID_WICPixelFormat64bppPRGBA,            GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
	{ GUID_WICPixelFormat64bppPBGRA,            GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM

	{ GUID_WICPixelFormat48bppRGBFixedPoint,    GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT
	{ GUID_WICPixelFormat48bppBGRFixedPoint,    GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT
	{ GUID_WICPixelFormat64bppRGBAFixedPoint,   GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT
	{ GUID_WICPixelFormat64bppBGRAFixedPoint,   GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT
	{ GUID_WICPixelFormat64bppRGBFixedPoint,    GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT
	{ GUID_WICPixelFormat48bppRGBHalf,          GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT
	{ GUID_WICPixelFormat64bppRGBHalf,          GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT

	{ GUID_WICPixelFormat128bppPRGBAFloat,      GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT
	{ GUID_WICPixelFormat128bppRGBFloat,        GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT
	{ GUID_WICPixelFormat128bppRGBAFixedPoint,  GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT
	{ GUID_WICPixelFormat128bppRGBFixedPoint,   GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT
	{ GUID_WICPixelFormat32bppRGBE,             GUID_WICPixelFormat128bppRGBAFloat }, // DXGI_FORMAT_R32G32B32A32_FLOAT

	{ GUID_WICPixelFormat32bppCMYK,             GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
	{ GUID_WICPixelFormat64bppCMYK,             GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
	{ GUID_WICPixelFormat40bppCMYKAlpha,        GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
	{ GUID_WICPixelFormat80bppCMYKAlpha,        GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM

	{ GUID_WICPixelFormat32bppRGB,              GUID_WICPixelFormat32bppRGBA }, // DXGI_FORMAT_R8G8B8A8_UNORM
	{ GUID_WICPixelFormat64bppRGB,              GUID_WICPixelFormat64bppRGBA }, // DXGI_FORMAT_R16G16B16A16_UNORM
	{ GUID_WICPixelFormat64bppPRGBAHalf,        GUID_WICPixelFormat64bppRGBAHalf }, // DXGI_FORMAT_R16G16B16A16_FLOAT
};

bool GetTargetPixelFormat(const GUID* pSourceFormat, GUID* pTargetFormat)
{
	*pTargetFormat = *pSourceFormat;
	for (size_t i = 0; i < _countof(g_WICConvert); ++i)
	{
		if (InlineIsEqualGUID(g_WICConvert[i].source, *pSourceFormat))
		{
			*pTargetFormat = g_WICConvert[i].target;
			return true;
		}
	}
	return false;
}

DXGI_FORMAT GetDXGIFormatFromPixelFormat(const GUID* pPixelFormat)
{
	for (size_t i = 0; i < _countof(g_WICFormats); ++i)
	{
		if (InlineIsEqualGUID(g_WICFormats[i].wic, *pPixelFormat))
		{
			return g_WICFormats[i].format;
		}
	}
	return DXGI_FORMAT_UNKNOWN;
}


struct ST_GRS_VERTEX
{
	XMFLOAT4 m_v4Position;
	XMFLOAT2 m_vTex;
};

struct ST_GRS_FRAME_MVP_BUFFER
{
	XMFLOAT4X4 m_MVP;
};

UINT UploadTexture(ComPtr<IWICBitmapSource>& pIBMP, ComPtr<IWICImagingFactory> pIWICFactory,
	ComPtr<IWICBitmapDecoder> pIWICDecoder, ComPtr<IWICBitmapFrameDecode> pIWICFrame,
	DXGI_FORMAT& stTextureFormat, UINT& nTextureW, UINT& nTextureH, UINT& nBPP, TCHAR* pszAppPath)
{
	GRS_THROW_IF_FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pIWICFactory)));
	TCHAR pszTextureFileName[MAX_PATH] = {};
	StringCchPrintf(pszTextureFileName, MAX_PATH, _T("%sTexture\\cat.jpg"), pszAppPath);
	GRS_THROW_IF_FAILED(pIWICFactory->CreateDecoderFromFilename(
		pszTextureFileName,
		NULL,
		GENERIC_READ,
		WICDecodeMetadataCacheOnDemand,
		&pIWICDecoder
	));

	GRS_THROW_IF_FAILED(pIWICDecoder->GetFrame(0, &pIWICFrame));

	WICPixelFormatGUID wpf = {};
	GRS_THROW_IF_FAILED(pIWICFrame->GetPixelFormat(&wpf));
	GUID tgFormat = {};

	if (GetTargetPixelFormat(&wpf, &tgFormat))
	{
		stTextureFormat = GetDXGIFormatFromPixelFormat(&tgFormat);
	}

	if (DXGI_FORMAT_UNKNOWN == stTextureFormat)
	{
		throw CGRSCOMException(S_FALSE);
	}



	if (!InlineIsEqualGUID(wpf, tgFormat))
	{
		ComPtr<IWICFormatConverter> pIConverter;
		GRS_THROW_IF_FAILED(pIWICFactory->CreateFormatConverter(&pIConverter));

		GRS_THROW_IF_FAILED(pIConverter->Initialize(
			pIWICFrame.Get(),
			tgFormat,
			WICBitmapDitherTypeNone,
			NULL,
			0.f,
			WICBitmapPaletteTypeCustom
		));
		GRS_THROW_IF_FAILED(pIConverter.As(&pIBMP));
	}
	else
	{
		GRS_THROW_IF_FAILED(pIWICFrame.As(&pIBMP));
	}
	GRS_THROW_IF_FAILED(pIBMP->GetSize(&nTextureW, &nTextureH));

	ComPtr<IWICComponentInfo> pIWICmntinfo;
	GRS_THROW_IF_FAILED(pIWICFactory->CreateComponentInfo(tgFormat, pIWICmntinfo.GetAddressOf()));

	WICComponentType type;
	GRS_THROW_IF_FAILED(pIWICmntinfo->GetComponentType(&type));

	if (type != WICPixelFormat)
	{
		throw CGRSCOMException(S_FALSE);
	}

	ComPtr<IWICPixelFormatInfo> pIWICPixelinfo;
	GRS_THROW_IF_FAILED(pIWICmntinfo.As(&pIWICPixelinfo));

	GRS_THROW_IF_FAILED(pIWICPixelinfo->GetBitsPerPixel(&nBPP));

	UINT nPicRowPitch = (uint64_t(nTextureW) * uint64_t(nBPP) + 7u) / 8u;
	return nPicRowPitch;
}



D3D12_PLACED_SUBRESOURCE_FOOTPRINT CopyToUploadHeap(ComPtr<ID3D12Resource> pITexture, ComPtr<ID3D12Resource> pITextureUpload,
	ComPtr<IWICBitmapSource> pIBMP, ComPtr<ID3D12Device4>	pID3D12Device4, UINT64 n64UploadBufferSize, UINT nTextureH,
	UINT nPicRowPitch)
{
	void* pbPicData = ::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, n64UploadBufferSize);
	if (nullptr == pbPicData)
	{
		throw CGRSCOMException(HRESULT_FROM_WIN32(GetLastError()));
	}

	GRS_THROW_IF_FAILED(pIBMP->CopyPixels(nullptr
		, nPicRowPitch
		, static_cast<UINT>(nPicRowPitch * nTextureH)
		, reinterpret_cast<BYTE*>(pbPicData)));

	UINT64 n64RequiredSize = 0u;
	UINT   nNumSubresources = 1u;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT stTxtLayouts = {};
	UINT64 n64TextureRowSizes = 0u;
	UINT   nTextureRowNum = 0u;

	D3D12_RESOURCE_DESC stDestDesc = pITexture->GetDesc();

	pID3D12Device4->GetCopyableFootprints(&stDestDesc
		, 0
		, nNumSubresources
		, 0
		, &stTxtLayouts
		, &nTextureRowNum
		, &n64TextureRowSizes
		, &n64RequiredSize);

	BYTE* pData = nullptr;
	GRS_THROW_IF_FAILED(pITextureUpload->Map(0, NULL, reinterpret_cast<void**>(&pData)));

	BYTE* pDestSlice = reinterpret_cast<BYTE*>(pData) + stTxtLayouts.Offset;
	const BYTE* pSrcSlice = reinterpret_cast<const BYTE*>(pbPicData);
	for (UINT y = 0; y < nTextureRowNum; ++y)
	{
		memcpy(pDestSlice + static_cast<SIZE_T>(stTxtLayouts.Footprint.RowPitch) * y
			, pSrcSlice + static_cast<SIZE_T>(nPicRowPitch) * y
			, nPicRowPitch);
	}

	pITextureUpload->Unmap(0, NULL);

	::HeapFree(::GetProcessHeap(), 0, pbPicData);
	return stTxtLayouts;
}

//——————————————————————————————————————————————————————
//用来计算权重
std::vector<float> CalcGaussWeights(float sigma)
{
	float twoSigma2 = 2.0f * sigma * sigma;

	// Estimate the blur radius based on sigma since sigma controls the "width" of the bell curve.
	// For example, for sigma = 3, the width of the bell curve is 
	int blurRadius = (int)ceil(2.0f * sigma);


	std::vector<float> weights;
	weights.resize(2 * blurRadius + 1);

	float weightSum = 0.0f;

	for (int i = -blurRadius; i <= blurRadius; ++i)
	{
		float x = (float)i;

		weights[i + blurRadius] = expf(-x * x / twoSigma2);

		weightSum += weights[i + blurRadius];
	}

	// Divide by the sum so all the weights add up to 1.0.
	for (int i = 0; i < weights.size(); ++i)
	{
		weights[i] /= weightSum;
	}

	return weights;
}
//——————————————————————————————————————————————————————


XMVECTOR g_v4EyePos = XMVectorSet(0.0f, 2.0f, -15.0f, 0.0f);
XMVECTOR g_v4LookAt = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);
XMVECTOR g_v4UpDir = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

double g_fPalstance = 80.0f * XM_PI / 180.0f;



LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR    lpCmdLine, int nCmdShow)
{
	::CoInitialize(nullptr);  //for WIC & COM

	const UINT nFrameBackBufCount = 3u;

	int									iWidth = 1024;
	int									iHeight = 768;
	UINT								nFrameIndex = 0;

	DXGI_FORMAT				emRenderTarget = DXGI_FORMAT_R8G8B8A8_UNORM;
	const float						faClearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	//————————————————————————————————————————————————————————
	//作为清空屏幕的颜色，以及render target的clear value。
	const float						f4RTTexClearColor[] = { 0.8f, 0.8f, 0.8f, 0.0f };
	//————————————————————————————————————————————————————————

	UINT								nDXGIFactoryFlags = 0U;
	UINT								nRTVDescriptorSize = 0U;
	UINT								nSRVDescriptorSize = 0U;
	UINT64								n64UploadBufferSize = 0;
	HWND								hWnd = nullptr;
	MSG									msg = {};
	TCHAR								pszAppPath[MAX_PATH] = {};

	float								fAspectRatio = 3.0f;
	float								fBoxSize = 3.0f;
	float								fTCMax = 1.0f;
	D3D12_VERTEX_BUFFER_VIEW			stVertexBufferView = {};
	D3D12_INDEX_BUFFER_VIEW				stIndexBufferView = {};
	UINT64								n64FenceValue = 0ui64;
	HANDLE								hEventFence = nullptr;
	ST_GRS_FRAME_MVP_BUFFER* pMVPBufferArr[3] = { 0 };

	SIZE_T								szMVPBuffer = GRS_UPPER(sizeof(ST_GRS_FRAME_MVP_BUFFER), 256);

	UINT								nTextureW = 0u;
	UINT								nTextureH = 0u;
	UINT								nBPP = 0u;
	UINT								nPicRowPitch = 0;
	DXGI_FORMAT							stTextureFormat = DXGI_FORMAT_UNKNOWN;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT	stTxtLayouts = {};
	D3D12_VIEWPORT						stViewPort = { 0.0f, 0.0f, static_cast<float>(iWidth), static_cast<float>(iHeight), D3D12_MIN_DEPTH, D3D12_MAX_DEPTH };
	D3D12_RECT							stScissorRect = { 0, 0, static_cast<LONG>(iWidth), static_cast<LONG>(iHeight) };

	ComPtr<IDXGIFactory5>				pIDXGIFactory5;
	ComPtr<IDXGIAdapter1>				pIAdapter1;
	ComPtr<ID3D12Device4>				pID3D12Device4;
	ComPtr<ID3D12CommandQueue>			pICMDQueue;

	ComPtr<IDXGISwapChain1>				pISwapChain1;
	ComPtr<IDXGISwapChain3>				pISwapChain3;
	ComPtr<ID3D12DescriptorHeap>		pIRTVHeap;
	ComPtr<ID3D12DescriptorHeap>		pISRVHeap;
	ComPtr<ID3D12Resource>				pIARenderTargets[nFrameBackBufCount];



	ComPtr<ID3D12Heap>					pITextureHeap;
	ComPtr<ID3D12Heap>					pIUploadHeap;
	ComPtr<ID3D12Resource>				pITexture;
	ComPtr<ID3D12Resource>				pITextureUpload;
	ComPtr<ID3D12Resource>			    pICBVUploadArr[3];



	ComPtr<ID3D12CommandAllocator>		pICMDAllocArr[3];
	ComPtr<ID3D12GraphicsCommandList>	pICMDListArr[3];

	ComPtr<ID3D12RootSignature>			pIRootSignature;
	ComPtr<ID3D12RootSignature>	 mPostProcessRootSignature;

	ComPtr<ID3D12PipelineState>			pIPipelineState;



	ComPtr<ID3D12PipelineState> pIHorzBlurPSO;
	ComPtr<ID3D12PipelineState> pIVertBlurPSO;


	ComPtr<ID3D12Resource>				pIVertexBuffer;
	ComPtr<ID3D12Resource>				pIIndexBuffer;

	ComPtr<ID3D12Resource>				pIBlurMap0;
	ComPtr<ID3D12Resource>				pIBlurMap1;

	ComPtr<ID3D12Fence>					pIFence;

	ComPtr<IWICImagingFactory>			pIWICFactory;
	ComPtr<IWICBitmapDecoder>			pIWICDecoder;
	ComPtr<IWICBitmapFrameDecode>		pIWICFrame;
	ComPtr<IWICBitmapSource>			pIBMP;

	D3D12_RESOURCE_DESC					stTextureDesc = {};

	UINT								nVertexCnt = 0;
	try
	{
		//1、获取文件路径
		{
			if (0 == ::GetModuleFileName(nullptr, pszAppPath, MAX_PATH))
			{
				GRS_THROW_IF_FAILED(HRESULT_FROM_WIN32(GetLastError()));
			}

			WCHAR* lastSlash = _tcsrchr(pszAppPath, _T('\\'));
			if (lastSlash)
			{
				*(lastSlash) = _T('\0');
			}

			lastSlash = _tcsrchr(pszAppPath, _T('\\'));
			if (lastSlash)
			{
				*(lastSlash) = _T('\0');
			}

			lastSlash = _tcsrchr(pszAppPath, _T('\\'));
			if (lastSlash)
			{
				*(lastSlash + 1) = _T('\0');
			}
		}
		//2、创建窗口
		{
			WNDCLASSEX wcex = {};
			wcex.cbSize = sizeof(WNDCLASSEX);
			wcex.style = CS_GLOBALCLASS;
			wcex.lpfnWndProc = WndProc;
			wcex.cbClsExtra = 0;
			wcex.cbWndExtra = 0;
			wcex.hInstance = hInstance;
			wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
			wcex.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
			wcex.lpszClassName = GRS_WND_CLASS_NAME;
			RegisterClassEx(&wcex);

			DWORD dwWndStyle = WS_OVERLAPPED | WS_SYSMENU;
			RECT rtWnd = { 0, 0, iWidth, iHeight };
			AdjustWindowRect(&rtWnd, dwWndStyle, FALSE);

			INT posX = (GetSystemMetrics(SM_CXSCREEN) - rtWnd.right - rtWnd.left) / 2;
			INT posY = (GetSystemMetrics(SM_CYSCREEN) - rtWnd.bottom - rtWnd.top) / 2;

			hWnd = CreateWindowW(GRS_WND_CLASS_NAME
				, GRS_WND_TITLE
				, dwWndStyle
				, posX
				, posY
				, rtWnd.right - rtWnd.left
				, rtWnd.bottom - rtWnd.top
				, nullptr
				, nullptr
				, hInstance
				, nullptr);

			if (!hWnd)
			{
				return FALSE;
			}
		}
		//3、加载纹理
		{
			nPicRowPitch = UploadTexture(pIBMP, pIWICFactory, pIWICDecoder, pIWICFrame, stTextureFormat, nTextureW, nTextureH, nBPP, pszAppPath);
		}
		//4、开启调试层
		{
#if defined(_DEBUG)
			{
				ComPtr<ID3D12Debug> debugController;
				if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
				{
					debugController->EnableDebugLayer();
					nDXGIFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
				}
			}
#endif
		}
		//5、创建DXGIFactory
		{
			GRS_THROW_IF_FAILED(CreateDXGIFactory2(nDXGIFactoryFlags, IID_PPV_ARGS(pIDXGIFactory5.GetAddressOf())));
		}
		//6、枚举适配器并创建Device
		{
			DXGI_ADAPTER_DESC1 stAdapterDesc = {};
			for (UINT nAdapterIndex = 0; DXGI_ERROR_NOT_FOUND != pIDXGIFactory5->EnumAdapters1(nAdapterIndex, &pIAdapter1); ++nAdapterIndex)
			{
				pIAdapter1->GetDesc1(&stAdapterDesc);

				if (stAdapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				{
					continue;
				}

				if (SUCCEEDED(D3D12CreateDevice(pIAdapter1.Get(), D3D_FEATURE_LEVEL_12_1, _uuidof(ID3D12Device), nullptr)))
				{
					break;
				}
			}
			GRS_THROW_IF_FAILED(D3D12CreateDevice(pIAdapter1.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&pID3D12Device4)));

			TCHAR pszWndTitle[MAX_PATH] = {};
			GRS_THROW_IF_FAILED(pIAdapter1->GetDesc1(&stAdapterDesc));
			::GetWindowText(hWnd, pszWndTitle, MAX_PATH);
			StringCchPrintf(pszWndTitle
				, MAX_PATH
				, _T("%s (GPU:%s)")
				, pszWndTitle
				, stAdapterDesc.Description);
			::SetWindowText(hWnd, pszWndTitle);

			nRTVDescriptorSize = pID3D12Device4->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			nSRVDescriptorSize = pID3D12Device4->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		}
		//7、创建命令队列
		{
			D3D12_COMMAND_QUEUE_DESC stQueueDesc = {};
			stQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
			GRS_THROW_IF_FAILED(pID3D12Device4->CreateCommandQueue(&stQueueDesc, IID_PPV_ARGS(&pICMDQueue)));
		}
		//8、创建命令分配器和命令列表
		{
			for (int i = 0; i < 3; i++) {
				GRS_THROW_IF_FAILED(pID3D12Device4->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT
					, IID_PPV_ARGS(&pICMDAllocArr[i])));
				GRS_THROW_IF_FAILED(pID3D12Device4->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT
					, pICMDAllocArr[i].Get(), pIPipelineState.Get(), IID_PPV_ARGS(&pICMDListArr[i])));
			}
		}
		//9、创建交换链
		{
			DXGI_SWAP_CHAIN_DESC1 stSwapChainDesc = {};
			stSwapChainDesc.BufferCount = nFrameBackBufCount;
			stSwapChainDesc.Width = iWidth;
			stSwapChainDesc.Height = iHeight;
			stSwapChainDesc.Format = emRenderTarget;
			stSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			stSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
			stSwapChainDesc.SampleDesc.Count = 1;

			GRS_THROW_IF_FAILED(pIDXGIFactory5->CreateSwapChainForHwnd(
				pICMDQueue.Get(),
				hWnd,
				&stSwapChainDesc,
				nullptr,
				nullptr,
				&pISwapChain1
			));

			GRS_THROW_IF_FAILED(pISwapChain1.As(&pISwapChain3));
			nFrameIndex = pISwapChain3->GetCurrentBackBufferIndex();

			D3D12_DESCRIPTOR_HEAP_DESC stRTVHeapDesc = {};
			//————————————————————————————————————————————————————
			//这里必须多一个描述符对的坑位，用来放pIBlurMap0的rtv描述符。以便它能够用来作为渲染目标
			stRTVHeapDesc.NumDescriptors = nFrameBackBufCount + 1;
			//————————————————————————————————————————————————————

			stRTVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			stRTVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

			GRS_THROW_IF_FAILED(pID3D12Device4->CreateDescriptorHeap(&stRTVHeapDesc, IID_PPV_ARGS(&pIRTVHeap)));
			D3D12_CPU_DESCRIPTOR_HANDLE stRTVHandle = pIRTVHeap->GetCPUDescriptorHandleForHeapStart();
			for (UINT i = 0; i < nFrameBackBufCount; i++)
			{
				GRS_THROW_IF_FAILED(pISwapChain3->GetBuffer(i, IID_PPV_ARGS(&pIARenderTargets[i])));
				pID3D12Device4->CreateRenderTargetView(pIARenderTargets[i].Get(), nullptr, stRTVHandle);
				stRTVHandle.ptr += nRTVDescriptorSize;
			}

			GRS_THROW_IF_FAILED(pIDXGIFactory5->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));
		}
		//10、创建描述符堆
		{
			D3D12_DESCRIPTOR_HEAP_DESC stSRVHeapDesc = {};
			stSRVHeapDesc.NumDescriptors = 8;
			stSRVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			stSRVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

			GRS_THROW_IF_FAILED(pID3D12Device4->CreateDescriptorHeap(&stSRVHeapDesc, IID_PPV_ARGS(&pISRVHeap)));

		}
		//11、创建根签名
		{
			D3D12_FEATURE_DATA_ROOT_SIGNATURE stFeatureData = {};
			stFeatureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
			if (FAILED(pID3D12Device4->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &stFeatureData, sizeof(stFeatureData))))
			{
				stFeatureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
			}
			D3D12_DESCRIPTOR_RANGE1 stDSPRanges1[2] = {};
			stDSPRanges1[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			stDSPRanges1[0].NumDescriptors = 1;
			stDSPRanges1[0].BaseShaderRegister = 0;
			stDSPRanges1[0].RegisterSpace = 0;
			stDSPRanges1[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
			stDSPRanges1[0].OffsetInDescriptorsFromTableStart = 0;
			stDSPRanges1[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
			stDSPRanges1[1].NumDescriptors = 1;
			stDSPRanges1[1].BaseShaderRegister = 0;
			stDSPRanges1[1].RegisterSpace = 0;
			stDSPRanges1[1].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
			stDSPRanges1[1].OffsetInDescriptorsFromTableStart = 0;



			D3D12_ROOT_PARAMETER1 stRootParameters1[2] = {};
			stRootParameters1[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			stRootParameters1[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
			stRootParameters1[0].DescriptorTable.NumDescriptorRanges = 1;
			stRootParameters1[0].DescriptorTable.pDescriptorRanges = &stDSPRanges1[0];
			stRootParameters1[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			stRootParameters1[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
			stRootParameters1[1].DescriptorTable.NumDescriptorRanges = 1;
			stRootParameters1[1].DescriptorTable.pDescriptorRanges = &stDSPRanges1[1];


			D3D12_STATIC_SAMPLER_DESC stSamplerDesc[1] = {};
			stSamplerDesc[0].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
			stSamplerDesc[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
			stSamplerDesc[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
			stSamplerDesc[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
			stSamplerDesc[0].MipLODBias = 0;
			stSamplerDesc[0].MaxAnisotropy = 0;
			stSamplerDesc[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
			stSamplerDesc[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
			stSamplerDesc[0].MinLOD = 0.0f;
			stSamplerDesc[0].MaxLOD = D3D12_FLOAT32_MAX;
			stSamplerDesc[0].ShaderRegister = 0;
			stSamplerDesc[0].RegisterSpace = 0;
			stSamplerDesc[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			D3D12_VERSIONED_ROOT_SIGNATURE_DESC stRootSignatureDesc = {};
			stRootSignatureDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
			stRootSignatureDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
			stRootSignatureDesc.Desc_1_1.NumParameters = _countof(stRootParameters1);
			stRootSignatureDesc.Desc_1_1.pParameters = stRootParameters1;
			stRootSignatureDesc.Desc_1_1.NumStaticSamplers = _countof(stSamplerDesc);
			stRootSignatureDesc.Desc_1_1.pStaticSamplers = stSamplerDesc;

			ComPtr<ID3DBlob> pISignatureBlob;
			ComPtr<ID3DBlob> pIErrorBlob;
			GRS_THROW_IF_FAILED(D3D12SerializeVersionedRootSignature(&stRootSignatureDesc
				, &pISignatureBlob
				, &pIErrorBlob));

			GRS_THROW_IF_FAILED(pID3D12Device4->CreateRootSignature(0
				, pISignatureBlob->GetBufferPointer()
				, pISignatureBlob->GetBufferSize()
				, IID_PPV_ARGS(&pIRootSignature)));





			//————————————————————————————————————————————————————————————
			//创建compute shader使用的根签名，三个根参数：一个是blur半径+blur权重的根常量，另外俩是两个资源view对应的描述符表
			CD3DX12_DESCRIPTOR_RANGE srvTable;
			srvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

			CD3DX12_DESCRIPTOR_RANGE uavTable;
			uavTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);

			CD3DX12_ROOT_PARAMETER slotRootParameter[3];

			slotRootParameter[0].InitAsConstants(12, 0);
			slotRootParameter[1].InitAsDescriptorTable(1, &srvTable);
			slotRootParameter[2].InitAsDescriptorTable(1, &uavTable);

			CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(3, slotRootParameter,
				0, nullptr,
				D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

			ComPtr<ID3DBlob> serializedRootSig = nullptr;
			GRS_THROW_IF_FAILED(D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
				serializedRootSig.GetAddressOf(), nullptr));

			GRS_THROW_IF_FAILED(pID3D12Device4->CreateRootSignature(
				0,
				serializedRootSig->GetBufferPointer(),
				serializedRootSig->GetBufferSize(),
				IID_PPV_ARGS(mPostProcessRootSignature.GetAddressOf())));

			//————————————————————————————————————————————————————————————

		}
		//12、创建PSO
		{
			ComPtr<ID3DBlob> pIBlobVertexShader;
			ComPtr<ID3DBlob> pIBlobPixelShader;
#if defined(_DEBUG)
			UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
			UINT compileFlags = 0;
#endif

			compileFlags |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;

			TCHAR pszShaderFileName[MAX_PATH] = {};
			StringCchPrintf(pszShaderFileName, MAX_PATH, _T("%sShader\\shaders.hlsl"), pszAppPath);
			GRS_THROW_IF_FAILED(D3DCompileFromFile(pszShaderFileName, nullptr, nullptr
				, "VSMain", "vs_5_0", compileFlags, 0, &pIBlobVertexShader, nullptr));
			GRS_THROW_IF_FAILED(D3DCompileFromFile(pszShaderFileName, nullptr, nullptr
				, "PSMain", "ps_5_0", compileFlags, 0, &pIBlobPixelShader, nullptr));

			D3D12_INPUT_ELEMENT_DESC stInputElementDescs[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
			};

			D3D12_GRAPHICS_PIPELINE_STATE_DESC stPSODesc = {};
			stPSODesc.InputLayout = { stInputElementDescs, _countof(stInputElementDescs) };
			stPSODesc.pRootSignature = pIRootSignature.Get();

			stPSODesc.VS.pShaderBytecode = pIBlobVertexShader->GetBufferPointer();
			stPSODesc.VS.BytecodeLength = pIBlobVertexShader->GetBufferSize();

			stPSODesc.PS.pShaderBytecode = pIBlobPixelShader->GetBufferPointer();
			stPSODesc.PS.BytecodeLength = pIBlobPixelShader->GetBufferSize();

			stPSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
			stPSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;

			stPSODesc.BlendState.AlphaToCoverageEnable = FALSE;
			stPSODesc.BlendState.IndependentBlendEnable = FALSE;
			stPSODesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

			stPSODesc.DepthStencilState.DepthEnable = FALSE;
			stPSODesc.DepthStencilState.StencilEnable = FALSE;

			stPSODesc.SampleMask = UINT_MAX;
			stPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			stPSODesc.NumRenderTargets = 1;
			stPSODesc.RTVFormats[0] = emRenderTarget;
			stPSODesc.SampleDesc.Count = 1;

			GRS_THROW_IF_FAILED(pID3D12Device4->CreateGraphicsPipelineState(&stPSODesc
				, IID_PPV_ARGS(&pIPipelineState)));



			StringCchPrintf(pszShaderFileName, MAX_PATH, _T("%sShader\\Blur.hlsl"), pszAppPath);
			ComPtr<ID3DBlob> pIBlobComputeShaderHorizontal;
			ComPtr<ID3DBlob> pIBlobComputeShaderVertical;

			GRS_THROW_IF_FAILED(D3DCompileFromFile(pszShaderFileName, nullptr, nullptr
				, "HorzBlurCS", "cs_5_0", compileFlags, 0, &pIBlobComputeShaderHorizontal, nullptr));
			GRS_THROW_IF_FAILED(D3DCompileFromFile(pszShaderFileName, nullptr, nullptr
				, "VertBlurCS", "cs_5_0", compileFlags, 0, &pIBlobComputeShaderVertical, nullptr));



			D3D12_COMPUTE_PIPELINE_STATE_DESC horzBlurPSO = {};
			horzBlurPSO.pRootSignature = mPostProcessRootSignature.Get();
			horzBlurPSO.CS =
			{
				pIBlobComputeShaderHorizontal->GetBufferPointer(),
				pIBlobComputeShaderHorizontal->GetBufferSize()
			};
			horzBlurPSO.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
			GRS_THROW_IF_FAILED(pID3D12Device4->CreateComputePipelineState(&horzBlurPSO, IID_PPV_ARGS(&pIHorzBlurPSO)));


			D3D12_COMPUTE_PIPELINE_STATE_DESC vertBlurPSO = {};
			vertBlurPSO.pRootSignature = mPostProcessRootSignature.Get();
			vertBlurPSO.CS =
			{
				pIBlobComputeShaderVertical->GetBufferPointer(),
				pIBlobComputeShaderVertical->GetBufferSize()
			};
			vertBlurPSO.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
			GRS_THROW_IF_FAILED(pID3D12Device4->CreateComputePipelineState(&vertBlurPSO, IID_PPV_ARGS(&pIVertBlurPSO)));

		}
		//13、创建默认堆
		{
			D3D12_HEAP_DESC stTextureHeapDesc = {};
			stTextureHeapDesc.SizeInBytes = GRS_UPPER(2 * nPicRowPitch * nTextureH, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
			stTextureHeapDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
			stTextureHeapDesc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
			stTextureHeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			stTextureHeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			stTextureHeapDesc.Flags = D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES | D3D12_HEAP_FLAG_DENY_BUFFERS;

			GRS_THROW_IF_FAILED(pID3D12Device4->CreateHeap(&stTextureHeapDesc, IID_PPV_ARGS(&pITextureHeap)));

		}
		//14、创建默认堆资源
		{

			stTextureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			stTextureDesc.MipLevels = 1;
			stTextureDesc.Format = stTextureFormat;
			stTextureDesc.Width = nTextureW;
			stTextureDesc.Height = nTextureH;
			stTextureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			stTextureDesc.DepthOrArraySize = 1;
			stTextureDesc.SampleDesc.Count = 1;
			stTextureDesc.SampleDesc.Quality = 0;


			GRS_THROW_IF_FAILED(pID3D12Device4->CreatePlacedResource(
				pITextureHeap.Get()
				, 0
				, &stTextureDesc
				, D3D12_RESOURCE_STATE_COPY_DEST
				, nullptr
				, IID_PPV_ARGS(&pITexture)));

			D3D12_RESOURCE_DESC stCopyDstDesc = pITexture->GetDesc();
			n64UploadBufferSize = 0;
			pID3D12Device4->GetCopyableFootprints(&stCopyDstDesc, 0, 1, 0, nullptr, nullptr, nullptr, &n64UploadBufferSize);



			D3D12_RESOURCE_DESC texDesc;
			ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
			texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			texDesc.Alignment = 0;
			texDesc.Width = iWidth;
			texDesc.Height = iHeight;
			texDesc.DepthOrArraySize = 1;
			texDesc.MipLevels = 1;
			texDesc.Format = emRenderTarget;
			texDesc.SampleDesc.Count = 1;
			texDesc.SampleDesc.Quality = 0;
			texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			//————————————————————————————————————————————————————
			//修改我们创建的资源的flag，用于render target。
			texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
			//————————————————————————————————————————————————————

			D3D12_CLEAR_VALUE stClear = {};
			stClear.Format = emRenderTarget;
			memcpy(&stClear.Color, &f4RTTexClearColor, 4 * sizeof(float));


			D3D12_HEAP_PROPERTIES stHeapProp = { D3D12_HEAP_TYPE_DEFAULT };

			GRS_THROW_IF_FAILED(pID3D12Device4->CreateCommittedResource(
				&stHeapProp,
				D3D12_HEAP_FLAG_NONE,
				&texDesc,
				D3D12_RESOURCE_STATE_COMMON,
				&stClear,
				IID_PPV_ARGS(&pIBlurMap0)));
			//————————————————————————————————————————————————————
			//第二个资源并不需要作为渲染目标，所以这里也不必加渲染目标的flag。
			texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
			//————————————————————————————————————————————————————
			GRS_THROW_IF_FAILED(pID3D12Device4->CreateCommittedResource(
				&stHeapProp,
				D3D12_HEAP_FLAG_NONE,
				&texDesc,
				D3D12_RESOURCE_STATE_COMMON,
				nullptr,
				IID_PPV_ARGS(&pIBlurMap1)));


		}
		//15、创建上传堆
		{
			D3D12_HEAP_DESC stUploadHeapDesc = {  };
			stUploadHeapDesc.SizeInBytes = GRS_UPPER(2 * n64UploadBufferSize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
			stUploadHeapDesc.Alignment = 0;
			stUploadHeapDesc.Properties.Type = D3D12_HEAP_TYPE_UPLOAD;
			stUploadHeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			stUploadHeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			stUploadHeapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

			GRS_THROW_IF_FAILED(pID3D12Device4->CreateHeap(&stUploadHeapDesc, IID_PPV_ARGS(&pIUploadHeap)));


		}
		//16、创建上传堆资源
		{
			D3D12_RESOURCE_DESC stUploadResDesc = {};
			stUploadResDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			stUploadResDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
			stUploadResDesc.Width = n64UploadBufferSize;
			stUploadResDesc.Height = 1;
			stUploadResDesc.DepthOrArraySize = 1;
			stUploadResDesc.MipLevels = 1;
			stUploadResDesc.Format = DXGI_FORMAT_UNKNOWN;
			stUploadResDesc.SampleDesc.Count = 1;
			stUploadResDesc.SampleDesc.Quality = 0;
			stUploadResDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			stUploadResDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

			GRS_THROW_IF_FAILED(pID3D12Device4->CreatePlacedResource(pIUploadHeap.Get()
				, 0
				, &stUploadResDesc
				, D3D12_RESOURCE_STATE_GENERIC_READ
				, nullptr
				, IID_PPV_ARGS(&pITextureUpload)));
		}
		//17、将纹理资源从系统内存拷贝到上传堆
		{
			stTxtLayouts = CopyToUploadHeap(pITexture, pITextureUpload, pIBMP, pID3D12Device4, n64UploadBufferSize, nTextureH, nPicRowPitch);
		}
		//18、将上传堆资源拷贝到默认堆
		{
			D3D12_TEXTURE_COPY_LOCATION stDstCopyLocation = {};
			stDstCopyLocation.pResource = pITexture.Get();
			stDstCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			stDstCopyLocation.SubresourceIndex = 0;

			D3D12_TEXTURE_COPY_LOCATION stSrcCopyLocation = {};
			stSrcCopyLocation.pResource = pITextureUpload.Get();
			stSrcCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			stSrcCopyLocation.PlacedFootprint = stTxtLayouts;

			pICMDListArr[0]->CopyTextureRegion(&stDstCopyLocation, 0, 0, 0, &stSrcCopyLocation, nullptr);

			D3D12_RESOURCE_BARRIER stResBar = {};
			stResBar.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			stResBar.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			stResBar.Transition.pResource = pITexture.Get();
			stResBar.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			stResBar.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			stResBar.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

			pICMDListArr[0]->ResourceBarrier(1, &stResBar);


			GRS_THROW_IF_FAILED(pICMDListArr[0]->Close());
			ID3D12CommandList* ppCommandLists[] = { pICMDListArr[0].Get() };
			pICMDQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);


			GRS_THROW_IF_FAILED(pID3D12Device4->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pIFence)));
			n64FenceValue = 1;


			hEventFence = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			if (hEventFence == nullptr)
			{
				GRS_THROW_IF_FAILED(HRESULT_FROM_WIN32(GetLastError()));
			}

			const UINT64 fence = n64FenceValue;
			GRS_THROW_IF_FAILED(pICMDQueue->Signal(pIFence.Get(), fence));
			n64FenceValue++;
			GRS_THROW_IF_FAILED(pIFence->SetEventOnCompletion(fence, hEventFence));

			WaitForSingleObject(hEventFence, INFINITE);
			GRS_THROW_IF_FAILED(pICMDAllocArr[0]->Reset());
			GRS_THROW_IF_FAILED(pICMDListArr[0]->Reset(pICMDAllocArr[0].Get(), pIPipelineState.Get()));
		}
		//19、顶点及索引数据
		ST_GRS_VERTEX stTriangleVertices[] = {
			{ {-1.0f * fBoxSize,  1.0f * fBoxSize, -1.0f * fBoxSize, 1.0f}, {0.0f * fTCMax, 0.0f * fTCMax}},
			{ {1.0f * fBoxSize,  1.0f * fBoxSize, -1.0f * fBoxSize, 1.0f}, {1.0f * fTCMax, 0.0f * fTCMax} },
			{ {-1.0f * fBoxSize, -1.0f * fBoxSize, -1.0f * fBoxSize, 1.0f}, {0.0f * fTCMax, 1.0f * fTCMax} },
			{ {-1.0f * fBoxSize, -1.0f * fBoxSize, -1.0f * fBoxSize, 1.0f}, {0.0f * fTCMax, 1.0f * fTCMax} },
			{ {1.0f * fBoxSize,  1.0f * fBoxSize, -1.0f * fBoxSize, 1.0f}, {1.0f * fTCMax, 0.0f * fTCMax}},
			{ {1.0f * fBoxSize, -1.0f * fBoxSize, -1.0f * fBoxSize, 1.0f}, {1.0f * fTCMax, 1.0f * fTCMax} },
			{ {1.0f * fBoxSize,  1.0f * fBoxSize, -1.0f * fBoxSize, 1.0f}, {0.0f * fTCMax, 0.0f * fTCMax} },
			{ {1.0f * fBoxSize,  1.0f * fBoxSize,  1.0f * fBoxSize, 1.0f}, {1.0f * fTCMax, 0.0f * fTCMax} },
			{ {1.0f * fBoxSize, -1.0f * fBoxSize, -1.0f * fBoxSize, 1.0f}, {0.0f * fTCMax, 1.0f * fTCMax} },
			{ {1.0f * fBoxSize, -1.0f * fBoxSize, -1.0f * fBoxSize, 1.0f}, {0.0f * fTCMax, 1.0f * fTCMax} },
			{ {1.0f * fBoxSize,  1.0f * fBoxSize,  1.0f * fBoxSize, 1.0f}, {1.0f * fTCMax, 0.0f * fTCMax} },
			{ {1.0f * fBoxSize, -1.0f * fBoxSize,  1.0f * fBoxSize, 1.0f}, {1.0f * fTCMax, 1.0f * fTCMax} },
			{ {1.0f * fBoxSize,  1.0f * fBoxSize,  1.0f * fBoxSize, 1.0f}, {0.0f * fTCMax, 0.0f * fTCMax}},
			{ {-1.0f * fBoxSize,  1.0f * fBoxSize,  1.0f * fBoxSize, 1.0f}, {1.0f * fTCMax, 0.0f * fTCMax}},
			{ {1.0f * fBoxSize, -1.0f * fBoxSize,  1.0f * fBoxSize, 1.0f}, {0.0f * fTCMax, 1.0f * fTCMax} },
			{ {1.0f * fBoxSize, -1.0f * fBoxSize,  1.0f * fBoxSize, 1.0f}, {0.0f * fTCMax, 1.0f * fTCMax}},
			{ {-1.0f * fBoxSize,  1.0f * fBoxSize,  1.0f * fBoxSize, 1.0f}, {1.0f * fTCMax, 0.0f * fTCMax}},
			{ {-1.0f * fBoxSize, -1.0f * fBoxSize,  1.0f * fBoxSize, 1.0f}, {1.0f * fTCMax, 1.0f * fTCMax}},
			{ {-1.0f * fBoxSize,  1.0f * fBoxSize,  1.0f * fBoxSize, 1.0f}, {0.0f * fTCMax, 0.0f * fTCMax}},
			{ {-1.0f * fBoxSize,  1.0f * fBoxSize, -1.0f * fBoxSize, 1.0f}, {1.0f * fTCMax, 0.0f * fTCMax}},
			{ {-1.0f * fBoxSize, -1.0f * fBoxSize,  1.0f * fBoxSize, 1.0f}, {0.0f * fTCMax, 1.0f * fTCMax}},
			{ {-1.0f * fBoxSize, -1.0f * fBoxSize,  1.0f * fBoxSize, 1.0f}, {0.0f * fTCMax, 1.0f * fTCMax}},
			{ {-1.0f * fBoxSize,  1.0f * fBoxSize, -1.0f * fBoxSize, 1.0f}, {1.0f * fTCMax, 0.0f * fTCMax}},
			{ {-1.0f * fBoxSize, -1.0f * fBoxSize, -1.0f * fBoxSize, 1.0f}, {1.0f * fTCMax, 1.0f * fTCMax}},
			{ {-1.0f * fBoxSize,  1.0f * fBoxSize,  1.0f * fBoxSize, 1.0f}, {0.0f * fTCMax, 0.0f * fTCMax} },
			{ {1.0f * fBoxSize,  1.0f * fBoxSize,  1.0f * fBoxSize, 1.0f}, {1.0f * fTCMax, 0.0f * fTCMax} },
			{ {-1.0f * fBoxSize,  1.0f * fBoxSize, -1.0f * fBoxSize, 1.0f}, {0.0f * fTCMax, 1.0f * fTCMax} },
			{ {-1.0f * fBoxSize,  1.0f * fBoxSize, -1.0f * fBoxSize, 1.0f}, {0.0f * fTCMax, 1.0f * fTCMax} },
			{ {1.0f * fBoxSize,  1.0f * fBoxSize,  1.0f * fBoxSize, 1.0f}, {1.0f * fTCMax, 0.0f * fTCMax} },
			{ {1.0f * fBoxSize,  1.0f * fBoxSize, -1.0f * fBoxSize, 1.0f}, {1.0f * fTCMax, 1.0f * fTCMax}},
			{ {-1.0f * fBoxSize, -1.0f * fBoxSize, -1.0f * fBoxSize, 1.0f}, {0.0f * fTCMax, 0.0f * fTCMax}},
			{ {1.0f * fBoxSize, -1.0f * fBoxSize, -1.0f * fBoxSize, 1.0f}, {1.0f * fTCMax, 0.0f * fTCMax}},
			{ {-1.0f * fBoxSize, -1.0f * fBoxSize,  1.0f * fBoxSize, 1.0f}, {0.0f * fTCMax, 1.0f * fTCMax}},
			{ {-1.0f * fBoxSize, -1.0f * fBoxSize,  1.0f * fBoxSize, 1.0f}, {0.0f * fTCMax, 1.0f * fTCMax}},
			{ {1.0f * fBoxSize, -1.0f * fBoxSize, -1.0f * fBoxSize, 1.0f}, {1.0f * fTCMax, 0.0f * fTCMax}},
			{ {1.0f * fBoxSize, -1.0f * fBoxSize,  1.0f * fBoxSize, 1.0f}, {1.0f * fTCMax, 1.0f * fTCMax}},
		};
		UINT32 pBoxIndices[] = {
			0,1,2,
			3,4,5,

			6,7,8,
			9,10,11,

			12,13,14,
			15,16,17,

			18,19,20,
			21,22,23,

			24,25,26,
			27,28,29,

			30,31,32,
			33,34,35,
		};
		//20、创建顶点缓冲区
		{

			const UINT nVertexBufferSize = sizeof(stTriangleVertices);

			D3D12_HEAP_PROPERTIES stHeapProp = { D3D12_HEAP_TYPE_UPLOAD };

			D3D12_RESOURCE_DESC stResSesc = {};
			stResSesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			stResSesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			stResSesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			stResSesc.Format = DXGI_FORMAT_UNKNOWN;
			stResSesc.Width = nVertexBufferSize;
			stResSesc.Height = 1;
			stResSesc.DepthOrArraySize = 1;
			stResSesc.MipLevels = 1;
			stResSesc.SampleDesc.Count = 1;
			stResSesc.SampleDesc.Quality = 0;

			GRS_THROW_IF_FAILED(pID3D12Device4->CreateCommittedResource(
				&stHeapProp,
				D3D12_HEAP_FLAG_NONE,
				&stResSesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&pIVertexBuffer)));

			UINT8* pVertexDataBegin = nullptr;
			D3D12_RANGE stReadRange = { 0, 0 };		// We do not intend to read from this resource on the CPU.
			GRS_THROW_IF_FAILED(pIVertexBuffer->Map(0, &stReadRange, reinterpret_cast<void**>(&pVertexDataBegin)));
			memcpy(pVertexDataBegin, stTriangleVertices, sizeof(stTriangleVertices));
			pIVertexBuffer->Unmap(0, nullptr);

			stVertexBufferView.BufferLocation = pIVertexBuffer->GetGPUVirtualAddress();
			stVertexBufferView.StrideInBytes = sizeof(ST_GRS_VERTEX);
			stVertexBufferView.SizeInBytes = nVertexBufferSize;
		}
		//21、创建索引缓冲区
		{

			const UINT nszIndexBuffer = sizeof(pBoxIndices);


			D3D12_HEAP_PROPERTIES stHeapProp = { D3D12_HEAP_TYPE_UPLOAD };

			D3D12_RESOURCE_DESC stIBResDesc = {};
			stIBResDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			stIBResDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
			stIBResDesc.Width = nszIndexBuffer;
			stIBResDesc.Height = 1;
			stIBResDesc.DepthOrArraySize = 1;
			stIBResDesc.MipLevels = 1;
			stIBResDesc.Format = DXGI_FORMAT_UNKNOWN;
			stIBResDesc.SampleDesc.Count = 1;
			stIBResDesc.SampleDesc.Quality = 0;
			stIBResDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			stIBResDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

			GRS_THROW_IF_FAILED(pID3D12Device4->CreateCommittedResource(
				&stHeapProp,
				D3D12_HEAP_FLAG_NONE,
				&stIBResDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&pIIndexBuffer)));

			UINT8* pIndexDataBegin = nullptr;
			D3D12_RANGE stReadRange = { 0, 0 };		// We do not intend to read from this resource on the CPU.
			GRS_THROW_IF_FAILED(pIIndexBuffer->Map(0, &stReadRange, reinterpret_cast<void**>(&pIndexDataBegin)));
			memcpy(pIndexDataBegin, pBoxIndices, nszIndexBuffer);
			pIIndexBuffer->Unmap(0, nullptr);

			stIndexBufferView.BufferLocation = pIIndexBuffer->GetGPUVirtualAddress();
			stIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
			stIndexBufferView.SizeInBytes = nszIndexBuffer;

		}
		//22、创建常量缓冲区
		{
			UINT64 n64BufferOffset = GRS_UPPER(n64UploadBufferSize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

			D3D12_RESOURCE_DESC stMVPResDesc = {};
			stMVPResDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			stMVPResDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
			stMVPResDesc.Width = szMVPBuffer;
			stMVPResDesc.Height = 1;
			stMVPResDesc.DepthOrArraySize = 1;
			stMVPResDesc.MipLevels = 1;
			stMVPResDesc.Format = DXGI_FORMAT_UNKNOWN;
			stMVPResDesc.SampleDesc.Count = 1;
			stMVPResDesc.SampleDesc.Quality = 0;
			stMVPResDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			stMVPResDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			GRS_THROW_IF_FAILED(pID3D12Device4->CreatePlacedResource(
				pIUploadHeap.Get()
				, n64BufferOffset
				, &stMVPResDesc
				, D3D12_RESOURCE_STATE_GENERIC_READ
				, nullptr
				, IID_PPV_ARGS(&pICBVUploadArr[0])));
			n64BufferOffset = GRS_UPPER(n64BufferOffset + szMVPBuffer, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

			GRS_THROW_IF_FAILED(pID3D12Device4->CreatePlacedResource(
				pIUploadHeap.Get()
				, n64BufferOffset
				, &stMVPResDesc
				, D3D12_RESOURCE_STATE_GENERIC_READ
				, nullptr
				, IID_PPV_ARGS(&pICBVUploadArr[1])));

			n64BufferOffset = GRS_UPPER(n64BufferOffset + szMVPBuffer, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
			GRS_THROW_IF_FAILED(pID3D12Device4->CreatePlacedResource(
				pIUploadHeap.Get()
				, n64BufferOffset
				, &stMVPResDesc
				, D3D12_RESOURCE_STATE_GENERIC_READ
				, nullptr
				, IID_PPV_ARGS(&pICBVUploadArr[2])));

			GRS_THROW_IF_FAILED(pICBVUploadArr[0]->Map(0, nullptr, reinterpret_cast<void**>(&pMVPBufferArr[0])));
			GRS_THROW_IF_FAILED(pICBVUploadArr[1]->Map(0, nullptr, reinterpret_cast<void**>(&pMVPBufferArr[1])));
			GRS_THROW_IF_FAILED(pICBVUploadArr[2]->Map(0, nullptr, reinterpret_cast<void**>(&pMVPBufferArr[2])));

		}
		//23、创建SRV资源视图
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC stSRVDesc = {};
			stSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			stSRVDesc.Format = stTextureDesc.Format;
			stSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			stSRVDesc.Texture2D.MipLevels = 1;
			pID3D12Device4->CreateShaderResourceView(pITexture.Get(), &stSRVDesc, pISRVHeap->GetCPUDescriptorHandleForHeapStart());


			//————————————————————————————————————————————————————————————————
			//这里描述符堆现在的情况：1是SRV，234是CBV，现在56要放pIBlurMap0的SRV和UAV，78要放pIBlurMap1的SRV和UAV
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = emRenderTarget;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MostDetailedMip = 0;
			srvDesc.Texture2D.MipLevels = 1;

			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};

			uavDesc.Format = emRenderTarget;
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			uavDesc.Texture2D.MipSlice = 0;

			CD3DX12_CPU_DESCRIPTOR_HANDLE stCPUHandle(pISRVHeap->GetCPUDescriptorHandleForHeapStart());
			//对句柄进行偏移，然后找对位置创建出相应的view。
			stCPUHandle.Offset(4, nSRVDescriptorSize);
			pID3D12Device4->CreateShaderResourceView(pIBlurMap0.Get(), &srvDesc, stCPUHandle);
			stCPUHandle.Offset(1, nSRVDescriptorSize);
			pID3D12Device4->CreateUnorderedAccessView(pIBlurMap0.Get(), nullptr, &uavDesc, stCPUHandle);
			stCPUHandle.Offset(1, nSRVDescriptorSize);
			pID3D12Device4->CreateShaderResourceView(pIBlurMap1.Get(), &srvDesc, stCPUHandle);
			stCPUHandle.Offset(1, nSRVDescriptorSize);
			pID3D12Device4->CreateUnorderedAccessView(pIBlurMap1.Get(), nullptr, &uavDesc, stCPUHandle);
			//————————————————————————————————————————————————————————————

		}
		//24、创建常量缓冲区视图
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = pICBVUploadArr[0]->GetGPUVirtualAddress();
			cbvDesc.SizeInBytes = static_cast<UINT>(szMVPBuffer);

			D3D12_CPU_DESCRIPTOR_HANDLE stCPUCBVHandle = pISRVHeap->GetCPUDescriptorHandleForHeapStart();
			stCPUCBVHandle.ptr += nSRVDescriptorSize;
			pID3D12Device4->CreateConstantBufferView(&cbvDesc, stCPUCBVHandle);
			cbvDesc.BufferLocation = pICBVUploadArr[1]->GetGPUVirtualAddress();
			stCPUCBVHandle.ptr += nSRVDescriptorSize;
			pID3D12Device4->CreateConstantBufferView(&cbvDesc, stCPUCBVHandle);
			cbvDesc.BufferLocation = pICBVUploadArr[2]->GetGPUVirtualAddress();
			stCPUCBVHandle.ptr += nSRVDescriptorSize;
			pID3D12Device4->CreateConstantBufferView(&cbvDesc, stCPUCBVHandle);
		}

		//————————————————————————————————————————————————————————
		//25、创建RTV资源视图
		{
			//注意需要在创建交换链的部分，创建RTV描述符堆时加一个位置来放置此描述符
			CD3DX12_CPU_DESCRIPTOR_HANDLE stRTVHandle(pIRTVHeap->GetCPUDescriptorHandleForHeapStart());
			stRTVHandle.Offset(3, nRTVDescriptorSize);
			pID3D12Device4->CreateRenderTargetView(pIBlurMap0.Get(), nullptr, stRTVHandle);
		}
		//————————————————————————————————————————————————————————


		//26、创建资源屏障
		D3D12_RESOURCE_BARRIER stBeginResBarrier[3] = {};


		for (int i = 0; i < 3; i++) {
			stBeginResBarrier[i].Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			stBeginResBarrier[i].Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			stBeginResBarrier[i].Transition.pResource = pIARenderTargets[i].Get();
			stBeginResBarrier[i].Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
			stBeginResBarrier[i].Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
			stBeginResBarrier[i].Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		}


		ULONGLONG n64tmFrameStart = ::GetTickCount64();
		ULONGLONG n64tmCurrent = n64tmFrameStart;
		double dModelRotationYAngle = 0.0f;


		ShowWindow(hWnd, nCmdShow);
		UpdateWindow(hWnd);
		UINT64 curFrameResource = 0;
		UINT64 nextFrameResource = 0;
		UINT64 lastFrameResource = 0;
		UINT64 curFenceValue = 1;

		GRS_THROW_IF_FAILED(pICMDListArr[2]->Close());
		GRS_THROW_IF_FAILED(pICMDListArr[1]->Close());
		GRS_THROW_IF_FAILED(pICMDListArr[1]->Reset(pICMDAllocArr[1].Get(), pIPipelineState.Get()));


		UINT blurTimes = 10;


		while (msg.message != WM_QUIT)
		{
			if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{
				//————————————————————————————————————————————————
				// 第一部分：执行当前帧
				// ———————————————————————————————————————————————
				curFrameResource = pISwapChain3->GetCurrentBackBufferIndex();
				GRS_THROW_IF_FAILED(pICMDListArr[curFrameResource]->Close());
				ID3D12CommandList* ppCommandLists[] = { pICMDListArr[curFrameResource].Get() };
				pICMDQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

				GRS_THROW_IF_FAILED(pISwapChain3->Present(0, 0));
				nextFrameResource = (curFrameResource + 1) % 3;
				lastFrameResource = (curFrameResource == 0) ? 2 : (curFrameResource - 1);
				nFrameIndex = nextFrameResource;




				//————————————————————————————————————————————————
				// 第二部分：等待上一帧
				// ———————————————————————————————————————————————
				GRS_THROW_IF_FAILED(pICMDQueue->Signal(pIFence.Get(), curFenceValue));
				GRS_THROW_IF_FAILED(pIFence->SetEventOnCompletion(curFenceValue - 1, hEventFence));
				WaitForSingleObject(hEventFence, INFINITE);
				GRS_THROW_IF_FAILED(pICMDAllocArr[lastFrameResource]->Reset());
				GRS_THROW_IF_FAILED(pICMDListArr[lastFrameResource]->Reset(pICMDAllocArr[lastFrameResource].Get(), pIPipelineState.Get()));
				curFenceValue++;




				//————————————————————————————————————————————————
				// 第三部分：录制下一帧
				// ———————————————————————————————————————————————
				{
					n64tmCurrent = ::GetTickCount64();
					dModelRotationYAngle += ((n64tmCurrent - n64tmFrameStart) / 1000.0f) * g_fPalstance;

					n64tmFrameStart = n64tmCurrent;

					if (dModelRotationYAngle > XM_2PI)
					{
						dModelRotationYAngle = fmod(dModelRotationYAngle, XM_2PI);
					}

					XMMATRIX xmRot = XMMatrixRotationY(static_cast<float>(dModelRotationYAngle));

					XMMATRIX xmMVP = XMMatrixMultiply(xmRot, XMMatrixLookAtLH(g_v4EyePos, g_v4LookAt, g_v4UpDir));

					xmMVP = XMMatrixMultiply(xmMVP, (XMMatrixPerspectiveFovLH(XM_PIDIV4, (FLOAT)iWidth / (FLOAT)iHeight, 0.1f, 1000.0f)));
					XMStoreFloat4x4(&pMVPBufferArr[nextFrameResource]->m_MVP, xmMVP);
				}

				//————————————————————————————————————
				//由于将要渲染到这个pIBlurMap0上，所以这里就进行一个状态转换。
				CD3DX12_RESOURCE_BARRIER stCommonToRT = CD3DX12_RESOURCE_BARRIER::Transition(pIBlurMap0.Get(),
					D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);
				pICMDListArr[nextFrameResource]->ResourceBarrier(1, &stCommonToRT);

				//把pIBlurMap0对应的RTV视图设置为Render target
				CD3DX12_CPU_DESCRIPTOR_HANDLE stRTVHandle(pIRTVHeap->GetCPUDescriptorHandleForHeapStart());
				//移动到我们的纹理的视图，让纹理作为渲染目标
				stRTVHandle.Offset(3, nRTVDescriptorSize);
				pICMDListArr[nextFrameResource]->OMSetRenderTargets(1, &stRTVHandle, FALSE, nullptr);
				//————————————————————————————————————


				pICMDListArr[nextFrameResource]->RSSetViewports(1, &stViewPort);
				pICMDListArr[nextFrameResource]->RSSetScissorRects(1, &stScissorRect);
				pICMDListArr[nextFrameResource]->ClearRenderTargetView(stRTVHandle, f4RTTexClearColor, 0, nullptr);

				pICMDListArr[nextFrameResource]->SetGraphicsRootSignature(pIRootSignature.Get());
				ID3D12DescriptorHeap* ppHeaps[] = { pISRVHeap.Get() };
				pICMDListArr[nextFrameResource]->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

				D3D12_GPU_DESCRIPTOR_HANDLE stGPUSRVHandle = pISRVHeap->GetGPUDescriptorHandleForHeapStart();
				pICMDListArr[nextFrameResource]->SetGraphicsRootDescriptorTable(0, stGPUSRVHandle);

				stGPUSRVHandle.ptr += nSRVDescriptorSize;
				pICMDListArr[nextFrameResource]->SetGraphicsRootDescriptorTable(1, stGPUSRVHandle);


				pICMDListArr[nextFrameResource]->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				pICMDListArr[nextFrameResource]->IASetVertexBuffers(0, 1, &stVertexBufferView);
				pICMDListArr[nextFrameResource]->IASetIndexBuffer(&stIndexBufferView);

				pICMDListArr[nextFrameResource]->DrawIndexedInstanced(_countof(pBoxIndices), 1, 0, 0, 0);



				auto weights = CalcGaussWeights(2.5f);
				int blurRadius = (int)weights.size() / 2;

				pICMDListArr[nextFrameResource]->SetComputeRootSignature(mPostProcessRootSignature.Get());

				pICMDListArr[nextFrameResource]->SetComputeRoot32BitConstants(0, 1, &blurRadius, 0);
				pICMDListArr[nextFrameResource]->SetComputeRoot32BitConstants(0, (UINT)weights.size(), weights.data(), 1);


				//————————————————————————————————————
				//已经渲染到纹理了,pIBlurMap0已经包含渲染管线输出的内容了，所以直接进行高斯blur就好了,
				//pIBlurMap0现在状态时Render target，需要改成Read
				CD3DX12_RESOURCE_BARRIER stUsedForComputeBarrier = CD3DX12_RESOURCE_BARRIER::Transition(pIBlurMap0.Get(),
					D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
				pICMDListArr[nextFrameResource]->ResourceBarrier(1, &stUsedForComputeBarrier);

				CD3DX12_RESOURCE_BARRIER stAcceptInputBarrier = CD3DX12_RESOURCE_BARRIER::Transition(pIBlurMap1.Get(),
					D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
				pICMDListArr[nextFrameResource]->ResourceBarrier(1, &stAcceptInputBarrier);
				//————————————————————————————————————




				for (int i = 0; i < blurTimes; i++) {
					//水平模糊
					CD3DX12_GPU_DESCRIPTOR_HANDLE stCPUHandle(pISRVHeap->GetGPUDescriptorHandleForHeapStart());
					stCPUHandle.Offset(4, nSRVDescriptorSize);
					CD3DX12_GPU_DESCRIPTOR_HANDLE mBlur0GpuSrv = stCPUHandle;
					stCPUHandle.Offset(1, nSRVDescriptorSize);
					CD3DX12_GPU_DESCRIPTOR_HANDLE mBlur0GpuUav = stCPUHandle;
					stCPUHandle.Offset(1, nSRVDescriptorSize);
					CD3DX12_GPU_DESCRIPTOR_HANDLE mBlur1GpuSrv = stCPUHandle;
					stCPUHandle.Offset(1, nSRVDescriptorSize);
					CD3DX12_GPU_DESCRIPTOR_HANDLE mBlur1GpuUav = stCPUHandle;

					pICMDListArr[nextFrameResource]->SetPipelineState(pIHorzBlurPSO.Get());
					pICMDListArr[nextFrameResource]->SetComputeRootDescriptorTable(1, mBlur0GpuSrv);
					pICMDListArr[nextFrameResource]->SetComputeRootDescriptorTable(2, mBlur1GpuUav);

					UINT numGroupsX = (UINT)ceilf(iWidth / 256.0f);
					pICMDListArr[nextFrameResource]->Dispatch(numGroupsX, iHeight, 1);
					CD3DX12_RESOURCE_BARRIER A = CD3DX12_RESOURCE_BARRIER::Transition(pIBlurMap0.Get(),
						D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
					pICMDListArr[nextFrameResource]->ResourceBarrier(1, &A);

					CD3DX12_RESOURCE_BARRIER B = CD3DX12_RESOURCE_BARRIER::Transition(pIBlurMap1.Get(),
						D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ);

					pICMDListArr[nextFrameResource]->ResourceBarrier(1, &B);




					//垂直模糊
					pICMDListArr[nextFrameResource]->SetPipelineState(pIVertBlurPSO.Get());
					pICMDListArr[nextFrameResource]->SetComputeRootDescriptorTable(1, mBlur1GpuSrv);
					pICMDListArr[nextFrameResource]->SetComputeRootDescriptorTable(2, mBlur0GpuUav);

					UINT numGroupsY = (UINT)ceilf(iHeight / 256.0f);
					pICMDListArr[nextFrameResource]->Dispatch(iWidth, numGroupsY, 1);


					//————————————————————————————————————
					// 模糊操作完成，为下一次for循环准备
					// 
					//垂直模糊完成之前没有变化，完成之后，我们模糊两次的结果是在pIBlurMap0上，而按照我们的循环，pIBlurMap0正好是再次模糊的输入
					//所以把资源状态修改一下，进入下一轮for循环。
					CD3DX12_RESOURCE_BARRIER C = CD3DX12_RESOURCE_BARRIER::Transition(pIBlurMap0.Get(),
						D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ);
					pICMDListArr[nextFrameResource]->ResourceBarrier(1, &C);

					CD3DX12_RESOURCE_BARRIER F = CD3DX12_RESOURCE_BARRIER::Transition(pIBlurMap1.Get(),
						D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
					pICMDListArr[nextFrameResource]->ResourceBarrier(1, &F);
					//————————————————————————————————————

				}
				//————————————————————————————————————
				//模糊操作完成，我们需要把后台缓冲区设置为渲染目标，然后把我们的模糊结果拷贝到后台缓冲区。

				//一个错误提示：D3D12 ERROR: ID3D12CommandList::ResourceBarrier: Before state (0x0: D3D12_RESOURCE_STATE_[COMMON|PRESENT]) of resource (0x00000168A3FA1680:'Unnamed ID3D12Resource Object') (subresource: 0) specified by transition barrier does not match with the state (0x4: D3D12_RESOURCE_STATE_RENDER_TARGET) specified in the previous call to ResourceBarrier [ RESOURCE_MANIPULATION ERROR #527: RESOURCE_BARRIER_BEFORE_AFTER_MISMATCH]
				//我们并没有自己指定后台缓冲区的资源初始状态，但是从上边错误可以发现是Present + common
				//而我们这里就是需要一个COPY_DEST，所以需要一个转换。
				// 
				//偏移回到起始位置，然后再适当偏移至当前设置后台缓冲区view。
				stRTVHandle.ptr -= 3 * nRTVDescriptorSize;
				stRTVHandle.ptr += (nFrameIndex * nRTVDescriptorSize);
				pICMDListArr[nextFrameResource]->OMSetRenderTargets(1, &stRTVHandle, FALSE, nullptr);

				CD3DX12_RESOURCE_BARRIER M = CD3DX12_RESOURCE_BARRIER::Transition(pIARenderTargets[nFrameIndex].Get(),
					D3D12_RESOURCE_STATE_PRESENT | D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
				pICMDListArr[nextFrameResource]->ResourceBarrier(1, &M);

				CD3DX12_RESOURCE_BARRIER N = CD3DX12_RESOURCE_BARRIER::Transition(pIBlurMap0.Get(),
					D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_SOURCE);
				pICMDListArr[nextFrameResource]->ResourceBarrier(1, &N);

				//最后把资源拷贝回到后台缓冲区。
				pICMDListArr[nextFrameResource]->CopyResource(pIARenderTargets[nFrameIndex].Get(), pIBlurMap0.Get());


				//因为我们两个资源的第一次使用的时候状态都是COMMON的，所以我们的循环也就每次把他们当成COMMON来处理
				//所以这里需要每次while循环结束都要回到COMMON状态，然后和下一轮循环衔接起来
				CD3DX12_RESOURCE_BARRIER G = CD3DX12_RESOURCE_BARRIER::Transition(pIBlurMap1.Get(),
					D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COMMON);
				pICMDListArr[nextFrameResource]->ResourceBarrier(1, &G);

				CD3DX12_RESOURCE_BARRIER H = CD3DX12_RESOURCE_BARRIER::Transition(pIBlurMap0.Get(),
					D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COMMON);
				pICMDListArr[nextFrameResource]->ResourceBarrier(1, &H);

				//我们的后台缓冲区下一轮循环会被Present，也是为了衔接需要转换成PRESENT，由于上边我们根据注释所说的
				//一个错误，最开始他的状态都是COMMON+PRESENT，所以这里我们也回到最开始，这样和第一次循环衔接起来，
				//而开始正好是有一个Present状态，
				CD3DX12_RESOURCE_BARRIER I = CD3DX12_RESOURCE_BARRIER::Transition(pIARenderTargets[nFrameIndex].Get(),
					D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT | D3D12_RESOURCE_STATE_COMMON);
				pICMDListArr[nextFrameResource]->ResourceBarrier(1, &I);
				//—————————————————————————————————————————————————————

			}
		}

	}
	catch (CGRSCOMException& e)
	{
		e;
	}
	return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_KEYDOWN:
	{
		USHORT n16KeyCode = (wParam & 0xFF);


		if (VK_UP == n16KeyCode || 'w' == n16KeyCode || 'W' == n16KeyCode)
		{
			g_v4EyePos = XMVectorAdd(g_v4EyePos, XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f));
		}

		if (VK_DOWN == n16KeyCode || 's' == n16KeyCode || 'S' == n16KeyCode)
		{
			g_v4EyePos = XMVectorAdd(g_v4EyePos, XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f));
		}

		if (VK_RIGHT == n16KeyCode || 'd' == n16KeyCode || 'D' == n16KeyCode)
		{
			g_v4EyePos = XMVectorAdd(g_v4EyePos, XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f));
		}

		if (VK_LEFT == n16KeyCode || 'a' == n16KeyCode || 'A' == n16KeyCode)
		{
			g_v4EyePos = XMVectorAdd(g_v4EyePos, XMVectorSet(-1.0f, 0.0f, 0.0f, 0.0f));
		}

		if (VK_RIGHT == n16KeyCode || 'q' == n16KeyCode || 'Q' == n16KeyCode)
		{
			g_v4EyePos = XMVectorAdd(g_v4EyePos, XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));
		}

		if (VK_LEFT == n16KeyCode || 'e' == n16KeyCode || 'E' == n16KeyCode)
		{
			g_v4EyePos = XMVectorAdd(g_v4EyePos, XMVectorSet(0.0f, -1.0f, 0.0f, 0.0f));
		}


		if (VK_ADD == n16KeyCode || VK_OEM_PLUS == n16KeyCode)
		{
			g_fPalstance += 10 * XM_PI / 180.0f;
			if (g_fPalstance > XM_PI)
			{
				g_fPalstance = XM_PI;
			}
		}

		if (VK_SUBTRACT == n16KeyCode || VK_OEM_MINUS == n16KeyCode)
		{
			g_fPalstance -= 10 * XM_PI / 180.0f;
			if (g_fPalstance < 0.0f)
			{
				g_fPalstance = XM_PI / 180.0f;
			}
		}
	}

	break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
