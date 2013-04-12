// --------------------------------------------------------------------------
// $Id: DreamColorCalib.h 135 2008-12-19 00:49:58Z omcf $
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

#ifndef DREAMCOLORCALIB_H_HAS_BEEN_INCLUDED
#define DREAMCOLORCALIB_H_HAS_BEEN_INCLUDED

// Calibration will probably only work properly if all
// of our targets reside on the same device - otherwise,
// we'll need to somehow prompt the user to move the probe!

#include <vector>
#include <map>

#include "Types.h"
#include "Plugin.h"
#include "Dict.h"

#include "plugins/DreamColor/DreamColorCtrl.h"
#include "plugins/DreamColor/DreamColorCalibRecord.h"

namespace Ookala {

// ----------------------------------------

class Color;
class Interpolate;
class Sensor;
class WsLut;
class DreamColorCalib: public Plugin
{
    public:
        DreamColorCalib();
        DreamColorCalib(const DreamColorCalib &src);
        virtual ~DreamColorCalib();
        DreamColorCalib & operator=(const DreamColorCalib &src);

        BEGIN_DICTITEM_ALLOC_FUNCS(DreamColorCalib)
            DICTITEM_ALLOC("DreamColorCalibRecord",  DreamColorCalibRecord)
        END_DICTITEM_ALLOC_FUNCS
        BEGIN_DICTITEM_DELETE_FUNCS
            DICTITEM_DELETE("DreamColorCalibRecord")
        END_DICTITEM_DELETE_FUNCS

        // If we're a calibration sort of plugin, we should allow people
        // to verify that we're still in a calibrated state. This involves
        // taking back in a calibration record that we generated, and
        // checking out some fields against current device state.
        virtual bool checkCalibRecord(CalibRecordDictItem *calibRec);

    protected:
        
        struct PluginData {
            DreamColorCtrl *disp;
            uint32_t        dispId;
            Sensor         *sensor;
            uint32_t        sensorId;
            Color          *color;
            Interpolate    *interp;
            WsLut          *lut;

            Npm             panelNpm;
            Yxy             panelRed,   panelGreen, panelBlue;
            Yxy             panelWhite;        
        };


        // Number of gray ramp measurements to take.
        uint32_t mNumGraySamples;

        // Tolerance values should all be > 0, even the minus.
        Yxy      mWhiteTolerancePlus, mWhiteToleranceMinus; 
        Yxy      mRedTolerancePlus,   mRedToleranceMinus; 
        Yxy      mGreenTolerancePlus, mGreenToleranceMinus; 
        Yxy      mBlueTolerancePlus,  mBlueToleranceMinus; 

        // Search for various calibration options. Currently, 
        // we're looking for:
        //
        //    DreamColorCalib.graySamples - number of gray ramp measurements
        //                                  to take (16) [integer].
        void gatherOptions(PluginChain *chain);

        // Search through any passed in arguments for a sane list
        // of targets to calibrate. This will only pay attention
        // to targets living on the same connection/device.
        //  
        // This looks for a stringArrayDictItem in the PluginChain
        // dict named "DreamColorCalib.targets". The strings defined
        // therein are the targets of type "DreamColorSpaceInfo"
        // stored in the same dict.
        std::vector<DreamColorSpaceInfo> gatherTargets(
                                            PluginChain *chain);

        // Gather all the plugins we'll need. We want:
        //   - DreamColorCtrl
        //   - Sensor, that's ready to go and not needing any
        //           user actions. Those should be handed higher up.
        // We want to look for these first in the chain, and 
        // if we don't find any there, look in the global registry
        // (at least for the sensor).
        bool gatherPlugins(PluginData   &pi,
                           PluginChain  *chain); 

        // Check for calibration targets that we should reset and
        // recalibrate. We'll look in the chain's dict to find
        // an array of strings, "DreamColorCalib::targets". This array
        // of strings names objects of type DreamColorSpaceInfo.
        //
        // Then, based on the targets, we'll first re-upload the 
        // target data to the unit, and then re-calibrate
        // the necessary targets.
        //
        //
        //   "DreamColorCalib::targets" - array of strings specifing 
        //                              DreamColorSpaceInfo objects to
        //                              process.
        //
        // 
        //    DreamColorCalib::whiteTolerance    (vec3, Yxy) -> same value for +-
        //    DreamColorCalib::redTolerance      (vec2, xy)  -> same value for +-
        //    DreamColorCalib::greenTolerance    (vec2, xy)  -> same value for +-
        //    DreamColorCalib::blueTolerance     (vec2, xy)  -> same value for +-
        //
        //    DreamColorCalib::whiteTolerancePlus  (vec3, Yxy) -> White + tolerance
        //    DreamColorCalib::whiteToleranceMinus (vec3, Yxy) -> White - tolerance
        //
        //    DreamColorCalib::redTolerancePlus  (vec2, xy) -> Red + tolerance
        //    DreamColorCalib::redToleranceMinus (vec2, xy) -> Red - tolerance
        //
        //    DreamColorCalib::greenTolerancePlus  (vec2, xy) -> Green + tolerance
        //    DreamColorCalib::greenToleranceMinus (vec2, xy) -> Green - tolerance
        //
        //    DreamColorCalib::blueTolerancePlus  (vec2, xy) -> Blue + tolerance
        //    DreamColorCalib::blueToleranceMinus (vec2, xy) -> Blue - tolerance



