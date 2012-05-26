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
#define USBASP_SHARED_VID   0x16C0  /* VOTI */
#define USBASP_SHARED_PID   0x05DC  /* Obdev's free shared PID */

/* USB error identifiers */
#define USB_ERROR_NOTFOUND  1
#define USB_ERROR_ACCESS    2
#define USB_ERROR_IO        3

char progname[] = "avrdudetest";
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


/* Interface - prog. */
static int usbasp_open(libusb_device_handle **device)
{
  libusb_init(&ctx);

if (usbOpenDevice(device, USBASP_SHARED_VID, "www.fischl.de",
            USBASP_SHARED_PID, "USBasp") != 0) {

      /* no USBasp found */
      fprintf(stderr,
          "error: could not find USB device "
          "\"USBasp\" with vid=0x%x pid=0x%x\n",
          USBASP_SHARED_VID, USBASP_SHARED_PID);
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


	usbasp_open(&usbhandle);

	libusb_exit(NULL);
	return 0;
}

