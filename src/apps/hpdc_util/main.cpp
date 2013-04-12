// --------------------------------------------------------------------------
// $Id: main.cpp 135 2008-12-19 00:49:58Z omcf $
// --------------------------------------------------------------------------
// Copyright (c) 2008 Hewlett-Packard Development Company, L.P.
// 
// Permission is hereby granted, free of charge, to any person obtaining 
// a copy of this software and associated documentation files (the "Software"), 
// to deal in the Software without restriction, including without limitation 
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR 
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
// OTHER DEALINGS IN THE SOFTWARE.
// --------------------------------------------------------------------------

// hpdc_util - a basic command-line interface to the HP DreamColor 
//             Professional Display support in libookala - used in
//             conjunction with read_sensor for development and 
//             debugging

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef __linux__
#include <unistd.h>
#endif

#include "wx/wxprec.h"
#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/wx.h"   
#endif
#include <wx/cmdline.h>

#include "Dict.h"
#include "DictHash.h"
#include "PluginChain.h"
#include "PluginRegistry.h"

#include "plugins/DreamColor/DreamColorCtrl.h"
#include "plugins/DreamColor/DreamColorCalibrationData.h"


void
usage(int argc, char **argv)
{
    fprintf(stderr, "USAGE: %s <options> <plugin_path>\n", argv[0]);
    fprintf(stderr, "\t-c N           Switch to color space N\n");     
    fprintf(stderr, "\t-q             Query current color space data\n");     
    fprintf(stderr, "\t-m             Query the current matrix\n");
    fprintf(stderr, "\t-M \"m00 ...\" Set the current matrix\n");
    fprintf(stderr, "\t-p filename    Download pre-Lut to filename\n");
    fprintf(stderr, "\t-P filename    Download post-Lut to filename\n");
    fprintf(stderr, "\t-e             Enable color processing\n");
    fprintf(stderr, "\t-E             Disable color processing\n");
    fprintf(stderr, "\t-g \"R G B\"     Enable pattern generator with a color\n");
    fprintf(stderr, "\t-G             Disable pattern generator\n");
    fprintf(stderr, "\t-o             Unlock the OSD\n");
    fprintf(stderr, "\t-O             Lock the OSD\n");
    fprintf(stderr, "\t-b brightness  Set the backlight brightness register (8 bits)\n");
    fprintf(stderr, "\t-d N           Copy the current color space to preset N\n");
    fprintf(stderr, "\t-t             Test torino state\n");
}

void
setupParser(wxCmdLineParser &parser)
{
    parser.AddSwitch("q", wxEmptyString, "Query current color space data");
    parser.AddSwitch("m", wxEmptyString, "Query the current matrix");
    parser.AddSwitch("e", wxEmptyString, "Enable color processing");
    parser.AddSwitch("E", wxEmptyString, "Disable color processing");
    parser.AddSwitch("G", wxEmptyString, "Disable pattern generator");
    parser.AddSwitch("o", wxEmptyString, "Unlock the OSD");
    parser.AddSwitch("O", wxEmptyString, "Lock the OSD");
    parser.AddSwitch("t", wxEmptyString, "Test torino state");

    parser.AddOption("c", wxEmptyString, "Switch to color space N");
    parser.AddOption("M", wxEmptyString, "Set the current matrix to \"m00 ...\"");
    parser.AddOption("p", wxEmptyString, "Download the pre-LUT to the given filename");
    parser.AddOption("P", wxEmptyString, "Download the post-LUT to the given filename");
    parser.AddOption("g", wxEmptyString, "Enable the pattern generator to \"R G B\"");
    parser.AddOption("b", wxEmptyString, "Set the backlight brightness register (8-bits)");
    parser.AddOption("d", wxEmptyString, "Copy the current color space data to preset N");

    parser.AddParam("Path to plugin files");
}

