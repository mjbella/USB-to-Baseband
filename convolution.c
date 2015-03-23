#include "convolution.h"

/*
 * TODO(mgyenik) lol this is dumb. Good thing we don't have to worry about
 * making it faster by being nice to cache lines or anything. This also needs
 * to use the symmetry of linear FIR filters to avoid half of the calculations
 */
void convolve(float *in, float *out, float *fir, int in_len, int out_len, int fir_len)
{
	int i;
	int j;

	for (i = 0; i < out_len; i++) {
		out[i] = 0.0f;
		for (j = 0; j < fir_len; j++) {
			int input_idx;

			input_idx = i - j;
			if (input_idx < 0)
				continue;
			if (input_idx > in_len)
				continue;

			out[i] += fir[j]*in[input_idx];
		}
	}
}

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
