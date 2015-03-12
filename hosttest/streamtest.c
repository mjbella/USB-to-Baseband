#include <stdio.h>

#include <types.h>
#include <symbol.h>

u8 dat[] = {0xaa, 0xff, 0x00, 0x55, 0x33};

int main(void)
{
	struct symbol tmp;
	struct bpsk_stream bpsk;

	init_bpsk_stream(&bpsk, dat, sizeof(dat));

	for_each_symbol(&tmp, &bpsk.stream)
		printf("%f ||| %f\n", tmp.i, tmp.q);
	
	return 0;
}
