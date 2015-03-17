/*
 * This file contains implementations for some basic symbol streams,
 */
#include <stdint.h>

#include "symbol.h"

struct iq bpsk_constellation[] = {
	{
		.i = -1.0f,
		.q = 0.0f,
	},
	{
		.i = 1.0f,
		.q = 0.0f,
	}
};

static int bpsk_stream_len(struct symbol_stream *ss)
{
	struct bpsk_stream *bs;

	bs = to_bpsk_stream(ss);
	return bs->bits - bs->bit_idx;
}

static int bpsk_get_symbol(struct symbol_stream *ss, struct iq *sym)
{
	u8 bit;
	int idx;
	u8 shift;
	struct bpsk_stream *bs;

	bs = to_bpsk_stream(ss);

	if (bs->bit_idx >= bs->bits) {
		return 0; /* stream drained */
	}

	/*
	 * TODO(mgyenik) doing these calculations each time is wasteful. We can
	 * speed up the implementation if needed by caching some of this
	 * intermediate state in the bpsk_stream struct and just updating it.
	 */
	idx = bs->bit_idx >> 3;
	shift = bs->bit_idx & 0x7;
	shift = 7 - shift;
	bit = bs->data[idx] >> shift;
	bit &= 0x1;

	*sym = bpsk_constellation[bit];

	bs->bit_idx++;
	return 1;
}

int init_bpsk_stream(struct bpsk_stream *bs, u8 *data, int n)
{
	bs->bit_idx = 0;
	bs->len = n;
	bs->bits = n*8;
	bs->data = data;
	bs->stream.get_symbol = bpsk_get_symbol;
	bs->stream.len = bpsk_stream_len;

	return 0;
}

/* Below is QPSK, very similar... */
struct iq qpsk_constellation[] = {
	{
		.i = -SQRT2_INV,
		.q = -SQRT2_INV,
	},
	{
		.i = -SQRT2_INV,
		.q = SQRT2_INV,
	},
	{
		.i = SQRT2_INV,
		.q = -SQRT2_INV,
	},
	{
		.i = SQRT2_INV,
		.q = SQRT2_INV,
	},
};

static int qpsk_stream_len(struct symbol_stream *ss)
{
	struct qpsk_stream *qs;

	qs = to_qpsk_stream(ss);
	return (qs->bits - qs->bit_idx)/2;
}

static int qpsk_get_symbol(struct symbol_stream *ss, struct iq *sym)
{
	u8 bits;
	int idx;
	u8 shift;
	struct qpsk_stream *qs;

	qs = to_qpsk_stream(ss);

	if (qs->bit_idx >= qs->bits) {
		return 0; /* stream drained */
	}

	/*
	 * TODO(mgyenik) doing these calculations each time is wasteful. We can
	 * speed up the implementation if needed by caching some of this
	 * intermediate state in the bpsk_stream struct and just updating it.
	 */
	idx = qs->bit_idx >> 3;
	shift = qs->bit_idx & 0x7;
	shift = 6 - shift;
	bits = qs->data[idx] >> shift;
	bits &= 0x3;

	*sym = qpsk_constellation[bits];

	qs->bit_idx += 2;
	return 1;
}

int init_qpsk_stream(struct qpsk_stream *qs, u8 *data, int n)
{
	qs->bit_idx = 0;
	qs->len = n;
	qs->bits = n*8;
	qs->data = data;
	qs->stream.get_symbol = qpsk_get_symbol;
	qs->stream.len = qpsk_stream_len;

	return 0;
}

static int sample_hold_len(struct symbol_stream *ss)
{
	int orig_len;
	struct symbol_stream *oss;
	struct sample_hold *sh;

	sh = to_sample_hold(ss);
	oss = sh->orig_stream;
	orig_len = sh->orig_stream->len(sh->orig_stream);
	return sh->hold*orig_len - sh->count;
}

static int get_sample_hold(struct symbol_stream *ss, struct iq *sym)
{
	struct sample_hold *sh;

	sh = to_sample_hold(ss);
	if (sh->count < sh->hold) {
		*sym = sh->current;
		sh->count++;
	} else {
		if(!sh->orig_stream->get_symbol(sh->orig_stream, &sh->current)) {
			return 0; /* stream drained */
		}
		sh->count = 0;
		*sym = sh->current;
	}

	return 1;
}

int init_sample_hold(struct sample_hold *sh, struct symbol_stream *ss, int hold_count)
{
	sh->hold = hold_count;
	sh->orig_stream = ss;
	sh->count = 0;

	sh->stream.get_symbol = get_sample_hold;
	sh->stream.len = sample_hold_len;
}
