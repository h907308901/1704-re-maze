#include <windows.h>
#pragma comment(lib, "kernel32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#include "resource.h"

#include "../common/field_97_64.h"
#include "../common/values.h"

int const FRAME_RATE = 25;
unsigned int const TIMER_ID = 5678;
const char szClassName[] = "MazePlayer";
const char szWindowName[] = "Maze";
const char szMazePath[] = "maze.dat";

void threadstartstop(bool start);

int x = 0, y = 0;
int nWidth, nHeight;
bool *wall_v, *wall_h;
const unsigned char encflag[64] = { 103,121,54,67,54,116,104,42,88,67,67,79,99,80,60,82,104,48,54,88,108,39,110,82,41,90,90,89,105,100,97,39,109,64,114,80,61,74,126,57,42,51,67,123,61,90,37,98,117,110,77,106,72,77,127,47,116,83,42,121,74,102,92,35 };
unsigned int const MAX_STEP_RECORD = 262144; // because the initial position needs recording, the actual max step count is MAX_STEP_RECORD - 1
// stepcount: written by main thread
// calccount: written by calc thread if it is alive, otherwise by main thread
unsigned int stepcount = 0, calccount = 0;
// flag_record: 0th is the initial flag
unsigned char flag_record[MAX_STEP_RECORD][64];
int step_record[MAX_STEP_RECORD]; // directions, 0th is not used

// directions: 0-left 1-up 2-right 3-down

// get the opposite direction
inline int invdir(int dir) {
	return (dir + 2) % 4;
}

bool getwallv(int x, int y) {
	return wall_v[y * (nWidth - 1) + x];
}

bool getwallh(int x, int y) {
	return wall_h[y * nWidth + x];
}

void drawwall(HDC hdc, int x, int y, bool vertical) {
	int real_x, real_y;
	real_x = 12 * 32 + 32 * (x + 1);
	real_y = 7 * 32 + 32 * (y + 1);
	MoveToEx(hdc, real_x, real_y, NULL);
	if (vertical)
		real_y -= 32;
	else
		real_x -= 32;
	LineTo(hdc, real_x, real_y);
	SetPixel(hdc, real_x, real_y, GetDCPenColor(hdc));
}

void OnPaint(HDC hdc) {
	static unsigned long long t1, framecount;
	if (t1 == 0 || framecount > 1000000000) {
		t1 = GetTickCount64() - 1;
		framecount = 0;
	}
	MoveToEx(hdc, 7 * 32, 2 * 32, NULL);
	LineTo(hdc, 18 * 32, 2 * 32);
	LineTo(hdc, 18 * 32, 13 * 32);
	LineTo(hdc, 7 * 32, 13 * 32);
	LineTo(hdc, 7 * 32, 2 * 32);
	RECT rect;
	char str[128], flagstr[128];
	rect.left = 32;
	rect.top = 16;
	rect.right = 608;
	rect.bottom = 48;
	if (stepcount <= calccount) {
		strncpy_s(flagstr, (char*)flag_record[stepcount], 64);
		wsprintf(str, "CURRENT: %s", flagstr);
	}
	else {
		wsprintf(str, "REMAINING: %d", stepcount - calccount);
	}
	DrawText(hdc, str, -1, &rect, DT_TOP | DT_LEFT);
	rect.top += 20;
	rect.bottom += 20;
	strncpy_s(flagstr, (char*)encflag, 64);
	wsprintf(str, "TARGET: %s", flagstr);
	DrawText(hdc, str, -1, &rect, DT_TOP | DT_LEFT);
	rect.left = 32;
	rect.top = 96;
	rect.right = 160;
	rect.bottom = 128;
	wsprintf(str, "FPS: %d", framecount * 1000 / (GetTickCount() - t1));
	DrawText(hdc, str, -1, &rect, DT_TOP | DT_LEFT);
	rect.top += 20;
	rect.bottom += 20;
	DrawText(hdc, "SET: 0, 0", -1, &rect, DT_TOP | DT_LEFT);
	rect.top += 20;
	rect.bottom += 20;
	wsprintf(str, "CUR: %d, %d", x, y);
	DrawText(hdc, str, -1, &rect, DT_TOP | DT_LEFT);
	rect.top += 20;
	rect.bottom += 20;
	wsprintf(str, "END: %d, %d", nWidth - 1, nHeight - 1);
	DrawText(hdc, str, -1, &rect, DT_TOP | DT_LEFT);
	rect.top += 40;
	rect.bottom += 40;
	wsprintf(str, "STEP: %d", stepcount);
	DrawText(hdc, str, -1, &rect, DT_TOP | DT_LEFT);
	rect.top += 20;
	rect.bottom += 20;
	wsprintf(str, "(MAX %d)", MAX_STEP_RECORD - 1);
	DrawText(hdc, str, -1, &rect, DT_TOP | DT_LEFT);
	int i, j;
	for (i = x - 5; i <= x + 5; i++) {
		for (j = y - 5; j <= y + 5; j++) {
			if (i >= 0 && i < nWidth && j >= 0 && j < nHeight) {
				if (getwallv(i, j)) drawwall(hdc, i - x, j - y, true);
				if (getwallh(i, j)) drawwall(hdc, i - x, j - y, false);
			}
			if ((i == -1 || i == nWidth - 1) && j >= 0 && j < nHeight) drawwall(hdc, i - x, j - y, true);
			if ((j == -1 || j == nHeight - 1) && i >= 0 && i < nWidth) drawwall(hdc, i - x, j - y, false);
		}
	}
	Ellipse(hdc, 12 * 32 + 4, 7 * 32 + 4, 13 * 32 - 4, 8 * 32 - 4);
	framecount++;
}

