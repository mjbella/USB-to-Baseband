/*
 * This file contains implementations for some basic symbol streams,
 */
#include <symbol.h>
#include <stdint.h>

struct symbol bpsk_lookup[] = {
	{
		.i = -1.0f,
		.q = 0.0f,
	},
	{
		.i = 1.0f,
		.q = 0.0f,
	}
};

int bpsk_get_symbol(struct symbol_stream *ss, struct symbol *sym)
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

	*sym = bpsk_lookup[bit];

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

	return 0;
}
