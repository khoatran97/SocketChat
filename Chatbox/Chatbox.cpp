// Chatbox.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "Chatbox.h"
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <process.h>
#include <windowsx.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")
#pragma warning(disable:4996)


#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define MAX_LOADSTRING 100
#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"


SOCKET Connect();
void receiveMessage(void* box);
void fromServer(void* null);
HWND Main; // Handle chatbox dialog
HWND Wnd; // Handle main window
char Username[25];

// Global Variables:
SOCKET Server;
bool isChatting = false;
bool isSignin = false;
HANDLE thHandle; // fromServer
HANDLE thHandle_; // receiveMgs

HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    Chatbox(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_CHATBOX, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CHATBOX));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}



//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CHATBOX));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = MAKEINTRESOURCEW(IDC_CHATBOX);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, 200, 50, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND  - process the application menu
//  WM_PAINT    - Paint the main window
//  WM_DESTROY  - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
	case WM_CREATE:
		Wnd = hWnd;
		break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code that uses hdc here...
            EndPaint(hWnd, &ps);
			DialogBox(hInst, MAKEINTRESOURCE(IDD_CHATBOX), hWnd, Chatbox);
			SendMessage(hWnd, WM_CLOSE, NULL, NULL);
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
void UI(void*null)
{
	HWND signup = GetDlgItem(Main, IDC_SIGNUP);
	HWND signin = GetDlgItem(Main, IDC_SIGNIN);
	HWND start = GetDlgItem(Main, IDC_START);
	HWND add = GetDlgItem(Main, IDC_ADD);
	HWND send = GetDlgItem(Main, IDC_SEND);
	HWND stop = GetDlgItem(Main, IDC_STOP);
	do
	{
		EnableWindow(signup, !isSignin);
		EnableWindow(signin, !isSignin);
		EnableWindow(start, isSignin && !isChatting);
		EnableWindow(add, isChatting);
		EnableWindow(send, isChatting);
		EnableWindow(stop, isChatting);
	} while (true);
}

SOCKET Connect()
{
	WSADATA wsaData;
	SOCKET ConnectSocket = INVALID_SOCKET;
	struct addrinfo *result = NULL, *ptr = NULL, hints;
	char strReceive[DEFAULT_BUFLEN];
	int iResult;
	int lenReceive = DEFAULT_BUFLEN;

	// Khởi tạo Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != 0) {
		MessageBox(0, L"Lỗi WSAStartup", L"Lỗi", MB_ICONERROR);
		SendMessage(Wnd, WM_CLOSE, NULL, NULL);
		return NULL;
	}

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	// Lấy địa chỉ IP và Port
	iResult = getaddrinfo("127.0.0.1", DEFAULT_PORT, &hints, &result);
	if (iResult != 0) {
		MessageBox(0, L"Lỗi getaddrinfo", L"Lỗi", MB_ICONERROR);
		WSACleanup();
		SendMessage(Wnd, WM_CLOSE, NULL, NULL);
		return NULL;
	}

	// Kết nối đến server đến khi thành công
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

		// Tạo SOCKET để kết nối
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype,
			ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) {
			MessageBox(0, L"Lỗi socket", L"Lỗi", MB_ICONERROR);
			WSACleanup();
			SendMessage(Wnd, WM_CLOSE, NULL, NULL);
			return NULL;
		}

		// Kết nối
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);

	if (ConnectSocket == INVALID_SOCKET) {
		MessageBox(0, L"Không thể kết nối đến server", L"Lỗi", MB_ICONERROR);
		WSACleanup();
		SendMessage(Wnd, WM_CLOSE, NULL, NULL);
		return NULL;
	}
	return ConnectSocket;
}

void receiveMessage(void* null)
{
	char str[200];
	do {
		int i = recv(Server, str, 200, 0);
		str[i] = '\0';
		if (strcmp(str, "#") == 0)
			break;
		if (strcmp(str, "chat") == 0)
			continue;
		TCHAR line_[230];
		mbstowcs(line_, str, strlen(str) + 1);
		HWND txtContent = GetDlgItem(Main, IDC_CONTENT);
		SendMessage(txtContent, EM_SETSEL, -1, -1);
		SendMessage(txtContent, EM_REPLACESEL, true, (LPARAM)line_);

		SendMessage(txtContent, EM_SETSEL, -1, -1);
		SendMessage(txtContent, EM_REPLACESEL, true, (LPARAM)"\n");

	} while (isChatting);
	isChatting = false;
	thHandle = (HANDLE)_beginthread(fromServer, 0, NULL);
	_endthread();
}
void fromServer(void* null)
{
	char str[200], name[25];
	do {
		int i = recv(Server, str, 200, 0);
		str[i] = '\0';
		if (strcmp(str, "chat") == 0)
		{
			isChatting = true;
			send(Server, "_chat", 5, 0);
			thHandle_ = (HANDLE)_beginthread(receiveMessage, 0, NULL);
			MessageBox(0, L"Bạn có 1 lời đè nghị trò chuyện", L"Trò chuyện", 0);
			break;
		}
	} while (true);
	_endthread();
}

void signOut()
{
	if (isChatting)
		send(Server, "#", 1, 0);
	Sleep(100);
	send(Server, "*", 1, 0);
}

