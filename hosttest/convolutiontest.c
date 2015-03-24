#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#include "convolution.h"

#define ARRAY_LEN(_a)	(sizeof(_a)/sizeof(_a[0]))
#define EPSILON 0.01f

float in[] = {1, 2, 3, 4};
float filter[] = {-1, 5, 3};
float out[10];

float expected[10] = {-1.0f, 3.0f, 10.0f, 17.0f, 29.0f, 12.0f, 0.0f, };

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

int main(int argc, char **argv)
{
	int i;

	print_array("Filter:", filter, ARRAY_LEN(filter));
	print_array("Input:", in, ARRAY_LEN(in));

	convolve(in, out, filter, ARRAY_LEN(in), ARRAY_LEN(out),
			ARRAY_LEN(filter));

	print_array("Output:", out, ARRAY_LEN(out));
	print_array("Expected:", expected, ARRAY_LEN(expected));

	for (i = 0; i < ARRAY_LEN(expected); i++) {
		if (!floateq(out[i], expected[i])) {
			puts("FAILED");
			return -1;
		}
	}

	return 0;
}
