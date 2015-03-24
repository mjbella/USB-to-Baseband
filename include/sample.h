#ifndef __SAMPLE_H
#define __SAMPLE_H

#include "iq.h"
#include "container.h"

/*
 * A sample stream generates samples from a symbol_stream.
 * Currently it's not really a "stream", it just populates an entire
 * packet's worth of baseband data.
 *
 * TODO(mgyenik) investigate lazy sample building
 * via a "get_sample" or "emit_sample".
 */
struct sample_stream {
	int (*emit_samples)(struct sample_stream *, struct IQdata *baseband_samples);
	int (*len)(struct sample_stream *);
};

/* filter_stream is a sample_stream implemented using an FIR filter */
struct filter_stream {
	int filter_len;
	float *filter_coeff;
	struct symbol_stream *sym_stream;
	struct sample_stream stream;
};

#define to_filter_stream(__ss) container_of(__ss, struct filter_stream, stream)

int fill_iq_bufs(struct symbol_stream *ss, float *i, float *q, int n);
int init_filter_stream(struct filter_stream *fs, struct symbol_stream *ss, float *coeffs, int filter_len);
int init_sample_hold(struct sample_hold *sh, struct symbol_stream *ss, int hold_count);

#endif /* __SAMPLE_H */