// Message handler for about box.
INT_PTR CALLBACK Chatbox(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
		Main = hDlg;
		Server = Connect();
		// Bat dau thread enable/disable cac button
		_beginthread(UI, 0, NULL);
        return (INT_PTR)TRUE;

    case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_SIGNUP:
		{
			HWND txtUsername = GetDlgItem(hDlg, IDC_USERNAME);
			HWND txtPassword = GetDlgItem(hDlg, IDC_PASSWORD);

			TCHAR username_[25], password_[25];
			char str[10];
			GetWindowText(txtUsername, username_, 25);
			GetWindowText(txtPassword, password_, 25);
			char username[25], password[25];
			wcstombs_s(NULL, username, username_, 25);
			wcstombs_s(NULL, password, password_, 25);

			// Yeu cau chuc nang dang nhap
			send(Server, "signup", strlen("signup"), 0);
			send(Server, username, strlen(username), 0);
			send(Server, password, strlen(password), 0);

			int i = recv(Server, str, 200, 0);
			str[i] = '\0';
			if (strcmp(str, "1") == 0)
			{
				MessageBox(0, L"Đăng ký thành công", L"Hoàn tất", 0);
			}
			else
				MessageBox(0, L"Đăng ký không thành công", L"Lỗi", MB_ICONERROR);
			break;
		}
		case IDC_SIGNIN:
		{
			HWND txtUsername = GetDlgItem(hDlg, IDC_USERNAME);
			HWND txtPassword = GetDlgItem(hDlg, IDC_PASSWORD);

			TCHAR username_[25], password_[25];
			char str[10];
			GetWindowText(txtUsername, username_, 25);
			GetWindowText(txtPassword, password_, 25);
			char username[25], password[25];
			wcstombs_s(NULL, username, username_, 25);
			wcstombs_s(NULL, password, password_, 25);

			// Yeu cau chuc nang dang nhap
			send(Server, "signin", strlen("signin"), 0);
			send(Server, username, strlen(username), 0);
			send(Server, password, strlen(password), 0);

			int i = recv(Server, str, 200, 0);
			str[i] = '\0';
			if (strcmp(str, "1") == 0)
			{
				isSignin = true;
				MessageBox(0, L"Đăng nhập thành công", L"Hoàn tất", 0);
				strcpy(Username, username);
				// Bat dau lang nghe yeu cau tu server
				thHandle = (HANDLE)_beginthread(fromServer, 0, NULL);
			}
			else
				MessageBox(0, L"Đăng nhập không thành công", L"Lỗi", MB_ICONERROR);
			break;
		}
		case IDC_START:
		{
			TerminateThread(thHandle, 0); // Ngung viec lang nghe yeu cau tu server
			TCHAR username_[300];
			char str[10];
			char username[300];
			HWND txtUsername = GetDlgItem(hDlg, IDC_LIST);
			GetWindowText(txtUsername, username_, 300);
			wcstombs_s(NULL, username, username_, 300);

			if (strcmp(Username, username) == 0)
			{
				MessageBox(0, L"Không thể trò chuyện với chính mình", L"Lỗi", MB_ICONERROR);
			}

			// Yeu cau chuc nang chat
			send(Server, "chat", 4, 0);
			send(Server, username, strlen(username), 0);

			int i = recv(Server, str, 200, 0);
			str[i] = '\0';

			if (strcmp(str, "1") == 0)
			{
				isChatting = true;
				MessageBox(0, L"Bạn có thể bắt đầu chat", L"Trò chuyện", 0);
				send(Server, "_chat", 5, 0);
				// Bat dau nhan tin nhan
				thHandle_ = (HANDLE)_beginthread(receiveMessage, 0, (void*)hDlg);
			}
			else
			{
				MessageBox(0, L"Tài khoản này đang offline hoặc không tồn tại", L"Lỗi", MB_ICONERROR);
				// Bat dau lang nghe yeu cau tu server
				thHandle = (HANDLE)_beginthread(fromServer, 0, NULL);
			}
			break;
		}
		case IDC_ADD:
		{
			TerminateThread(thHandle_, 0); // Ngung viec nhan tin nhan tu server
			// Yeu cau chuc nang them thanh vien
			send(Server, "add", 3, 0);
			char str[10];
			TCHAR username_[300];
			char username[300];
			HWND txtUsername = GetDlgItem(hDlg, IDC_LIST);
			GetWindowText(txtUsername, username_, 300);
			wcstombs_s(NULL, username, username_, 300);
			send(Server, username, strlen(username), 0);
			int i = recv(Server, str, 10, 0);
			thHandle_ = (HANDLE)_beginthread(receiveMessage, 0, (void*)hDlg); // Bat dau viec nhan tin nhan tu server
			str[i] = '\0';
			if (strcmp(str, "0") == 0)
				MessageBox(0, L"Không thêm được người dùng nào vào cuộc trò chuyện này", L"Lỗi", 0);
			else
			{
				MessageBox(0, L"Thêm thành công", L"Hoàn tất", 0);
			}
			break;
		}
		case IDC_SEND:
		{
			if (!isChatting)
			{
				MessageBox(0, L"Cuộc trò chuyện đã kết thúc", L"Lỗi", MB_ICONERROR);
				HWND txtContent = GetDlgItem(Main, IDC_CONTENT);
				SetWindowText(txtContent, L"");
			}
			else
			{
				TCHAR str_[200];
				char str[200];

				HWND txtSend = GetDlgItem(hDlg, IDC_MESSAGE);
				GetWindowText(txtSend, str_, 200);
				wcstombs_s(NULL, str, str_, 200);
				int i;
				do
				{
					i = send(Server, str, strlen(str), 0);
				} while (i <= 0);
			}
			break;
		}
		case IDC_STOP:
			isChatting = false;
			HWND txtContent = GetDlgItem(Main, IDC_CONTENT);
			SetWindowText(txtContent, L"");
			// Gui tin hieu ngung chat den user
			send(Server, "#", 1, 0);
			break;
		}
		break;
	case WM_CLOSE:
		signOut();
		EndDialog(hDlg, LOWORD(wParam));
		return (INT_PTR)TRUE;
    }
    return (INT_PTR)FALSE;
}
