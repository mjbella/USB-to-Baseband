#ifndef __CONVOLUTION_H
#define __CONVOLUTION_H

void convolve(float *in, float *out, float *fir, int in_len, int out_len,
		int fir_len);

float convolve_point(float *in, float *fir, int in_len, int out_idx,
		int fir_len);

#endif /* __CONVOLUTION_H */
