#ifndef __IQ_H
#define __IQ_H

#include "iq.h"
#include "types.h"

// IQ Data Buffer Length
#define IQ_BUFFER   1024

/* TODO(mgyenik) CMSIS complex types here? */
struct iq {
	float i;
	float q;
};

/* struct for the RF data format */
struct IQdata {
	int16_t I[IQ_BUFFER];
	int16_t Q[IQ_BUFFER];
};

#endif /* __IQ_H */
