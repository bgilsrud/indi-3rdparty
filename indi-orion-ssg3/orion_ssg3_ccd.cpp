/**
 * Copyright (C) 2020 Ben Gilsrud
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

#include <iostream>
#include <math.h>
#include <unistd.h>
#include <inttypes.h>
#include "orion_ssg3_ccd.h"
#include "orion_ssg3.h"
#include "config.h"

std::unique_ptr<SSG3CCD> ssg3CCD(new SSG3CCD());

void ISGetProperties(const char *dev)
{
    ssg3CCD->ISGetProperties(dev);
}

void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num)
{
    ssg3CCD->ISNewSwitch(dev, name, states, names, num);
}

void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int num)
{
    ssg3CCD->ISNewText(dev, name, texts, names, num);
}

void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num)
{
    ssg3CCD->ISNewNumber(dev, name, values, names, num);
}

void ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[],
               char *names[], int n)
{
    INDI_UNUSED(dev);
    INDI_UNUSED(name);
    INDI_UNUSED(sizes);
    INDI_UNUSED(blobsizes);
    INDI_UNUSED(blobs);
    INDI_UNUSED(formats);
    INDI_UNUSED(names);
    INDI_UNUSED(n);
}

void ISSnoopDevice(XMLEle *root)
{
    ssg3CCD->ISSnoopDevice(root);
}

SSG3CCD::SSG3CCD()
{
    InExposure = false;
    capturing  = false;

    setVersion(ORION_SSG3_VERSION_MAJOR, ORION_SSG3_VERSION_MINOR);
}

/*******************************************************************************
** Client is asking us to establish connection to the device
*******************************************************************************/
bool SSG3CCD::Connect()
{
    uint32_t cap = 0;
    std::string ccd;
    int rc;

    rc = orion_ssg3_open(&ssg3);
    /* DEBUG!! */
    rc = 0;
    if (rc) {
        LOGF_INFO("Unable to find Orion StartShoot G3: %s\n", strerror(-rc));
        return false;
    }

    cap |= CCD_CAN_BIN;
    cap |= CCD_CAN_SUBFRAME;
    cap |= CCD_HAS_COOLER;
    /* FIXME once we know how to drive guide port:
    cap |= CCD_HAS_ST4_PORT; */
    /* FIXME once we know how to identify color vs mono:
    cap |= CCD_HAS_BAYER; */
    /* FIXME once we know how to abort exposures
    cap |= CCD_CAN_ABORT; */

    SetCCDCapability(cap);

    return true;
}

/**
 * Client is asking us to terminate connection to the device
 */
bool SSG3CCD::Disconnect()
{
    orion_ssg3_close(&ssg3);
    LOG_INFO("Successfully disconnected!");
    return true;
}

/**
 * INDI is asking us for our default device name
 */
const char *SSG3CCD::getDefaultName()
{
    return "SSG3";
}

/**
 * INDI is asking us to init our properties.
 */
bool SSG3CCD::initProperties()
{
    uint8_t gain;
    uint8_t offset;

    /* Must init parent properties first */
    INDI::CCD::initProperties();

    // Add Debug Control.
    addDebugControl();

    /* Add Gain number property */
    orion_ssg3_get_gain(&ssg3, &gain);
    IUFillNumber(GainN, "GAIN", "Gain", "%hhu", 0, 255, 1, gain);
    IUFillNumberVector(&GainNP, GainN, 1, getDeviceName(), "CCD_GAIN", "Gain", IMAGE_SETTINGS_TAB, IP_RW, 0, IPS_IDLE);

    /* Add Offset number property */
    orion_ssg3_get_offset(&ssg3, &offset);
    IUFillNumber(OffsetN, "OFFSET", "Offset", "%hhu", 0, 255, 1, offset);
    IUFillNumberVector(&OffsetNP, OffsetN, 1, getDeviceName(), "CCD_OFFSET", "Offset", IMAGE_SETTINGS_TAB, IP_RW, 0,
                       IPS_IDLE);

    /* Add Temp number property */
    IUFillNumber(CCDTempN, "CCDTEMP", "CCD Temperature [°C]", "%.1f", -128.5, 128.5, 0.1, -128.5);
    IUFillNumberVector(&CCDTempNP, CCDTempN, 1, getDeviceName(), "CCDTemp", "CCD Temp", IMAGE_INFO_TAB, IP_RO, 0,
                       IPS_IDLE);

    PrimaryCCD.setMinMaxStep("CCD_EXPOSURE", "CCD_EXPOSURE_VALUE", 0.001, 3600, .001, false);

    setDefaultPollingPeriod(250);

    return true;
}

/*******************************************************************************
** INDI is asking us to update the properties because there is a change in
** CONNECTION status. This fucntion is called whenever the device is
** connected or disconnected.
*******************************************************************************/
bool SSG3CCD::updateProperties()
{
    // Call parent update properties first
    INDI::CCD::updateProperties();

    if (isConnected())
    {
        // Let's get parameters now from CCD
        setupParams();

        // Start the timer
        SetTimer(POLLMS);
        defineNumber(&GainNP);
        defineNumber(&OffsetNP);
        defineNumber(&CCDTempNP);
    }
    else
    {
        deleteProperty(GainNP.name);
        deleteProperty(OffsetNP.name);
        deleteProperty(CCDTempNP.name);
    }

    return true;
}

