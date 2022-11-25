//-----------------------------------【头文件包含】-----------------------------------
#include<Windows.h>
//------------------------------------------------------------------------------------


//-----------------------------------【宏定义部分】-----------------------------------
#define WINDOWTITLE L"致我们永不熄灭的游戏开发梦想~"
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
//------------------------------------------------------------------------------------


//-----------------------------------【全局变量部分】-----------------------------------
float g_posX = 0;
float g_posY = 0;
HINSTANCE g_hInstance = 0;
int g_nCmdShow = 0;
HWND g_hwnd = 0;
//------------------------------------------------------------------------------------



//-----------------------------------【函数声明部分】-----------------------------------
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void InitWindow();//初始化窗口
void Init();//初始化程序
void UpdateScene();//更新绘制的数据
void DrawScene();//进行绘制操作
void Run();
//------------------------------------------------------------------------------------




//-----------------------------------【WinMain函数】-----------------------------------
//Win32程序的入口
//------------------------------------------------------------------------------------
int WINAPI WinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpCmdLine,
	_In_ int nCmdShow)
{
	g_hInstance = hInstance;
	g_nCmdShow = nCmdShow;

	Init();

	Run();

	return 0;
}
//-----------------------------------【初始化创建窗口】-----------------------------------
// 初始化窗口
//--------------------------------------------------------------------------------------
void InitWindow()
{
	WNDCLASSEX wndClass = { 0 };
	wndClass.cbSize = sizeof(WNDCLASSEX);
	wndClass.style = CS_HREDRAW | CS_VREDRAW;
	wndClass.lpfnWndProc = WndProc;
	wndClass.hInstance = g_hInstance;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hIcon = nullptr;
	wndClass.hCursor = nullptr;
	wndClass.hbrBackground = nullptr;
	wndClass.lpszMenuName = nullptr;
	wndClass.lpszClassName = L"ForTheDreamOfGameDevelop";



	if (!RegisterClassEx(&wndClass))
	{
		MessageBox(0, L"RegisterClass Failed.", 0, 0);
		return;
	}


	g_hwnd = CreateWindow(L"ForTheDreamOfGameDevelop", WINDOWTITLE,
		WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH,
		WINDOW_HEIGHT, NULL, NULL, g_hInstance, NULL);

	if (!g_hwnd)
	{
		MessageBox(0, L"CreateWindow Failed.", 0, 0);
		return;
	}


	MoveWindow(g_hwnd, 250, 80, WINDOW_WIDTH, WINDOW_HEIGHT, true);
	ShowWindow(g_hwnd, g_nCmdShow);
	UpdateWindow(g_hwnd);
}


//-----------------------------------【窗口处理函数】-----------------------------------
// 处理外来信息
//--------------------------------------------------------------------------------------
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
			DestroyWindow(hwnd);
		else if (wParam == VK_LEFT)
			g_posX -= 1.0f;
		else if (wParam == VK_RIGHT)
			g_posX += 1.0f;
		break;
	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}
//-----------------------------------【程序初始化】-----------------------------------
// 初始化窗口和数据等
//--------------------------------------------------------------------------------------
void Init()
{
	InitWindow();

	g_posX = 400.0f;
	g_posY = 1.0f;
}



//-----------------------------------【更新绘制的数据】-----------------------------------
// 处理外来信息
//--------------------------------------------------------------------------------------
void UpdateScene()
{
	g_posY += 0.001f;
}
//-----------------------------------【进行绘制】-----------------------------------
// 处理外来信息
//--------------------------------------------------------------------------------------
void DrawScene()
{
	HDC hdc = GetDC(g_hwnd);
	TextOut(hdc, g_posX, g_posY, L"hello", 5);
	ReleaseDC(g_hwnd, hdc);
}

//-----------------------------------【游戏运行函数】-----------------------------------
// 游戏循环框架封装在其中
//--------------------------------------------------------------------------------------
void Run()
{
	MSG msg = { 0 };
	while (msg.message != WM_QUIT)
	{
		if (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else
		{
			UpdateScene();
			DrawScene();
		}
	}
}