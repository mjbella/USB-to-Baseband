#include <stdio.h>

#include "types.h"
#include "symbol.h"
#include "sample.h"

#define ARRAY_LEN(__arr) (sizeof(__arr)/sizeof((__arr)[0]))

#define BPSK_FD 3
#define HOLD_BPSK_FD 4

#define FILTERED_BPSK_FD 5
#define FILTERED_QPSK_FD 6

#define QPSK_FD 7

u8 dat[] = {0xaa, 0xaa, 0xf0, 0xff, 0x00, 0x55, 0x33, 0x11};
float taps[] = {-0.0069882,-0.0050989,-0.0025001,0.0005886,0.0038782,0.0070346,0.0097091,0.011573,0.01235,0.011854,0.010009,0.0068689,0.0026273,-0.0023929,-0.007755,-0.012939,-0.017379,-0.020511,-0.021822,-0.02089,-0.017436,-0.011349,-0.0027058,0.0082187,0.020966,0.03492,0.049342,0.063423,0.076344,0.087328,0.095702,0.10095,0.10273,0.10095,0.095702,0.087328,0.076344,0.063423,0.049342,0.03492,0.020966,0.0082187,-0.0027058,-0.011349,-0.017436,-0.02089,-0.021822,-0.020511,-0.017379,-0.012939,-0.007755,-0.0023929,0.0026273,0.0068689,0.010009,0.011854,0.01235,0.011573,0.0097091,0.0070346,0.0038782,0.0005886,-0.0025001,-0.0050989,-0.0069882};

int main(void)
{
	int i;
	int bpsk_len;
	int qpsk_len;
	int hold_len;
	struct iq tmp;
	struct IQdata bb_data;
	struct bpsk_stream bpsk;
	struct qpsk_stream qpsk;
	struct sample_hold hold;
	struct filter_stream fs;

	init_bpsk_stream(&bpsk, dat, sizeof(dat));
	bpsk_len = bpsk.stream.len(&bpsk.stream);
	printf("bpsk_stream_length: %d\n", bpsk_len);
	for_each_symbol(&tmp, &bpsk.stream)
		dprintf(BPSK_FD, "%f,%f\n", tmp.i, tmp.q);

	init_qpsk_stream(&qpsk, dat, sizeof(dat));
	qpsk_len = qpsk.stream.len(&qpsk.stream);
	printf("qpsk_stream_length: %d\n", qpsk_len);
	for_each_symbol(&tmp, &qpsk.stream)
		dprintf(QPSK_FD, "%f,%f\n", tmp.i, tmp.q);

	init_bpsk_stream(&bpsk, dat, sizeof(dat));
	bpsk_len = bpsk.stream.len(&bpsk.stream);
	init_sample_hold(&hold, &bpsk.stream, IQ_BUFFER/bpsk_len);
	hold_len = hold.stream.len(&hold.stream);
	printf("bpsk_stream_length round 2: %d\n", bpsk_len);
	printf("sample hold length: %d\n", hold_len);

	for_each_symbol(&tmp, &hold.stream)
		dprintf(HOLD_BPSK_FD, "%f,%f\n", tmp.i, tmp.q);

	init_bpsk_stream(&bpsk, dat, sizeof(dat));
	bpsk_len = bpsk.stream.len(&bpsk.stream);
	init_sample_hold(&hold, &bpsk.stream, IQ_BUFFER/bpsk_len);
	hold_len = hold.stream.len(&hold.stream);
	init_filter_stream(&fs, &hold.stream, taps, ARRAY_LEN(taps));
	fs.stream.emit_samples(&fs.stream, &bb_data);
	for (i = 0; i < IQ_BUFFER; i++) {
		dprintf(FILTERED_BPSK_FD, "%f\n", bb_data.I[i]);
	}

	return 0;
}
