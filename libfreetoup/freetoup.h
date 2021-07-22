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

typedef struct ft_frame_info {
    int width;
    int height;
    int flag;
    int timestamp;
} FreeToupFrameInfoV2;

typedef void (* PITOUPCAM_BLACKBALANCE_CALLBACK)(const unsigned short aSub[3], void* pCtx);
typedef void (* PITOUPCAM_WHITEBALANCE_CALLBACK)(const int aGain[3], void* pCtx);
typedef void (* PTOUPCAM_EVENT_CALLBACK)(unsigned nEvent, void* pCallbackCtx);
typedef void (* PTOUPCAM_DATA_CALLBACK_V3)(const void* pData, const FreeToupFrameInfoV2* pInfo, int bSnap, void* pCallbackCtx);




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

int FreeToup_Flush(HFreeToup ft);

int FreeToup_get_AutoExpoEnable(HFreeToup ft, int *aee);

int FreeToup_ST4PlusGuide(HFreeToup ft, int dir, int ms);

int FreeToup_AbbOnce(HFreeToup ft, PITOUPCAM_BLACKBALANCE_CALLBACK fnBBProc, void* pBBCtx); /* auto black balance "once". This function must be called AFTER Toupcam_StartXXXX */
int FreeToup_AwbInit(HFreeToup h, PITOUPCAM_WHITEBALANCE_CALLBACK fnWBProc, void* pWBCtx);
int FreeToup_get_BlackBalance(HFreeToup h, unsigned short aSub[3]);
int FreeToup_get_Brightness(HFreeToup h, int* Brightness);
int FreeToup_get_Contrast(HFreeToup h, int* Contrast);
int FreeToup_get_eSize(HFreeToup h, unsigned* pnResolutionIndex);
int FreeToup_get_ExpoAGain(HFreeToup h, unsigned short* AGain); /* percent, such as 300 */
int FreeToup_get_ExpTimeRange(HFreeToup h, unsigned* nMin, unsigned* nMax, unsigned* nDef);
int FreeToup_get_Gamma(HFreeToup h, int* Gamma); /* percent */
int FreeToup_get_Hue(HFreeToup h, int* Hue);
int FreeToup_get_LevelRange(HFreeToup h, unsigned short aLow[4], unsigned short aHigh[4]);
int FreeToup_get_MaxBitDepth(HFreeToup h); /* get the max bit depth of this camera, such as 8, 10, 12, 14, 16 */
int FreeToup_get_Option(HFreeToup h, unsigned iOption, int* piValue);
int FreeToup_get_RawFormat(HFreeToup h, unsigned* nFourCC, unsigned* bitsperpixel);
int FreeToup_get_ResolutionNumber(HFreeToup h);
int FreeToup_get_Resolution(HFreeToup h, unsigned nResolutionIndex, int* pWidth, int* pHeight);
int FreeToup_get_Saturation(HFreeToup h, int* Saturation);
int FreeToup_get_Speed(HFreeToup h, unsigned short* pSpeed);
int FreeToup_get_Temperature(HFreeToup h, short* pTemperature);
int FreeToup_put_WhiteBalanceGain(HFreeToup h, int aGain[3]);
int FreeToup_PullImageV2(HFreeToup h, void* pImageData, int bits, FreeToupFrameInfoV2* pInfo);
int FreeToup_PullStillImageV2(HFreeToup h, void* pImageData, int bits, FreeToupFrameInfoV2* pInfo);
int FreeToup_put_AutoExpoEnable(HFreeToup h, int bAutoExposure);
int FreeToup_put_BlackBalance(HFreeToup h, unsigned short aSub[3]);
int FreeToup_put_Brightness(HFreeToup h, int Brightness);
int FreeToup_put_Contrast(HFreeToup h, int Contrast);
int FreeToup_put_eSize(HFreeToup h, unsigned nResolutionIndex);
int FreeToup_put_ExpoAGain(HFreeToup h, unsigned short AGain); /* percent */
int FreeToup_put_ExpoTime(HFreeToup h, unsigned Time); /* in microseconds */
int FreeToup_put_Gamma(HFreeToup h, int Gamma);  /* percent */
int FreeToup_put_Hue(HFreeToup h, int Hue);
int FreeToup_put_LevelRange(HFreeToup h, unsigned short aLow[4], unsigned short aHigh[4]);
int FreeToup_put_Mode(HFreeToup h, int bSkip); /* skip or bin */
int FreeToup_put_Option(HFreeToup h, unsigned iOption, int iValue);
int FreeToup_put_RealTime(HFreeToup h, int val);
int FreeToup_put_Roi(HFreeToup h, unsigned xOffset, unsigned yOffset, unsigned xWidth, unsigned yHeight);
int FreeToup_put_Saturation(HFreeToup h, int Saturation);
int FreeToup_put_Speed(HFreeToup h, unsigned short nSpeed);
int FreeToup_put_Temperature(HFreeToup h, short nTemperature);
int FreeToup_put_WhiteBalanceGain(HFreeToup h, int aGain[3]);
int FreeToup_Snap(HFreeToup h, unsigned nResolutionIndex);  /* still image snap */
int FreeToup_StartPullModeWithCallback(HFreeToup h, PTOUPCAM_EVENT_CALLBACK pEventCallback, void* pCallbackContext);
int FreeToup_StartPushModeV3(HFreeToup h, PTOUPCAM_DATA_CALLBACK_V3 pDataCallback, void* pDataCallbackCtx, PTOUPCAM_EVENT_CALLBACK pEventCallback, void* pEventCallbackContext);
int FreeToup_Stop(HFreeToup h);
int FreeToup_Trigger(HFreeToup h, unsigned short nNumber);


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* FREETOUP_H */
