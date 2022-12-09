//-----------------------------------【头文件】----------------------------------------
// 这部分你就拽过去得了，浏览一下就行，以后想研究再研究吧
//--------------------------------------------------------------------------------------
#include <SDKDDKVer.h>//用于根据您希望程序支持的操作系统从Windows头控制将哪些函数、常量等包含到代码中。copy的注释，知道和Windows相关就得了。删了都不报错
#define WIN32_LEAN_AND_MEAN // 从 Windows 头中排除极少使用的资料
#include <windows.h>
#include <tchar.h>//字符串相关的，这里使用的Windows的字符串，东西很乱。
#include <strsafe.h>//字符串相关的
#include <wrl.h>		//添加WTL支持 方便使用COM，就是COM只能指针需要的
#include <dxgi1_6.h>//DXGI的头
#include <DirectXMath.h>//我们涉及的变换矩阵，向量的头
#include <d3d12.h>       //for d3d12
#include <d3dcompiler.h>//D3D 编译相关的
#if defined(_DEBUG)
#include <dxgidebug.h>//DXGI有一个独自的调试，下面还有一个flag相关的
#endif
#include <wincodec.h>   //for WIC，就是纹理相关的
#include "d3dx12.h"


//new header
#include"../imgui/imgui.h"
#include"../imgui/imgui_impl_win32.h"
#include"../imgui/imgui_impl_dx12.h"

#include <windowsx.h>//for the following macro : #define GET_X_LPARAM(lp)                        ((int)(short)LOWORD(lp))
#include <vector>

//——————————————————————————————————————————————

#include <assimp/Importer.hpp>     
#include <assimp/scene.h>          
#include <assimp/postprocess.h>

#pragma comment(lib, "assimp-vc143-mtd.lib")
//————————————————————————————————————————————————————————


//for the imgui
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

//-----------------------------------【命名空间、链接库】----------------------------------------
// 链接库是重点，DX必须的东西
//-----------------------------------------------------------------------------------------------
using namespace Microsoft;
using namespace Microsoft::WRL;
using namespace DirectX;

#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")






float Theta = 1.5f * XM_PI;
float Phi = XM_PIDIV4;
float Radius = 20.0f;
POINT LastMousePos;


//-----------------------------------【宏定义】----------------------------------------
// 具体作用如下
//-----------------------------------------------------------------------------------------------
//窗口类的名
#define GRS_WND_CLASS_NAME _T("GRS Game Window Class")
//窗口的名
#define GRS_WND_TITLE	_T("致我们永不熄灭的游戏开发梦想~")

//新定义的宏用于上取整除法
#define GRS_UPPER_DIV(A,B) ((UINT)(((A)+((B)-1))/(B)))

//更简洁的向上边界对齐算法 内存管理中常用 请记住
#define GRS_UPPER(A,B) ((UINT)(((A)+((B)-1))&~(B - 1)))
//用于判断函数执行错误的，如果出错了就会引发异常
#define GRS_THROW_IF_FAILED(hr) {HRESULT _hr = (hr);if (FAILED(_hr)){ throw CGRSCOMException(_hr); }}

//-----------------------------------【抛出异常相关的类】----------------------------------------
// 这部分C++基础哦，不懂的话，自己找点资料看明白
//-----------------------------------------------------------------------------------------------
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

//-----------------------------------【纹理相关的黑盒】----------------------------------------
// 可以暂时不看，注意UploadTexture函数内部的路径，是否和你的文件路径一致
//-----------------------------------------------------------------------------------------------
struct WICTranslate
{
	GUID wic;
	DXGI_FORMAT format;
};

