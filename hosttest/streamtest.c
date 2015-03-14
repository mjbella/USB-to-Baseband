#include <stdio.h>

#include "types.h"
#include "symbol.h"

u8 dat[] = {0xaa, 0xff, 0x00, 0x55, 0x33};

int main(void)
{
	struct symbol tmp;
	struct bpsk_stream bpsk;
	struct qpsk_stream qpsk;

	init_bpsk_stream(&bpsk, dat, sizeof(dat));

	puts("=== BPSK STREAM ===");
	for_each_symbol(&tmp, &bpsk.stream)
		printf("%f ||| %f\n", tmp.i, tmp.q);

	init_qpsk_stream(&qpsk, dat, sizeof(dat));

	puts("=== QPSK STREAM ===");
	for_each_symbol(&tmp, &qpsk.stream)
		printf("%f ||| %f\n", tmp.i, tmp.q);

	return 0;
}
