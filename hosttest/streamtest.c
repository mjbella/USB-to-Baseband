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
//float taps[] = {0.0141, -0.0290, -0.0167, 0.0511, 0.0188, -0.0982, -0.0200, 0.3156, 0.5205, 0.3156, -0.0200, -0.0982, 0.0188, 0.0511, -0.0167, -0.0290, 0.0141};
//float taps[] = {0.0295, 0.0304, 0.0311, 0.0318, 0.0324, 0.0328, 0.0331, 0.0333, 0.0334, 0.0333, 0.0331, 0.0328, 0.0324, 0.0318, 0.0311, 0.0304, 0.0295};
//float taps[] = {0.0133, -0.0137, -0.0426, -0.0497, -0.0161, 0.0595, 0.1554, 0.2358, 0.2671, 0.2358, 0.1554, 0.0595, -0.0161, -0.0497, -0.0426, -0.0137, 0.0133};
//float taps[] = {0.0106, -0.0091, -0.0188, 0.0326, 0.0265, -0.0851, -0.0321, 0.3109, 0.5342, 0.3109, -0.0321, -0.0851, 0.0265, 0.0326, -0.0188, -0.0091, 0.0106};
//float taps[] = {0.0389, 0.0446, 0.0500, 0.0548, 0.0589, 0.0623, 0.0648, 0.0663, 0.0668, 0.0663, 0.0648, 0.0623, 0.0589, 0.0548, 0.0500, 0.0446, 0.0389};
float taps[] = {-0.0020074,-0.0010851,-8.0001e-05,0.0010043,0.0021632,0.0033917,0.004684,0.0060335,0.0074332,0.0088754,0.010352,0.011854,0.013374,0.014901,0.016426,0.017939,0.019431,0.020892,0.022313,0.023683,0.024995,0.026237,0.027404,0.028485,0.029474,0.030364,0.031148,0.031821,0.032379,0.032816,0.033131,0.033321,0.033385,0.033321,0.033131,0.032816,0.032379,0.031821,0.031148,0.030364,0.029474,0.028485,0.027404,0.026237,0.024995,0.023683,0.022313,0.020892,0.019431,0.017939,0.016426,0.014901,0.013374,0.011854,0.010352,0.0088754,0.0074332,0.0060335,0.004684,0.0033917,0.0021632,0.0010043,-8.0001e-05,-0.0010851,-0.0020074};
//float taps[] = {0.021f, 0.096f, 0.146f, 0.096f, 0.021f};

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