static WICTranslate g_WICFormats[] = {//WIC格式与DXGI像素格式的对应表，该表中的格式为被支持的格式
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

// WIC 像素格式转换表.
struct WICConvert
{
	GUID source;
	GUID target;
};

static WICConvert g_WICConvert[] = {
	// 目标格式一定是最接近的被支持的格式
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
{//查表确定兼容的最接近格式是哪个
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
{//查表确定最终对应的DXGI格式是哪一个
	for (size_t i = 0; i < _countof(g_WICFormats); ++i)
	{
		if (InlineIsEqualGUID(g_WICFormats[i].wic, *pPixelFormat))
		{
			return g_WICFormats[i].format;
		}
	}
	return DXGI_FORMAT_UNKNOWN;
}

UINT UploadTexture(ComPtr<IWICBitmapSource>& pIBMP, ComPtr<IWICImagingFactory> pIWICFactory,
	ComPtr<IWICBitmapDecoder> pIWICDecoder, ComPtr<IWICBitmapFrameDecode> pIWICFrame,
	DXGI_FORMAT& stTextureFormat, UINT& nTextureW, UINT& nTextureH, UINT nBPP, TCHAR* pszAppPath, TCHAR* TextureName)
{
	//使用纯COM方式创建WIC类厂对象，也是调用WIC第一步要做的事情
	GRS_THROW_IF_FAILED(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pIWICFactory)));
	TCHAR pszTextureFileName[MAX_PATH] = {};
	//使用WIC类厂对象接口加载纹理图片，并得到一个WIC解码器对象接口，图片信息就在这个接口代表的对象中了

	StringCchPrintf(pszTextureFileName, MAX_PATH, _T("%sTexture\\%s"), pszAppPath, TextureName);
	GRS_THROW_IF_FAILED(pIWICFactory->CreateDecoderFromFilename(
		pszTextureFileName,              // 文件名
		NULL,                            // 不指定解码器，使用默认
		GENERIC_READ,                    // 访问权限
		WICDecodeMetadataCacheOnDemand,  // 若需要就缓冲数据 
		&pIWICDecoder                    // 解码器对象
	));

	// 获取第一帧图片(因为GIF等格式文件可能会有多帧图片，其他的格式一般只有一帧图片)
	// 实际解析出来的往往是位图格式数据
	GRS_THROW_IF_FAILED(pIWICDecoder->GetFrame(0, &pIWICFrame));

	WICPixelFormatGUID wpf = {};
	//获取WIC图片格式
	GRS_THROW_IF_FAILED(pIWICFrame->GetPixelFormat(&wpf));
	GUID tgFormat = {};

	//通过第一道转换之后获取DXGI的等价格式
	if (GetTargetPixelFormat(&wpf, &tgFormat))
	{
		stTextureFormat = GetDXGIFormatFromPixelFormat(&tgFormat);
	}

	if (DXGI_FORMAT_UNKNOWN == stTextureFormat)
	{// 不支持的图片格式 目前退出了事 
	 // 一般 在实际的引擎当中都会提供纹理格式转换工具，
	 // 图片都需要提前转换好，所以不会出现不支持的现象
		throw CGRSCOMException(S_FALSE);
	}

	// 定义一个位图格式的图片数据对象接口
	//ComPtr<IWICBitmapSource>pIBMP;
	//移到外面作为参数！！！！！！！！！！！！！！


	if (!InlineIsEqualGUID(wpf, tgFormat))
	{// 这个判断很重要，如果原WIC格式不是直接能转换为DXGI格式的图片时
	 // 我们需要做的就是转换图片格式为能够直接对应DXGI格式的形式
		//创建图片格式转换器
		ComPtr<IWICFormatConverter> pIConverter;
		GRS_THROW_IF_FAILED(pIWICFactory->CreateFormatConverter(&pIConverter));

		//初始化一个图片转换器，实际也就是将图片数据进行了格式转换
		GRS_THROW_IF_FAILED(pIConverter->Initialize(
			pIWICFrame.Get(),                // 输入原图片数据
			tgFormat,						 // 指定待转换的目标格式
			WICBitmapDitherTypeNone,         // 指定位图是否有调色板，现代都是真彩位图，不用调色板，所以为None
			NULL,                            // 指定调色板指针
			0.f,                             // 指定Alpha阀值
			WICBitmapPaletteTypeCustom       // 调色板类型，实际没有使用，所以指定为Custom
		));
		// 调用QueryInterface方法获得对象的位图数据源接口
		GRS_THROW_IF_FAILED(pIConverter.As(&pIBMP));
	}
	else
	{
		//图片数据格式不需要转换，直接获取其位图数据源接口
		GRS_THROW_IF_FAILED(pIWICFrame.As(&pIBMP));
	}
	//获得图片大小（单位：像素）
	GRS_THROW_IF_FAILED(pIBMP->GetSize(&nTextureW, &nTextureH));

	//获取图片像素的位大小的BPP（Bits Per Pixel）信息，用以计算图片行数据的真实大小（单位：字节）
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

	// 到这里终于可以得到BPP了，这也是我看的比较吐血的地方，为了BPP居然饶了这么多环节
	GRS_THROW_IF_FAILED(pIWICPixelinfo->GetBitsPerPixel(&nBPP));

	// 计算图片实际的行大小（单位：字节），这里使用了一个上取整除法即（A+B-1）/B ，
	// 这曾经被传说是微软的面试题,希望你已经对它了如指掌
	UINT nPicRowPitch = (uint64_t(nTextureW) * uint64_t(nBPP) + 7u) / 8u;
	return nPicRowPitch;
}

D3D12_PLACED_SUBRESOURCE_FOOTPRINT CopyToUploadHeap(ComPtr<ID3D12Resource> pITexture, ComPtr<ID3D12Resource> pITextureUpload,
	ComPtr<IWICBitmapSource> pIBMP, ComPtr<ID3D12Device4>	pID3D12Device4, UINT64 n64UploadBufferSize, UINT nTextureH,
	UINT nPicRowPitch)
{
	//按照资源缓冲大小来分配实际图片数据存储的内存大小
	void* pbPicData = ::HeapAlloc(::GetProcessHeap(), HEAP_ZERO_MEMORY, n64UploadBufferSize);
	if (nullptr == pbPicData)
	{
		throw CGRSCOMException(HRESULT_FROM_WIN32(GetLastError()));
	}

	//从图片中读取出数据
	GRS_THROW_IF_FAILED(pIBMP->CopyPixels(nullptr
		, nPicRowPitch
		, static_cast<UINT>(nPicRowPitch * nTextureH)   //注意这里才是图片数据真实的大小，这个值通常小于缓冲的大小
		, reinterpret_cast<BYTE*>(pbPicData)));

	//{//下面这段代码来自DX12的示例，直接通过填充缓冲绘制了一个黑白方格的纹理
	// //还原这段代码，然后注释上面的CopyPixels调用可以看到黑白方格纹理的效果
	//	const UINT rowPitch = nPicRowPitch; //nTextureW * 4; //static_cast<UINT>(n64UploadBufferSize / nTextureH);
	//	const UINT cellPitch = rowPitch >> 3;		// The width of a cell in the checkboard texture.
	//	const UINT cellHeight = nTextureW >> 3;	// The height of a cell in the checkerboard texture.
	//	const UINT textureSize = static_cast<UINT>(n64UploadBufferSize);
	//	UINT nTexturePixelSize = static_cast<UINT>(n64UploadBufferSize / nTextureH / nTextureW);

	//	UINT8* pData = reinterpret_cast<UINT8*>(pbPicData);

	//	for (UINT n = 0; n < textureSize; n += nTexturePixelSize)
	//	{
	//		UINT x = n % rowPitch;
	//		UINT y = n / rowPitch;
	//		UINT i = x / cellPitch;
	//		UINT j = y / cellHeight;

	//		if (i % 2 == j % 2)
	//		{
	//			pData[n] = 0x00;		// R
	//			pData[n + 1] = 0x00;	// G
	//			pData[n + 2] = 0x00;	// B
	//			pData[n + 3] = 0xff;	// A
	//		}
	//		else
	//		{
	//			pData[n] = 0xff;		// R
	//			pData[n + 1] = 0xff;	// G
	//			pData[n + 2] = 0xff;	// B
	//			pData[n + 3] = 0xff;	// A
	//		}
	//	}
	//}

	//获取向上传堆拷贝纹理数据的一些纹理转换尺寸信息
	//对于复杂的DDS纹理这是非常必要的过程
	UINT64 n64RequiredSize = 0u;
	UINT   nNumSubresources = 1u;  //我们只有一副图片，即子资源个数为1
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

	//因为上传堆实际就是CPU传递数据到GPU的中介
	//所以我们可以使用熟悉的Map方法将它先映射到CPU内存地址中
	//然后我们按行将数据复制到上传堆中
	//需要注意的是之所以按行拷贝是因为GPU资源的行大小
	//与实际图片的行大小是有差异的,二者的内存边界对齐要求是不一样的
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
	//取消映射 对于易变的数据如每帧的变换矩阵等数据，可以撒懒不用Unmap了，
	//让它常驻内存,以提高整体性能，因为每次Map和Unmap是很耗时的操作
	//因为现在起码都是64位系统和应用了，地址空间是足够的，被长期占用不会影响什么
	pITextureUpload->Unmap(0, NULL);

	//释放图片数据，做一个干净的程序员
	::HeapFree(::GetProcessHeap(), 0, pbPicData);
	return stTxtLayouts;
}



void print(float i)
{
	TCHAR pszTextureFileName[MAX_PATH] = {};
	//使用WIC类厂对象接口加载纹理图片，并得到一个WIC解码器对象接口，图片信息就在这个接口代表的对象中了
	StringCchPrintf(pszTextureFileName, MAX_PATH, _T("--%f--"), i);
	OutputDebugString(pszTextureFileName);
}
void printEndline()
{
	TCHAR pszTextureFileName[MAX_PATH] = {};
	//使用WIC类厂对象接口加载纹理图片，并得到一个WIC解码器对象接口，图片信息就在这个接口代表的对象中了
	StringCchPrintf(pszTextureFileName, MAX_PATH, _T("\n"));
	OutputDebugString(pszTextureFileName);
}


struct Vertex
{
	XMFLOAT3 position;
	XMFLOAT3 normal;
	XMFLOAT3 tangent;
	XMFLOAT2 texcoord;
};

struct Mesh
{
	std::vector<Vertex> vertices;
	std::vector<std::uint32_t> indices;
};


std::vector<Mesh> meshes;

void LoadModels(const char* modelFilename)
{
	assert(modelFilename != nullptr);
	const std::string filePath(modelFilename);

	Assimp::Importer importer;
	const std::uint32_t flags{ aiProcessPreset_TargetRealtime_Fast | aiProcess_ConvertToLeftHanded };
	const aiScene* scene{ importer.ReadFile(filePath.c_str(), aiProcess_ConvertToLeftHanded |     // 转为左手系
	aiProcess_GenBoundingBoxes |        // 获取碰撞盒
	aiProcess_Triangulate |             // 将多边形拆分
	aiProcess_ImproveCacheLocality |    // 改善缓存局部性
	aiProcess_SortByPType) };
	assert(scene != nullptr);

	assert(scene->HasMeshes());

	for (std::uint32_t i = 0U; i < scene->mNumMeshes; ++i)
	{
		aiMesh* mesh{ scene->mMeshes[i] };
		assert(mesh != nullptr);

		Mesh tempMesh;

		{
			// Positions and Normals
			const std::size_t numVertices{ mesh->mNumVertices };
			assert(numVertices > 0U);
			tempMesh.vertices.resize(numVertices);
			for (std::uint32_t i = 0U; i < numVertices; ++i)
			{
				tempMesh.vertices[i].position = XMFLOAT3(reinterpret_cast<const float*>(&mesh->mVertices[i]));
				tempMesh.vertices[i].normal = XMFLOAT3(reinterpret_cast<const float*>(&mesh->mNormals[i]));
				//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!获取切线
				tempMesh.vertices[i].tangent = XMFLOAT3(reinterpret_cast<const float*>(&mesh->mTangents[i]));
			}


			// Indices
			const std::uint32_t numFaces{ mesh->mNumFaces };
			assert(numFaces > 0U);
			for (std::uint32_t i = 0U; i < numFaces; ++i)
			{
				const aiFace* face = &mesh->mFaces[i];
				assert(face != nullptr);
				// We only allow triangles
				assert(face->mNumIndices == 3U);

				tempMesh.indices.push_back(face->mIndices[0U]);
				tempMesh.indices.push_back(face->mIndices[1U]);
				tempMesh.indices.push_back(face->mIndices[2U]);
			}

			// Texture Coordinates (if any)
			if (mesh->HasTextureCoords(0U))
			{
				assert(mesh->GetNumUVChannels() == 1U);
				const aiVector3D* aiTextureCoordinates{ mesh->mTextureCoords[0U] };
				assert(aiTextureCoordinates != nullptr);
				for (std::uint32_t i = 0U; i < numVertices; i++)
				{
					tempMesh.vertices[i].texcoord = XMFLOAT2(reinterpret_cast<const float*>(&aiTextureCoordinates[i]));
				}
			}
		}

		meshes.push_back(tempMesh);
	}
}


struct ST_GRS_VERTEX
{
	XMFLOAT4 m_v4Position;
	XMFLOAT2 m_vTex;
};

struct ST_GRS_FRAME_MVP_BUFFER
{
	XMFLOAT4X4 m_MVP;			//经典的Model-view-projection(MVP)矩阵.
	XMFLOAT4X4 m_ObjectToWorld;
	XMVECTOR m_ViewPos;
	XMFLOAT3 lightDir;
	float baseColorIntensity;
	float metallicIntensity;
	float roughnessIntensity;
	float subsurface;
	float _specular;
	float specularTint;
	float anisotropic;
	float sheen;
	float sheenTint;
	float clearcoat;
	float clearcoatGloss;
};


XMVECTOR g_v4EyePos = XMVectorSet(0.0f, 2.0f, -15.0f, 0.0f); //眼睛位置
XMVECTOR g_v4LookAt = XMVectorSet(0.0f, 0.0f, 0.0f, 0.0f);    //眼睛所盯的位置
XMVECTOR g_v4UpDir = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);    //头部正上方位置

double g_fPalstance = 10.0f * XM_PI / 180.0f;	//物体旋转的角速度，单位：弧度/秒


