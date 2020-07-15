/**
 * Orion StarShoot G3 driver
 * This code was reverse engineered by inspecting USB traffic.
 *
 * Copyright (c) 2020 Ben Gilsrud (bgilsrud@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>. 
 */

#include <errno.h>
#include <libusb-1.0/libusb.h>
#include <inttypes.h>
#include <stdlib.h>
#include <endian.h>
#include "orion_ssg3.h"

#define ORION_SSG3_VID 0x07ee
#define ORION_SSG3_PID 0x0502
#define ORION_SSG3_INTERFACE_NUM 0
#define ORION_SSG3_BULK_EP 0x82

/* These are the defaults that Orion Camera Studio sets */
#define ORION_SSG3_DEFAULT_OFFSET 127
#define ORION_SSG3_DEFAULT_GAIN 185
#define ORION_SSG3_DEFAULT_BINNING 1

#define ICX419_EFFECTIVE_X_START 3
#define ICX419_EFFECTIVE_X_COUNT 752
#define ICX419_EFFECTIVE_Y_START 12
#define ICX419_EFFECTIVE_Y_COUNT 582
#define ICX419_PIXEL_SIZE_X 8.6
#define ICX419_PIXEL_SIZE_Y 8.4

enum {
    SSG3_CMD_BINNING = 13,
    SSG3_CMD_START_EXPOSURE = 16,
    SSG3_CMD_COOLER = 22,
    SSG3_CMD_GAIN_OFFSET = 25,
    SSG3_CMD_X_READOUT_START = 31,
    SSG3_CMD_X_READOUT_END = 32,
    SSG3_CMD_Y_READOUT_START = 33,
    SSG3_CMD_Y_READOUT_END = 34,
};

static int orion_ssg3_set_gain_offset(struct orion_ssg3 *ssg3, uint8_t gain,
        uint8_t offset);

/**
 * Convert libusb error codes to standard errno
 * @param lu_err: The libusb error code to be translated
 * @return: The corresponding errno
 */
static int libusb_to_errno(int lu_err)
{
    int rc = ENOTTY;

    switch (lu_err) {
    case LIBUSB_ERROR_IO:
        rc = EIO;
        break;
    case LIBUSB_ERROR_INVALID_PARAM:
        rc = EINVAL;
        break;
    case LIBUSB_ERROR_ACCESS:
        rc = EPERM;
        break;
    case LIBUSB_ERROR_NO_DEVICE:
        rc = ENODEV;
        break;
    case LIBUSB_ERROR_BUSY:
        rc = EBUSY;
        break;
    case LIBUSB_ERROR_TIMEOUT:
        rc = ETIMEDOUT;
        break;
    case LIBUSB_ERROR_OVERFLOW:
        rc = EOVERFLOW;
        break;
    case LIBUSB_ERROR_PIPE:
        rc = EPIPE;
        break;
    case LIBUSB_ERROR_INTERRUPTED:
        rc = EINTR;
        break;
    case LIBUSB_ERROR_NO_MEM:
        rc = ENOMEM;
        break;
    case LIBUSB_ERROR_NOT_SUPPORTED:
        rc = ENOMEM;
        break;
    }

    return rc;
}

/**
 * Open a connection to an Orion StarShoot G3 device.
 * @param ssg3: A orion_ssg3 struct that will be used to handle the connection
 * @return: 0 on success, -errno on failure
 */