void OnMove(int dir) {
	// ensure it is possible to increase step count
	if (stepcount >= MAX_STEP_RECORD - 1) return;
	// passable?
	if (dir == 0 && (x <= 0 || getwallv(x - 1, y))) return;
	if (dir == 1 && (y <= 0 || getwallh(x, y - 1))) return;
	if (dir == 2 && (x >= nWidth - 1 || getwallv(x, y))) return;
	if (dir == 3 && (y >= nHeight - 1 || getwallh(x, y))) return;
	// judge stepback
	if (stepcount > 0 && invdir(dir) == step_record[stepcount]) {
		stepcount--;
		if (stepcount <= calccount) {
			threadstartstop(false);
			calccount = stepcount;
		}
	}
	else {
		int index = stepcount + 1;
		step_record[index] = dir;
		stepcount++;
		threadstartstop(true);
	}
	// perform move
	switch (dir) {
	case 0:
		x--;
		break;
	case 1:
		y--;
		break;
	case 2:
		x++;
		break;
	case 3:
		y++;
		break;
	}
}

DWORD WINAPI ThreadProc(LPVOID lpParameter) {
	while (stepcount > calccount) {
		int index = calccount + 1;
		int dir = step_record[index];
		memcpy(flag_record[index], flag_record[index - 1], 64);
		for (int i = 0; i < 64; i++)
			flag_record[index][i] -= 32;
		for (int i = 0; i < 5000; i++) {
			field_97_64_add(flag_record[index], add[dir]);
			field_97_64_mul(flag_record[index], mul[dir]);
		}
		for (int i = 0; i < 64; i++)
			flag_record[index][i] += 32;
		calccount++;
	}
	return 0;
}

