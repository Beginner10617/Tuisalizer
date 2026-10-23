#define FFT_IMPLEMENTATION
#include "src/fft.h"
#include <stdio.h>
int main() { 
	float arr[5] = {0, 1, 0, -1, 0};
	float complex *out = fft(arr, 5);
	for(int i = 0; i < 5; i++) printf("%d : %f\n", i, cabs(out[i]));
	return 0; 
}
