/*
 * This file is part of the libopencm3 project.
 *
 * Copyright (C) 2010 Gareth McMullin <gareth@blacksphere.co.nz>
 *
 * This library is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/usb.h>
#include <libopencm3/stm32/f4/nvic.h>
#include <libopencm3/stm32/pwr.h>
#include <libopencmsis/core_cm3.h>

#include "types.h"
#include "ring.h"

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

/*******************************************************
 *          My struct for the RF data format           *
 ******************************************************/
struct IQdata{
	int16_t I[IQ_BUFFER];
	int16_t Q[IQ_BUFFER];
};

// USB Stuff!!!!
static const struct usb_device_descriptor dev = {
	.bLength = USB_DT_DEVICE_SIZE,
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = 0x0200,
    .bDeviceClass = USB_CLASS_CDC,
	.bDeviceSubClass = 0,
	.bDeviceProtocol = 0,
	.bMaxPacketSize0 = 64,
	.idVendor = 0x0483,
	.idProduct = 0x5740,
	.bcdDevice = 0x0200,
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 3,
	.bNumConfigurations = 1,
};

/*
 * This notification endpoint isn't implemented. According to CDC spec it's
 * optional, but its absence causes a NULL pointer dereference in the
 * Linux cdc_acm driver.
 */
static const struct usb_endpoint_descriptor comm_endp[] = {{
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x83,
	.bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
	.wMaxPacketSize = 16,
	.bInterval = 255,
} };

static const struct usb_endpoint_descriptor data_endp[] = {{
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x01,
	.bmAttributes = USB_ENDPOINT_ATTR_BULK,
	.wMaxPacketSize = 64,
	.bInterval = 1,
}, {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x82,
	.bmAttributes = USB_ENDPOINT_ATTR_BULK,
	.wMaxPacketSize = 64,
	.bInterval = 1,
} };