int orion_ssg3_open(struct orion_ssg3 *ssg3)
{
    int rc;
    int cnt;
    int i;
    libusb_device **list;
    libusb_device *dev;
    struct libusb_device_descriptor desc;
    int found = 0;

    libusb_init(NULL);

    rc = libusb_get_device_list(NULL, &list);
    if (rc < 0) {
        return rc;
    }
    cnt = rc;

    for (i = 0; i < cnt; i++) {
        dev = list[i];
        rc = libusb_get_device_descriptor(dev, &desc);
        if (rc) {
            continue;
        }
        if ((desc.idVendor == ORION_SSG3_VID) &&
            (desc.idProduct == ORION_SSG3_PID)) {
            found = 1;
            break;
        }
    }

    if (!found) {
        return -ENODEV;
    }

	rc = libusb_open(dev, &ssg3->devh);
	if (rc) {
		return -libusb_to_errno(rc);
	}
    rc = libusb_claim_interface(ssg3->devh, ORION_SSG3_INTERFACE_NUM);
	if (rc) {
		libusb_close(ssg3->devh);
		return -libusb_to_errno(rc);
	}

    /* Set defaults since we don't know how to read them from the camera */
    ssg3->offset = ORION_SSG3_DEFAULT_OFFSET;
    ssg3->gain = ORION_SSG3_DEFAULT_GAIN;
    ssg3->bin_x = ORION_SSG3_DEFAULT_BINNING;
    ssg3->bin_y = ORION_SSG3_DEFAULT_BINNING;
    ssg3->x1 = ICX419_EFFECTIVE_X_START;
    ssg3->x_count = ICX419_EFFECTIVE_X_COUNT;
    ssg3->y1 = ICX419_EFFECTIVE_Y_START;
    ssg3->y_count = ICX419_EFFECTIVE_Y_COUNT;

    rc = orion_ssg3_set_gain_offset(ssg3, ssg3->gain, ssg3->offset);
    if (rc) {
        return rc;
    }

    rc = orion_ssg3_set_binning(ssg3, ssg3->bin_x, ssg3->bin_y);
    if (rc) {
        return rc;
    }
    return 0;
}

/**
 * Close a connection to an Orion StarShoot G3 device.
 * @param ssg3: A orion_ssg3 struct that was used to open a connection
 * @return: 0 on success, -errno on failure
 */
int orion_ssg3_close(struct orion_ssg3 *ssg3)
{
    libusb_release_interface(ssg3->devh, ORION_SSG3_INTERFACE_NUM);
	libusb_close(ssg3->devh);

	return 0;
}

/**
 * Helper function for sending commands to Orion SSG3.
 * All of the command/control of the SSG3 is done using control transfers.
 */
static int orion_ssg3_control_set(struct orion_ssg3 *ssg3, uint16_t cmd, uint16_t wValue, uint16_t wIndex)
{
    int rc;
    uint8_t bmRequestType;

    bmRequestType = LIBUSB_RECIPIENT_DEVICE | LIBUSB_REQUEST_TYPE_RESERVED
                    | LIBUSB_ENDPOINT_OUT;
    rc = libusb_control_transfer(ssg3->devh, bmRequestType, cmd, wValue, wIndex, NULL, 0, 10);

    if (rc) {
        return -libusb_to_errno(rc);
    }

    return 0;
}

/**
 * Helper function for sending commands to Orion SSG3.
 * All of the command/control of the SSG3 is done using control transfers.
 */
__attribute__((unused)) static int orion_ssg3_control_get(struct orion_ssg3 *ssg3, uint16_t cmd,
        uint16_t wValue, uint16_t wIndex, uint8_t *data, uint16_t wLength)
{
    int rc;
    uint8_t bmRequestType;

    bmRequestType = LIBUSB_RECIPIENT_DEVICE | LIBUSB_REQUEST_TYPE_RESERVED
                    | LIBUSB_ENDPOINT_IN;
    rc = libusb_control_transfer(ssg3->devh, bmRequestType, cmd, wValue, wIndex, data, wLength, 10);

    if (rc) {
        return -libusb_to_errno(rc);
    }

    return 0;
}

/* This was observed via usb traffic dumps, though I don't know what it means */
#define SSG3_COOLER_WVALUE 0x003b

int orion_ssg3_set_cooling(struct orion_ssg3 *ssg3, int on)
{
    return orion_ssg3_control_set(ssg3, SSG3_CMD_COOLER, SSG3_COOLER_WVALUE,
            on ? 1 : 0);
}

/**
 * Set the gain and offset.
 * I would like to be able to set gain or offset independently, but none of the
 * usb dumps show a read transaction for these values. So, instead, we manually
 * track the gain and offset in the ssg3 structure.
 * @param ssg3: The ssg3 structure that handles the connection to the device
 * @param gain: The gain to be set. This is 0-255
 * @param offset: The gain to be set. This is 0-255
 * @return: 0 on success, -errno on failure
 */
static int orion_ssg3_set_gain_offset(struct orion_ssg3 *ssg3, uint8_t gain,
        uint8_t offset)
{
    int rc;

    rc = orion_ssg3_control_set(ssg3, SSG3_CMD_GAIN_OFFSET, (gain << 8) | offset, 2);
    if (!rc) {
        ssg3->offset = offset;
        ssg3->gain = gain;
    }

    return rc;
}

