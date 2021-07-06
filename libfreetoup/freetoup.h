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

/**
 * Get list of supported cameras that are found
 * @param list: Array of devices that are found
 * @return: -errno on failure, the number of cameras found on success
 */
int FreeToup_EnumV2(FreeToupDeviceV2 list[FREETOUP_MAX]);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* FREETOUP_H */
