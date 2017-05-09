// fieldtest.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "../common/field_97_64.h"
#include "../common/values.h"

int main()
{
	unsigned char value[64];
	for (int i = 0; i < 64; i++)
		printf("%d*x^%d+", add[0][i], i);
	printf("\n\n");
	memcpy(value, add[0], 64);
	field_97_64_inv_fermat(value);
	for (int i = 0; i < 64; i++)
		printf("%d*x^%d+", value[i], i);
	printf("\n\n");
	system("pause");
    return 0;
}