int orion_ssg3_set_gain(struct orion_ssg3 *ssg3, uint8_t gain)
{
    return orion_ssg3_set_gain_offset(ssg3, gain, ssg3->offset);
}

int orion_ssg3_set_offset(struct orion_ssg3 *ssg3, uint8_t offset)
{
    return orion_ssg3_set_gain_offset(ssg3, ssg3->gain, offset);
}

/**
 * Set the CCD binning
 * OCS allows for any combination of 1/2 binning (1x1, 1x2, 2x1, 2x2). I don't
 * know how the x/y bin is set, but I'm guessing that x is sent in the MSB. It
 * would be easy to confirm with an additional packet capture.
 * @param ssg3: The ssg3 structure used to control the camera
 * @param x: The number of pixels to bin x
 * @param y: The number of pixels to bin y
 * @return: 0 on success, -errno otherwise
 */
int orion_ssg3_set_binning(struct orion_ssg3 *ssg3, uint8_t x, uint8_t y)
{
    int rc;

    if (x > 2 || x < 1 || y > 2 || y < 1) {
        return -EINVAL;
    }
    rc = orion_ssg3_control_set(ssg3, SSG3_CMD_BINNING, (x << 8) | y, 0);
    if (!rc) {
        /* This complicated logic is to maintain the same ROI when changing binning */
        if (x < ssg3->bin_x) {
            ssg3->x1 = ssg3->bin_x * ssg3->x1 / x;
            ssg3->x_count = ssg3->bin_x * ssg3->x_count / x;
        }
        if (y < ssg3->bin_y) {
            ssg3->y1 = ssg3->bin_y * ssg3->y1 / y;
            ssg3->y_count = ssg3->bin_y * ssg3->y_count / y;
        }

        if (y != ssg3->bin_y) {
        }
        if (ssg3->x1 <  ICX419_EFFECTIVE_X_START) {
            ssg3->x1 = ICX419_EFFECTIVE_X_START;
        }
        if (ssg3->x1 + ssg3->x_count > ICX419_EFFECTIVE_X_COUNT) {
            ssg3->x_count = ICX419_EFFECTIVE_X_COUNT;
        }
        if (ssg3->y1 <  ICX419_EFFECTIVE_Y_START) {
            ssg3->y1 = ICX419_EFFECTIVE_Y_START;
        }
        if (ssg3->y1 + ssg3->y_count > ICX419_EFFECTIVE_Y_COUNT) {
            ssg3->y_count = ICX419_EFFECTIVE_Y_COUNT;
        }

        ssg3->bin_x = x;
        ssg3->bin_y = y;
    }

    return rc;
}

/**
 * Start an exposure on the camera.
 * This sequence mimics what Orion Camera Studio does. For each exposure, OCS
 * sends a sequence of commands that specify the offsets at which the data that
 * is clocked out of the CCD should be sent over the USB. This would allow the
 * same firmware to be used for CCDs that have different geometries without any
 * changes.
 */
int orion_ssg3_start_exposure(struct orion_ssg3 *ssg3, uint32_t msec)
{
    int rc;
    rc = orion_ssg3_control_set(ssg3, SSG3_CMD_START_EXPOSURE, msec, msec >> 16);
    if (!rc) {
        gettimeofday(&ssg3->exp_done_time, NULL);
        ssg3->exp_done_time.tv_sec += msec / 1000;
        ssg3->exp_done_time.tv_usec += (msec % 1000) * 1000; 
        if (ssg3->exp_done_time.tv_usec > 1000000) {
            ssg3->exp_done_time.tv_sec++;
            ssg3->exp_done_time.tv_sec -= 1000000;
        }
    }

    return rc;
}

static int get_x2(struct orion_ssg3 *ssg3)
{
    return ssg3->x1 + ssg3->x_count - 1;
}

static int get_y2(struct orion_ssg3 *ssg3)
{
    return ssg3->y1 + ssg3->y_count - 1;
}

/**
 * Set the subframe
 * The subframe selects which region of the CCD data will be sent to the host
 * via USB.
 * @param x1: The x offset in the effective pixel area
 * @param x_count: The number of horizontal pixels after x1
 * @param y1: The y offset in the effective piyel area
 * @param y_count: The number of vertical rows after y1
 * @return: 0 on success, -errno on failure
 */
