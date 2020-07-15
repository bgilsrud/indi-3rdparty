/**
 * Orion StarShoot G3 driver
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

#ifndef ORION_SSG3_H
#define ORION_SSG3_H
#include <stdint.h>
#include <sys/time.h>
#include <libusb-1.0/libusb.h>

struct orion_ssg3 {
    libusb_device_handle *devh;
    uint8_t gain;
    uint8_t offset;
    uint16_t bin_x;
    uint16_t bin_y;
    uint16_t x1;
    uint16_t x_count;
    uint16_t y1;
    uint16_t y_count;
    struct timeval exp_done_time;
};

int orion_ssg3_open(struct orion_ssg3 *ssg3);
int orion_ssg3_close(struct orion_ssg3 *ssg3);

int orion_ssg3_set_cooling(struct orion_ssg3 *ssg3, int on);
int orion_ssg3_set_gain(struct orion_ssg3 *ssg3, uint8_t gain);
int orion_ssg3_set_offset(struct orion_ssg3 *ssg3, uint8_t offset);
int orion_ssg3_set_binning(struct orion_ssg3 *ssg3, uint8_t x, uint8_t y);
int orion_ssg3_subframe(struct orion_ssg3 *ssg3, uint16_t x1, uint16_t x_count,
        uint16_t y1, uint16_t y_count);
int orion_ssg3_start_exposure(struct orion_ssg3 *ssg3, uint32_t msec);
int orion_ssg3_image_download(struct orion_ssg3 *ssg3, uint8_t *buf, int len);

int orion_ssg3_get_gain(struct orion_ssg3 *ssg3, uint8_t *gain);
int orion_ssg3_get_offset(struct orion_ssg3 *ssg3, uint8_t *offset);
int orion_ssg3_get_image_width(struct orion_ssg3 *ssg3);
int orion_ssg3_get_image_height(struct orion_ssg3 *ssg3);
int orion_ssg3_get_pixel_bit_size(struct orion_ssg3 *ssg3);
int orion_ssg3_get_pixel_size_x(struct orion_ssg3 *ssg3);
int orion_ssg3_get_pixel_size_y(struct orion_ssg3 *ssg3);
int orion_ssg3_exposure_done(struct orion_ssg3 *ssg3);
#endif /* ORION_SSG3_H */
