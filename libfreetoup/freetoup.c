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
    TOUP_CMD_ST4_NORTH = 130,
    TOUP_CMD_ST4_SOUTH = 131,
    TOUP_CMD_ST4_EAST = 132,
    TOUP_CMD_ST4_WEST = 133,
    TOUP_CMD_ST4_STOP = 134,
};

int FreeToup_EnumV2(FreeToupDeviceV2 cams[FREETOUP_MAX])
{
    int rc;
    libusb_device **list;
    ssize_t cnt;
    ssize_t i;
    struct libusb_device_descriptor desc;
    const struct ft_camera *cam;
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
            strncpy(cams[found].displayname, cam->name, sizeof(cams[found].displayname) - 1);
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
    if (rc)
        goto fail;

    rc = FreeToup_get_SerialNumber(ft, NULL);
    if (rc)
        goto fail;
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
    uint32_t eeprom_len;
    int rc;

    if (ft->sn[0]) {
        if (sn)
            strcpy(sn, ft->sn);
        return 0;
    }
    rc = libusb_control_transfer(ft->handle, TOUPCAM_CONTROL_WRITE,
            TOUP_CMD_READ_EEPROM, 0, 0, (uint8_t *) &eeprom_len, 4, TOUPCAM_DEFAULT_CONTROL_TIMEOUT_MS);
    if (rc != 4) {
        return -EIO;
    }
    eeprom_len = le32toh(eeprom_len);
    rc = libusb_control_transfer(ft->handle, TOUPCAM_CONTROL_WRITE,
            TOUP_CMD_READ_EEPROM, eeprom_len, 0, (uint8_t *) ft->sn, 32, TOUPCAM_DEFAULT_CONTROL_TIMEOUT_MS);
    if (rc < 0) {
        return -EIO;
    }

    if (sn)
        strncpy(sn, ft->sn, 32);

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
    strncpy(date, ft->sn, 8);
    date[8] = 0;
    date[0] = '2';
    date[1] = '0';
    return 0;
}

int FreeToup_get_Revision(HFreeToup ft, uint16_t *revision)
{
    uint16_t val = 0;
    /* There is no USB traffic for this command. It's probably in the EEPROM data */
    *revision = le16toh(val);
    return 0;
}

int FreeToup_Flush(HFreeToup ft)
{
    /* TODO */
    return 0;
}


int FreeToup_ST4PlusGuide(HFreeToup ft, int dir, int ms)
{
    uint16_t wValue = ms / 30;
    uint16_t wIndex= ms % 30;
    int rc;

    if (dir < 0 || dir > 4)
        return -1;

    rc = libusb_control_transfer(ft->handle, TOUPCAM_CONTROL_WRITE,
            TOUP_CMD_ST4_NORTH + dir, wValue, wIndex, NULL, 0, TOUPCAM_DEFAULT_CONTROL_TIMEOUT_MS);
    if (rc < 0) {
        return rc;
    }
    return 0;
}

int FreeToup_AbbOnce(HFreeToup ft, PITOUPCAM_BLACKBALANCE_CALLBACK fnBBProc, void* pBBCtx)
{
    /* TODO */
    return 0;
}

