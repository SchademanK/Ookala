// --------------------------------------------------------------------------
// $Id: DreamColorCtrl.h 135 2008-12-19 00:49:58Z omcf $
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

#ifndef DREAMCOLORCTRL_H_HAS_BEEN_INCLUDED
#define DREAMCOLORCTRL_H_HAS_BEEN_INCLUDED

#ifdef EXIMPORT
    #undef EXIMPORT
#endif

#ifdef _WIN32
#ifdef DREAMCOLOR_EXPORTS
#define EXIMPORT _declspec(dllexport)
#else
#define EXIMPORT _declspec(dllimport)
#endif

#else
#define EXIMPORT
#endif

#include <vector>
#include <map>

#include "Types.h"
#include "Plugin.h"
#include "Dict.h"
#include "Ddc.h"

#include "plugins/DreamColor/DreamColorSpaceInfo.h"
#include "plugins/DreamColor/DreamColorCalibrationData.h"

namespace Ookala {

struct _DreamColorCtrl;
struct DisplayConn;
class PluginChain;
class EXIMPORT DreamColorCtrl: public Plugin
{
    public:
        DreamColorCtrl();
        DreamColorCtrl(const DreamColorCtrl &src);
        virtual ~DreamColorCtrl();
        DreamColorCtrl & operator=(const DreamColorCtrl &src);


        BEGIN_DICTITEM_ALLOC_FUNCS(DreamColorCtrl)
            DICTITEM_ALLOC("DreamColorSpaceInfo",       DreamColorSpaceInfo)
            DICTITEM_ALLOC("DreamColorCalibrationData", 
                                                    DreamColorCalibrationData)
        END_DICTITEM_ALLOC_FUNCS
        BEGIN_DICTITEM_DELETE_FUNCS
            DICTITEM_DELETE("DreamColorSpaceInfo")
            DICTITEM_DELETE("DreamColorCalibrationData")
        END_DICTITEM_DELETE_FUNCS

        // Return keys to the devices that we've found. Keys should be
        // persistant across re-enumeration of devices. The device
        // key of 0 is reserved as the 'default' device - that is,
        // we don't much care which device gets hit with the command.
        // 
        // This is done for consistancy with the sensor interface.
        virtual std::vector<uint32_t> devices();

        // Return if a given device/connection id key is valid
        virtual bool deviceValid(uint32_t connId);

        // Read in initial state of the display, and setup
        // a communications channel.
        //
        // Returns false if we can't init DDC or find a display
        // that seems to be anything we recognize.
        virtual bool      enumerate(PluginChain *chain = NULL);

        // =================================================
        //
        // Device-specific magic numbers. We'll often
        // have to wait a little while for things to occur
        //
        // -------------------------------------------------


        // How long (in seconds) to wait after enabling the 
        // pattern generator for the image to come up, or after
        // a disable, how long for the video to come back 
        virtual uint32_t patternGeneratorEnableTime(
                                   uint32_t     connId = 0,
                                   PluginChain *chain  = NULL);
        
        virtual uint32_t patternGeneratorDisableTime(
                                   uint32_t     connId = 0,
                                   PluginChain *chain  = NULL);

        // How long (in seconds) does it take for the pattern   
        // generator (enabled) to switch to a new color
        virtual uint32_t patternGeneratorSettleTime(
                                    uint32_t     connId = 0,
                                    PluginChain *chain  = NULL);


        // After a major adjustment to the brightess backlight 
        // register, how long should we wait (in seconds) for
        // the device to settle
        virtual uint32_t backlightBrightnessSettleTime(
                                    uint32_t     connId = 0,
                                    PluginChain *chain  = NULL);


        // After tweaking the backlight registers (not brightness)
        // just a hair, how long should we want (in seconds) 
        virtual uint32_t backlightGenericSettleTime(
                                    uint32_t     connId = 0,
                                    PluginChain *chain  = NULL);
        
        // =================================================
        //
        // MCCS commands:
        //
        // -------------------------------------------------

        // Test and set the lockout of the OSD. This isn't to
        // say if the OSD is displayed - but rather if its 
        // locked from being activated by the user.
        virtual bool       getOsdLocked(bool        &locked, 
                                        uint32_t     connId = 0,
                                        PluginChain *chain  = NULL);
        virtual bool       setOsdLocked(bool         locked, 
                                        uint32_t     connId = 0,
                                        PluginChain *chain  = NULL);

