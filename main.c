#include <stdint.h>
#include <stdio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencmsis/core_cm3.h>

#include "cdcacm.h"
#include "ring.h"
#include "types.h"
#include "iq.h"
#include "dac.h"

// Ring Buffer Size
#define BUFFER_SIZE 1024

// IQ Data Buffer Length
#define IQ_BUFFER   1024

// Packet data length in bytes
#define RFDATA_LEN  64

// Symbol Length (in dac samples)
#define SYMB_LEN    50

// I & Q Amplitudes
#define IAMP        1000
#define QAMP        1000

struct ring input_ring;
uint8_t input_ring_buffer[BUFFER_SIZE];

extern uint16_t waveform[256];

/*******************************************************
 * My structs for the different types of data packets! *
 ******************************************************/
struct rfdata {
	uint8_t carrier_sync[2];    // Period of constant phase (for clock recovery to sync)
	uint8_t preamble[2];	    // Preamble for the IQ alignment
	uint8_t type;		    // Packet type (control vs data vs who knows)
	uint8_t data[59];	    // Data!!!
};

union txdata {
	struct rfdata packet;
	uint8_t bytes[RFDATA_LEN];  // Update the length of this if you change the above struct!
};

// Computer side packet parsing function
// Read out of the ring buffer until we have a full packet!!!!
static int parse_cmd_packet(struct ring *ring, union txdata *output){
	// Find the header bytes of our packet first, then read out the remaining data
	int32_t next, len, i;
	int32_t state = 0;
	int32_t retry = 0;
	uint8_t buffer[128];

	// I made this packet header up!! YAY ^_^
	const char headder[4] = {0x3A, 0x21, 0x55, 0x53};

	// Read the buffer until we get to a packet start byte!
	// OR stop when the buffer is empty!
	do{
		next = ring_read_ch(ring, 0);
		if(next < 0){
			// If the buffer is empty when we first call this function then return right away.
			if(retry == 0) return -1;
			else{
				retry++;
				// If the buffer is empty after we get one or more chars, wait for it to get full again.
				for(i=0; i < 1000; i++);
			}
		}
		else{
			// Find each byte of the packet header in order
			if(next == headder[state]) state++;
			else state = 0;
		}
	// keep going while we are still looking for more of the headder chars,
	// and we have only re-checked 5 or fewer times.
	}while( ( state < 4 ) && ( retry < 5 ) );

	// We didn't find a packet! Return witih an error.
	if(next < 0) return -1;

	len = ring_read(ring, buffer, 56);
	if(len != -55){
		// if we dont have a whole packet worth of data, then wait.
		for(i=0; i < 1000; i++);
		// Read the remaining data out
		ring_read(ring, buffer+len, 56-len);
	}
	// We should have all the data now

	// Setup the first part of the packet!
	output->packet.carrier_sync[0] = 0;
	output->packet.carrier_sync[1] = 0;
	output->packet.preamble[0]	= 0x0A;
	output->packet.preamble[1]	= 0x5F;

	// For now type is always 0
	output->packet.type = 0;

	// Fill our tx packet with the data!!
	for(i=0; i < 56; i++){
		output->packet.data[i] = buffer[i];
	}

	return 0;
}

// Take our rfdata struct and turn it into baseband dac samples!!
static void generate_baseband(union txdata *output, struct IQdata *BBdata){
	(void)output;
	(void)BBdata;
}

static void clocks(void)
{
	rcc_clock_setup_hse_3v3(&hse_8mhz_3v3[CLOCK_3V3_168MHZ]); /* Turn down for what */

	/* Port A and C are on AHB1 */
	rcc_periph_clock_enable(RCC_GPIOC); /* For timing measure pin */

	rcc_periph_clock_enable(RCC_GPIOD); /* For LEDs */
}

static void gpio_setup(void)
{
	// LED Stuff!!!
	gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO12);
	/* Set the digital test output on PC1 */
	gpio_mode_setup(GPIOC, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO1);
	gpio_set_output_options(GPIOC, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, GPIO1);

	/* Set PA4 for DAC channel 1 to analogue, ignoring drive mode. */
	gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO4);

	/* Set PA5 for DAC channel 1 to analogue, ignoring drive mode. */
	gpio_mode_setup(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO5);

}

int main(void)
{
	//u8 tx_buffer;
	int rx_status, i;

	union txdata txdata;

	// Setup the ring buffer
	ring_init(&input_ring, input_ring_buffer, BUFFER_SIZE);

	/* DAC test data */
	uint16_t j, x;
	for (j = 0; j < 256; j++) {
		if (j < 20) {
			x = 4095;
		} else if (j < 121) {
			x = 100 + ((j*j) >> 5);
		} else if (j < 170) {
			x = 10*j/2;
		} else if (j < 246) {
			x = 10*(j + (80 - j/2));
		} else {
			x = 100;
		}
		waveform[j] = x;
	}

	clocks();
	gpio_setup();
	timer_setup();
	dma_setup();
	dac_setup();

	cdcacm_init(&input_ring);

	while (1) {
	   	__WFI();
		gpio_toggle(GPIOD, GPIO12);

		rx_status = parse_cmd_packet(&input_ring, &txdata);
		for(i=0; i<100000; i++);
		printf("qwerty\r\n");

		if(rx_status > 0){
		    printf("zomg asdf");
		}

		//ring_stat = ring_read_ch(&input_ring, &tx_buffer);
		//if(ring_stat > 0)
		//{
		//	usbd_ep_write_packet(usbd_dev, 0x82, &tx_buffer, 1);
		//}
	}
}