static const struct {
	struct usb_cdc_header_descriptor header;
	struct usb_cdc_call_management_descriptor call_mgmt;
	struct usb_cdc_acm_descriptor acm;
	struct usb_cdc_union_descriptor cdc_union;
} __attribute__((packed)) cdcacm_functional_descriptors = {
	.header = {
		.bFunctionLength = sizeof(struct usb_cdc_header_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_HEADER,
		.bcdCDC = 0x0110,
	},
	.call_mgmt = {
		.bFunctionLength =
			sizeof(struct usb_cdc_call_management_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_CALL_MANAGEMENT,
		.bmCapabilities = 0,
		.bDataInterface = 1,
	},
	.acm = {
		.bFunctionLength = sizeof(struct usb_cdc_acm_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_ACM,
		.bmCapabilities = 0,
	},
	.cdc_union = {
		.bFunctionLength = sizeof(struct usb_cdc_union_descriptor),
		.bDescriptorType = CS_INTERFACE,
		.bDescriptorSubtype = USB_CDC_TYPE_UNION,
		.bControlInterface = 0,
		.bSubordinateInterface0 = 1,
	 }
};

static const struct usb_interface_descriptor comm_iface[] = {{
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 0,
	.bAlternateSetting = 0,
	.bNumEndpoints = 1,
	.bInterfaceClass = USB_CLASS_CDC,
	.bInterfaceSubClass = USB_CDC_SUBCLASS_ACM,
	.bInterfaceProtocol = USB_CDC_PROTOCOL_AT,
	.iInterface = 0,

	.endpoint = comm_endp,

	.extra = &cdcacm_functional_descriptors,
	.extralen = sizeof(cdcacm_functional_descriptors)
} };

static const struct usb_interface_descriptor data_iface[] = {{
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 1,
	.bAlternateSetting = 0,
	.bNumEndpoints = 2,
	.bInterfaceClass = USB_CLASS_DATA,
	.bInterfaceSubClass = 0,
	.bInterfaceProtocol = 0,
	.iInterface = 0,

	.endpoint = data_endp,
} };

static const struct usb_interface ifaces[] = {{
	.num_altsetting = 1,
	.altsetting = comm_iface,
}, {
	.num_altsetting = 1,
	.altsetting = data_iface,
} };

static const struct usb_config_descriptor config = {
	.bLength = USB_DT_CONFIGURATION_SIZE,
	.bDescriptorType = USB_DT_CONFIGURATION,
	.wTotalLength = 0,
	.bNumInterfaces = 2,
	.bConfigurationValue = 1,
	.iConfiguration = 0,
	.bmAttributes = 0x80,
	.bMaxPower = 0x32,

	.interface = ifaces,
};

static const char * usb_strings[] = {
	"Black Sphere Technologies",
	"CDC-ACM Demo",
	"DEMO",
};

/* Buffer to be used for control requests. */
uint8_t usbd_control_buffer[128];

int parse_cmd_packet(struct ring *ring, union txdata *output);
void generate_baseband(union txdata *output, struct IQdata *BBdata);
int _write(int file, char *ptr, int len);

static int cdcacm_control_request(usbd_device *usbd_dev,
	struct usb_setup_data *req, uint8_t **buf, uint16_t *len,
	void (**complete)(usbd_device *usbd_dev, struct usb_setup_data *req))
{
	(void)complete;
	(void)buf;
	(void)usbd_dev;

	switch (req->bRequest) {
	case USB_CDC_REQ_SET_CONTROL_LINE_STATE: {
		/*
		 * This Linux cdc_acm driver requires this to be implemented
		 * even though it's optional in the CDC spec, and we don't
		 * advertise it in the ACM functional descriptor.
		 */
		return 1;
		}
	case USB_CDC_REQ_SET_LINE_CODING:
		if (*len < sizeof(struct usb_cdc_line_coding)) {
			return 0;
		}

		return 1;
	}
	return 0;
}

static void cdcacm_data_rx_cb(usbd_device *usbd_dev, uint8_t ep)
{
	(void)ep;
	int len;
	u8 buf[64];

	len = usbd_ep_read_packet(usbd_dev, 0x01, buf, 64);

	if (len) {
		ring_safe_write(&input_ring, buf, len);
	}
}

static void cdcacm_set_config(usbd_device *usbd_dev, uint16_t wValue)
{
	(void)wValue;

	usbd_ep_setup(usbd_dev, 0x01, USB_ENDPOINT_ATTR_BULK, 64,
			cdcacm_data_rx_cb);
	usbd_ep_setup(usbd_dev, 0x82, USB_ENDPOINT_ATTR_BULK, 64, NULL);
	usbd_ep_setup(usbd_dev, 0x83, USB_ENDPOINT_ATTR_INTERRUPT, 16, NULL);

	usbd_register_control_callback(
				usbd_dev,
				USB_REQ_TYPE_CLASS | USB_REQ_TYPE_INTERFACE,
				USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
				cdcacm_control_request);
}


// Computer side packet parsing function
// Read out of the ring buffer until we have a full packet!!!!
int parse_cmd_packet(struct ring *ring, union txdata *output){
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

/* No!
int add_one_symb(struct IQdata *data, int iamp, int qamp, int offset){
	uint16_t k;
	for(k=0; k < SYMB_LEN; k++){
		data->I[offset+k] = iamp;
		data->Q[offset+k] = qamp;
	}
	return offset + SYMB_LEN;
}
*/

// Take our rfdata struct and turn it into baseband dac samples!!
void generate_baseband(union txdata *output, struct IQdata *BBdata){
	uint16_t i, j, k, iamp, qamp, offset = 0;
    uint8_t tmp, nbits;

	(void)BBdata;
	for(i = 0; i < RFDATA_LEN; i++){
		for(j = 0; j < 4; j++){
			tmp = output->bytes[i];	// Get our packet data as bytes; grab the nth one
			nbits = (tmp & 0xD0)>>6;// Grab only the top two bits and shift them down to the low end of the byte
			tmp <<= 2;		// Move the next two bits into our the masked off area of our tmp variable

			// Constellation made by this code!
			// +Q
			//  * 01    * 00
			//
			//
			//  * 11    * 10
			// -I&Q            +I
			switch(nbits){
			case 0:
				iamp = IAMP;
				qamp = QAMP;
				break;
			case 1:
				// put a symbol in the IQ data buffer
				iamp = -IAMP;
				qamp = QAMP;
				break;
			case 2:
				// put a symbol in the IQ data buffer
				iamp = IAMP;
				qamp = -QAMP;
				break;
			case 3:
				// put a symbol in the IQ data buffer
				iamp = -IAMP;
				qamp = -QAMP;
				break;
			default:
				/* Not possible, as only two bits are used for nbits. */
				return;
			}

			// put a symbol in the IQ data buffer
			for(k=0; k < SYMB_LEN; k++){
				BBdata->I[offset+k] = iamp;
				BBdata->Q[offset+k] = qamp;
			}
			offset += k; /* XXX NO IDEA LOL FIXME */
		}
	}
}

usbd_device *usbd_dev;

int _write(int __attribute__((unused)) file, char *ptr, int len)
{
	int ret;
	int i = 0;

	do {
		ret = usbd_ep_write_packet(usbd_dev, 0x82, ptr+i, len-i);
		if (ret < 0) {
			errno = EIO;
			return -1;
		}

		i += 0;
	} while (i < len);

	return len;
}

int main(void)
{
	//u8 tx_buffer;
	int rx_status, i;
	rcc_clock_setup_hse_3v3(&hse_8mhz_3v3[CLOCK_3V3_120MHZ]);

	union txdata txdata;

	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_OTGFS);

	// Setup the ring buffer
	ring_init(&input_ring, input_ring_buffer, BUFFER_SIZE);

	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE,
			GPIO9 | GPIO11 | GPIO12);
	gpio_set_af(GPIOA, GPIO_AF10, GPIO9 | GPIO11 | GPIO12);

	// LED Stuff!!!
	rcc_periph_clock_enable(RCC_GPIOD);
	gpio_mode_setup(GPIOD, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO12);

	nvic_enable_irq(NVIC_OTG_FS_IRQ);

	usbd_dev = usbd_init(&otgfs_usb_driver, &dev, &config,
			usb_strings, 3,
			usbd_control_buffer, sizeof(usbd_control_buffer));

	usbd_register_set_config_callback(usbd_dev, cdcacm_set_config);

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

/**************************
 *	ISRs go here
 *************************/
void otg_fs_isr(){
	usbd_poll(usbd_dev);
	//gpio_toggle(GPIOD, GPIO12);
}
