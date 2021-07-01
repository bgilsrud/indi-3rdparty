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

#include <errno.h>
#include <libusb-1.0/libusb.h>
#include <string.h>
#include "freetoup.h"
#include "ft_cameras.h"

int freetoup_EnumV2(FreeToupDeviceV2 cams[FREETOUP_MAX_CAMERAS])
{
    int rc;
    libusb_device **list;
    libusb_device *ft_device;
    ssize_t cnt;
    ssize_t i;
    struct libusb_device_descriptor desc;
    struct ft_camera *cam;
    int found = 0;

    rc = libusb_init(NULL);
    if (rc) {
        return -ENOTTY;
    }

    cnt = libusb_get_device_list(NULL, &list);
    if (cnt < 0) {
        return libusb_error_to_posix(cnt);
    }

    ft_device = NULL;
    for (i = 0; (i < cnt) && (found < FREETOUP_MAX_CAMERAS); i++) {
        libusb_device *device = list[i];

        rc = libusb_get_device_descriptor(device, &desc);
        if (rc) {
            continue;
        }
        cam = ft_camera_get_by_vid_pid(desc.idVendor, desc.idProduct);
        if (cam) {
            strncpy(cams[found].displayname, cam->name, sizeof(cams[found].displayname));
            /* FIXME: Figure out what this should be */
            strncpy(cams[found].id, cam->name, sizeof(cams[found].id));
            found++;
        }
    }

    return found;
}
