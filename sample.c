#include "symbol.h"
#include "sample.h"
#include <stdio.h>

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
		//printf("convolve\n");
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

/* Takes n symbols from a stream and buffers them into separate buffers for each phase */
int fill_iq_bufs(struct symbol_stream *ss, float *i, float *q, int n)
{
	int j;
	struct iq tmp;

	for (j = 0; j < n; j++) {
		if (!ss->get_symbol(ss, &tmp)) {
			return j;
		}

		i[j] = tmp.i;
		q[j] = tmp.q;
	}

	return 0;
}

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

static int filter_emit_samples(struct sample_stream *ss, struct IQdata *bb_data)
{
	int len;
	int j;
	struct symbol;
	struct filter_stream *fs;

	len = ss->len(ss);
	float i[len];
	float q[len];

	fs = to_filter_stream(ss);
	
	if (fill_iq_bufs(fs->sym_stream, i, q, len)) {
		return -1;
	}

	/* Apply pulse shaping filter to both in-phase and quadrature-phase components */
	for (j = 0; j < len; j++) {
		bb_data->I[j] = convolve_point(i, fs->filter_coeff, len, j, fs->filter_len);
		bb_data->Q[j] = convolve_point(q, fs->filter_coeff, len, j, fs->filter_len);
	}

	return 0;
}

static int filter_stream_len(struct sample_stream * ss)
{
	struct filter_stream *fs;

	fs = to_filter_stream(ss);
	return fs->sym_stream->len(fs->sym_stream);
}

int init_filter_stream(struct filter_stream *fs, struct symbol_stream *ss, float *coeffs, int filter_len)
{
	fs->filter_coeff = coeffs;
	fs->filter_len = filter_len;
	fs->sym_stream = ss;

	fs->stream.emit_samples = filter_emit_samples;
	fs->stream.len = filter_stream_len;

	return 0;
}