int 
main(int argc, char **argv)
{
    Ookala::PluginRegistry     reg;
    Ookala::DreamColorCtrl    *disp; 
    //int                option;
    
    bool     optionColorspace         = false;
    int32_t  optionColorspaceValue    = 0;

    bool     optionQuery              = false;

    bool     optionMatrix             = false;
    bool     optionSetMatrix          = false;
    double   optionNewMatrix[9];

    bool     optionGetPreLut          = false;
    bool     optionGetPostLut         = false;
    char *   optionGetPreLutFilename  = NULL;
    char *   optionGetPostLutFilename = NULL;
    
    bool     optionDoEnable           = false;
    bool     optionDoDisable          = false;

    bool     optionDoPatternEnable    = false;
    bool     optionDoPatternDisable   = false;
    uint32_t optionDoPatternColor[3]; 

    bool     optionLockOsd            = false;
    bool     optionUnlockOsd          = false;

    bool     optionBrightness         = false;
    uint32_t optionBrightnessValue    = 0;

    bool     optionCopyPreset         = false;
    uint32_t optionCopyPresetDst      = 0;

    bool     optionTorinoReady        = false;

    
    wxInitializer initializer;

    wxString        argStr;
    wxCmdLineParser parser;

    parser.SetCmdLine(argc, argv);

    setupParser(parser);

    if (parser.Parse(false)) {
        usage(argc, argv);
        return 1;
    }

    if (parser.Found("c", &argStr)) {
        optionColorspace      = true;
        optionColorspaceValue = atoi(argStr.c_str());
    }

    if (parser.Found("q")) {
        optionQuery = true;
    }

    if (parser.Found("m")) {
        optionMatrix = true;
    }

    if (parser.Found("M", &argStr)) {
       optionSetMatrix = true;
       sscanf(argStr.c_str(), "%lf %lf %lf %lf %lf %lf %lf %lf %lf",
                       &optionNewMatrix[0], &optionNewMatrix[1], &optionNewMatrix[2], 
                       &optionNewMatrix[3], &optionNewMatrix[4], &optionNewMatrix[5], 
                       &optionNewMatrix[6], &optionNewMatrix[7], &optionNewMatrix[8]);
    }
        
    if (parser.Found("p", &argStr)) {
        optionGetPreLut         = true;
#ifdef _WIN32
        optionGetPreLutFilename = _strdup(argStr.c_str());
#else
        optionGetPreLutFilename = strdup(argStr.c_str());
#endif
    }

    if (parser.Found("P", &argStr)) {
        optionGetPostLut         = true;

#ifdef _WIN32
        optionGetPostLutFilename = _strdup(argStr.c_str());
#else
        optionGetPostLutFilename = strdup(argStr.c_str());
#endif
    }

    if (parser.Found("e")) {
        optionDoEnable = true;
    }

    if (parser.Found("E")) {
        optionDoDisable = true;
    }

    if (parser.Found("g", &argStr)) {
        optionDoPatternEnable  = true;
        sscanf(argStr.c_str(), "%d %d %d", 
                    &optionDoPatternColor[0],
                    &optionDoPatternColor[1],
                    &optionDoPatternColor[2]);
    }

    if (parser.Found("G")) {
        optionDoPatternDisable = true;
    }

    if (parser.Found("o")) {
        optionUnlockOsd = true;
    }

    if (parser.Found("O")) {
        optionLockOsd = true;
    }

    if (parser.Found("b", &argStr)) {
        optionBrightness      = true;
        optionBrightnessValue = atoi(argStr.c_str());
    }

    if (parser.Found("d", &argStr)) {
        optionCopyPreset      = true;
        optionCopyPresetDst   = atoi(argStr.c_str());
    }

    if (parser.Found("t")) {
        optionTorinoReady = true;
    }

    std::string pluginDir = (std::string)parser.GetParam();
    std::string pluginPath;

#ifdef _WIN32
    pluginPath = pluginDir + std::string("\\NvI2c.dll");
    if (!reg.loadPlugin( pluginPath.c_str())) {
        fprintf(stderr, "Error loading %s\n", pluginPath.c_str());
        return 1;
    }

    pluginPath = pluginDir + std::string("\\WsLut.dll");
    if (!reg.loadPlugin( pluginPath.c_str())) {
        fprintf(stderr, "Error loading %s\n", pluginPath.c_str());
        return 1;
    }

    pluginPath = pluginDir + std::string("\\DreamColor.dll");
    if (!reg.loadPlugin( pluginPath.c_str())) {
        fprintf(stderr, "Error loading %s\n", pluginPath.c_str());
        return 1;
    }
#else

    pluginPath = pluginDir + std::string("/libDevI2c.so");
    if (!reg.loadPlugin(pluginPath.c_str())) {
        fprintf(stderr, "Error loading %s\n", pluginPath.c_str());
        return 1;
    }

    pluginPath = pluginDir + std::string("/libWsLut.so");
    if (!reg.loadPlugin(pluginPath.c_str())) {
        fprintf(stderr, "Error loading %s\n", pluginPath.c_str());
        return 1;
    }


    pluginPath = pluginDir + std::string("/libDreamColor.so");
    if (!reg.loadPlugin(pluginPath.c_str())) {
        fprintf(stderr, "Error loading %s\n", pluginPath.c_str());
        return 1;
    }
#endif


    std::vector<Ookala::Plugin *> plugins = reg.queryByName("DreamColorCtrl");
    if (plugins.empty()) {
        fprintf(stderr, "No DC Ctrl found\n");
        return 1;
    }

    disp = (Ookala::DreamColorCtrl *)(plugins[0]);

    disp->enumerate();

    if ((disp->devices()).empty()) {
        fprintf(stderr, "ERROR: No display found\n");
        return 1;      
    }

    if (optionTorinoReady) {

    }

    if (optionDoEnable) {
        if (!disp->setColorProcessingEnabled(true)) {
            fprintf(stderr, "ERROR: Can't enable color processing\n");
            fprintf(stderr, "%s\n", disp->errorString().c_str());
            return 1;
        }
    }

    if (optionDoDisable) {
        if (!disp->setColorProcessingEnabled(false)) {
            fprintf(stderr, "ERROR: Can't disable color processing\n");
            fprintf(stderr, "%s\n", disp->errorString().c_str());
            return 1;
        }
    }

    if (optionColorspace) {
        if (!disp->setColorSpace(optionColorspaceValue)) {
            fprintf(stderr, "ERROR: Unable to select colorspace %d\n",
                                            optionColorspaceValue);
            fprintf(stderr, "%s\n", disp->errorString().c_str());
            return 1;
        }
    }

    if (optionLockOsd) {
        if (!disp->setOsdLocked(true)) {
            fprintf(stderr, "ERROR: Unable to lock OSD\n");
            fprintf(stderr, "%s\n", disp->errorString().c_str());
            return 1;
        }
    }

    if (optionUnlockOsd) {
        if (!disp->setOsdLocked(false)) {
            fprintf(stderr, "ERROR: Unable to unlock OSD\n");
            fprintf(stderr, "%s\n", disp->errorString().c_str());
            return 1;
        }
    }

    if (optionBrightness) {
        if (!disp->setBacklightRegRaw(0, 0, 0, optionBrightnessValue,
                                     false, false, false, true)) {
            fprintf(stderr, "ERROR: Unable to set brightness\n");
            fprintf(stderr, "%s\n", disp->errorString().c_str());
            return 1;
        }

        // Wait for register values to settle if we're going to
        // query them - otherwise our query will have garbage
        // in the registers.
        if (optionQuery) {
#ifdef _WIN32
            printf("Sleeping on line hpdc_util::main.cpp:407\n");
            Sleep(4000);
#else
            sleep(4);
#endif
        }
    }

    if (optionQuery) {
        Ookala::DreamColorSpaceInfo *info;

        uint32_t       currSpace, firmware;
        uint32_t       blParam[4];
        uint32_t       backlightTemp, backlightHours;
        bool           osdLocked;

        Ookala::DictItem *item = reg.createDictItem("DreamColorSpaceInfo");
        if (!item) {
            printf("Alloc failed\n");
            return false;
        }

        info = (Ookala::DreamColorSpaceInfo *)item;
        if (!info) {
            printf("Cast error\n");
            return false;
        }

        if (!disp->getColorSpace(currSpace)) {
            fprintf(stderr, "ERROR: Unable to get current colorspace\n");
            fprintf(stderr, "%s\n", disp->errorString().c_str());
            return 1;
        }

        if (!disp->getColorSpaceInfo(*info)) {
            fprintf(stderr, "ERROR: Unable to get current colorspace data\n");
            fprintf(stderr, "%s\n", disp->errorString().c_str());
            return 1;
        }

        // If we changed color space presets, we need to wait for the
        // backlight to settle before reading it's values.
        if (!disp->getBacklightRegRaw(blParam[0], blParam[1],
                                     blParam[2], blParam[3])) {
            fprintf(stderr, "ERROR: Unable to get current backlight data\n");
            fprintf(stderr, "%s\n", disp->errorString().c_str());
            return 1;
        }

        if (!disp->getBacklightTemp(backlightTemp)) {
            fprintf(stderr, "ERROR: Unable to get current backlight temp\n");
            fprintf(stderr, "%s\n", disp->errorString().c_str());
            return 1;
        }

        if (!disp->getBacklightHours(backlightHours)) {
            fprintf(stderr, "ERROR: Unable to get current backlight time\n");
            fprintf(stderr, "%s\n", disp->errorString().c_str());
            return 1;
        }

        if (!disp->getOsdLocked(osdLocked)) {
            fprintf(stderr, "ERROR: Unable to get OSD lock status\n");
            fprintf(stderr, "%s\n", disp->errorString().c_str());
            return 1;
        }

        if (!disp->getFirmwareVersion(firmware)) {
            fprintf(stderr, "ERROR: Unable to firmware version\n");
            fprintf(stderr, "%s\n", disp->errorString().c_str());
            return 1;
        }

        printf("Current preset:         %d\n", currSpace);
        printf("Name:                   %s\n", info->getName().c_str());
        printf("Enabled:                %d\n", info->getEnabled());
        printf("Calibrated:             %d\n", info->getCalibrated());
        printf("Calib. hours remaining: %d\n", info->getCalibratedTimeRemaining());
        printf("White (xyY):            %f %f %f\n", 
                   info->getWhite().x, info->getWhite().y, info->getWhite().Y);
        printf("Red (xy):               %f %f\n",
                   info->getRed().x, info->getRed().y);
        printf("Green (xy):             %f %f\n",
                   info->getGreen().x, info->getGreen().y);
        printf("Blue (xy):              %f %f\n",
                   info->getBlue().x, info->getBlue().y);
        printf("TRC Gamma:              %f\n", info->getTrcGamma());
        printf("TRC A0:                 %f\n", info->getTrcA0());
        printf("TRC A1:                 %f\n", info->getTrcA1());
        printf("TRC A2:                 %f\n", info->getTrcA2());
        printf("TRC A3:                 %f\n", info->getTrcA3());

        printf("Backlight Registers:    p0 [Y]:     %4d\n", blParam[0]);
        printf("                        p1 [x]:     %4d\n", blParam[1]);
        printf("                        p2 [y]:     %4d\n", blParam[2]);
        printf("                        brightness: %4d\n", blParam[3]);
        printf("Backlight Temprature:   %d C\n", backlightTemp);
        printf("Backlight Hours:        %d\n",   backlightHours);
        printf("OSD Locked:             %d\n",   osdLocked);
        printf("Firmware (Torino):      %d\n",   firmware);
    }

    if (optionMatrix) {
        Ookala::Mat33 matrix;

        if (!disp->getMatrix(matrix)) {
            fprintf(stderr, "ERROR: Unable to get current colorspace matrix\n");
            fprintf(stderr, "%s\n", disp->errorString().c_str());
            return 1;
        }

        printf("Current Matrix:\n");
        printf("\t%f %f %f\n", matrix.m00, matrix.m01, matrix.m02);
        printf("\t%f %f %f\n", matrix.m10, matrix.m11, matrix.m12);
        printf("\t%f %f %f\n", matrix.m20, matrix.m21, matrix.m22);
    }

    if (optionGetPreLut) {
        FILE *fid;
        std::vector<uint32_t> redLut, greenLut, blueLut;
        std::vector<uint32_t>::iterator redIt, greenIt, blueIt;

        fprintf(stderr, "Downloading Pre-Lut...\n");

        if (!disp->getPreLut(redLut, greenLut, blueLut)) {
            fprintf(stderr, "ERROR: Unable to get pre-Luts\n");
            fprintf(stderr, "%s\n", disp->errorString().c_str());
            return 1;
        }

        fid = fopen(optionGetPreLutFilename, "w");
        if (!fid) {
            fprintf(stderr, "ERROR: Can't open %s\n", 
                        optionGetPreLutFilename);
        } else {
            for (redIt   = redLut.begin(),
                 greenIt = greenLut.begin(),
                 blueIt  = blueLut.begin();
        
                 redIt   != redLut.end(),
                 greenIt != greenLut.end(),
                 blueIt  != blueLut.end();

                 ++redIt, ++greenIt, ++blueIt) {
                fprintf(fid, "%4d %4d %4d\n", *redIt, *greenIt, *blueIt);
            }
            fclose(fid); 
        }
    }



    if (optionGetPostLut) {
        FILE *fid;
        std::vector<uint32_t> redLut, greenLut, blueLut;
        std::vector<uint32_t>::iterator redIt, greenIt, blueIt;

        fprintf(stderr, "Downloading Post-Lut...\n");

        if (!disp->getPostLut(redLut, greenLut, blueLut)) {
            fprintf(stderr, "ERROR: Unable to get Post-Luts\n");
            fprintf(stderr, "%s\n", disp->errorString().c_str());
            return 1;
        }

        fid = fopen(optionGetPostLutFilename, "w");
        if (!fid) {
            fprintf(stderr, "ERROR: Can't open %s\n", 
                        optionGetPostLutFilename);
        } else {
            for (redIt   = redLut.begin(),
                 greenIt = greenLut.begin(),
                 blueIt  = blueLut.begin();
        
                 redIt   != redLut.end(),
                 greenIt != greenLut.end(),
                 blueIt  != blueLut.end();

                 ++redIt, ++greenIt, ++blueIt) {
                fprintf(fid, "%4d %4d %4d\n", *redIt, *greenIt, *blueIt);
            }
            fclose(fid); 
        }
    }

    if (optionDoPatternEnable) {

        if (!disp->setPatternGeneratorColor(
                        optionDoPatternColor[0],
                        optionDoPatternColor[1],
                        optionDoPatternColor[2])) {

            fprintf(stderr, "ERROR: Unable to set pattern generator\n");
            fprintf(stderr, "%s\n", disp->errorString().c_str());
            return 1;
        }



        if (!disp->setPatternGeneratorEnabled(true)) {
            fprintf(stderr, "ERROR: Unable to enable pattern generator\n");
            fprintf(stderr, "%s\n", disp->errorString().c_str());
            return 1;
        }
    }

    if (optionDoPatternDisable) {
        if (!disp->setPatternGeneratorEnabled(false)) {
            fprintf(stderr, "ERROR: Unable to disable pattern generator\n");
            fprintf(stderr, "%s\n", disp->errorString().c_str());
            return 1;
        }
    }

    if (optionCopyPreset) {
        Ookala::DreamColorCalibrationData *calib;

        Ookala::DictItem *item = reg.createDictItem("DreamColorCalibrationData");
        if (!item) {
            printf("Alloc failed\n");
            return 1;
        }
        calib = (Ookala::DreamColorCalibrationData *)item;


        if (!disp->getCalibration(*calib)) {
            fprintf(stderr, "ERROR: Can't download calibation data\n");
            fprintf(stderr, "%s\n", disp->errorString().c_str());
            return 1;
        }

        if (!disp->setCalibration(optionCopyPresetDst, *calib)) {
            fprintf(stderr, "ERROR: Can't upload calibation data\n");
            fprintf(stderr, "%s\n", disp->errorString().c_str());
            return 1;
        }
    }
    

    if (optionSetMatrix) {
        uint32_t csIdx;
        Ookala::DreamColorCalibrationData *calib;

        Ookala::DictItem *item = reg.createDictItem("DreamColorCalibrationData");
        if (!item) {
            printf("Alloc failed\n");
            return 1;
        }
        calib = (Ookala::DreamColorCalibrationData *)item;


        if (!disp->getColorSpace(csIdx)) {
            fprintf(stderr, "ERROR: Unable to get current color space\n");
            fprintf(stderr, "%s\n", disp->errorString().c_str());
            return 1;
        }

        if (!disp->getCalibration(*calib)) {
            fprintf(stderr, "ERROR: Can't download calibation data\n");
            fprintf(stderr, "%s\n", disp->errorString().c_str());
            return 1;
        }
    }


    return 0;
}