        // Query the number of hours on the backlight
        virtual bool       getBacklightHours(uint32_t    &hours,
                                             uint32_t     connId = 0,
                                             PluginChain *chain  = NULL);

        // Query the temperature (in C) of the backlight.
        virtual bool       getBacklightTemp(uint32_t    &temp, 
                                            uint32_t     connId = 0,
                                            PluginChain *chain  = NULL);

        // Query the firmware version - This returns the
        // version of Torino firmware that is running (TL:).
        // There's also the Safe-Light version (SL:), but
        // that isn't accessible here.
        virtual bool       getFirmwareVersion(uint32_t    &version,    
                                              uint32_t     connId = 0,
                                              PluginChain *chain  = NULL);

        // =================================================
        //
        // Color Calibration:
        //
        // -------------------------------------------------

        // Return the number of color spaces that we support
        virtual uint32_t   numColorSpace(uint32_t     connId = 0,
                                         PluginChain *chain  = NULL);

        // Retrieve the currently set color space
        virtual bool       getColorSpace(uint32_t     &csIdx, 
                                         uint32_t     connId = 0,
                                         PluginChain *chain  = NULL);

        // Change the currently set color space
        virtual bool       setColorSpace(uint32_t     csIdx, 
                                         uint32_t     connId = 0,
                                         PluginChain *chain  = NULL);

        // NOTE: To enable, we need to re-select the current color space.
        //       And there isn't a way to read the state currently.
        //     
        // Enable or disable color processing
        virtual bool  setColorProcessingEnabled(bool         enable,
                                                uint32_t     connId = 0,
                                                PluginChain *chain  = NULL);

        // Return the bit depth of the pattern generator
        virtual uint32_t patternGeneratorBitDepth(uint32_t     connId = 0,
                                                  PluginChain *chain  = NULL);

        // Enable or disable the pattern generator
        virtual bool setPatternGeneratorEnabled(bool         enable, 
                                                 uint32_t     connId = 0,
                                                 PluginChain *chain  = NULL);
        
        // Set the color displayed by the pattern generator
        virtual bool setPatternGeneratorColor(
                                 uint32_t red,  uint32_t green,
                                 uint32_t blue, uint32_t connId = 0,
                                 PluginChain *chain  = NULL);

        // This does not change the current color space, but 
        // just queries the currently set color space.
        virtual bool getColorSpaceInfo(DreamColorSpaceInfo &value,
                                       uint32_t            connId = 0,
                                       PluginChain        *chain  = NULL);
                                        
        // This is like getColorSpaceInfo(), but it does not change
        // the active color space and allows you to specify which
        // color space to pull from
        virtual bool getColorSpaceInfoByIdx(DreamColorSpaceInfo &value,
                                            uint32_t            csIdx,
                                            uint32_t            connId = 0, 
                                            PluginChain        *chain  = NULL);

        // Return the number of bits in the LUTs       
        virtual uint32_t getPreLutBitDepth(uint32_t     connId = 0,
                                           PluginChain *chain  = NULL);

        virtual uint32_t getPostLutBitDepth(uint32_t     connId = 0,
                                            PluginChain *chain  = NULL);


        // Return the number of entries in the LUTs
        virtual uint32_t getPreLutLength(uint32_t      connId = 0,
                                         PluginChain  *chain  = NULL);
        virtual uint32_t getPostLutLength(uint32_t     connId = 0,
                                          PluginChain *chain  = NULL);


        // Retrieve the LUTs for the current color space
        virtual bool getPreLut(std::vector<uint32_t> &redLut,
                               std::vector<uint32_t> &greenLut,
                               std::vector<uint32_t> &blueLut,
                               uint32_t     connId = 0,
                               PluginChain *chain  = NULL);

        virtual bool getPostLut(std::vector<uint32_t> &redLut,
                                std::vector<uint32_t> &greenLut,
                                std::vector<uint32_t> &blueLut,
                                uint32_t     connId = 0,
                                PluginChain *chain  = NULL);


