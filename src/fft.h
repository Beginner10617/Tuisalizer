#ifndef FFT_H
#define FFT_H
#include<stdio.h>
#include <complex.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
float complex *fft(float *x, size_t sz);
#ifdef FFT_IMPLEMENTATION
float complex *fft(float *x, size_t sz) {
	if (sz == 1){
		float complex *out = malloc(sizeof(float complex));
		*out = *x + 0 * I;
		return out;
	}
	float *even = malloc(sizeof(float) * sz / 2);
	float *odd = malloc(sizeof(float) * sz / 2);
	assert(odd && even);
	for(int i = 0; i < sz / 2; i++){
		even[i] = x[2 * i];
		odd[i] = x[2 * i + 1];
	}
	float complex *E = fft(even, sz / 2);
	float complex *O = fft(odd, sz / 2);
	free(even);
	free(odd);
	float complex *out = malloc(sizeof(float complex) * sz);
	assert(out);
	for(int i = 0; i < sz / 2; i++){
		float angle = -2 * M_PI * i / sz;
		double complex Ci = cosf(angle) + sinf(angle) * I;
		double complex ti = Ci * O[i];
		out[i] = E[i] + ti;
		out[i + sz / 2] = E[i] - ti;
	}
	free(O); free(E);
	return out;
}
#endif
#endif
