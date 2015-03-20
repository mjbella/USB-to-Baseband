#include <stdint.h>
#include <stdio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencmsis/core_cm3.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/dac.h>
#include <libopencm3/stm32/dma.h>

#include "cdcacm.h"
#include "ring.h"
#include "types.h"
#include "iq.h"

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

#define PERIOD 1152

struct ring input_ring;
uint8_t input_ring_buffer[BUFFER_SIZE];

/* Globals */
uint8_t waveform[256];


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

static void timer_setup(void)
{
	/* Enable TIM2 clock. */
	rcc_periph_clock_enable(RCC_TIM2);
	timer_reset(TIM2);
	/* Timer global mode: - No divider, Alignment edge, Direction up */
	timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT,
		       TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
	timer_continuous_mode(TIM2);
	timer_set_period(TIM2, PERIOD);
	timer_disable_oc_output(TIM2, TIM_OC2 | TIM_OC3 | TIM_OC4);
	timer_enable_oc_output(TIM2, TIM_OC1);
	timer_disable_oc_clear(TIM2, TIM_OC1);
	timer_disable_oc_preload(TIM2, TIM_OC1);
	timer_set_oc_slow_mode(TIM2, TIM_OC1);
	timer_set_oc_mode(TIM2, TIM_OC1, TIM_OCM_TOGGLE);
	timer_set_oc_value(TIM2, TIM_OC1, 500);
	timer_disable_preload(TIM2);
	/* Set the timer trigger output (for the DAC) to the channel 1 output
	   compare */
	timer_set_master_mode(TIM2, TIM_CR2_MMS_COMPARE_OC1REF);
	timer_enable_counter(TIM2);
}

static void dac_setup(void)
{
	/* Enable the DAC clock on APB1 */
	rcc_periph_clock_enable(RCC_DAC);
	/* Setup the DAC channel 1, with timer 2 as trigger source.
	 * Assume the DAC has woken up by the time the first transfer occurs */
	dac_trigger_enable(CHANNEL_1);
	dac_set_trigger_source(DAC_CR_TSEL1_T2);
	dac_dma_enable(CHANNEL_1);
	dac_enable(CHANNEL_1);
}

void dma1_stream5_isr(void)
{
	if (dma_get_interrupt_flag(DMA1, DMA_STREAM5, DMA_TCIF)) {
		dma_clear_interrupt_flags(DMA1, DMA_STREAM5, DMA_TCIF);
		/* Toggle PC1 just to keep aware of activity and frequency. */
		gpio_toggle(GPIOC, GPIO1);
	}
}

static void dma_setup(void)
{
	/* DAC channel 1 uses DMA controller 1 Stream 5 Channel 7. */
	/* Enable DMA1 clock and IRQ */
	rcc_periph_clock_enable(RCC_DMA1);
	nvic_enable_irq(NVIC_DMA1_STREAM5_IRQ);
	dma_stream_reset(DMA1, DMA_STREAM5);
	dma_set_priority(DMA1, DMA_STREAM5, DMA_SxCR_PL_LOW);
	dma_set_memory_size(DMA1, DMA_STREAM5, DMA_SxCR_MSIZE_8BIT);
	dma_set_peripheral_size(DMA1, DMA_STREAM5, DMA_SxCR_PSIZE_8BIT);
	dma_enable_memory_increment_mode(DMA1, DMA_STREAM5);
	dma_enable_circular_mode(DMA1, DMA_STREAM5);
	dma_set_transfer_mode(DMA1, DMA_STREAM5,
				DMA_SxCR_DIR_MEM_TO_PERIPHERAL);
	/* The register to target is the DAC1 8-bit right justified data
	   register */
	dma_set_peripheral_address(DMA1, DMA_STREAM5, (uint32_t) &DAC_DHR8R1);
	/* The array v[] is filled with the waveform data to be output */
	dma_set_memory_address(DMA1, DMA_STREAM5, (uint32_t) waveform);
	dma_set_number_of_data(DMA1, DMA_STREAM5, 256);
	dma_enable_transfer_complete_interrupt(DMA1, DMA_STREAM5);
	dma_channel_select(DMA1, DMA_STREAM5, DMA_SxCR_CHSEL_7);
	dma_enable_stream(DMA1, DMA_STREAM5);
}

static void clocks(void)
{
	rcc_clock_setup_hse_3v3(&hse_8mhz_3v3[CLOCK_3V3_168MHZ]); /* Turn down for what */

	/* Port A and C are on AHB1 */
	rcc_periph_clock_enable(RCC_GPIOA); /* For DAC output */
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
		if (j < 10) {
			x = 10;
		} else if (j < 121) {
			x = 10 + ((j*j) >> 7);
		} else if (j < 170) {
			x = j/2;
		} else if (j < 246) {
			x = j + (80 - j/2);
		} else {
			x = 10;
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
