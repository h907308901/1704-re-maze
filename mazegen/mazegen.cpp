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

#include "../common/field_97_64.h"
#include "../common/values.h"

//const char flag[] = "flag{ThisIsTheTestFlag!20170328_1234567012345670123456701234567}";

void savemaze() {
	FILE* file;
	fopen_s(&file, "maze.dat", "wb");
	fwrite(&WIDTH, sizeof(int), 1, file);
	fwrite(&HEIGHT, sizeof(int), 1, file);
	fwrite(&wall_v, sizeof(wall_v), 1, file);
	fwrite(&wall_h, sizeof(wall_h), 1, file);
	fclose(file);
}

void removewall(int x1, int y1, int x2, int y2) {
	int dx = x2 - x1;
	int dy = y2 - y1;
	if (dx == 1) wall_v[y1][x1] = false;
	if (dx == -1) wall_v[y1][x1 - 1] = false;
	if (dy == 1) wall_h[y1][x1] = false;
	if (dy == -1) wall_h[y1 - 1][x1] = false;
}

void genmaze(int x, int y) {
	count++;
	cell[y][x] = true;
	int neighbor[4][2];
	neighbor[0][0] = x - 1;
	neighbor[0][1] = y;
	neighbor[1][0] = x + 1;
	neighbor[1][1] = y;
	neighbor[2][0] = x;
	neighbor[2][1] = y - 1;
	neighbor[3][0] = x;
	neighbor[3][1] = y + 1;
	int table[4];
	int i, j, k, x2, y2;
	for (i = 0; i < 4; i++) table[i] = i;
	for (i = 4; i > 0; i--) {
		j = rand() % i;
		k = table[j];
		table[j] = table[i - 1];
		x2 = neighbor[k][0];
		y2 = neighbor[k][1];
		if (x2 >= 0 && x2 < WIDTH && y2 >= 0 && y2 < HEIGHT) {
			if (!cell[y2][x2]) {
				removewall(x, y, x2, y2);
				genmaze(x2, y2);
			}
		}
	}
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


void flagencrypt() {
	unsigned char encflag[64];
	unsigned char a[4][64], b[4][64]; // simplify the calculation to a*x+b
	unsigned char mul_inv[4][64];
	char str[128];
	FILE* f;
	if (fopen_s(&f, "flag.txt", "rb")) {
		printf("error opening flag.txt\n");
		return;
	}
	if (fread(encflag, 64, 1, f) != 1) {
		printf("wrong flag size\n");
		fclose(f);
		return;
	}
	fclose(f);
	DWORD time;
	time = GetTickCount();
	printf("encrypting flag...\n");
	printf("-preparing...\n");
	// let x=0, we get b
	for (int i = 0; i < 4; i++) {
		printf("--calculating b %d/4\n", i + 1);
		memset(b[i], 0, 64);
		for (int j = 0; j < 5000; j++) {
			field_97_64_add(b[i], add[i]);
			field_97_64_mul(b[i], mul[i]);
		}
	}
	// let x=1, we get a+b, then calculate a
	for (int i = 0; i < 4; i++) {
		printf("--calculating a %d/4\n", i + 1);
		memset(a[i], 0, 64);
		a[i][0] = 1;
		for (int j = 0; j < 5000; j++) {
			field_97_64_add(a[i], add[i]);
			field_97_64_mul(a[i], mul[i]);
		}
		field_97_64_sub(a[i], b[i]);
	}
	printf("-performing...\n");
	//memcpy(encflag, flag, 64);
	for (int i = 0; i < 64; i++)
		encflag[i] -= 32;
	for (int i = 0; i < movecount; i++) {
		//printf("%d%%\r", i * 100 / movecount);
		field_97_64_mul(encflag, a[move[i]]);
		field_97_64_add(encflag, b[move[i]]);
	}
	for (int i = 0; i < 64; i++)
		encflag[i] += 32;
	printf("-encrypted flag is:\n");
	for (int i = 0; i < 64; i++) {
		printf("%d", encflag[i]);
		if (i < 63) printf(",");
	}
	printf("\n");
	printf("-elasped time %fs\n", (GetTickCount() - time) / 1000.0);
	time = GetTickCount();
	printf("verifying...\n");
	printf("-preparing...\n");
	memcpy(mul_inv, mul, 4 * 64);
	for (int i = 0; i < 4; i++) {
		printf("--calculating inverse %d/4\n", i + 1);
		field_97_64_inv_fermat(mul_inv[i]);
	}
	// let x=0, we get b
	for (int i = 0; i < 4; i++) {
		printf("--calculating b %d/4\n", i + 1);
		memset(b[i], 0, 64);
		for (int j = 0; j < 5000; j++) {
			field_97_64_mul(b[i], mul_inv[i]);
			field_97_64_sub(b[i], add[i]);
		}
	}
	// let x=1, we get a+b, then calculate a
	for (int i = 0; i < 4; i++) {
		printf("--calculating a %d/4\n", i + 1);
		memset(a[i], 0, 64);
		a[i][0] = 1;
		for (int j = 0; j < 5000; j++) {
			field_97_64_mul(a[i], mul_inv[i]);
			field_97_64_sub(a[i], add[i]);
		}
		field_97_64_sub(a[i], b[i]);
	}
	printf("-performing...\n");
	for (int i = 0; i < 64; i++)
		encflag[i] -= 32;
	for (int i = movecount - 1; i >= 0; i--) {
		//printf("%d%%\r", (movecount - i) * 100 / movecount);
		field_97_64_mul(encflag, a[move[i]]);
		field_97_64_add(encflag, b[move[i]]);
	}
	for (int i = 0; i < 64; i++)
		encflag[i] += 32;
	strncpy_s(str, (char*)encflag, 64);
	printf("%s\n", str);
	printf("-elasped time %fs\n", (GetTickCount() - time) / 1000.0);
}

int main()
{
	DWORD time;
	int i, j;
	for (i = 0; i < WIDTH; i++)
		for (j = 0; j < HEIGHT; j++)
			cell[j][i] = false;
	for (i = 0; i < WIDTH - 1; i++)
		for (j = 0; j < HEIGHT; j++)
			wall_v[j][i] = true;
	for (i = 0; i < WIDTH; i++)
		for (j = 0; j < HEIGHT - 1; j++)
			wall_h[j][i] = true;
	//srand(GetTickCount());
	unsigned int seed;
	printf("seed:");
	scanf_s("%u", &seed);
	srand(seed);
	time = GetTickCount();
	printf("generating...\n");
	genmaze(rand() % WIDTH, rand() % HEIGHT);
	printf("elasped time %fs\n", (GetTickCount() - time) / 1000.0);
	for (i = 0; i < WIDTH; i++)
		for (j = 0; j < HEIGHT; j++)
			cell[j][i] = false;
	time = GetTickCount();
	printf("solving...\n");
	solvemaze(0, 0);
	printf("elasped time %fs\n", (GetTickCount() - time) / 1000.0);
	time = GetTickCount();
	printf("saving...\n");
	savemaze();
	printf("elasped time %fs\n", (GetTickCount() - time) / 1000.0);
	flagencrypt();
	system("pause");
	return 0;
}
