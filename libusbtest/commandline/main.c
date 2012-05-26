/*
 * libusbx example program to list devices on the bus
 * Copyright © 2007 Daniel Drake <dsd@gentoo.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <stdio.h>
#include <sys/types.h>
#include <errno.h>

#include <libusb.h>

static libusb_context *ctx = NULL;

/* USB identifiers */
#define USBDEVICE_SHARED_VID   0x16C0  /* VOTI */
#define USBDEVICE_SHARED_PID   0x05DC  /* Obdev's free shared PID */

/* USB error identifiers */
#define USB_ERROR_NOTFOUND  1
#define USB_ERROR_ACCESS    2
#define USB_ERROR_IO        3

char progname[] = "libusbtest";
char verbose = 2;

static int libusb_to_errno(int result)
{
    switch (result) {
    case LIBUSB_SUCCESS:
        return 0;
    case LIBUSB_ERROR_IO:
        return EIO;
    case LIBUSB_ERROR_INVALID_PARAM:
        return EINVAL;
    case LIBUSB_ERROR_ACCESS:
        return EACCES;
    case LIBUSB_ERROR_NO_DEVICE:
        return ENXIO;
    case LIBUSB_ERROR_NOT_FOUND:
        return ENOENT;
    case LIBUSB_ERROR_BUSY:
        return EBUSY;
#ifdef ETIMEDOUT
    case LIBUSB_ERROR_TIMEOUT:
        return ETIMEDOUT;
#endif
#ifdef EOVERFLOW
    case LIBUSB_ERROR_OVERFLOW:
        return EOVERFLOW;
#endif
    case LIBUSB_ERROR_PIPE:
        return EPIPE;
    case LIBUSB_ERROR_INTERRUPTED:
        return EINTR;
    case LIBUSB_ERROR_NO_MEM:
        return ENOMEM;
    case LIBUSB_ERROR_NOT_SUPPORTED:
        return ENOSYS;
    default:
        return ERANGE;
    }
}


static int usbOpenDevice(libusb_device_handle **device, int vendor,
             char *vendorName, int product, char *productName)
{
    libusb_device_handle *handle = NULL;
    int                  errorCode = USB_ERROR_NOTFOUND;
    static int           didUsbInit = 0;
    int j;
    int r;

    if(!didUsbInit){
        didUsbInit = 1;
        libusb_init(&ctx);
    }

    libusb_device **dev_list;
    int dev_list_len = libusb_get_device_list(ctx, &dev_list);

    for (j=0; j<dev_list_len; ++j) {
        libusb_device *dev = dev_list[j];
        struct libusb_device_descriptor descriptor;
    libusb_get_device_descriptor(dev, &descriptor);
    if (descriptor.idVendor == vendor && descriptor.idProduct == product) {
            char    string[256];
        /* we need to open the device in order to query strings */
            r = libusb_open(dev, &handle);
            if (!handle) {
                 errorCode = USB_ERROR_ACCESS;
                 fprintf(stderr,
                "%s: Warning: cannot open USB device: %d %s\n",
                progname, r, strerror(libusb_to_errno(r)));
                    continue;
                }
                if (vendorName == NULL && productName == NULL) {
            /* name does not matter */
                    break;
                }
                /* now check whether the names match: */
        r = libusb_get_string_descriptor_ascii(handle, descriptor.iManufacturer & 0xff, (unsigned char *)string, sizeof(string));
                if (r < 0) {
                    errorCode = USB_ERROR_IO;
                    fprintf(stderr,
                "%s: Warning: cannot query manufacturer for device: %s\n",
                progname, strerror(libusb_to_errno(r)));
                } else {
                    errorCode = USB_ERROR_NOTFOUND;
            if (verbose > 1)
                fprintf(stderr,
                "%s: seen device from vendor ->%s<-\n",
                progname, string);
                    if (strcmp(string, vendorName) == 0){
            r = libusb_get_string_descriptor_ascii(handle, descriptor.iProduct & 0xff, (unsigned char *)string, sizeof(string));
                        if (r < 0) {
                            errorCode = USB_ERROR_IO;
                            fprintf(stderr,
                    "%s: Warning: cannot query product for device: %s\n",
                    progname, strerror(libusb_to_errno(r)));
                        } else {
                            errorCode = USB_ERROR_NOTFOUND;
                if (verbose > 1)
                    fprintf(stderr,
                    "%s: seen product ->%s<-\n",
                    progname, string);
                            if(strcmp(string, productName) == 0)
                                break;
                        }
                    }
                }
                libusb_close(handle);
                handle = NULL;
            }
    }
    if (handle != NULL){
        errorCode = 0;
        *device = handle;
    }
    return errorCode;
}