        // This returns the 3x3 matrix associated with the 
        // current color space, in row-major order.
        virtual bool getMatrix(Mat33       &matrix, 
                               uint32_t     connId = 0,
                               PluginChain *chain  = NULL);

        // Retrieve the 4 register values of interest from the 
        // backlight.
        //    p0         - "Almost Y"
        //    p1         - "Almost x"
        //    p2         - "Almost y"
        //    brightness - The "brightness", in cd/m^2, displayed
        //                 on the color space OSD that you can ramp
        //                 up and down.
        virtual bool getBacklightRegRaw(uint32_t    &p0, uint32_t &p1,
                                        uint32_t    &p2, uint32_t &brightness,
                                        uint32_t     connId = 0,
                                        PluginChain *chain  = NULL);

        // After setting backlight registers, you should wait for a 
        // little white (a few seconds) in order to let the backlight
        // settle into the new values. 
        //
        // There are two different settling steps. The first settle is for 
        // the register values to be updated. If you set new registers
        // and immediately read them back, there will probably be garbage
        // in them. It takes about 3 seconds for a full-brightness swing
        // for the new register values to take hold.
        //
        // The second settling is for the front-screen colorimetry to
        // settle. Not sure how long this takes yet.
        virtual bool setBacklightRegRaw(uint32_t     p0,    uint32_t  p1,
                                        uint32_t     p2,    uint32_t  brightness,
                                        bool         setP0, bool      setP1, 
                                        bool         setP2, 
                                        bool         setBrightness,
                                        uint32_t     connId = 0,
                                        PluginChain *chain  = NULL);
        
        // Given an normalized non-linear value, convert it back to
        // linear through the TRC.
        virtual double evalTrcToLinear(double              nonlinearX,
                                       DreamColorSpaceInfo info,
                                       uint32_t            connId = 0,
                                       PluginChain        *chain  = NULL);

        // Compute the value of the brightness register that gives
        // us a certain luminance. Note the brightness control
        // only moves in increments of 1 cd/m^2, and ranges between
        // 50 cd/m^2 [0x33] and 250 cd/m^2 [0xff].
        //
        // It's not entirely clear at this point what to do if
        // you really want to exceed that range.
        virtual uint32_t brightnessRegister(double       luminance,
                                            uint32_t     connId = 0,
                                            PluginChain *chain  = NULL);


        // As the previous method, but you need to specify all 
        // the parameters yourself.
        virtual double evalTrcToLinearParam(double       nonlinearX,
                                            double       a0,         double a1, 
                                            double       a2,         double a3,
                                            double       gamma,
                                            uint32_t     connId = 0, 
                                            PluginChain *chain  = NULL);



        // Upload all calibration data to a particular color space preset.
        //
        // NOTE: This will have the side effect of changing your 
        //       currently active color space to csIdx.
        virtual bool setCalibration(uint32_t                         csIdx,
                                    const DreamColorCalibrationData &data,
                                    uint32_t                         connId = 0, 
                                    PluginChain                     *chain  = NULL);

        // Download all necessary calibration data for the current preset
        virtual bool getCalibration(DreamColorCalibrationData &data,
                                    uint32_t                   connId = 0, 
                                    PluginChain               *chain  = NULL);

    protected:
    
        // Get back a DDC plugin (or NULL) that corresponds to a
        // DisplaConn structs' key.
        //
        //    connId is the key into _our_ hash of connections,
        //    devId  is the key into the plugins' hash of connections
        Ddc *         getPluginFromKey(uint32_t    connId, 
                                       uint32_t    &devId,
                                       PluginChain *chain);

        // Get back the connection data associated with a connection
        // key. Can return false if the connId is not a valid key.
        bool          getConnFromKey(uint32_t     connId, 
                                     DisplayConn *conn,
                                     PluginChain *chain);


        // Send a message to retrieve a 16-bit LUT, and then read
        // out the values. 
        //
        //   lutPMB    - The memory block ID for the LUT (1-post, 3-pre)
        //   lutLength - Number of entries in the LUT
        //   redLut    - Storage for the red channel
        //   greenLut  - 
        //   blueLut   -              
        bool          downloadLut16(uint32_t               lutPMB,
                                    uint32_t               lutLength,
                                    std::vector<uint32_t> &redLut,
                                    std::vector<uint32_t> &greenLut,
                                    std::vector<uint32_t> &blueLut,
                                    uint32_t               connId,
                                    PluginChain           *chain);

