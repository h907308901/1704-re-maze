#pragma comment(linker, "/stack:0x10000000")

#include "stdafx.h"

int const WIDTH = 1024;
int const HEIGHT = 1024;
int const CELL_SIZE = 4;
int const MAX_MOVE = WIDTH * HEIGHT;

int count = 0;
bool cell[HEIGHT][WIDTH];
bool wall_v[HEIGHT][WIDTH - 1];
bool wall_h[HEIGHT - 1][WIDTH];

int movecount = 0;
char move[MAX_MOVE];
// 0=left 1=up 2=right 3=down


void drawwall(HDC hdc, int x, int y, bool vertical) {
	int real_x, real_y;
	real_x = CELL_SIZE * (x + 1);
	real_y = CELL_SIZE * (y + 1);
	MoveToEx(hdc, real_x, real_y, NULL);
	if (vertical)
		real_y -= CELL_SIZE;
	else
		real_x -= CELL_SIZE;
	LineTo(hdc, real_x, real_y);
	SetPixel(hdc, real_x, real_y, GetDCPenColor(hdc));
}

void fillcell(HDC hdc, int x, int y, HBRUSH hbr) {
	RECT rect;
	rect.left = x * CELL_SIZE;
	rect.top = y * CELL_SIZE;
	rect.right = (x + 1) * CELL_SIZE;
	rect.bottom = (y + 1) * CELL_SIZE;
	FillRect(hdc, &rect, hbr);
}

void drawmaze() {
	int width, height;
	width = CELL_SIZE * WIDTH;
	height = CELL_SIZE * HEIGHT;
	HDC hdc = CreateCompatibleDC(NULL);
	BITMAPINFO bmi;
	memset(&bmi, 0, sizeof(BITMAPINFO));
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = width;
	bmi.bmiHeader.biHeight = height;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;
	void* pbits;
	HBITMAP hdib = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, &pbits, NULL, 0);
	SelectObject(hdc, hdib);
	RECT rect;
	rect.left = 0;
	rect.top = 0;
	rect.right = width;
	rect.bottom = height;
	HBRUSH hbr;
	FillRect(hdc, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));
	SetDCPenColor(hdc, 0);
	int i, j;
	int x = 0, y = 0;
	hbr = CreateSolidBrush(RGB(255, 100, 100));
	fillcell(hdc, 0, 0, hbr);
	for (i = 0; i < movecount; i++) {
		switch (move[i]) {
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
		fillcell(hdc, x, y, hbr);
	}
	DeleteObject(hbr);
	for (i = 0; i < WIDTH - 1; i++)
		for (j = 0; j < HEIGHT; j++)
			if (wall_v[j][i])
				drawwall(hdc, i, j, true);
	for (i = 0; i < WIDTH; i++)
		for (j = 0; j < HEIGHT - 1; j++)
			if (wall_h[j][i])
				drawwall(hdc, i, j, false);
	FILE* file;
	fopen_s(&file, "maze.bmp", "wb");
	BITMAPFILEHEADER bfh;
	memset(&bfh, 0, sizeof(BITMAPFILEHEADER));
	SIZE_T size = width * height * 4;
	bfh.bfType = 0x4d42;
	bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	bfh.bfSize = size;
	fwrite(&bfh, sizeof(BITMAPFILEHEADER), 1, file);
	fwrite(&bmi.bmiHeader, sizeof(BITMAPINFOHEADER), 1, file);
	fwrite(pbits, size, 1, file);
	fclose(file);
	DeleteObject(hdib);
	DeleteDC(hdc);
}


bool solvemaze(int x, int y) {
	cell[y][x] = true;
	if (x == WIDTH - 1 && y == HEIGHT - 1) return true;
	int neighbor[4][2];
	neighbor[0][0] = x - 1;
	neighbor[0][1] = y;
	neighbor[1][0] = x;
	neighbor[1][1] = y - 1;
	neighbor[2][0] = x + 1;
	neighbor[2][1] = y;
	neighbor[3][0] = x;
	neighbor[3][1] = y + 1;
	int i, x2, y2;
	for (i = 0; i < 4; i++) {
		x2 = neighbor[i][0];
		y2 = neighbor[i][1];
		if (x2 >= 0 && x2 < WIDTH && y2 >= 0 && y2 < HEIGHT) {
			if ((i == 0 && !wall_v[y][x - 1]) ||
				(i == 1 && !wall_h[y - 1][x]) ||
				(i == 2 && !wall_v[y][x]) ||
				(i == 3 && !wall_h[y][x])
				) {
				if (!cell[y2][x2]) {
					move[movecount] = i;
					movecount++;
					if (solvemaze(x2, y2)) return true;
					movecount--;
				}
			}
		}
	}
	return false;
}

int main()
{
	int i, j;
	FILE* f;
	if (fopen_s(&f, "maze.dat", "rb")) {
		printf("error opening maze.dat\n");
		return 0;
	}
	fread(&i, sizeof(i), 1, f);
	fread(&j, sizeof(j), 1, f);
	if (i != WIDTH || j != HEIGHT) {
		printf("wrong map size\n");
		fclose(f);
		return 0;
	}
	fread(&wall_v, sizeof(wall_v), 1, f);
	fread(&wall_h, sizeof(wall_h), 1, f);
	fclose(f);
	for (i = 0; i < WIDTH; i++)
		for (j = 0; j < HEIGHT; j++)
			cell[j][i] = false;
	solvemaze(0, 0);
	printf("move count: %d\n", movecount);
	drawmaze();
    return 0;
}

