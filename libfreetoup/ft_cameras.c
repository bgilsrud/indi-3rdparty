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

#include "ft_cameras.h"
#include "freetoup.h"
#include <stddef.h>


FreeToupModelV2 ar0130m_model = {
    .flag = (FREETOUP_FLAG_CMOS | FREETOUP_FLAG_ROI_HARDWARE | FREETOUP_FLAG_MONO |
             FREETOUP_FLAG_ST4 | FREETOUP_FLAG_RAW12 | FREETOUP_FLAG_TRIGGER_SOFTWARE |
             FREETOUP_FLAG_TRIGGER_SINGLE | FREETOUP_FLAG_BLACKLEVEL | FREETOUP_FLAG_RAW8),
    .maxspeed = 0,
    .preview = 1,
    .still= 1,
    .maxfanspeed = 1,
    .xpixsz = 3.75,
    .ypixsz = 3.75
};

static const struct ft_camera supported_cameras[] = {
    {
        .name = "Meade LPI-GM",
        .vendor_id = 0x0549,
        .product_id = 0xe004,
        .sensor = NULL,
        .model = &ar0130m_model
    },
    {0}
};

const struct ft_camera *ft_camera_get_by_vid_pid(uint16_t vid, uint16_t pid)
{
    const struct ft_camera *cam;

    for (cam = supported_cameras; cam->name; cam++) {
        if (vid == cam->vendor_id && pid == cam->product_id) {
            return cam;
        }
    }

    return NULL;
}