/* USB function call identifiers */
#define USBASP_FUNC_CONNECT    1
#define USBASP_FUNC_DISCONNECT 2
#define USBASP_FUNC_TRANSMIT   3
#define USBASP_FUNC_READFLASH  4
#define USBASP_FUNC_ENABLEPROG 5
#define USBASP_FUNC_WRITEFLASH 6
#define USBASP_FUNC_READEEPROM 7
#define USBASP_FUNC_WRITEEEPROM 8
#define USBASP_FUNC_SETLONGADDRESS 9
#define USBASP_FUNC_SETISPSCK 10
#define USBASP_FUNC_TPI_CONNECT      11
#define USBASP_FUNC_TPI_DISCONNECT   12
#define USBASP_FUNC_TPI_RAWREAD      13
#define USBASP_FUNC_TPI_RAWWRITE     14
#define USBASP_FUNC_TPI_READBLOCK    15
#define USBASP_FUNC_TPI_WRITEBLOCK   16
#define USBASP_FUNC_GETCAPABILITIES 127


static int usbdevice_transmit(libusb_device_handle *device,
               unsigned char receive, unsigned char functionid,
               unsigned char send[4], unsigned char * buffer, int buffersize)
{
  int nbytes;
  nbytes = libusb_control_transfer(device,
                   (LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | (receive << 7)) & 0xff,
                   functionid & 0xff,
                   ((send[1] << 8) | send[0]) & 0xffff,
                   ((send[3] << 8) | send[2]) & 0xffff,
                   (unsigned char *)buffer,
                   buffersize & 0xffff,
                   5000);
  if(nbytes < 0){
    fprintf(stderr, "%s: error: usbasp_transmit: %s\n", progname, strerror(libusb_to_errno(nbytes)));
    return -1;
  }
  return nbytes;
}


/* Universal functions: for both SPI and TPI */
static int usbasp_initialize(libusb_device_handle *device)
{
  unsigned char temp[4];
  unsigned char res[10];
  unsigned int length;

  /* get capabilities */
  memset(temp, 0, sizeof(temp));
  length = usbdevice_transmit(device, 1, USBASP_FUNC_GETCAPABILITIES, temp, res, sizeof(res));
  if(length)
  {
      fprintf(stderr, "got %d bytes: %X %X %X %X %X %X %X %X %X %X", length, res[0], res[1], res[2], res[3], res[4], res[5], res[6], res[7], res[8], res[9]);
  }
  else
  {
      fprintf(stderr, "no capabilities");
  }

  return 0;
}


/* Interface - prog. */
static int usbdevice_open (libusb_device_handle **device)
{
    libusb_init(&ctx);

    if (usbOpenDevice(device, USBDEVICE_SHARED_VID, "Embedded Creations",
            USBDEVICE_SHARED_PID, "libusbtest") != 0)
    {

        /* no USBasp found */
        fprintf(stderr, "error: could not find USB device "
                "\"libusbtest\" with vid=0x%x pid=0x%x\n", USBDEVICE_SHARED_VID,
                USBDEVICE_SHARED_PID);
        return -1;
    }

    return 0;
}


static void print_devs(libusb_device **devs)
{
	libusb_device *dev;
	int i = 0;

	while ((dev = devs[i++]) != NULL) {
		struct libusb_device_descriptor desc;
		int r = libusb_get_device_descriptor(dev, &desc);
		if (r < 0) {
			fprintf(stderr, "failed to get device descriptor");
			return;
		}

		printf("%04x:%04x (bus %d, device %d)\n",
			desc.idVendor, desc.idProduct,
			libusb_get_bus_number(dev), libusb_get_device_address(dev));
	}
}



int main(void)
{
	libusb_device **devs;
	libusb_device_handle *usbhandle;
	int r;
	ssize_t cnt;

	r = libusb_init(NULL);
	if (r < 0)
		return r;

	cnt = libusb_get_device_list(NULL, &devs);
	if (cnt < 0)
		return (int) cnt;

	print_devs(devs);
	libusb_free_device_list(devs, 1);


	if(!usbdevice_open(&usbhandle))
	    usbasp_initialize(usbhandle);

	libusb_exit(NULL);
	return 0;
}

