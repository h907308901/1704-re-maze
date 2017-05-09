/****** NOTE: THE CODE IS ALTERED FOR AVX2 INSTRUCTION SET OPTIMIZATION ******/

#include <string.h>
#include "field_97_64.h"

// Implementation of finite field GF_(97^64)

// About GF_(p^n)
// If p(x) is an n-degree irreducible polynomial in ring F_p[x], then p(x)F_p[x] generates a principal ideal of F_p[x].
// It can be proved F_p[x]/(p(x)) is an (p^n)-order field.

// About the Implementation of GF_(97^64)
// An element in F_97[x]/(p(x)) (p(x) is a 64-degree irreducible polynomial) can be represented as
// a0+a1*x+a2*x^2+...+a63*x^63, and each coefficient is in F_97. So it can be represented as a 97-base
// representation of a number in 0~(97^64-1). So a 64-byte array is used, while each byte only represents
// 0~97. As for p(x), also the modulus, its 65 coefficients are recorded. Be aware that the arrays are
// "little-endian", that is, the cofficient of the constant term is the first, while the cofficient of
// the highest-degree term is the last.

// The modulus(p(x)) we use
const unsigned char modulus[65] = { 1, 0, 54, 21, 9, 85, 80, 42, 24, 9, 84, 15, 88, 53, 74, 39, 62, 90, 1, 77, 59, 28, 89, 90, 2, 50, 6, 82, 3, 79, 34, 12, 6, 12, 34, 79, 3, 82, 6, 50, 2, 90, 89, 28, 59, 77, 1, 90, 62, 39, 74, 53, 88, 15, 84, 9, 24, 42, 80, 85, 9, 21, 54, 0, 1 };

// Calculate a 127-degree polynomial modulo the modulus
// Internally used
void inline polynomial_mod_128(unsigned char var[128]) {
	for (int i = 63; i >= 0; i--) {
		while (var[i + 64] != 0) {
			for (int j = 0; j < 65; j++)
				var[i + j] = var[i + j] + 97 - modulus[j];
			for (int j = 0; j < 65; j++)
				var[i + j] %= 97;
		}
	}
}

// Calculate n times of the coefficients of a 63-degree polynomial
// Internally used
void inline polynomial_scale_64(unsigned char var[64], int n) {
	for (int i = 0; i < 64; i++)
		var[i] = ((int)var[i] * n) % 97;
}

// Calculate an addition in field 97^64
void field_97_64_add(unsigned char var1[64], const unsigned char var2[64]) {
	for (int i = 0; i < 64; i++)
		var1[i] += var2[i];
	for (int i = 0; i < 64; i++)
		var1[i] %= 97;
}

// Calculate a subtraction in field 97^64
void field_97_64_sub(unsigned char var1[64], const unsigned char var2[64]) {
	for (int i = 0; i < 64; i++)
		var1[i] = var1[i] + 97 - var2[i];
	for (int i = 0; i < 64; i++)
		var1[i] %= 97;
}

// Calculate a multiplication in field 97^64
void field_97_64_mul(unsigned char var1[64], const unsigned char var2[64]) {
	unsigned char result[128], var3[64];
	memset(result, 0, 128);
	for (int i = 0; i < 64; i++) {
		memcpy(var3, var2, 64);
		polynomial_scale_64(var3, var1[i]);
		field_97_64_add(result + i, var3);
	}
	polynomial_mod_128(result);
	memcpy(var1, result, 64);
}

// Calculate the inverse of an element in field 97^64 using Fermat's Little Theorem (low efficiency)
void field_97_64_inv_fermat(unsigned char var[64]) {
	unsigned char result[64], var1[64], var2[64];
	// 97^64-2=96(97+97^2+...+97^63)+95
	memset(result, 0, 64);
	result[0] = 1;
	memcpy(var1, var, 64);
	for (int i = 1; i < 64; i++) {
		memcpy(var2, var1, 64);
		for (int j = 1; j < 97; j++)
			field_97_64_mul(var1, var2);
		field_97_64_mul(result, var1);
	}
	memcpy(var1, result, 64);
	for (int i = 1; i < 96; i++)
		field_97_64_mul(result, var1);
	for (int i = 1; i < 96; i++)
		field_97_64_mul(result, var);
	memcpy(var, result, 64);
}

// TODO: inverse using EEA