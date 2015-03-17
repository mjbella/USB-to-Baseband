#include <stdio.h>

#include "types.h"
#include "symbol.h"
#include "sample.h"

#define ARRAY_LEN(__arr) (sizeof(__arr)/sizeof((__arr)[0]))

#define BPSK_FD 3
#define QPSK_FD 4

#define FILTERED_BPSK_FD 5
#define FILTERED_QPSK_FD 6

u8 dat[] = {0xaa, 0xaa, 0xf0, 0xff, 0x00, 0x55, 0x33, 0x11};
float taps[] = {0.021f, 0.096f, 0.146f, 0.096f, 0.021f};

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

	init_filter_stream(&fs, &hold.stream, taps, ARRAY_LEN(taps));
	//for_each_symbol(&tmp, &hold.stream)
	//	dprintf(FILTERED_BPSK_FD, "%f,%f\n", tmp.i, tmp.q);

	fs.stream.emit_samples(&fs.stream, &bb_data);
	for (i = 0; i < IQ_BUFFER; i++) {
		dprintf(FILTERED_BPSK_FD, "%d\n", bb_data.I[i]);
	}

	return 0;
}
