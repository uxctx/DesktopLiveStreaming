// DesktopLiveStreaming.cpp : 定义应用程序的入口点。
//

#include "stdafx.h"

#include "DesktopLiveStreaming.h"

#include "AppContext.h"
#include "DesktopImageSource.h"
#include "RingCacheBuffer.h"
#include "DXGIDesktopOutputSystem.h"
#include "AudioSoureAACStream.h"
#include "HLS_Server.h"

#include "Connection.h"
#include "FlvLiveStream.h"
#include "resource.h"
#include <shellapi.h>

#define MAX_LOADSTRING 100


// 全局变量: 
HINSTANCE hInst;                                // 当前实例
WCHAR szTitle[MAX_LOADSTRING];                  // 标题栏文本
WCHAR szWindowClass[MAX_LOADSTRING];            // 主窗口类名

HWND hdlg;

// 此代码模块中包含的函数的前向声明: 
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
INT_PTR CALLBACK PanelProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

class DXGIDesktopOutputSystem* d3dsys = nullptr;

int port = 80;
int bitrate = 4000;
bool usr_BitBlt = false;
static	char portstr[8];
static	char bitratestr[8];
static	char myurl[80];

HWND EDIT_PORT;
HWND EDIT_BITRATE;
HWND BTN_START;
HWND  CHECK_BitBlt;

HWND hWnd;

void Start()
{

	MyContext->desktopSource = new DesktopImageSource(d3dsys, bitrate);
	bool init_ok = AppContext::context->desktopSource->Init();
	MyContext->accSource = new AudioSoureAACStream(AACbitRate);

	

	if(usr_BitBlt)
		d3dsys->DXG1NextFramecanWork = false;

	MyContext->desktopSource->Start();//后启动
	//MyContext->accSource->mcrophoneInput = false;
	MyContext->accSource->Strat();

	if (MyContext->live_model == LiveModel::http_flv)
	{
		MyContext->flvServer = new FlvLiveStream();//
		MyContext->flvServer->start();
	}
	else
	{
		MyContext->hlsServer = new HLS_Server();
		MyContext->hlsServer->start();
	}

	init_res_table();//web静态资源
	IOCPServer *server = new IOCPServer(port, event_hander);

	switch (server->bind_listen())
	{
	case port_occupation:
		MessageBox(hdlg,L"该端口被占用",L"错误", MB_OKCANCEL);
		exit(0);
		break;
	case fatal_error:
		//MessageBox(hdlg, L"可能端口被占用", L"错误", MB_OKCANCEL);
		exit(0);
		break;
	case success:
		server->start();
		EnableWindow(EDIT_PORT, false);
		EnableWindow(EDIT_BITRATE, false);
		EnableWindow(BTN_START, false);
		EnableWindow(CHECK_BitBlt,false);
		sprintf(myurl,"http://127.0.0.1:%d",port);
		ShellExecuteA(NULL, "open", myurl, NULL, NULL, SW_SHOWNORMAL);
		break;
	default:
		break;
	}

}

HINSTANCE ApphInstance;
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	CoInitialize(NULL);

	ApphInstance = hInstance;

	BOOL init_ok = false;

	MyContext = new AppContext();

	MyContext->rCache = new RingCacheBuffer(500);

	d3dsys = new DXGIDesktopOutputSystem();

	init_ok = d3dsys->Init();

	strcpy(portstr, "80");
	strcpy(bitratestr, "2000");
	
	// 初始化全局字符串
	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_DESKTOPLIVESTREAMING, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	// 执行应用程序初始化: 
	if (!InitInstance(hInstance, nCmdShow))
	{
		return FALSE;
	}

	//HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_DESKTOPLIVESTREAMING));

	MSG msg;

	EDIT_PORT = GetDlgItem(hdlg, IDC_EDIT_PORT);
	EDIT_BITRATE = GetDlgItem(hdlg, IDC_EDIT_BITRATE);
	BTN_START = GetDlgItem(hdlg, IDC_BUTTON_START);
	CHECK_BitBlt = GetDlgItem(hdlg, IDC_CHECK_BitBlt);
	SetWindowTextA(EDIT_PORT, portstr);
	SetWindowTextA(EDIT_BITRATE, bitratestr);

	CheckRadioButton(hdlg, IDC_RADIO_HTTPFLV, IDC_RADIO_HLS, IDC_RADIO_HTTPFLV);
	MyContext->live_model = LiveModel::http_flv;

	//MSG msg;

	// 主消息循环: 
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		/*if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
		{*/
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		//}
	}
	CoUninitialize();

	return (int)msg.wParam;

	
}


//
//  函数: MyRegisterClass()
//
//  目的: 注册窗口类。
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEXW wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_DESKTOPLIVESTREAMING));
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	//wcex.lpszMenuName = MAKEINTRESOURCEW(IDC_DESKTOPLIVESTREAMING);
	wcex.lpszMenuName = 0;
	wcex.lpszClassName = szWindowClass;
	wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
	
	return RegisterClassExW(&wcex);
}

//
//   函数: InitInstance(HINSTANCE, int)
//
//   目的: 保存实例句柄并创建主窗口
//
//   注释: 
//
//        在此函数中，我们在全局变量中保存实例句柄并
//        创建和显示主程序窗口。
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
	hInst = hInstance; // 将实例句柄存储在全局变量中

	 hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, 380, 220, nullptr, nullptr, hInstance, nullptr);

	if (!hWnd)
	{
		return FALSE;
	}

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	switch (message)
	{
	case WM_CREATE:
	{
		hdlg = CreateDialog(ApphInstance, MAKEINTRESOURCE(IDD_DIALOGMainGG), hWnd, (DLGPROC)PanelProc);
		//SetParent(hdlg, hWnd);

		ShowWindow(hdlg, SW_SHOWNA);
	}
		break;
	case WM_COMMAND:
	{
		
	}
	break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		// TODO: 在此处添加使用 hdc 的任何绘图代码...
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

// “关于”框的消息处理程序。
INT_PTR CALLBACK PanelProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_CTLCOLORDLG:
		return (INT_PTR)GetStockObject(WHITE_BRUSH);
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
	{
		int wmId = LOWORD(wParam);
		// 分析菜单选择: 
		switch (wmId)
		{
		case IDC_RADIO_HTTPFLV:
			MyContext->live_model = LiveModel::http_flv;
			break;
		case IDC_RADIO_HLS:
			MyContext->live_model = LiveModel::hls;
			break;
		case IDC_BUTTON_START:
			GetWindowTextA(EDIT_PORT, portstr, 7);
			GetWindowTextA(EDIT_BITRATE, bitratestr, 7);
			usr_BitBlt=IsDlgButtonChecked(hdlg, IDC_CHECK_BitBlt);

			port = atoi(portstr);
			bitrate = atoi(bitratestr);
			if (bitrate < 1000)
				bitrate = 1000;
			if (bitrate > 4500)
				bitrate = 4500;
			if (port > 90000)
				port = 8099;

			Start();
			break;
		/*case IDM_ABOUT:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);*/
			break;
		case IDM_EXIT:
			DestroyWindow(hWnd);
			break;
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
		}
	}
		break;
	}
	return (INT_PTR)FALSE;
}