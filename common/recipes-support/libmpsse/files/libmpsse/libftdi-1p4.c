#include <stdio.h>
#include <stdint.h>
#include <libftdi1/ftdi.h>
#include <libusb-1.0/libusb.h>

// clear buffer correctly
#define SIO_TCIFLUSH 2
#define SIO_TCOFLUSH 1
int ftdi_tciflush(struct ftdi_context *ftdi)
{
  if (ftdi == NULL || ftdi->usb_dev == NULL)
    printf("%s() USB device unavailable\n", __func__);

  if (libusb_control_transfer(ftdi->usb_dev, FTDI_DEVICE_OUT_REQTYPE,
        SIO_RESET_REQUEST, SIO_TCIFLUSH,
        ftdi->index, NULL, 0, ftdi->usb_write_timeout) < 0)
    printf("%s() FTDI purge of RX buffer failed\n", __func__);

  // Invalidate data in the readbuffer
  ftdi->readbuffer_offset = 0;
  ftdi->readbuffer_remaining = 0;
  return 0;
}

int ftdi_tcoflush(struct ftdi_context *ftdi)
{
  if (ftdi == NULL || ftdi->usb_dev == NULL)
    printf("%s() USB device unavailable\n", __func__);

  if (libusb_control_transfer(ftdi->usb_dev, FTDI_DEVICE_OUT_REQTYPE,
        SIO_RESET_REQUEST, SIO_TCOFLUSH,
        ftdi->index, NULL, 0, ftdi->usb_write_timeout) < 0)
    printf("%s() FTDI purge of TX buffer failed\n", __func__);

  return 0;
}

int ftdi_tcioflush(struct ftdi_context *ftdi)
{
  int ret = -1;

  do {
    if (ftdi == NULL || ftdi->usb_dev == NULL) {
      printf("%s() USB device unavailable\n", __func__);
      break;
    }

    if ( ftdi_tcoflush(ftdi) < 0) {
      printf("%s() Failed to do ftdi_tcoflush\n", __func__);
      break;
    }

    if ( ftdi_tciflush(ftdi) < 0) {
      printf("%s() Failed to do ftdi_tciflush\n", __func__);
      break;
    }
    ret = 0;
  } while(0);

  return ret;
}

