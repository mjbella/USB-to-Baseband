#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#include "convolution.h"

#define ARRAY_LEN(_a)	(sizeof(_a)/sizeof(_a[0]))
#define EPSILON 0.01f
#define OUT_LEN 100

struct testcase {
	float *in;
	size_t in_len;
	float *filter;
	size_t filter_len;
	float expected[OUT_LEN];
};

/* Build a literal array */
#define ARRAY(...) {__VA_ARGS__}

/* Create an anonymous array (C99/GCC), evaluating to the array address */
#define ANONYMOUS_ARRAY(...)	((float[]){__VA_ARGS__})

/* struct testcase contains pointers to in and filter, and literal expected */
#define IN		ANONYMOUS_ARRAY
#define FILTER		ANONYMOUS_ARRAY
#define EXPECTED	ARRAY

#define TEST_CASE(_in, _filter, _expected)	{	\
	.in = (_in),				\
	.in_len = ARRAY_LEN(_in),		\
	.filter = (_filter),			\
	.filter_len = ARRAY_LEN(_filter),	\
	.expected = _expected,				\
}

struct testcase testcases[] = {
	TEST_CASE(IN(1, 2, 3, 4),
		FILTER(-1, 5, 3),
		EXPECTED(-1.0f, 3.0f, 10.0f, 17.0f, 29.0f, 12.0f)),
	TEST_CASE(IN(1, 2, 3, 4),
		FILTER(-1, 5, 3, -1, -1),
		EXPECTED(-1.0f, 3.0f, 10.0f, 16.0f, 26.0f, 7.0f, -7.0f, -4.0f )),
};

static bool floateq(float a, float b)
{
	return fabsf(a - b) < EPSILON;
}

static void print_array(const char *name, float *arr, size_t len)
{
	int i;

	printf("%-10s {", name);
	for (i = 0; i < len; i++)
		printf("%f, ", arr[i]);
	puts("}");
}

int test(struct testcase *testcase)
{
	int i;
	float out[OUT_LEN];

	print_array("Filter:", testcase->filter, testcase->filter_len);
	print_array("Input:", testcase->in, testcase->in_len);

	convolve(testcase->in, out, testcase->filter, testcase->in_len,
		OUT_LEN, testcase->filter_len);

	print_array("Output:", out, OUT_LEN);
	print_array("Expected:", testcase->expected, OUT_LEN);

	for (i = 0; i < OUT_LEN; i++) {
		if (!floateq(out[i], testcase->expected[i])) {
			puts("FAILED");
			return 1;
		}
	}

	return 0;
}

int main(int argc, char **argv)
{
	int i;

	for (i = 0; i < ARRAY_LEN(testcases); i++) {
		printf("\nTest %d:\n", i);
		if (test(&testcases[i]))
			return -1;
	}

	return 0;
}