int FreeToup_AwbInit(HFreeToup h, PITOUPCAM_WHITEBALANCE_CALLBACK fnWBProc, void* pWBCtx)
{
    /* TODO */
    return 0;
}
int FreeToup_get_AutoExpoEnable(HFreeToup h, int* bAutoExposure)
{
    /* TODO */
    return 0;
}
int FreeToup_get_BlackBalance(HFreeToup h, unsigned short aSub[3])
{
    /* TODO */
    return 0;
}
int FreeToup_get_Brightness(HFreeToup h, int* Brightness)
{
    /* TODO */
    return 0;
}
int FreeToup_get_Contrast(HFreeToup h, int* Contrast)
{
    /* TODO */
    return 0;
}
int FreeToup_get_eSize(HFreeToup h, unsigned* pnResolutionIndex)
{
    /* TODO */
    return 0;
}
int FreeToup_get_ExpoAGain(HFreeToup h, unsigned short* AGain)
{
    /* TODO */
    return 0;
}
int FreeToup_get_ExpTimeRange(HFreeToup h, unsigned* nMin, unsigned* nMax, unsigned* nDef)
{
    /* TODO */
    return 0;
}
int FreeToup_get_Gamma(HFreeToup h, int* Gamma)
{
    /* TODO */
    return 0;
}
int FreeToup_get_Hue(HFreeToup h, int* Hue)
{
    /* TODO */
    return 0;
}
int FreeToup_get_LevelRange(HFreeToup h, unsigned short aLow[4], unsigned short aHigh[4])
{
    /* TODO */
    return 0;
}
int FreeToup_get_MaxBitDepth(HFreeToup h)
{
    /* TODO */
    return 0;
}
int FreeToup_get_Option(HFreeToup h, unsigned iOption, int* piValue)
{
    /* TODO */
    return 0;
}
int FreeToup_get_RawFormat(HFreeToup h, unsigned* nFourCC, unsigned* bitsperpixel)
{
    /* TODO */
    return 0;
}
int FreeToup_get_ResolutionNumber(HFreeToup h)
{
    /* TODO */
    return 0;
}
int FreeToup_get_Resolution(HFreeToup h, unsigned nResolutionIndex, int* pWidth, int* pHeight)
{
    /* TODO */
    return 0;
}
int FreeToup_get_Saturation(HFreeToup h, int* Saturation)
{
    /* TODO */
    return 0;
}
int FreeToup_get_Speed(HFreeToup h, unsigned short* pSpeed)
{
    /* TODO */
    return 0;
}
int FreeToup_get_Temperature(HFreeToup h, short* pTemperature)
{
    /* TODO */
    return 0;
}
int FreeToup_put_WhiteBalanceGain(HFreeToup h, int aGain[3])
{
    /* TODO */
    return 0;
}
int FreeToup_PullImageV2(HFreeToup h, void* pImageData, int bits, FreeToupFrameInfoV2* pInfo)
{
    /* TODO */
    return 0;
}
int FreeToup_PullStillImageV2(HFreeToup h, void* pImageData, int bits, FreeToupFrameInfoV2* pInfo)
{
    /* TODO */
    return 0;
}
int FreeToup_put_AutoExpoEnable(HFreeToup h, int bAutoExposure)
{
    /* TODO */
    return 0;
}
int FreeToup_put_BlackBalance(HFreeToup h, unsigned short aSub[3])
{
    /* TODO */
    return 0;
}
int FreeToup_put_Brightness(HFreeToup h, int Brightness)
{
    /* TODO */
    return 0;
}
int FreeToup_put_Contrast(HFreeToup h, int Contrast)
{
    /* TODO */
    return 0;
}
int FreeToup_put_eSize(HFreeToup h, unsigned nResolutionIndex)
{
    /* TODO */
    return 0;
}
int FreeToup_put_ExpoAGain(HFreeToup h, unsigned short AGain)
{
    /* TODO */
    return 0;
}
int FreeToup_put_ExpoTime(HFreeToup h, unsigned Time)
{
    /* TODO */
    return 0;
}
int FreeToup_put_Gamma(HFreeToup h, int Gamma)
{
    /* TODO */
    return 0;
}
int FreeToup_put_Hue(HFreeToup h, int Hue)
{
    /* TODO */
    return 0;
}
int FreeToup_put_LevelRange(HFreeToup h, unsigned short aLow[4], unsigned short aHigh[4])
{
    /* TODO */
    return 0;
}
int FreeToup_put_Mode(HFreeToup h, int bSkip)
{
    /* TODO */
    return 0;
}
int FreeToup_put_Option(HFreeToup h, unsigned iOption, int iValue)
{
    /* TODO */
    return 0;
}
int FreeToup_put_RealTime(HFreeToup h, int val)
{
    /* TODO */
    return 0;
}
int FreeToup_put_Roi(HFreeToup h, unsigned xOffset, unsigned yOffset, unsigned xWidth, unsigned yHeight)
{
    /* TODO */
    return 0;
}
int FreeToup_put_Saturation(HFreeToup h, int Saturation)
{
    /* TODO */
    return 0;
}
int FreeToup_put_Speed(HFreeToup h, unsigned short nSpeed)
{
    /* TODO */
    return 0;
}
int FreeToup_put_Temperature(HFreeToup h, short nTemperature)
{
    /* TODO */
    return 0;
}
int FreeToup_Snap(HFreeToup h, unsigned nResolutionIndex)
{
    /* TODO */
    return 0;
}
int FreeToup_StartPullModeWithCallback(HFreeToup h, PTOUPCAM_EVENT_CALLBACK pEventCallback, void* pCallbackContext)
{
    /* TODO */
    return 0;
}
int FreeToup_StartPushModeV3(HFreeToup h, PTOUPCAM_DATA_CALLBACK_V3 pDataCallback, void* pDataCallbackCtx, PTOUPCAM_EVENT_CALLBACK pEventCallback, void* pEventCallbackContext)
{
    /* TODO */
    return 0;
}
int FreeToup_Stop(HFreeToup h)
{
    /* TODO */
    return 0;
}
int FreeToup_Trigger(HFreeToup h, unsigned short nNumber)
{
    /* TODO */
    return 0;
}

int FreeToup_get_ExpoAGainRange(HFreeToup h, unsigned short* nMin, unsigned short* nMax, unsigned short* nDef)
{
    /* TODO */
    return 0;
}

int FreeToup_get_WhiteBalanceGain(HFreeToup h, int aGain[3])
{
    /* TODO */
    return 0;
}

int FreeToup_put_TempTintInit(HFreeToup h, PITOUPCAM_TEMPTINT_CALLBACK fnTTProc, void* pTTCtx)
{
    /* TODO */
    return 0;
}
int FreeToup_put_TempTint(HFreeToup h, int nTemp, int nTint)
{
    /* TODO */
    return 0;
}
int FreeToup_AwbOnce(HFreeToup h, PITOUPCAM_TEMPTINT_CALLBACK fnTTProc, void* pTTCtx)
{
    /* TODO */
    return 0;
}