        // Upload a 16-bit LUT to a particular PMB.  All necessary
        // torino checks take place within.
        //
        //   lutPMB    - The memory block ID for the LUT (1-post, 3-pre)
        //   lutLength - Number of entries in the LUT
        //   redLut    - Storage for the red channel
        //   greenLut  - 
        //   blueLut   -              
        //   uploadingAllData - if we're in the process of sending
        //                      the entire data set, or just a component
        bool uploadLut16(uint32_t                     lutPMB,
                         uint32_t                     lutLength,
                         const std::vector<uint32_t> &redLut,
                         const std::vector<uint32_t> &greenLut,
                         const std::vector<uint32_t> &blueLut,
                         bool                         uploadingAllData,
                         uint32_t                     connId,
                         PluginChain                 *chain);

        // Query the device for color space info. Accepted values for
        // are 0->numColorSpace()-1, and 0xff - which pulls from the
        // currently set color space.
        bool          downloadColorSpaceInfo(DreamColorSpaceInfo &info,
                                             uint32_t             csIdx,  
                                             uint32_t             connId,
                                             PluginChain         *chain);
 
        // UploadingAllData = true means we're going to follow
        // with sending pre/3x3/post/registers. Otherwise, we're
        // just sending the color space info. This will do Torino
        // checks, but will _not_ commit/save the values.
        bool uploadColorSpaceInfo(DreamColorSpaceInfo  value,
                                  bool                 uploadingAllData,
                                  uint32_t             connId,
                                  PluginChain         *chain);


        // This uploads a 3x3 matrix associated with the current
        // color space. Matrix is in row-major order. 
        //
        // Thios will handle all the necessary Tornio checks
        bool uploadMatrix(const Mat33  &matrix,
                          bool         uploadingAllData,
                          uint32_t     connId,
                          PluginChain *chain);


        // This will handle all the necessary Torino checks, based
        // on if we're sending all data, or just register data.
        bool uploadBacklightRegRaw(uint32_t     p0,    uint32_t  p1,
                                   uint32_t     p2,    uint32_t  brightness,
                                   bool         setP0, bool      setP1, 
                                   bool         setP2, bool      setBrightness,
                                   bool         uploadingAllData,
                                   uint32_t     connId,
                                   PluginChain *chain);


        // Occasionally, we'll read too fast for things to handle. In
        // that case, we should back-off for a slight bit and try
        // again. This unfortunatly happens more often that we would
        // like...
        bool          retryGetVcpFeature(uint8_t   opcode, uint16_t &currVal, 
                                         uint16_t &maxVal, uint32_t  connId,
                                         PluginChain *chain);

        // This tests the status of the Torino unit. This should
        // be true before sending any data to the PMBs. 
        //
        // If the return value was false, semething went horribly
        // wrong, beyone the fact that the unit was busy, or
        // if the unit is not idle.
        bool          torinoIdle(uint32_t connId, PluginChain *chain);

        // Tornio can have various states other than 'idle', which
        // you're free to specify as the 'currState' and 'maxState'.
        //
        // Returns false if communications failed or if we're
        // not in the specified state.
        bool          torinoInState(uint16_t currState, uint16_t     maxState, 
                                    uint32_t connId,    PluginChain *chain);

        // Take 4 bytes of little endian float data and
        // turn it back into a float.
        double        fourBytesToFloat(uint8_t *buf);

        void          floatToFourBytes(double in, uint8_t *out);

        // Idle for at least the given number of seconds. Also, while we're
        // at it, test for cancellation. Returns false if we were cancelled.
        bool          idle(double seconds, PluginChain *chain);
    
        // Enumerate displays, at the very least.
        virtual bool  _preRun(PluginChain *chain);

        // Respond to a few commands:
        //
        //    DreamColorCtrl::setColorSpace [INT] --> Change to a particular preset.
        //
        virtual bool  _run(PluginChain *chain);

    private:

        _DreamColorCtrl *mDreamColorCtrlData;

};

}; // namespace Ookala 

#endif