/*******************************************************************************
** Setting up CCD parameters
*******************************************************************************/
void SSG3CCD::setupParams()
{
    SetCCDParams(orion_ssg3_get_image_width(&ssg3),
                 orion_ssg3_get_image_height(&ssg3),
                 orion_ssg3_get_pixel_bit_size(&ssg3),
                 orion_ssg3_get_pixel_size_x(&ssg3),
                 orion_ssg3_get_pixel_size_y(&ssg3));

    /* calculate how much memory we need for the primary CCD buffer */
    PrimaryCCD.setFrameBufferSize(PrimaryCCD.getXRes() * PrimaryCCD.getYRes() * PrimaryCCD.getBPP() / 8);
}

/*******************************************************************************
** Set binning
*******************************************************************************/
bool SSG3CCD::UpdateCCDBin(int x, int y)
{
    int rc;

    rc = orion_ssg3_set_binning(&ssg3, x, y);
    if (!rc) {
        PrimaryCCD.setBin(x, y);
        return true;
    }
    return false;
}

/**
 * Client is asking to start exposure
 * @param duration: The requested exposure duration, in seconds
 * @return: true if exposure started successfully, false otherwise
 */
bool SSG3CCD::StartExposure(float duration)
{
    int rc;

    ExposureRequest = duration;

    rc = orion_ssg3_start_exposure(&ssg3, duration * 1000);
    if (rc) {
        return false;
    }
    PrimaryCCD.setExposureDuration(duration);

    gettimeofday(&ExpStart, nullptr);

    InExposure = true;
    LOG_INFO("Exposure has begun.");

    return true;
}

/**
 * Client is asking us to abort an exposure
 */
bool SSG3CCD::AbortExposure()
{
    /* FIXME: Find out if there's a command to send to the SSG3 to abort the exposure */
    InExposure = false;
    return true;
}

/**
 * How much longer until exposure is done?
 */
float SSG3CCD::CalcTimeLeft()
{
    double timesince;
    double timeleft;
    struct timeval now;
    gettimeofday(&now, nullptr);

    timesince = (double)(now.tv_sec * 1000.0 + now.tv_usec / 1000) -
                (double)(ExpStart.tv_sec * 1000.0 + ExpStart.tv_usec / 1000);
    timesince = timesince / 1000;

    timeleft = ExposureRequest - timesince;
    return timeleft;
}

/**
 * Client is asking us to set a new number
 */
bool SSG3CCD::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (!strcmp(dev, getDeviceName()))
    {
        if (!strcmp(name, GainNP.name))
        {
            IUUpdateNumber(&GainNP, values, names, n);
            GainNP.s = IPS_OK;
            IDSetNumber(&GainNP, nullptr);

            return true;
        }

        if (!strcmp(name, OffsetNP.name))
        {
            IUUpdateNumber(&OffsetNP, values, names, n);
            OffsetNP.s = IPS_OK;
            IDSetNumber(&OffsetNP, nullptr);

            return true;
        }
    }

    // If we didn't process anything above, let the parent handle it.
    return INDI::CCD::ISNewNumber(dev, name, values, names, n);
}

/**
 * Client is asking us to set a new switch
 */
bool SSG3CCD::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    return INDI::CCD::ISNewSwitch(dev, name, states, names, n);
}

/**
 * Main device loop. We check for exposure progress
 */
void SSG3CCD::TimerHit()
{
    long timeleft;

    if (isConnected() == false) {
        return;
    }

    if (InExposure) {
        timeleft = CalcTimeLeft();

        if (orion_ssg3_exposure_done(&ssg3)) {
            /* We're done exposing */
            LOG_INFO("Exposure done, downloading image...");
            PrimaryCCD.setExposureLeft(0);
            InExposure = false;
            grabImage();
        } else {
            PrimaryCCD.setExposureLeft(timeleft);
        }
    }

    SetTimer(POLLMS);
    return;
}

/**
 * Save configuration items
 */
bool SSG3CCD::saveConfigItems(FILE *fp)
{
    INDI::CCD::saveConfigItems(fp);

    IUSaveConfigNumber(fp, &GainNP);
    IUSaveConfigNumber(fp, &OffsetNP);

    return true;
}

/**
 * Download image from SSG3
 */
void SSG3CCD::grabImage()
{
    int sz;
    uint8_t *image = PrimaryCCD.getFrameBuffer();
    int rc;

    sz = PrimaryCCD.getFrameBufferSize();
    rc = orion_ssg3_image_download(&ssg3, image, sz);
    if (rc) {
        LOGF_INFO("Image download failed: %s", strerror(-rc));
    }

    ExposureComplete(&PrimaryCCD);

    LOG_INFO("Exposure complete.");
}
