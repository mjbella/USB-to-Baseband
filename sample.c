#include "convolution.h"
#include "symbol.h"
#include "sample.h"

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
