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

/* struct for the normalized baseband data */
struct IQdata {
	float I[IQ_BUFFER];
	float Q[IQ_BUFFER];
};

#endif /* __IQ_H */