//-----------------------------------【主函数】--------------------------------------------
//-----------------------------------------------------------------------------------------------
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR    lpCmdLine, int nCmdShow)
{
	::CoInitialize(nullptr);  //for WIC & COM

	const UINT nFrameBackBufCount = 3u;

	int									iWidth = 1024;
	int									iHeight = 768;
	UINT								nFrameIndex = 0;

	DXGI_FORMAT							emRenderTarget = DXGI_FORMAT_R8G8B8A8_UNORM;
	float								f4RTTexClearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
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
	D3D12_VERTEX_BUFFER_VIEW			stVertexBufferViewDeferred = {};
	D3D12_INDEX_BUFFER_VIEW				stIndexBufferViewDeferred = {};
	D3D12_VERTEX_BUFFER_VIEW			stVertexBufferView = {};
	D3D12_INDEX_BUFFER_VIEW				stIndexBufferView = {};
	UINT64								n64FenceValue = 0ui64;
	HANDLE								hEventFence = nullptr;

	ST_GRS_FRAME_MVP_BUFFER* pMVPBuffer = nullptr;
	SIZE_T								szMVPBuffer = GRS_UPPER(sizeof(ST_GRS_FRAME_MVP_BUFFER), 256);

	UINT								nTextureW = 0u;
	UINT								nTextureH = 0u;
	UINT								nBPP = 0u;
	UINT								nPicRowPitch = 0;//new
	DXGI_FORMAT							stTextureFormat = DXGI_FORMAT_UNKNOWN;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT	stTxtLayoutsBaseColor = {};
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT	stTxtLayoutsNormal = {};
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT	stTxtLayoutsMetallic = {};
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT	stTxtLayoutsRoughness = {};

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
	ComPtr<ID3D12DescriptorHeap>		pIDSVHeap;
	ComPtr<ID3D12DescriptorHeap>		pISRVHeapImgui;
	ComPtr<ID3D12Resource>				pIARenderTargets[nFrameBackBufCount];
	ComPtr<ID3DBlob> pIBlobVertexShader;
	ComPtr<ID3DBlob> pIBlobPixelShader;
	ComPtr<ID3DBlob> pIBlobVertexShaderDeferred;
	ComPtr<ID3DBlob> pIBlobPixelShaderDeferred;

	ComPtr<ID3D12Heap>					pITextureHeap;
	ComPtr<ID3D12Heap>					pIUploadHeap;
	ComPtr<ID3D12Resource>				pITexBaseColor;
	ComPtr<ID3D12Resource>				pITexMetallic;
	ComPtr<ID3D12Resource>				pITexNormal;
	ComPtr<ID3D12Resource>				pITexRoughness;
	ComPtr<ID3D12Resource>				pITexBaseColorUpload;
	ComPtr<ID3D12Resource>				pITexNormalUpload;
	ComPtr<ID3D12Resource>				pITexMetallicUpload;
	ComPtr<ID3D12Resource>				pITexRoughnessUpload;
	ComPtr<ID3D12Resource>			    pICBVUpload;
	ComPtr<ID3D12Resource>				pIDeferredTextureA;
	ComPtr<ID3D12Resource>				pIDeferredTextureB;

	ComPtr<ID3D12CommandAllocator>		pICMDAlloc;
	ComPtr<ID3D12GraphicsCommandList>	pICMDList;
	ComPtr<ID3D12RootSignature>			pIRootSignature;
	ComPtr<ID3D12RootSignature>			pIRootSignatureDeferred;

	ComPtr<ID3D12PipelineState>			pIPipelineState;
	ComPtr<ID3D12PipelineState>			pIPipelineStateDeferred;
	ComPtr<ID3D12Resource>				pIDepthStencilBuffer;
	ComPtr<ID3D12Resource>				pIVertexBufferDeferred;
	ComPtr<ID3D12Resource>				pIIndexBufferDeferred;
	ComPtr<ID3D12Resource>				pIVertexBuffer;
	ComPtr<ID3D12Resource>				pIIndexBuffer;

	ComPtr<ID3D12Fence>					pIFence;

	ComPtr<IWICImagingFactory>			pIWICFactory;
	ComPtr<IWICBitmapDecoder>			pIWICDecoder;
	ComPtr<IWICBitmapFrameDecode>		pIWICFrame;
	ComPtr<IWICBitmapSource>			pIBMPBaseColor;
	ComPtr<IWICBitmapSource>			pIBMPNormal;
	ComPtr<IWICBitmapSource>			pIBMPMetallic;
	ComPtr<IWICBitmapSource>			pIBMPRoughness;



	D3D12_RESOURCE_DESC					stTextureDesc = {};

	UINT								nVertexCnt = 0;
	try
	{
		//1、得到当前的工作目录pszAppPath，方便我们使用相对路径来访问各种资源文件，可以直接拿pszAppPath来用
		{
			if (0 == ::GetModuleFileName(nullptr, pszAppPath, MAX_PATH))
			{
				GRS_THROW_IF_FAILED(HRESULT_FROM_WIN32(GetLastError()));
			}

			WCHAR* lastSlash = _tcsrchr(pszAppPath, _T('\\'));
			if (lastSlash)
			{//删除Exe文件名
				*(lastSlash) = _T('\0');
			}

			lastSlash = _tcsrchr(pszAppPath, _T('\\'));
			if (lastSlash)
			{//删除x64路径
				*(lastSlash) = _T('\0');
			}

			lastSlash = _tcsrchr(pszAppPath, _T('\\'));
			if (lastSlash)
			{//删除Debug 或 Release路径
				*(lastSlash + 1) = _T('\0');
			}
		}
		//创建ImGui上下文，需要在创建窗口之前创建。
		{
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
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
			wcex.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);//防止无聊的背景重绘
			wcex.lpszClassName = GRS_WND_CLASS_NAME;
			RegisterClassEx(&wcex);

			DWORD dwWndStyle = WS_OVERLAPPED | WS_SYSMENU;
			RECT rtWnd = { 0, 0, iWidth, iHeight };
			AdjustWindowRect(&rtWnd, dwWndStyle, FALSE);

			// 计算窗口居中的屏幕坐标
			INT posX = (GetSystemMetrics(SM_CXSCREEN) - rtWnd.right - rtWnd.left) / 2;
			INT posY = (GetSystemMetrics(SM_CYSCREEN) - rtWnd.bottom - rtWnd.top) / 2;

			hWnd = CreateWindowW(GRS_WND_CLASS_NAME
				, GRS_WND_TITLE
				, dwWndStyle //这里会关闭全屏效果。
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

		//加载模型
		{
			LoadModels("Resources/models/Cyborg_Weapon.fbx");
		}




		//3、使用WIC创建并加载一个2D纹理
		{
			//前俩和后俩返回值不一样！！！！！！！！！！！！！！！！！！
			//当成黑盒
			TCHAR baseColorName[] = _T("Weapon_BaseColor.png");
			nPicRowPitch = UploadTexture(pIBMPBaseColor, pIWICFactory, pIWICDecoder, pIWICFrame, stTextureFormat, nTextureW, nTextureH, nBPP, pszAppPath, baseColorName);

			TCHAR nomalName[] = _T("Weapon_Normal.png");
			nPicRowPitch = UploadTexture(pIBMPNormal, pIWICFactory, pIWICDecoder, pIWICFrame, stTextureFormat, nTextureW, nTextureH, nBPP, pszAppPath, nomalName);

			TCHAR metallicName[] = _T("Weapon_Metallic.png");
			nPicRowPitch = UploadTexture(pIBMPMetallic, pIWICFactory, pIWICDecoder, pIWICFrame, stTextureFormat, nTextureW, nTextureH, nBPP, pszAppPath, metallicName);

			TCHAR roughnessName[] = _T("Weapon_Roughness.png");
			nPicRowPitch = UploadTexture(pIBMPRoughness, pIWICFactory, pIWICDecoder, pIWICFrame, stTextureFormat, nTextureW, nTextureH, nBPP, pszAppPath, roughnessName);


		}

		//4、打开显示子系统的调试支持
		{
#if defined(_DEBUG)
			{
				ComPtr<ID3D12Debug> debugController;
				if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
				{
					debugController->EnableDebugLayer();
					// 打开附加的调试支持
					nDXGIFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
				}
			}
#endif
		}

		//5、创建DXGI Factory对象
		{
			GRS_THROW_IF_FAILED(CreateDXGIFactory2(nDXGIFactoryFlags, IID_PPV_ARGS(pIDXGIFactory5.GetAddressOf())));
		}

		//6、枚举适配器，并选择合适的适配器来创建3D设备对象
		{
			DXGI_ADAPTER_DESC1 stAdapterDesc = {};//适配器描述的结构，待会获取描述时，用这个结构体来接收
			//利用DXGIFactory来循环遍历你的所有显卡。
			for (UINT nAdapterIndex = 0; DXGI_ERROR_NOT_FOUND != pIDXGIFactory5->EnumAdapters1(nAdapterIndex, &pIAdapter1); ++nAdapterIndex)
			{
				pIAdapter1->GetDesc1(&stAdapterDesc);//从当前正在遍历的显卡中获取显卡的描述信息，用准备好的结构体接收

				if (stAdapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				{//跳过软件虚拟适配器设备
					continue;
				}
				//检查适配器对D3D支持的兼容级别，这里直接要求支持12.1的能力，注意返回接口的那个参数被置为了nullptr，这样
				//就不会实际创建一个设备了，也不用我们啰嗦的再调用release来释放接口。这也是一个重要的技巧，请记住！
				if (SUCCEEDED(D3D12CreateDevice(pIAdapter1.Get(), D3D_FEATURE_LEVEL_12_1, _uuidof(ID3D12Device), nullptr)))
				{
					break;
				}
			}
			// 创建D3D12.1的设备，这里是真正创建出来设备。
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

		//7、创建直接命令队列
		{
			D3D12_COMMAND_QUEUE_DESC stQueueDesc = {};
			stQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
			GRS_THROW_IF_FAILED(pID3D12Device4->CreateCommandQueue(&stQueueDesc, IID_PPV_ARGS(&pICMDQueue)));
		}

		//8、创建命令列表分配器
		{
			GRS_THROW_IF_FAILED(pID3D12Device4->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT
				, IID_PPV_ARGS(&pICMDAlloc)));
			// 创建图形命令列表
			GRS_THROW_IF_FAILED(pID3D12Device4->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT
				, pICMDAlloc.Get(), pIPipelineStateDeferred.Get(), IID_PPV_ARGS(&pICMDList)));

		}

		//9、创建交换链
		{
			DXGI_SWAP_CHAIN_DESC1 stSwapChainDesc = {};
			stSwapChainDesc.BufferCount = nFrameBackBufCount;
			stSwapChainDesc.Width = iWidth;
			stSwapChainDesc.Height = iHeight;
			stSwapChainDesc.Format = emRenderTarget;
			stSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			stSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			stSwapChainDesc.SampleDesc.Count = 1;

			GRS_THROW_IF_FAILED(pIDXGIFactory5->CreateSwapChainForHwnd(
				pICMDQueue.Get(),		// Swap chain needs the queue so that it can force a flush on it.
				hWnd,
				&stSwapChainDesc,
				nullptr,
				nullptr,
				&pISwapChain1
			));

			GRS_THROW_IF_FAILED(pISwapChain1.As(&pISwapChain3));
			nFrameIndex = pISwapChain3->GetCurrentBackBufferIndex();

			D3D12_DESCRIPTOR_HEAP_DESC stRTVHeapDesc = {};
			stRTVHeapDesc.NumDescriptors = nFrameBackBufCount + 2;
			stRTVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
			stRTVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

			GRS_THROW_IF_FAILED(pID3D12Device4->CreateDescriptorHeap(&stRTVHeapDesc, IID_PPV_ARGS(&pIRTVHeap)));

			//创建RTV的描述符
			D3D12_CPU_DESCRIPTOR_HANDLE stRTVHandle = pIRTVHeap->GetCPUDescriptorHandleForHeapStart();
			for (UINT i = 0; i < nFrameBackBufCount; i++)
			{
				GRS_THROW_IF_FAILED(pISwapChain3->GetBuffer(i, IID_PPV_ARGS(&pIARenderTargets[i])));
				pID3D12Device4->CreateRenderTargetView(pIARenderTargets[i].Get(), nullptr, stRTVHandle);
				stRTVHandle.ptr += nRTVDescriptorSize;
			}
			// 关闭ALT+ENTER键切换全屏的功能，因为我们没有实现OnSize处理（创建窗口的时候关了），所以先关闭
			GRS_THROW_IF_FAILED(pIDXGIFactory5->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));
		}

		//10、创建SRV和CBV堆
		{
			D3D12_DESCRIPTOR_HEAP_DESC stSRVHeapDesc = {};
			stSRVHeapDesc.NumDescriptors = 7;
			//第一个是srv的纹理，第二个是cbv，第三四个是AB两个延迟渲染的GBuffer，因为他俩将来是第二个pass的输入
			stSRVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			stSRVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

			GRS_THROW_IF_FAILED(pID3D12Device4->CreateDescriptorHeap(&stSRVHeapDesc, IID_PPV_ARGS(&pISRVHeap)));
		}

		//——————————————————————————————————————
		//创建DSV的堆
		{
			D3D12_DESCRIPTOR_HEAP_DESC stDSVHeapDesc = {};
			stDSVHeapDesc.NumDescriptors = 1;
			stDSVHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
			stDSVHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

			GRS_THROW_IF_FAILED(pID3D12Device4->CreateDescriptorHeap(&stDSVHeapDesc, IID_PPV_ARGS(&pIDSVHeap)));
		}
		//——————————————————————————————————————

		//ImGui Init 
		{
			D3D12_DESCRIPTOR_HEAP_DESC SrvHeapDesc;
			SrvHeapDesc.NumDescriptors = 1;
			SrvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			SrvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			SrvHeapDesc.NodeMask = 0;
			GRS_THROW_IF_FAILED(pID3D12Device4->CreateDescriptorHeap(
				&SrvHeapDesc, IID_PPV_ARGS(pISRVHeapImgui.GetAddressOf())));


			ImGuiIO& io = ImGui::GetIO();
			(void)io;
			ImGui::StyleColorsDark();
			ImGui_ImplWin32_Init(hWnd);
			ImGui_ImplDX12_Init(pID3D12Device4.Get(), nFrameBackBufCount, emRenderTarget, pISRVHeapImgui.Get(),
				pISRVHeapImgui->GetCPUDescriptorHandleForHeapStart(), pISRVHeapImgui->GetGPUDescriptorHandleForHeapStart());
		}

		//编译shader
		{

#if defined(_DEBUG)
			// Enable better shader debugging with the graphics debugging tools.
			UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
			UINT compileFlags = 0;
#endif

			//编译为行矩阵形式	   new
			compileFlags |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;
			ComPtr<ID3DBlob> error;
			TCHAR pszShaderFileName[MAX_PATH] = {};
			StringCchPrintf(pszShaderFileName, MAX_PATH, _T("%sShader\\shaderDeferred.hlsl"), pszAppPath);
			GRS_THROW_IF_FAILED(D3DCompileFromFile(pszShaderFileName, nullptr, nullptr
				, "VSMain", "vs_5_0", compileFlags, 0, &pIBlobVertexShaderDeferred, &error));

			//::OutputDebugStringA((char*)error->GetBufferPointer());

			GRS_THROW_IF_FAILED(D3DCompileFromFile(pszShaderFileName, nullptr, nullptr
				, "PSMain", "ps_5_0", compileFlags, 0, &pIBlobPixelShaderDeferred, nullptr));
		}

		//11、创建根签名
		{
			D3D12_FEATURE_DATA_ROOT_SIGNATURE stFeatureData = {};
			// 检测是否支持V1.1版本的根签名
			stFeatureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
			if (FAILED(pID3D12Device4->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &stFeatureData, sizeof(stFeatureData))))
			{
				stFeatureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
			}

			D3D12_DESCRIPTOR_RANGE1 stDSPRanges1[3] = {};








			// 在GPU上执行SetGraphicsRootDescriptorTable后，我们不修改命令列表中的SRV，因此我们可以使用默认Rang行为:
			// D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE
			//SRV
			stDSPRanges1[0].NumDescriptors = 1;
			stDSPRanges1[0].BaseShaderRegister = 0;
			stDSPRanges1[0].RegisterSpace = 0;
			stDSPRanges1[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;

			stDSPRanges1[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
			stDSPRanges1[0].OffsetInDescriptorsFromTableStart = 0;
			//CBV
			stDSPRanges1[1].NumDescriptors = 1;
			stDSPRanges1[1].BaseShaderRegister = 0;
			stDSPRanges1[1].RegisterSpace = 0;
			stDSPRanges1[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
			stDSPRanges1[1].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_NONE;
			stDSPRanges1[1].OffsetInDescriptorsFromTableStart = 0;



			D3D12_ROOT_PARAMETER1 stRootParameters1[3] = {};
			//SRV
			stRootParameters1[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			stRootParameters1[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
			stRootParameters1[0].DescriptorTable.NumDescriptorRanges = 1;
			stRootParameters1[0].DescriptorTable.pDescriptorRanges = &stDSPRanges1[0];
			//CBV
			stRootParameters1[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			stRootParameters1[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;		//CBV是所有Shader可见
			stRootParameters1[1].DescriptorTable.NumDescriptorRanges = 1;
			stRootParameters1[1].DescriptorTable.pDescriptorRanges = &stDSPRanges1[1];


			stDSPRanges1[2].NumDescriptors = 3;
			stDSPRanges1[2].BaseShaderRegister = 1;
			stDSPRanges1[2].RegisterSpace = 0;
			stDSPRanges1[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			stDSPRanges1[2].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
			stDSPRanges1[2].OffsetInDescriptorsFromTableStart = 0;

			//创建出另外三张纹理的根参数!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			stRootParameters1[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			stRootParameters1[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
			stRootParameters1[2].DescriptorTable.NumDescriptorRanges = 1;
			stRootParameters1[2].DescriptorTable.pDescriptorRanges = &stDSPRanges1[2];





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

			//OutputDebugStringA(static_cast<char*>(pIErrorBlob->GetBufferPointer()));

			//::OutputDebugStringA((char*)pIErrorBlob->GetBufferPointer());

			GRS_THROW_IF_FAILED(pID3D12Device4->CreateRootSignature(0
				, pISignatureBlob->GetBufferPointer()
				, pISignatureBlob->GetBufferSize()
				, IID_PPV_ARGS(&pIRootSignatureDeferred)));



			//————————————————————————————————————————————————————
			//两个渲染目标
			D3D12_ROOT_PARAMETER1 stRootParameters2[2] = {};
			stDSPRanges1[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			stDSPRanges1[0].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
			stDSPRanges1[0].OffsetInDescriptorsFromTableStart = 0;
			stDSPRanges1[0].NumDescriptors = 1;
			stDSPRanges1[0].BaseShaderRegister = 0;
			stDSPRanges1[0].RegisterSpace = 0;

			stDSPRanges1[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			stDSPRanges1[1].Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE;
			stDSPRanges1[1].OffsetInDescriptorsFromTableStart = 0;
			stDSPRanges1[1].NumDescriptors = 1;
			stDSPRanges1[1].BaseShaderRegister = 1;
			stDSPRanges1[1].RegisterSpace = 0;

			stRootParameters2[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			stRootParameters2[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
			stRootParameters2[0].DescriptorTable.NumDescriptorRanges = 1;
			stRootParameters2[0].DescriptorTable.pDescriptorRanges = &stDSPRanges1[0];

			stRootParameters2[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			stRootParameters2[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
			stRootParameters2[1].DescriptorTable.NumDescriptorRanges = 1;
			stRootParameters2[1].DescriptorTable.pDescriptorRanges = &stDSPRanges1[1];
			stRootSignatureDesc.Desc_1_1.NumStaticSamplers = _countof(stSamplerDesc);
			stRootSignatureDesc.Desc_1_1.pStaticSamplers = stSamplerDesc;
			stRootSignatureDesc.Desc_1_1.NumParameters = _countof(stRootParameters2);
			stRootSignatureDesc.Desc_1_1.pParameters = stRootParameters2;

			GRS_THROW_IF_FAILED(D3D12SerializeVersionedRootSignature(&stRootSignatureDesc
				, &pISignatureBlob
				, &pIErrorBlob));
			//::OutputDebugStringA((char*)pIErrorBlob->GetBufferPointer());
			GRS_THROW_IF_FAILED(pID3D12Device4->CreateRootSignature(0
				, pISignatureBlob->GetBufferPointer()
				, pISignatureBlob->GetBufferSize()
				, IID_PPV_ARGS(&pIRootSignature)));



		}

		//12、编译Shader创建渲染管线状态对象
		{
			// Define the vertex input layout.
			D3D12_INPUT_ELEMENT_DESC stInputElementDescs[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
			};

			D3D12_INPUT_ELEMENT_DESC stInputElementDescsDeferred[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				//!!!!!!!!!!!!!!!!!!!!!!!!!!!添加输入布局
				{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
			};

			// 创建 graphics pipeline state object (PSO)对象
			D3D12_GRAPHICS_PIPELINE_STATE_DESC stPSODesc = {};
			stPSODesc.InputLayout = { stInputElementDescsDeferred, _countof(stInputElementDescsDeferred) };
			stPSODesc.pRootSignature = pIRootSignatureDeferred.Get();

			stPSODesc.VS.pShaderBytecode = pIBlobVertexShaderDeferred->GetBufferPointer();
			stPSODesc.VS.BytecodeLength = pIBlobVertexShaderDeferred->GetBufferSize();

			stPSODesc.PS.pShaderBytecode = pIBlobPixelShaderDeferred->GetBufferPointer();
			stPSODesc.PS.BytecodeLength = pIBlobPixelShaderDeferred->GetBufferSize();

			stPSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
			stPSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;

			stPSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			stPSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
			stPSODesc.DepthStencilState.DepthEnable = true;//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			stPSODesc.SampleMask = UINT_MAX;
			stPSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			stPSODesc.NumRenderTargets = 2;
			stPSODesc.RTVFormats[0] = emRenderTarget;
			stPSODesc.RTVFormats[1] = emRenderTarget;
			stPSODesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			stPSODesc.SampleDesc.Count = 1;

			GRS_THROW_IF_FAILED(pID3D12Device4->CreateGraphicsPipelineState(&stPSODesc
				, IID_PPV_ARGS(&pIPipelineStateDeferred)));



			//——————————————————————————————————————————————————————
#if defined(_DEBUG)
// Enable better shader debugging with the graphics debugging tools.
			UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
			UINT compileFlags = 0;
#endif

			//编译为行矩阵形式	   new
			compileFlags |= D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;

			TCHAR pszShaderFileName[MAX_PATH] = {};
			StringCchPrintf(pszShaderFileName, MAX_PATH, _T("%sShader\\shaders.hlsl"), pszAppPath);
			GRS_THROW_IF_FAILED(D3DCompileFromFile(pszShaderFileName, nullptr, nullptr
				, "VSMain", "vs_5_0", compileFlags, 0, &pIBlobVertexShader, nullptr));
			GRS_THROW_IF_FAILED(D3DCompileFromFile(pszShaderFileName, nullptr, nullptr
				, "PSMain", "ps_5_0", compileFlags, 0, &pIBlobPixelShader, nullptr));



			stPSODesc.InputLayout = { stInputElementDescs, _countof(stInputElementDescs) };
			stPSODesc.pRootSignature = pIRootSignature.Get();

			stPSODesc.VS.pShaderBytecode = pIBlobVertexShader->GetBufferPointer();
			stPSODesc.VS.BytecodeLength = pIBlobVertexShader->GetBufferSize();

			stPSODesc.PS.pShaderBytecode = pIBlobPixelShader->GetBufferPointer();
			stPSODesc.PS.BytecodeLength = pIBlobPixelShader->GetBufferSize();
			GRS_THROW_IF_FAILED(pID3D12Device4->CreateGraphicsPipelineState(&stPSODesc
				, IID_PPV_ARGS(&pIPipelineState)));

		}
		//——————————————————————————————————————————————————————
		//创建出深度模板缓冲区和相关的view
		{

			D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
			depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
			depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

			D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
			depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
			depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
			depthOptimizedClearValue.DepthStencil.Stencil = 0;

			CD3DX12_HEAP_PROPERTIES heapProperties2 = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
			CD3DX12_RESOURCE_DESC tex2D = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, iWidth, iHeight, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

			GRS_THROW_IF_FAILED(pID3D12Device4->CreateCommittedResource(
				&heapProperties2,
				D3D12_HEAP_FLAG_NONE,
				&tex2D,
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				&depthOptimizedClearValue,
				IID_PPV_ARGS(&pIDepthStencilBuffer)));

			pID3D12Device4->CreateDepthStencilView(pIDepthStencilBuffer.Get(), &depthStencilDesc, pIDSVHeap->GetCPUDescriptorHandleForHeapStart());

		}
		//——————————————————————————————————————————————————————



		//创建出两个作为Gbuffer的纹理
		{
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
			texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

			D3D12_CLEAR_VALUE stClear = {};
			stClear.Format = emRenderTarget;
			memcpy(&stClear.Color, &f4RTTexClearColor, 4 * sizeof(float));
			D3D12_HEAP_PROPERTIES stHeapProp = { D3D12_HEAP_TYPE_DEFAULT };

			GRS_THROW_IF_FAILED(pID3D12Device4->CreateCommittedResource(
				&stHeapProp,
				D3D12_HEAP_FLAG_NONE,
				&texDesc,
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				&stClear,
				IID_PPV_ARGS(&pIDeferredTextureA)));

			GRS_THROW_IF_FAILED(pID3D12Device4->CreateCommittedResource(
				&stHeapProp,
				D3D12_HEAP_FLAG_NONE,
				&texDesc,
				D3D12_RESOURCE_STATE_RENDER_TARGET,
				&stClear,
				IID_PPV_ARGS(&pIDeferredTextureB)));
		}

		//13、创建纹理的默认堆
		{
			D3D12_HEAP_DESC stTextureHeapDesc = {};
			stTextureHeapDesc.SizeInBytes = GRS_UPPER(2 * nPicRowPitch * nTextureH, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
			stTextureHeapDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
			stTextureHeapDesc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;		//默认堆类型
			stTextureHeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			stTextureHeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			stTextureHeapDesc.Flags = D3D12_HEAP_FLAG_DENY_RT_DS_TEXTURES | D3D12_HEAP_FLAG_DENY_BUFFERS;

			GRS_THROW_IF_FAILED(pID3D12Device4->CreateHeap(&stTextureHeapDesc, IID_PPV_ARGS(&pITextureHeap)));
		}

		//14、创建2D纹理
		{

			// 根据图片信息，填充2D纹理资源的信息结构体
			stTextureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			stTextureDesc.MipLevels = 1;
			stTextureDesc.Format = stTextureFormat;
			stTextureDesc.Width = nTextureW;
			stTextureDesc.Height = nTextureH;
			stTextureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			stTextureDesc.DepthOrArraySize = 1;
			stTextureDesc.SampleDesc.Count = 1;
			stTextureDesc.SampleDesc.Quality = 0;


			//-----------------------------------------------------------------------------------------------------------
			//使用“定位方式”来创建纹理，注意下面这个调用内部实际已经没有存储分配和释放的实际操作了，所以性能很高
			//同时可以在这个堆上反复调用CreatePlacedResource来创建不同的纹理，当然前提是它们不在被使用的时候，才考虑
			//重用堆
			GRS_THROW_IF_FAILED(pID3D12Device4->CreatePlacedResource(
				pITextureHeap.Get()
				, 0
				, &stTextureDesc				//可以使用CD3DX12_RESOURCE_DESC::Tex2D来简化结构体的初始化
				, D3D12_RESOURCE_STATE_COPY_DEST
				, nullptr
				, IID_PPV_ARGS(&pITexBaseColor)));
			//-----------------------------------------------------------------------------------------------------------
			//获取上传堆资源缓冲的大小，这个尺寸通常大于实际图片的尺寸
			D3D12_RESOURCE_DESC stCopyDstDesc = pITexBaseColor->GetDesc();
			n64UploadBufferSize = 0;
			pID3D12Device4->GetCopyableFootprints(&stCopyDstDesc, 0, 1, 0, nullptr, nullptr, nullptr, &n64UploadBufferSize);

			D3D12_HEAP_PROPERTIES stHeapProp = { D3D12_HEAP_TYPE_DEFAULT };
			stTextureDesc.Format = stTextureFormat;
			GRS_THROW_IF_FAILED(pID3D12Device4->CreateCommittedResource(
				&stHeapProp,
				D3D12_HEAP_FLAG_NONE,
				&stTextureDesc,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(&pITexNormal)));
			stTextureDesc.Format = stTextureFormat;

			GRS_THROW_IF_FAILED(pID3D12Device4->CreateCommittedResource(
				&stHeapProp,
				D3D12_HEAP_FLAG_NONE,
				&stTextureDesc,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(&pITexMetallic)));
			stTextureDesc.Format = stTextureFormat;
			GRS_THROW_IF_FAILED(pID3D12Device4->CreateCommittedResource(
				&stHeapProp,
				D3D12_HEAP_FLAG_NONE,
				&stTextureDesc,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(&pITexRoughness)));
		}

		//15、创建上传堆
		{
			D3D12_HEAP_DESC stUploadHeapDesc = {  };
			stUploadHeapDesc.SizeInBytes = GRS_UPPER(2 * n64UploadBufferSize, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);
			stUploadHeapDesc.Alignment = 0;
			stUploadHeapDesc.Properties.Type = D3D12_HEAP_TYPE_UPLOAD;		//上传堆类型
			stUploadHeapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			stUploadHeapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			stUploadHeapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_BUFFERS;

			GRS_THROW_IF_FAILED(pID3D12Device4->CreateHeap(&stUploadHeapDesc, IID_PPV_ARGS(&pIUploadHeap)));
		}

		//16、使用“定位方式”创建用于上传纹理数据的缓冲资源
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
				, IID_PPV_ARGS(&pITexBaseColorUpload)));


			D3D12_HEAP_PROPERTIES stHeapProp = { D3D12_HEAP_TYPE_UPLOAD };

			GRS_THROW_IF_FAILED(pID3D12Device4->CreateCommittedResource(
				&stHeapProp,
				D3D12_HEAP_FLAG_NONE,
				&stUploadResDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&pITexNormalUpload)));
			GRS_THROW_IF_FAILED(pID3D12Device4->CreateCommittedResource(
				&stHeapProp,
				D3D12_HEAP_FLAG_NONE,
				&stUploadResDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&pITexMetallicUpload)));
			GRS_THROW_IF_FAILED(pID3D12Device4->CreateCommittedResource(
				&stHeapProp,
				D3D12_HEAP_FLAG_NONE,
				&stUploadResDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&pITexRoughnessUpload)));

		}

		//17、加载图片数据至上传堆，即完成第一个Copy动作，从memcpy函数可知这是由CPU完成的
		{
			stTxtLayoutsBaseColor = CopyToUploadHeap(pITexBaseColor, pITexBaseColorUpload, pIBMPBaseColor, pID3D12Device4, n64UploadBufferSize, nTextureH, nPicRowPitch);
			stTxtLayoutsNormal = CopyToUploadHeap(pITexNormal, pITexNormalUpload, pIBMPNormal, pID3D12Device4, n64UploadBufferSize, nTextureH, nPicRowPitch);
			stTxtLayoutsMetallic = CopyToUploadHeap(pITexMetallic, pITexMetallicUpload, pIBMPMetallic, pID3D12Device4, n64UploadBufferSize, nTextureH, nPicRowPitch);
			stTxtLayoutsRoughness = CopyToUploadHeap(pITexRoughness, pITexRoughnessUpload, pIBMPRoughness, pID3D12Device4, n64UploadBufferSize, nTextureH, nPicRowPitch);
		}

		//18、把纹理从上传堆复制到默认堆
		{
			//向直接命令列表发出从上传堆复制纹理数据到默认堆的命令，执行并同步等待，即完成第二个Copy动作，由GPU上的复制引擎完成
				//注意此时直接命令列表还没有绑定PSO对象，因此它也是不能执行3D图形命令的，但是可以执行复制命令，因为复制引擎不需要什么
				//额外的状态设置之类的参数
			D3D12_TEXTURE_COPY_LOCATION stDstCopyLocation = {};
			stDstCopyLocation.pResource = pITexBaseColor.Get();
			stDstCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			stDstCopyLocation.SubresourceIndex = 0;

			D3D12_TEXTURE_COPY_LOCATION stSrcCopyLocation = {};
			stSrcCopyLocation.pResource = pITexBaseColorUpload.Get();
			stSrcCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			stSrcCopyLocation.PlacedFootprint = stTxtLayoutsBaseColor;


			pICMDList->CopyTextureRegion(&stDstCopyLocation, 0, 0, 0, &stSrcCopyLocation, nullptr);

			stDstCopyLocation.pResource = pITexNormal.Get();
			stSrcCopyLocation.pResource = pITexNormalUpload.Get();
			pICMDList->CopyTextureRegion(&stDstCopyLocation, 0, 0, 0, &stSrcCopyLocation, nullptr);

			stDstCopyLocation.pResource = pITexMetallic.Get();
			stSrcCopyLocation.pResource = pITexMetallicUpload.Get();
			pICMDList->CopyTextureRegion(&stDstCopyLocation, 0, 0, 0, &stSrcCopyLocation, nullptr);

			stDstCopyLocation.pResource = pITexRoughness.Get();
			stSrcCopyLocation.pResource = pITexRoughnessUpload.Get();
			pICMDList->CopyTextureRegion(&stDstCopyLocation, 0, 0, 0, &stSrcCopyLocation, nullptr);

			//设置一个资源屏障，同步并确认复制操作完成
			//直接使用结构体然后调用的形式
			D3D12_RESOURCE_BARRIER stResBar = {};
			stResBar.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			stResBar.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			stResBar.Transition.pResource = pITexBaseColor.Get();
			stResBar.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
			stResBar.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
			stResBar.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

			pICMDList->ResourceBarrier(1, &stResBar);

			stResBar.Transition.pResource = pITexNormal.Get();
			pICMDList->ResourceBarrier(1, &stResBar);

			stResBar.Transition.pResource = pITexMetallic.Get();
			pICMDList->ResourceBarrier(1, &stResBar);

			stResBar.Transition.pResource = pITexRoughness.Get();
			pICMDList->ResourceBarrier(1, &stResBar);


			//或者使用D3DX12库中的工具类调用的等价形式，下面的方式更简洁一些
			//pICMDList->ResourceBarrier(1
			//	, &CD3DX12_RESOURCE_BARRIER::Transition(pITexture.Get()
			//	, D3D12_RESOURCE_STATE_COPY_DEST
			//	, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
			//);

			//---------------------------------------------------------------------------------------------
			// 执行命令列表并等待纹理资源上传完成，这一步是必须的
			GRS_THROW_IF_FAILED(pICMDList->Close());
			ID3D12CommandList* ppCommandLists[] = { pICMDList.Get() };
			pICMDQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

			//---------------------------------------------------------------------------------------------
			// 17、创建一个同步对象——围栏，用于等待渲染完成，因为现在Draw Call是异步的了
			GRS_THROW_IF_FAILED(pID3D12Device4->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pIFence)));
			n64FenceValue = 1;

			//---------------------------------------------------------------------------------------------
			// 18、创建一个Event同步对象，用于等待围栏事件通知
			hEventFence = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			if (hEventFence == nullptr)
			{
				GRS_THROW_IF_FAILED(HRESULT_FROM_WIN32(GetLastError()));
			}

			//---------------------------------------------------------------------------------------------
			// 19、等待纹理资源正式复制完成先
			const UINT64 fence = n64FenceValue;
			GRS_THROW_IF_FAILED(pICMDQueue->Signal(pIFence.Get(), fence));
			n64FenceValue++;
			GRS_THROW_IF_FAILED(pIFence->SetEventOnCompletion(fence, hEventFence));

			WaitForSingleObject(hEventFence, INFINITE);
		}

		ST_GRS_VERTEX quadVertex[] = {
			{ {-1.0f,  1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
			 { {1.0f,  1.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
			 { {1.0f,  -1.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
			 { {-1.0f,  -1.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
		};

		UINT32 quadIndices[] = {
			0,1,2,
			0,2,3
		};


		const std::uint32_t vertexBufferSize = meshes[0].vertices.size() * sizeof(Vertex);
		const std::uint32_t indexBufferSize = meshes[0].indices.size() * sizeof(std::uint32_t);
		//20、创建顶点缓冲
		{

			D3D12_HEAP_PROPERTIES stHeapProp = { D3D12_HEAP_TYPE_UPLOAD };

			D3D12_RESOURCE_DESC stResSesc = {};
			stResSesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			stResSesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			stResSesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			stResSesc.Format = DXGI_FORMAT_UNKNOWN;
			stResSesc.Width = vertexBufferSize;
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
				IID_PPV_ARGS(&pIVertexBufferDeferred)));

			UINT8* pVertexDataBegin = nullptr;
			D3D12_RANGE stReadRange = { 0, 0 };		// We do not intend to read from this resource on the CPU.
			GRS_THROW_IF_FAILED(pIVertexBufferDeferred->Map(0, &stReadRange, reinterpret_cast<void**>(&pVertexDataBegin)));
			memcpy(pVertexDataBegin, &meshes[0].vertices[0], vertexBufferSize);
			pIVertexBufferDeferred->Unmap(0, nullptr);

			stVertexBufferViewDeferred.BufferLocation = pIVertexBufferDeferred->GetGPUVirtualAddress();
			stVertexBufferViewDeferred.StrideInBytes = sizeof(Vertex);//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			stVertexBufferViewDeferred.SizeInBytes = vertexBufferSize;

			//——————————————————————————————————————————————————————————

			const UINT quadVertexSize = sizeof(quadVertex);
			stResSesc.Width = quadVertexSize;

			GRS_THROW_IF_FAILED(pID3D12Device4->CreateCommittedResource(
				&stHeapProp,
				D3D12_HEAP_FLAG_NONE,
				&stResSesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&pIVertexBuffer)));

			GRS_THROW_IF_FAILED(pIVertexBuffer->Map(0, &stReadRange, reinterpret_cast<void**>(&pVertexDataBegin)));
			memcpy(pVertexDataBegin, quadVertex, sizeof(quadVertex));
			pIVertexBuffer->Unmap(0, nullptr);

			stVertexBufferView.BufferLocation = pIVertexBuffer->GetGPUVirtualAddress();
			stVertexBufferView.StrideInBytes = sizeof(ST_GRS_VERTEX);
			stVertexBufferView.SizeInBytes = quadVertexSize;
			//——————————————————————————————————————————————————————————
		}

		//21、创建索引缓冲区		
		{




			D3D12_HEAP_PROPERTIES stHeapProp = { D3D12_HEAP_TYPE_UPLOAD };

			D3D12_RESOURCE_DESC stIBResDesc = {};
			stIBResDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			stIBResDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
			stIBResDesc.Width = indexBufferSize;
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
				IID_PPV_ARGS(&pIIndexBufferDeferred)));

			UINT8* pIndexDataBegin = nullptr;
			D3D12_RANGE stReadRange = { 0, 0 };
			GRS_THROW_IF_FAILED(pIIndexBufferDeferred->Map(0, &stReadRange, reinterpret_cast<void**>(&pIndexDataBegin)));
			memcpy(pIndexDataBegin, &meshes[0].indices[0], indexBufferSize);
			pIIndexBufferDeferred->Unmap(0, nullptr);

			stIndexBufferViewDeferred.BufferLocation = pIIndexBufferDeferred->GetGPUVirtualAddress();
			stIndexBufferViewDeferred.Format = DXGI_FORMAT_R32_UINT;
			stIndexBufferViewDeferred.SizeInBytes = indexBufferSize;


			//————————————————————————————————————————————

			const UINT quadIndexSize = sizeof(quadIndices);


			stIBResDesc.Width = quadIndexSize;


			GRS_THROW_IF_FAILED(pID3D12Device4->CreateCommittedResource(
				&stHeapProp,
				D3D12_HEAP_FLAG_NONE,
				&stIBResDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&pIIndexBuffer)));

			GRS_THROW_IF_FAILED(pIIndexBuffer->Map(0, &stReadRange, reinterpret_cast<void**>(&pIndexDataBegin)));
			memcpy(pIndexDataBegin, quadIndices, quadIndexSize);
			pIIndexBuffer->Unmap(0, nullptr);

			stIndexBufferView.BufferLocation = pIIndexBuffer->GetGPUVirtualAddress();
			stIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
			stIndexBufferView.SizeInBytes = quadIndexSize;


			//————————————————————————————————————————————


		}

		//22、在上传堆上以“定位方式”创建常量缓冲
		{
			//这里没有把顶点和索引加入到默认堆中，所以这里不需要加上索引。
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
			// 创建常量缓冲 注意缓冲尺寸设置为256边界对齐大小
			GRS_THROW_IF_FAILED(pID3D12Device4->CreatePlacedResource(
				pIUploadHeap.Get()
				, n64BufferOffset
				, &stMVPResDesc
				, D3D12_RESOURCE_STATE_GENERIC_READ
				, nullptr
				, IID_PPV_ARGS(&pICBVUpload)));

			// Map 之后就不再Unmap了 直接复制数据进去 这样每帧都不用map-copy-unmap浪费时间了
			GRS_THROW_IF_FAILED(pICBVUpload->Map(0, nullptr, reinterpret_cast<void**>(&pMVPBuffer)));

		}

		//23、最终创建SRV描述符   
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC stSRVDesc = {};
			stSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			stSRVDesc.Format = stTextureDesc.Format;
			stSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			stSRVDesc.Texture2D.MipLevels = 1;
			pID3D12Device4->CreateShaderResourceView(pITexBaseColor.Get(), &stSRVDesc, pISRVHeap->GetCPUDescriptorHandleForHeapStart());

			CD3DX12_CPU_DESCRIPTOR_HANDLE stCPUHandle(pISRVHeap->GetCPUDescriptorHandleForHeapStart());
			stCPUHandle.Offset(2, nSRVDescriptorSize);
			stSRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			stSRVDesc.Format = stTextureDesc.Format;
			stSRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			stSRVDesc.Texture2D.MipLevels = 1;
			pID3D12Device4->CreateShaderResourceView(pIDeferredTextureA.Get(), &stSRVDesc, stCPUHandle);

			stCPUHandle.Offset(1, nSRVDescriptorSize);
			pID3D12Device4->CreateShaderResourceView(pIDeferredTextureB.Get(), &stSRVDesc, stCPUHandle);

			//为新增的纹理创建出相应的描述符
			stCPUHandle.Offset(1, nSRVDescriptorSize);
			pID3D12Device4->CreateShaderResourceView(pITexMetallic.Get(), &stSRVDesc, stCPUHandle);
			stCPUHandle.Offset(1, nSRVDescriptorSize);
			pID3D12Device4->CreateShaderResourceView(pITexNormal.Get(), &stSRVDesc, stCPUHandle);
			stCPUHandle.Offset(1, nSRVDescriptorSize);
			pID3D12Device4->CreateShaderResourceView(pITexRoughness.Get(), &stSRVDesc, stCPUHandle);

		}

		//24、创建CBV描述符
		{
			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.BufferLocation = pICBVUpload->GetGPUVirtualAddress();
			cbvDesc.SizeInBytes = static_cast<UINT>(szMVPBuffer);

			D3D12_CPU_DESCRIPTOR_HANDLE stCPUCBVHandle = pISRVHeap->GetCPUDescriptorHandleForHeapStart();
			stCPUCBVHandle.ptr += nSRVDescriptorSize;

			pID3D12Device4->CreateConstantBufferView(&cbvDesc, stCPUCBVHandle);
		}
		//————————————————————————————————————————————
		//创建RTV
		{
			CD3DX12_CPU_DESCRIPTOR_HANDLE stRTVHandle(pIRTVHeap->GetCPUDescriptorHandleForHeapStart());
			stRTVHandle.Offset(3, nRTVDescriptorSize);
			pID3D12Device4->CreateRenderTargetView(pIDeferredTextureA.Get(), nullptr, stRTVHandle);
			stRTVHandle.Offset(1, nRTVDescriptorSize);
			pID3D12Device4->CreateRenderTargetView(pIDeferredTextureB.Get(), nullptr, stRTVHandle);
		}
		//————————————————————————————————————————————

		//25、填充资源屏障结构，为下面的循环所准备的
		D3D12_RESOURCE_BARRIER stBeginResBarrier = {};
		stBeginResBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		stBeginResBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		stBeginResBarrier.Transition.pResource = pIARenderTargets[nFrameIndex].Get();
		stBeginResBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		stBeginResBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		stBeginResBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		D3D12_RESOURCE_BARRIER stEndResBarrier = {};
		stEndResBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		stEndResBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		stEndResBarrier.Transition.pResource = pIARenderTargets[nFrameIndex].Get();
		stEndResBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		stEndResBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		stEndResBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

		//26、记录帧开始时间，和当前时间，以循环结束为界
		ULONGLONG n64tmFrameStart = ::GetTickCount64();
		ULONGLONG n64tmCurrent = n64tmFrameStart;
		//计算旋转角度需要的变量
		double dModelRotationYAngle = 0.0f;


		//27、开始消息循环，并在其中不断渲染
		ShowWindow(hWnd, nCmdShow);
		UpdateWindow(hWnd);


		//initialize the property of parameter
		pMVPBuffer->lightDir = XMFLOAT3(239/256.0, 14/256.0, 0);
		pMVPBuffer->baseColorIntensity = 3.597;
		pMVPBuffer->metallicIntensity =0.888;
		pMVPBuffer->roughnessIntensity = 0.905;



		//开始消息循环，并在其中不断渲染，这里循环框架和作者不一样，因为我是为了和上次讲的衔接一下，其次代码原作者的框架高级一些，等到我们玩封装的时候再使用那套。
		while (msg.message != WM_QUIT)
		{
			if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			else
			{

				ImGui_ImplDX12_NewFrame();
				ImGui_ImplWin32_NewFrame();
				ImGui::NewFrame();

				bool show_demo_window = true;
				//ImGui::ShowDemoWindow(&show_demo_window);
				ImGui::Begin("Clear Control Pannel");
				float lightDir[3];
				ImGui::ColorEdit3("lightDir", lightDir);
				pMVPBuffer->lightDir = XMFLOAT3(lightDir);
				ImGui::SliderFloat("BaseColor", &(pMVPBuffer->baseColorIntensity), 0.2, 4.0);
				ImGui::SliderFloat("Metallic", &(pMVPBuffer->metallicIntensity), 0.2, 2.0);
				ImGui::SliderFloat("Roughness", &(pMVPBuffer->roughnessIntensity), 0.2, 2.0);
				ImGui::SliderFloat("subsurface", &(pMVPBuffer->subsurface), 0.0, 1.0);
				ImGui::SliderFloat("specular", &(pMVPBuffer->_specular), 0.2, 2.0);
				ImGui::SliderFloat("specularTint", &(pMVPBuffer->specularTint), 0.2, 2.0);
				ImGui::SliderFloat("anisotropic", &(pMVPBuffer->anisotropic), 0.2, 2.0);
				ImGui::SliderFloat("sheen", &(pMVPBuffer->sheen), 0.2, 2.0);
				ImGui::SliderFloat("sheenTint", &(pMVPBuffer->sheenTint), 0.2, 2.0);
				ImGui::SliderFloat("clearcoat", &(pMVPBuffer->clearcoat), 0.2, 2.0);
				ImGui::SliderFloat("clearcoatGloss", &(pMVPBuffer->clearcoatGloss), 0.2, 2.0);
				
				ImGui::End();
				ImGui::Render();





				//---------------------------------------------------------------------------------------------
				//Update
				//---------------------------------------------------------------------------------------------
				{
					float x = Radius * sinf(Phi) * cosf(Theta);
					float z = Radius * sinf(Phi) * sinf(Theta);
					float y = Radius * cosf(Phi);

					// Build the view matrix.
					XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
					XMVECTOR target = XMVectorZero();
					XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

					XMMATRIX view = XMMatrixLookAtLH(pos, target, up);


					n64tmCurrent = ::GetTickCount64();
					//dModelRotationYAngle += ((n64tmCurrent - n64tmFrameStart) / 1000.0f) * g_fPalstance;

					n64tmFrameStart = n64tmCurrent;

					//旋转角度是2PI周期的倍数，去掉周期数，只留下相对0弧度开始的小于2PI的弧度即可
					if (dModelRotationYAngle > XM_2PI)
					{
						dModelRotationYAngle = fmod(dModelRotationYAngle, XM_2PI);
					}
					XMMATRIX xmScale = XMMatrixScaling(10, 10, 10);
					//模型矩阵 model
					XMMATRIX xmScaleRot = XMMatrixMultiply(xmScale, XMMatrixRotationY(static_cast<float>(dModelRotationYAngle)));

					XMStoreFloat4x4(&pMVPBuffer->m_ObjectToWorld, xmScaleRot);
					pMVPBuffer->m_ViewPos = pos;
					

					//计算 模型矩阵 model * 视矩阵 view
					XMMATRIX xmMVP = XMMatrixMultiply(xmScaleRot, view);

					//投影矩阵 projection
					xmMVP = XMMatrixMultiply(xmMVP, (XMMatrixPerspectiveFovLH(XM_PIDIV4, (FLOAT)iWidth / (FLOAT)iHeight, 0.1f, 1000.0f)));
					//计算出旋转矩阵，然后拷贝到上传堆中的常量缓冲区中。
					XMStoreFloat4x4(&pMVPBuffer->m_MVP, xmMVP);
				}


				GRS_THROW_IF_FAILED(pICMDAlloc->Reset());
				GRS_THROW_IF_FAILED(pICMDList->Reset(pICMDAlloc.Get(), pIPipelineStateDeferred.Get()));

				nFrameIndex = pISwapChain3->GetCurrentBackBufferIndex();




				D3D12_CPU_DESCRIPTOR_HANDLE stRTVHandle = pIRTVHeap->GetCPUDescriptorHandleForHeapStart();
				//stRTVHandle.ptr += (nFrameIndex * nRTVDescriptorSize);
				stRTVHandle.ptr += 3 * nRTVDescriptorSize;
				CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(pIDSVHeap->GetCPUDescriptorHandleForHeapStart());
				pICMDList->OMSetRenderTargets(2, &stRTVHandle, TRUE, &dsvHandle);//改为TURE
				//https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12graphicscommandlist-omsetrendertargets

				pICMDList->RSSetViewports(1, &stViewPort);
				pICMDList->RSSetScissorRects(1, &stScissorRect);
				pICMDList->ClearRenderTargetView(stRTVHandle, f4RTTexClearColor, 0, nullptr);
				stRTVHandle.ptr += nRTVDescriptorSize;
				pICMDList->ClearRenderTargetView(stRTVHandle, f4RTTexClearColor, 0, nullptr);
				pICMDList->ClearDepthStencilView(pIDSVHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

				pICMDList->SetGraphicsRootSignature(pIRootSignatureDeferred.Get());
				ID3D12DescriptorHeap* ppHeaps[] = { pISRVHeap.Get() };
				pICMDList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

				D3D12_GPU_DESCRIPTOR_HANDLE stGPUSRVHandle = pISRVHeap->GetGPUDescriptorHandleForHeapStart();
				pICMDList->SetGraphicsRootDescriptorTable(0, stGPUSRVHandle);//对应第一个SRV即basecolor
				stGPUSRVHandle.ptr += nSRVDescriptorSize;
				pICMDList->SetGraphicsRootDescriptorTable(1, stGPUSRVHandle);//对应CBV
				//描述符堆上第一个是basecolor，第二个是cbv，第三四个是作为Gbufffer的俩srv，四五六才是三个新加的纹理
				stGPUSRVHandle.ptr += 3 * nSRVDescriptorSize;
				pICMDList->SetGraphicsRootDescriptorTable(2, stGPUSRVHandle);

				pICMDList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				pICMDList->IASetVertexBuffers(0, 1, &stVertexBufferViewDeferred);
				pICMDList->IASetIndexBuffer(&stIndexBufferViewDeferred);

				pICMDList->DrawIndexedInstanced(meshes[0].indices.size(), 1, 0, 0, 0);


				GRS_THROW_IF_FAILED(pICMDList->Close());

				ID3D12CommandList* ppCommandLists[] = { pICMDList.Get() };
				pICMDQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);


				//这里不要重置命令分配器
				GRS_THROW_IF_FAILED(pICMDList->Reset(pICMDAlloc.Get(), pIPipelineState.Get()));


				CD3DX12_RESOURCE_BARRIER H = CD3DX12_RESOURCE_BARRIER::Transition(pIDeferredTextureA.Get(),
					D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				pICMDList->ResourceBarrier(1, &H);
				CD3DX12_RESOURCE_BARRIER A = CD3DX12_RESOURCE_BARRIER::Transition(pIDeferredTextureB.Get(),
					D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
				pICMDList->ResourceBarrier(1, &A);

				//-------------------------------------------------------------——————————————————————

				stBeginResBarrier.Transition.pResource = pIARenderTargets[nFrameIndex].Get();
				pICMDList->ResourceBarrier(1, &stBeginResBarrier);

				stRTVHandle.ptr -= 4 * nRTVDescriptorSize;
				stRTVHandle.ptr += (nFrameIndex * nRTVDescriptorSize);
				pICMDList->OMSetRenderTargets(1, &stRTVHandle, FALSE, &dsvHandle);//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

				pICMDList->RSSetViewports(1, &stViewPort);
				pICMDList->RSSetScissorRects(1, &stScissorRect);
				pICMDList->ClearRenderTargetView(stRTVHandle, f4RTTexClearColor, 0, nullptr);

				pICMDList->SetGraphicsRootSignature(pIRootSignature.Get());
				pICMDList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

				stGPUSRVHandle = pISRVHeap->GetGPUDescriptorHandleForHeapStart();
				stGPUSRVHandle.ptr += 2 * nSRVDescriptorSize;
				pICMDList->SetGraphicsRootDescriptorTable(0, stGPUSRVHandle);
				stGPUSRVHandle.ptr += nSRVDescriptorSize;
				pICMDList->SetGraphicsRootDescriptorTable(1, stGPUSRVHandle);

				//注意我们使用的渲染手法是三角形列表，也就是通常的Mesh网格
				pICMDList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				pICMDList->IASetVertexBuffers(0, 1, &stVertexBufferView);
				pICMDList->IASetIndexBuffer(&stIndexBufferView);

				pICMDList->DrawIndexedInstanced(_countof(quadIndices), 1, 0, 0, 0);




				pICMDList->SetDescriptorHeaps(1, pISRVHeapImgui.GetAddressOf());
				ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), pICMDList.Get());


				stEndResBarrier.Transition.pResource = pIARenderTargets[nFrameIndex].Get();
				pICMDList->ResourceBarrier(1, &stEndResBarrier);

				CD3DX12_RESOURCE_BARRIER B = CD3DX12_RESOURCE_BARRIER::Transition(pIDeferredTextureA.Get(),
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
				pICMDList->ResourceBarrier(1, &B);
				CD3DX12_RESOURCE_BARRIER C = CD3DX12_RESOURCE_BARRIER::Transition(pIDeferredTextureB.Get(),
					D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
				pICMDList->ResourceBarrier(1, &C);

				GRS_THROW_IF_FAILED(pICMDList->Close());

				ppCommandLists[0] = { pICMDList.Get() };
				pICMDQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);


				GRS_THROW_IF_FAILED(pISwapChain3->Present(1, 0));


				//---------------------------------------------------------------------------------------------
				//开始同步GPU与CPU的执行，先记录围栏标记值
				//---------------------------------------------------------------------------------------------
				//如果不同步，下一轮循环就会马上Reset命令分配器，而如果此时GPU还没有执行完命令就给清空了，就会出问题
				const UINT64 fence = n64FenceValue;
				GRS_THROW_IF_FAILED(pICMDQueue->Signal(pIFence.Get(), fence));
				n64FenceValue++;
				GRS_THROW_IF_FAILED(pIFence->SetEventOnCompletion(fence, hEventFence));

				WaitForSingleObject(hEventFence, INFINITE);//同步！
			}
		}

		//destruct imgui
		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}
	catch (CGRSCOMException& e)
	{//发生了COM异常
		e;
	}
	return 0;
}


//-----------------------------------【窗口处理函数】--------------------------------------------
//里面主要是通过我们输入，修改观察点的位置和物体旋转的速度
//-----------------------------------------------------------------------------------------------


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam)) {
		return true;
	}
	//直接加上下面一句会出错。没初始化上下文，因为这个窗口创建的时候就需要执行了。
	const ImGuiIO imio = ImGui::GetIO();
	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_KEYDOWN:
	{
		USHORT n16KeyCode = (wParam & 0xFF);

		//根据用户输入变换
		//XMVECTOR g_v4EyePos = XMVectorSet(0.0f, 5.0f, -10.0f, 0.0f); //眼睛位置
		//XMVECTOR g_v4LookAt = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);  //眼睛所盯的位置
		//XMVECTOR g_v4UpDir = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);  //头部正上方位置

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
			//double g_fPalstance = 10.0f * XM_PI / 180.0f;	//物体旋转的角速度，单位：弧度/秒
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

	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
		LastMousePos.x = GET_X_LPARAM(lParam);
		LastMousePos.y = GET_Y_LPARAM(lParam);

		SetCapture(hWnd);
		return 0;
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
		ReleaseCapture();
		return 0;
	case WM_MOUSEMOVE:
		if (imio.WantCaptureMouse) {
			break;
		}
		{
			if ((wParam & MK_LBUTTON) != 0)
			{
				// Make each pixel correspond to a quarter of a degree.
				float dx = XMConvertToRadians(0.25f * static_cast<float>(GET_X_LPARAM(lParam) - LastMousePos.x));
				float dy = XMConvertToRadians(0.25f * static_cast<float>(GET_Y_LPARAM(lParam) - LastMousePos.y));

				// Update angles based on input to orbit camera around box.
				Theta -= dx;
				Phi -= dy;

				// Restrict the angle mPhi.
				Phi = Phi < 0.1f ? 0.1f : (Phi > 3.1415926535f - 0.1f ? 3.1415926535f - 0.1f : Phi);
			}
			else if ((wParam & MK_RBUTTON) != 0)
			{
				// Make each pixel correspond to 0.005 unit in the scene.
				float dx = 0.005f * static_cast<float>(GET_X_LPARAM(lParam) - LastMousePos.x);
				float dy = 0.005f * static_cast<float>(GET_Y_LPARAM(lParam) - LastMousePos.y);

				// Update the camera radius based on input.
				Radius += dx - dy;
				// Restrict the radius.
				Radius = Radius < 10.0f ? 10.0f : (Radius > 25.0f ? 25.0f : Radius);
			}

			LastMousePos.x = GET_X_LPARAM(lParam);
			LastMousePos.y = GET_Y_LPARAM(lParam);
		}
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
