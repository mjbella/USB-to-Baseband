#include "convolution.h"

/*
 * Computes contributions of each input point to the output.
 * Useful for doing convolution from a stream instead of big array.
 */
float convolve_point(float *in, float *fir, int in_len, int out_idx, int fir_len)
{
	int j;
	float out;

	out = 0.0f;
	for (j = 0; j < fir_len; j++) {
		int input_idx;

		input_idx = out_idx - j;
		if (input_idx < 0)
			continue;
		if (input_idx > in_len)
			continue;

		out += fir[j]*in[input_idx];
	}

	return out;
}

void convolve(float *in, float *out, float *fir, int in_len, int out_len,
		int fir_len)
{
	int i;

	for (i = 0; i < out_len; i++)
		out[i] = convolve_point(in, fir, in_len, i, fir_len);
}
