#ifndef __SYMBOL_H
#define __SYMBOL_H

#include <types.h>

/* TODO(mgyenik) find this a better home... */
#define container_of(ptr, type, member) ({ \
	const typeof( ((type *)0)->member ) *__mptr = (ptr); \
	(type *)( (char *)__mptr - __builtin_offsetof(type,member) );})

/* This will blow up if the struct symbol_stream in your struct is not named stream */
#define to_bpsk_stream(__ss) container_of(__ss, struct bpsk_stream, stream)

/*
 * Easy macro for iterating through all symbols in stream.
 * arguments: pointer to store current symbol, stream to go through
 */
 /*
#define for_each_symbol(__sym, __ss) \
	for (int i = (__ss)->get_symbol((__ss), (__sym)); \
		 i != 0; \
		 i = (__ss)->get_symbol((__ss), (__sym)))
*/
#define for_each_symbol(__sym, __ss) \
		 while ((__ss)->get_symbol((__ss), (__sym)))

/* TODO(mgyenik) CMSIS complex types here? */
struct symbol {
	float i;
	float q;
};

/*
 * A symbol stream is a handle that contains the function
 * for the specific implementation of symbol_stream
 */
struct symbol_stream {
	int (*get_symbol)(struct symbol_stream *, struct symbol *);
};

struct bpsk_stream {
	int len;
	int bits;
	int bit_idx;
	struct symbol_stream stream;
	u8 *data;
};

int bpsk_get_symbol(struct symbol_stream *ss, struct symbol *sym);
int init_bpsk_stream(struct bpsk_stream *bs, u8 *data, int n);
#endif /* __SYMBOL_H */
