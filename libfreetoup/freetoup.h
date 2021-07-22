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

#define FREETOUP_FLAG_BINSKIP_SUPPORTED (1 << 0)
#define FREETOUP_FLAG_CGHDR (1 << 1)
#define FREETOUP_FLAG_CG (1 << 2)
#define FREETOUP_FLAG_FAN (1 << 3)
#define FREETOUP_FLAG_GETTEMPERATURE (1 << 4)
#define FREETOUP_FLAG_MONO (1 << 5)
#define FREETOUP_FLAG_RAW10 (1 << 6)
#define FREETOUP_FLAG_RAW12 (1 << 7)
#define FREETOUP_FLAG_RAW14 (1 << 8)
#define FREETOUP_FLAG_RAW16 (1 << 9)
#define FREETOUP_FLAG_ROI_HARDWARE (1 << 10)
#define FREETOUP_FLAG_ST4 (1 << 11)
#define FREETOUP_FLAG_TEC_ONOFF (1 << 12)

#define FREETOUP_BLACKLEVEL8_MAX 0

#define FREETOUP_EVENT_BLACK 0
#define FREETOUP_EVENT_DFC 1
#define FREETOUP_EVENT_DISCONNECTED 2
#define FREETOUP_EVENT_ERROR 3
#define FREETOUP_EVENT_EXPOSURE 4
#define FREETOUP_EVENT_FACTORY 5
#define FREETOUP_EVENT_FFC 6
#define FREETOUP_EVENT_IMAGE 7
#define FREETOUP_EVENT_NOFRAMETIMEOUT 8
#define FREETOUP_EVENT_STILLIMAGE 9
#define FREETOUP_EVENT_TEMPTINT 10
#define FREETOUP_EVENT_TRIGGERFAIL 11
#define FREETOUP_EVENT_WBGAIN 12

#define FREETOUP_OPTION_BINNING 0
#define FREETOUP_OPTION_BITDEPTH 1
#define FREETOUP_OPTION_BLACKLEVEL 2
#define FREETOUP_OPTION_CG 3
#define FREETOUP_OPTION_FAN 4
#define FREETOUP_OPTION_FRAMERATE 5
#define FREETOUP_OPTION_NOFRAME_TIMEOUT 6
#define FREETOUP_OPTION_RAW 7
#define FREETOUP_OPTION_RGB 8
#define FREETOUP_OPTION_TEC 9
#define FREETOUP_OPTION_TRIGGER 10

typedef struct ft_resolution {
    uint32_t width;
    uint32_t height;
} FreeToupResolution;

typedef struct ft_model_v2 {
    uint32_t flag;
    int maxspeed;
    int preview;
    int still;
    uint32_t maxfanspeed;
    float xpixsz;
    float ypixsz;
    FreeToupResolution res[FREETOUP_MAX];
} FreeToupModelV2;

typedef struct {
    char displayname[64];
    char id[64];
    FreeToupModelV2 *model;
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
typedef void (* PITOUPCAM_TEMPTINT_CALLBACK)(const int nTemp, const int nTint, void* pCtx);   /* once white balance, Temp/Tint Mode */



typedef int HRESULT;

#define SUCCEEDED(x) (x >= 0)
#define FAILED(x) (x < 0)

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
int FreeToup_get_ExpoAGainRange(HFreeToup h, unsigned short* nMin, unsigned short* nMax, unsigned short* nDef);
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
int FreeToup_get_WhiteBalanceGain(HFreeToup h, int aGain[3]);

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
int FreeToup_put_TempTintInit(HFreeToup h, PITOUPCAM_TEMPTINT_CALLBACK fnTTProc, void* pTTCtx);
int FreeToup_put_TempTint(HFreeToup h, int nTemp, int nTint);
int FreeToup_AwbOnce(HFreeToup h, PITOUPCAM_TEMPTINT_CALLBACK fnTTProc, void* pTTCtx);



#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* FREETOUP_H */