        virtual bool _run(PluginChain *chain);

        // We need to make sure we can find a DreamColorCtrl plugin 
        // and a WsLut plugin. Otherwise, fail before we start
        // executing the long part.
        virtual bool _checkDeps();

        // goalWhite    - the target white point we want
        // backlightReg - {p0, p1, p2, brightness}, to be set 
        //                 over the course of the loop
        // currWhite    - the white point measured at the finish.
        //
        // Returns false if we failed to converge on a decent
        // solution.
        //
        // NOTE: Color processing should be disabled before 
        //        entering this function.
        // NOTE: Pattern generator should be enabled before
        //        entering this function.

        virtual bool backlightLoop(PluginData  &pi,
                                   PluginChain *chain,
                                   const Yxy   &goalWhite,    
                                   uint32_t    *backlightReg,
                                   Yxy         &currWhite);

        // NOTE: Pattern generator should be enabled before
        //        entering this function.
        virtual bool measurePrimaries(PluginData   &pi,
                                      PluginChain  *chain,
                                      Yxy          &red,
                                      Yxy          &green,
                                      Yxy          &blue);
                                
        // NOTE: Pattern generator should be enabled and 
        //       color processing disabled.
        //
        // We'll take the raw grey measurements and conver them 
        // back to panel RGB. Then, we'll store the panel RGB
        //  (linear, not nonlinear) in the maps ramp{R,G,B}.
        //
        // This won't measure white - because presumable we've
        // already done that to get the XYZ->RGB matrix. But
        // it will process it if you'll pass it in.
        //
        //
        virtual bool measureGrayRamp(PluginData  &pi,
                                     PluginChain *chain,
                                     DreamColorCalibrationData &calib,
                                     const Npm   &panelNpm,            
                                     uint32_t     numSteps,
                                     std::map<double, double> &rampR,
                                     std::map<double, double> &rampG,
                                     std::map<double, double> &rampB,
                                     Mat33                     calibMat);


        bool estimateBlack(PluginData  &pi,
                           PluginChain *chain,
                           const Yxy   &measuredRed,
                           const Yxy   &measuredGreen,
                           const Yxy   &measuredBlue,
                           Yxy         &estimatedBlack);

        double estimateBlackError(PluginData  &pi,
                                  const Yxy   &goal,
                                  const Yxy   &estimatedBlack,
                                  const Yxy   &measurement);

        // Computes the 3x3 matrix for calibration, and sets it
        // in the CalibrationData, ready for uploading.
        //
        // This assumes that we've already computed the panel
        // XYZ -> rgb matrix (which we would need for the gray
        // ramp measurements.
        bool compute3x3CalibMatrix(PluginData                &pi,
                                   DreamColorCalibrationData &calib,
                                   const Npm                 &panelNpm);
                                    

        // Compute the pre and post luts, and store them in 
        // the CalibrationData struct.
        bool computeLuts(PluginData                &pi,
                         DreamColorCalibrationData &calib,
                         std::map<double, double>  &rampR,
                         std::map<double, double>  &rampG,
                         std::map<double, double>  &rampB,
                         const Npm                 &panelNpm);
                    

        // Given our currently measure white point (whiteYxy), estimates
        // of the primaries, and the goal white point (goalWhiteYxy),
        // figure out the linear RGB values of the goal white point.
        bool computeGoalRgb(Color  *color, 
                            const Yxy &red,       const Yxy  &green,
                            const Yxy &blue,      const Yxy  &white,
                            const Yxy &goalWhite, Rgb        &rgb);

        bool validate(PluginData                      &pi,
                      DreamColorSpaceInfo              target,
                      PluginChain                     *chain);

        // Takes care of checking if the chain was cancelled, and
        // performs any cleanup necessary. Returns true if we were
        // cancelled and should bail immediately.
        bool wasCancelled(PluginData      &pi,
                          PluginChain     *chain,
                          bool             patternGenEnabled);

        // Idle for at least the given number of seconds. Also, while we're
        // at it, test for cancellation. Returns false if we were cancelled.
        bool          idle(double seconds, PluginChain *chain);

};

}; // namespace Ookala

#endif
