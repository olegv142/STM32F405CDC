#include "usbd_cdc_if.h"
#include "usb_device.h"

#define BUFF_SZ 1024

static uint8_t  rx_buf[BUFF_SZ];
static uint8_t  tx_buf[BUFF_SZ];
static uint32_t rx_sz;
static uint32_t tx_sz;

static inline int usb_connected(void)
{
	return hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED;
}

static inline int usb_busy(void)
{
	USBD_CDC_HandleTypeDef* hcdc = (USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData;
	return !hcdc || hcdc->TxState != 0;
}

void usb_rx(uint8_t* buf, uint32_t len)
{
	if (rx_sz + len <= BUFF_SZ) {
		memcpy(rx_buf + rx_sz, buf, len);
		rx_sz += len;
	}
}

void usb_run(void)
{
	if (!usb_connected()) {
		return;
	}
	if (usb_busy()) {
		// Don't do anything till the previous packet transmission completion
		return;
	}
	if (tx_sz)
	{
		// Transmit completed, lets do some cleanup
		if (!(tx_sz % USB_FS_MAX_PACKET_SIZE)) {
			// The STM libs don't bother sending zero length packets to mark end of
			// transmission in case the last packet had max allowed size. So do it now.
			CDC_Transmit_FS(tx_buf, 0); // ZLP
		}
		tx_sz = 0;
		return;
	}
	if (rx_sz)
	{
		memcpy(tx_buf, rx_buf, rx_sz);
		tx_sz = rx_sz;
		rx_sz = 0;
		CDC_Transmit_FS(tx_buf, tx_sz);
	}
}

