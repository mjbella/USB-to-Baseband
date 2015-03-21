#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/dac.h>
#include <libopencm3/stm32/dma.h>

#include "dac.h"

#define PERIOD 1152

uint16_t waveform[256];

void dma_setup(void)
{
	rcc_periph_clock_enable(RCC_DMA1);
	nvic_enable_irq(NVIC_DMA1_STREAM5_IRQ);

	dma_stream_reset(DMA1, DMA_STREAM5);
	dma_stream_reset(DMA1, DMA_STREAM6);

	dma_set_priority(DMA1, DMA_STREAM5, DMA_SxCR_PL_LOW);
	dma_set_priority(DMA1, DMA_STREAM6, DMA_SxCR_PL_LOW);

	/* Use 16-bits here since we'll do a full 12-bit conversion */
	dma_set_memory_size(DMA1, DMA_STREAM5, DMA_SxCR_MSIZE_16BIT);
	dma_set_memory_size(DMA1, DMA_STREAM6, DMA_SxCR_MSIZE_16BIT);

	/* Use 16-bits here since we'll do a full 12-bit conversion */
	dma_set_peripheral_size(DMA1, DMA_STREAM5, DMA_SxCR_PSIZE_16BIT);
	dma_set_peripheral_size(DMA1, DMA_STREAM6, DMA_SxCR_PSIZE_16BIT);

	/* Auto-increment source memry address */
	dma_enable_memory_increment_mode(DMA1, DMA_STREAM5);
	dma_enable_memory_increment_mode(DMA1, DMA_STREAM6);

	/* Set up circular transfer that loops back to beginning TODO(myenik) remove this for individual packet transfers? */
	dma_enable_circular_mode(DMA1, DMA_STREAM5);
	dma_enable_circular_mode(DMA1, DMA_STREAM6);

	/* We're transferring memory contents to the DAC peripheral */
	dma_set_transfer_mode(DMA1, DMA_STREAM5, DMA_SxCR_DIR_MEM_TO_PERIPHERAL);
	dma_set_transfer_mode(DMA1, DMA_STREAM6, DMA_SxCR_DIR_MEM_TO_PERIPHERAL);

	/* The target is the DAC 12-bit right justified data register (so we must use 16-bit mem size above) */
	dma_set_peripheral_address(DMA1, DMA_STREAM5, (uint32_t) &DAC_DHR12R1);
	dma_set_peripheral_address(DMA1, DMA_STREAM6, (uint32_t) &DAC_DHR12R2);

	/* TODO(myenik) change these for baseband packets */
	dma_set_memory_address(DMA1, DMA_STREAM5, (uint32_t)waveform);
	dma_set_memory_address(DMA1, DMA_STREAM6, (uint32_t)waveform);
	dma_set_number_of_data(DMA1, DMA_STREAM5, 256);
	dma_set_number_of_data(DMA1, DMA_STREAM6, 256);

	dma_channel_select(DMA1, DMA_STREAM5, DMA_SxCR_CHSEL_7);
	dma_channel_select(DMA1, DMA_STREAM6, DMA_SxCR_CHSEL_7);
	dma_enable_stream(DMA1, DMA_STREAM5);
	dma_enable_stream(DMA1, DMA_STREAM6);
}

void timer_setup(void)
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

void dac_setup(void)
{
	rcc_periph_clock_enable(RCC_GPIOA); /* For DAC output */
	rcc_periph_clock_enable(RCC_DAC); /* Enable the DAC clock on APB1 */

	// Setup the DAC channels 1 & 2, with timer 2 as trigger source.
	// Assume the DAC has woken up by the time the first transfer occurs
	dac_trigger_enable(CHANNEL_1);
	dac_trigger_enable(CHANNEL_2);
	dac_set_trigger_source(DAC_CR_TSEL1_T2);
	dac_set_trigger_source(DAC_CR_TSEL2_T2);
	dac_dma_enable(CHANNEL_1);
	dac_dma_enable(CHANNEL_2);
	dac_enable(CHANNEL_1);
	dac_enable(CHANNEL_2);
}

void dma1_stream5_isr(void)
{
	/* TODO(myenik) actually clear both interrupt flags and use this for next-packet queueing */
	if (dma_get_interrupt_flag(DMA1, DMA_STREAM5, DMA_TCIF)) {
		dma_clear_interrupt_flags(DMA1, DMA_STREAM5, DMA_TCIF);
		/* Toggle PC1 just to keep aware of activity and frequency. */
		gpio_toggle(GPIOC, GPIO1);
	}
}
