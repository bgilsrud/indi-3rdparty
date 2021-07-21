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

#include <endian.h>
#include <errno.h>
#include <libusb-1.0/libusb.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "freetoup.h"
#include "ft_cameras.h"
#include "config.h"

enum {
    TOUP_CMD_READ_SENSOR_REG = 10,
    TOUP_CMD_WRITE_SENSOR_REG = 11,
    TOUP_CMD_SCRAMBLE_KEY = 22,
    TOUP_CMD_FW_VER = 30,
    TOUP_CMD_HW_VER = 31,
    TOUP_CMD_READ_EEPROM = 32,
    TOUP_CMD_CRYPT_CHALLENGE = 73,
    TOUP_CMD_CRYPT_RESPONSE = 105,
};

int FreeToup_EnumV2(FreeToupDeviceV2 cams[FREETOUP_MAX])
{
    int rc;
    libusb_device **list;
    ssize_t cnt;
    ssize_t i;
    struct libusb_device_descriptor desc;
    struct ft_camera *cam;
    int found = 0;

    rc = libusb_init(NULL);
    if (rc) {
        return rc;
    }

    cnt = libusb_get_device_list(NULL, &list);
    if (cnt < 0) {
        return cnt;
    }

    for (i = 0; (i < cnt) && (found < FREETOUP_MAX); i++) {
        libusb_device *device = list[i];

        rc = libusb_get_device_descriptor(device, &desc);
        if (rc) {
            continue;
        }
        cam = ft_camera_get_by_vid_pid(desc.idVendor, desc.idProduct);
        if (cam) {
            strncpy(cams[found].displayname, cam->name, sizeof(cams[found].displayname));
            /* FIXME: Figure out what this should be */
            snprintf(cams[found].id, sizeof(cams[found].id), "ft-%u-%u",
                libusb_get_bus_number(device), libusb_get_device_address(device));
            found++;
        }
    }

    return found;
}

/* The touptek cameras implement a vendor defined interface that consist of two bulk endpoints */
#define TOUPTEK_USB_INTERFACE_NUMBER 0

#define TOUPCAM_CONTROL_WRITE (LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE)
#define TOUPCAM_CONTROL_READ (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE)
#define TOUPCAM_DEFAULT_CONTROL_TIMEOUT_MS 500
#define TOUPCAM_STATUS_SUCCESS 0x08

/**
 * The touptek cameras scramble the data of certain commands. It seems this was
 * in order to prevent reverse engineering (LOL). The key is set with this
 * command. I couldn't bring myself to call this encryption because it's a
 * bitshift and XOR. There's no reason for use to set the key to anything other
 * than 0, which effectively disables the scrambling.
 */
static int ft_set_scramble_key(HFreeToup ft, uint16_t key)
{
    int rc;
    uint8_t rsp[2];
    rc = libusb_control_transfer(ft->handle, TOUPCAM_CONTROL_WRITE,
            TOUP_CMD_SCRAMBLE_KEY, key, 0, rsp, 2, TOUPCAM_DEFAULT_CONTROL_TIMEOUT_MS);
    if (rc != 1) {
        return -EIO;
    }
    if (rsp[0] != TOUPCAM_STATUS_SUCCESS) {
        return -1;
    }

    ft->key = key;
    return 0;
}

HFreeToup FreeToup_Open(const char *id)
{
    // discover devices
    libusb_device **list;
    libusb_device *found = NULL;
    ssize_t cnt;
    ssize_t i = 0;
    int bus;
    int addr;
    int rc;
    int claimed = 0;
    HFreeToup ft = NULL;

    rc = sscanf(id, "ft-%d-%d", &bus, &addr);
    if (rc != 2) {
        fprintf(stderr, "sscanf failed %d %s\n", rc, id);
        return NULL;
    }

    cnt = libusb_get_device_list(NULL, &list);
    if (cnt < 0) {
        return NULL;
    }
    for (i = 0; i < cnt; i++) {
        libusb_device *device = list[i];
        if ((libusb_get_bus_number(device) == bus) &&
            (libusb_get_device_address(device) == addr)) {

            found = device;
            break;
        }
    }
    if (!found) {
        fprintf(stderr, "not found\n");
        goto fail;
    }
    ft = malloc(sizeof(*ft));
    if (!ft)
        goto done;

    memset(ft, 0 , sizeof(*ft));
    rc = libusb_open(found, &ft->handle);
    if (rc)
        goto fail;

    rc = libusb_claim_interface(ft->handle, TOUPTEK_USB_INTERFACE_NUMBER);
    if (rc != LIBUSB_SUCCESS)
        goto fail;

    rc = ft_set_scramble_key(ft, 0);
    if (rc) {
        goto fail;
    }
done:
    libusb_free_device_list(list, 1);

    return ft;

fail:
    libusb_free_device_list(list, 1);
    if (claimed) {
        libusb_release_interface(ft->handle, TOUPTEK_USB_INTERFACE_NUMBER);
    }
    if (ft) {
        if (ft->handle) {
            libusb_close(ft->handle);
        }
        free(ft);
        ft = NULL;
    }
    return NULL;
}

void FreeToup_Close(HFreeToup ft)
{
    if (!ft) {
        return;
    }
    if (ft->handle) {
        libusb_release_interface(ft->handle, TOUPTEK_USB_INTERFACE_NUMBER);
        libusb_close(ft->handle);
    }
    free(ft);
}

const char *FreeToup_Version(void)
{
    return PACKAGE_VERSION;
}

/**
 * Serial number format is as follows:
 * TPYYMMDD.*
 */
int FreeToup_get_SerialNumber(HFreeToup ft, char *sn)
{
    if (ft->sn[0]) {
        memset(sn, 0, 32);
    }
    return 0;
}

int FreeToup_get_FwVersion(HFreeToup ft, char *fwver)
{
    int rc;
    rc = libusb_control_transfer(ft->handle, TOUPCAM_CONTROL_WRITE,
            TOUP_CMD_FW_VER, 0, 0, (uint8_t *) fwver, 16, TOUPCAM_DEFAULT_CONTROL_TIMEOUT_MS);
    if (rc < 0) {
        return rc;
    }

    return 0;
}

int FreeToup_get_HwVersion(HFreeToup ft, char *hwver)
{
    int rc;
    rc = libusb_control_transfer(ft->handle, TOUPCAM_CONTROL_WRITE,
            TOUP_CMD_HW_VER, 0, 0, (uint8_t *) hwver, 16, TOUPCAM_DEFAULT_CONTROL_TIMEOUT_MS);
    if (rc < 0) {
        return rc;
    }

    return 0;
}

int FreeToup_get_FpgaVersion(HFreeToup ft, char *fpgaver)
{
    *fpgaver = 0;
    /* There is no USB traffic for this command. Maybe it's encoded in the fw somewhere? */
    return 0;
}

int FreeToup_get_ProductionDate(HFreeToup ft, char *date)
{
    int rc;
    rc = libusb_control_transfer(ft->handle, TOUPCAM_CONTROL_WRITE,
            TOUP_CMD_HW_VER, 0, 0, (uint8_t *) date, 10, TOUPCAM_DEFAULT_CONTROL_TIMEOUT_MS);
    if (rc < 0) {
        return rc;
    }

    return 0;
}

int FreeToup_get_Revision(HFreeToup ft, uint16_t *revision)
{
    int rc;
    uint16_t val = 0;
    /* There is no USB traffic for this command. It's probably in the EEPROM data */
    *revision = le16toh(val);
    return 0;
}