int orion_ssg3_subframe(struct orion_ssg3 *ssg3, uint16_t x1, uint16_t x_count,
        uint16_t y1, uint16_t y_count)
{
    int rc;

    ssg3->x1 = ICX419_EFFECTIVE_X_START + x1;
    ssg3->x_count = x_count;

    ssg3->y1 = ICX419_EFFECTIVE_Y_START + y1;
    ssg3->y_count = y_count;

    rc = orion_ssg3_control_set(ssg3, SSG3_CMD_X_READOUT_START, ssg3->x1, 0);
    rc = orion_ssg3_control_set(ssg3, SSG3_CMD_X_READOUT_END, get_x2(ssg3), 0);

    rc = orion_ssg3_control_set(ssg3, SSG3_CMD_Y_READOUT_START, ssg3->y1, 0);
    rc = orion_ssg3_control_set(ssg3, SSG3_CMD_Y_READOUT_END, get_y2(ssg3), 0);
    return rc;
}

/**
 * Download an image
 * The ICX419 CCD is an interlace device, which means that the even
 * and odd fields (are clocked out seperately). This is seen in the download
 * process in that the first half of the download contains the odd lines (1, 3,
 * 5, ...) followed by the even lines. So, we need to de-interlace this to
 * assemble the actual frame.
 * @param ssg3: The ssg3 structure used to communicate with the camera
 * @param buf: The buffer to store the frame in
 * @param len: The number of bytes available in buf
 */
int orion_ssg3_image_download(struct orion_ssg3 *ssg3, uint8_t *buf, int len)
{
    int needed;
    int got;
    uint16_t *tmp;
    int x, y;
    uint16_t *frame;
    int rc;

    needed = ssg3->x_count * ssg3->y_count * 2; /* 2 bytes/pixel */

    if (needed < len) {
        return -ENOSPC;
    }
    frame = (uint16_t *) buf;
    
    tmp = malloc(needed);
    if (!tmp) {
        return -ENOMEM;
    }

    rc = libusb_bulk_transfer(ssg3->devh, ORION_SSG3_BULK_EP, (unsigned char *) tmp, needed, &got, 5000);
    if (rc) {
        rc = -libusb_to_errno(rc);
        goto done;
    }
    if (needed != got) {
        /*FIXME: print a message */
    }

    for (y = 0; y < ssg3->y_count; y += 2) {
        for (x = 0; x < ssg3->x_count; x++) {
            /* The raw pixel data is sent big-endian */
            /* Even field */
            frame[x + y * ssg3->x_count] = be16toh(tmp[x + (y + ssg3->y_count/2) * ssg3->x_count]);
            /* Odd field */
            frame[x + (y + 1) * ssg3->x_count] = be16toh(tmp[x + y * ssg3->x_count]);
        }
    }
done:
    free(tmp);

    return rc;
}

int orion_ssg3_get_gain(struct orion_ssg3 *ssg3, uint8_t *gain)
{
    *gain = ssg3->gain;

    return 0;
}

int orion_ssg3_get_offset(struct orion_ssg3 *ssg3, uint8_t *offset)
{
    *offset = ssg3->offset;

    return 0;
}

int orion_ssg3_get_image_width(struct orion_ssg3 *ssg3)
{
    return ssg3->x1 + ssg3->x_count - 1;
}

int orion_ssg3_get_image_height(struct orion_ssg3 *ssg3)
{
    return ssg3->y1 + ssg3->y_count - 1;
}

int orion_ssg3_get_pixel_bit_size(struct orion_ssg3 *ssg3)
{
    (void) ssg3;
    return 16;
}

int orion_ssg3_get_pixel_size_x(struct orion_ssg3 *ssg3)
{
    (void) ssg3;
    return ICX419_PIXEL_SIZE_X;
}

int orion_ssg3_get_pixel_size_y(struct orion_ssg3 *ssg3)
{
    (void) ssg3;
    return ICX419_PIXEL_SIZE_Y;
}

int orion_ssg3_exposure_done(struct orion_ssg3 *ssg3)
{
    struct timeval now;

    gettimeofday(&now, NULL);
    if ((now.tv_sec > ssg3->exp_done_time.tv_sec) ||
        ((now.tv_sec == ssg3->exp_done_time.tv_sec) &&
         (now.tv_usec >= ssg3->exp_done_time.tv_usec))) {
        return 1;
    }
        
    return 0;
}
