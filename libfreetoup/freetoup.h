/**
 * Copyright (C) 2021 Ben Gilsrud
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef FREETOUP_H
#define FREETOUP_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h>
#include <libusb-1.0/libusb.h>

#define FREETOUP_MAX 16

typedef struct {
    char displayname[64];
    char id[64];
} FreeToupDeviceV2;

struct ft_sensor {
};

struct ft_camera {
    const char *name; /* The camera model name */
    uint16_t vendor_id;  /* The USB vendor ID */
    uint16_t product_id;  /* The USB product ID */
    struct ft_sensor *sensor; /* The sensor that the camera has */
};

struct ft_handle {
    libusb_device_handle *handle;
    uint16_t key;   /* The scramble key in use */
    char sn[32];    /* Serial number */
};

typedef struct ft_handle *HFreeToup;

/**
 * Get the version of the library.
 * @param list: Array of devices that are found
 * @return: -errno on failure, the number of cameras found on success
 */
const char *FreeToup_Version(void);

/**
 * Get list of supported cameras that are found
 * @param list: Array of devices that are found
 * @return: -errno on failure, the number of cameras found on success
 */
int FreeToup_EnumV2(FreeToupDeviceV2 list[FREETOUP_MAX]);

/**
 * Open a handle to a FreeToup camera
 * @param id: The id field from the FreeToupDeviceV2 structure for the camera
 *            to be opened
 * @return: A point to the handle on success, NULL on failure.
 */
HFreeToup FreeToup_Open(const char *id);

void FreeToup_Close(HFreeToup ft);

int FreeToup_get_SerialNumber(HFreeToup h, char sn[32]);

/**
 * Get the firmware version of a camera.
 * @param ft: The freetoup handle from Open
 * @param fwver: A buffer that can store at least 16 bytes
 * @return: 0 on success, < 0 on error
 */
int FreeToup_get_FwVersion(HFreeToup ft, char *fwver);

/**
 * Get the hardware version of a camera.
 * @param ft: The freetoup handle from Open
 * @param hwver: A buffer that can store at least 16 bytes
 * @return: 0 on success, < 0 on error
 */
int FreeToup_get_HwVersion(HFreeToup ft, char *hwver);

/**
 * Get the FPGA version of a camera.
 * @param ft: The freetoup handle from Open
 * @param hwver: A buffer that can store at least 16 bytes
 * @return: 0 on success, < 0 on error
 */
int FreeToup_get_FpgaVersion(HFreeToup ft, char *fpgaver);

/**
 * Get the revision of a camera.
 * @param ft: The freetoup handle from Open
 * @param revision: Buffer to store revision in
 * @return: 0 on success, < 0 on error
 */
int FreeToup_get_Revision(HFreeToup ft, uint16_t *revision);

/**
 * Get the production date of a camera.
 * @param ft: The freetoup handle from Open
 * @param date: A buffer that can store at least 10 bytes
 * @return: 0 on success, < 0 on error
 */
int FreeToup_get_ProductionDate(HFreeToup ft, char *date);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* FREETOUP_H */