LRESULT CALLBACK WindowProc(
	_In_ HWND   hwnd,
	_In_ UINT   uMsg,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam
) {
	PAINTSTRUCT ps;
	HDC hdc, hcdc;
	HBITMAP hbmp;
	switch (uMsg) {
	case WM_TIMER:
		if ((UINT_PTR)wParam == TIMER_ID) {
			InvalidateRect(hwnd, NULL, FALSE);
			return 0;
		}
		break;
	case WM_KEYDOWN:
		switch (wParam) {
		case VK_LEFT:
			OnMove(0);
			break;
		case VK_UP:
			OnMove(1);
			break;
		case VK_RIGHT:
			OnMove(2);
			break;
		case VK_DOWN:
			OnMove(3);
			break;
		}
		break;
	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		hcdc = CreateCompatibleDC(hdc);
		hbmp = CreateCompatibleBitmap(hcdc, 640, 480);
		SelectObject(hcdc, hbmp);
		DefWindowProc(hwnd, WM_ERASEBKGND, (WPARAM)hcdc, NULL);
		OnPaint(hcdc);
		BitBlt(hdc, 0, 0, 640, 480, hcdc, 0, 0, SRCCOPY);
		DeleteObject(hbmp);
		DeleteDC(hcdc);
		EndPaint(hwnd, &ps);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

INT_PTR CALLBACK DialogProc(
	_In_ HWND   hwndDlg,
	_In_ UINT   uMsg,
	_In_ WPARAM wParam,
	_In_ LPARAM lParam
) {
	char str[256];
	switch (uMsg) {
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCANCEL:
			EndDialog(hwndDlg, 0);
			break;
		case IDOK:
			if (GetDlgItemText(hwndDlg, IDC_EDIT1, str, 256) == 63) {
				memcpy(flag_record, str, 63);
				flag_record[0][63] = '\x80';
				EndDialog(hwndDlg, 1);
			}
			else {
				MessageBox(NULL, "length mismatch", "Error", MB_ICONERROR);
			}
			break;
		}
		return TRUE;
	default:
		return FALSE;
	}
}

int _stdcall WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nCmdShow
) {
	HANDLE hFile = INVALID_HANDLE_VALUE, hFileMapping = NULL;
	void* lpBaseAddress = NULL;
	HWND hWnd = NULL, hDlg = NULL;
	RECT rect;
	hFile = CreateFile(szMazePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		MessageBox(NULL, "failed to load maze data", "Error", MB_ICONERROR);
		return 1;
	}
	hFileMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
	CloseHandle(hFile);
	if (!hFileMapping) {
		MessageBox(NULL, "failed to load maze data", "Error", MB_ICONERROR);
		return 1;
	}
	lpBaseAddress = MapViewOfFile(hFileMapping, FILE_MAP_READ, 0, 0, 0);
	CloseHandle(hFileMapping);
	if (!lpBaseAddress) {
		MessageBox(NULL, "failed to load maze data", "Error", MB_ICONERROR);
		return 1;
	}
	nWidth = ((int*)lpBaseAddress)[0];
	nHeight = ((int*)lpBaseAddress)[1];
	wall_v = (bool*)((int*)lpBaseAddress + 2);
	wall_h = (bool*)((int*)lpBaseAddress + 2) + (nWidth - 1) * nHeight;
	WNDCLASSEX wndclass;
	memset(&wndclass, 0, sizeof(WNDCLASSEX));
	wndclass.cbSize = sizeof(WNDCLASSEX);
	wndclass.hInstance = hInstance;
	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = DefWindowProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass.lpszClassName = szClassName;
	RegisterClassEx(&wndclass);
	const DWORD dwStyle = WS_CAPTION | WS_SYSMENU | WS_BORDER | WS_MINIMIZEBOX;
	rect = { 0, 0, 640, 480 };
	AdjustWindowRect(&rect, dwStyle, FALSE);
	hWnd = CreateWindowEx(0, szClassName, szWindowName, dwStyle, CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, NULL, NULL, hInstance, NULL);
	SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)WindowProc);
	ShowWindow(hWnd, 1);
	UpdateWindow(hWnd);
	SetTimer(hWnd, TIMER_ID, 1000 / FRAME_RATE, NULL);
	if (!DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), hWnd, DialogProc))
		PostQuitMessage(0);
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0) != 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	threadstartstop(false);
	DestroyWindow(hWnd);
	UnregisterClass(szClassName, hInstance);
	UnmapViewOfFile(lpBaseAddress);
	return 0;
}

void threadstartstop(bool start) {
	static HANDLE hThread;
	if (start) {
		if (hThread == NULL) {
			hThread = CreateThread(NULL, 0, ThreadProc, NULL, 0, NULL);
		}
		else if (WaitForSingleObject(hThread, 0) == WAIT_OBJECT_0) {
			CloseHandle(hThread);
			hThread = CreateThread(NULL, 0, ThreadProc, NULL, 0, NULL);
		}
	}
	else {
		if (hThread != NULL) {
			TerminateThread(hThread, 0);
			WaitForSingleObject(hThread, INFINITE);
			CloseHandle(hThread);
			hThread = NULL;
		}
	}
}