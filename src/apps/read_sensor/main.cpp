// --------------------------------------------------------------------------
// $Id: main.cpp.K10A 135 2008-12-19 00:49:58Z omcf $
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

// read_sensor - color sensor development applet that demonstrates
//               direct connection, configuration, and reading
//               of an instrument through the libookala interface.

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef __linux__
#include <unistd.h>
#include <usb.h>
#endif

#include "wx/wxprec.h"
#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#ifndef WX_PRECOMP
    #include "wx/wx.h"   
#endif

#include "wxUtils.h"

#include <wx/cmdline.h>

#include "Dict.h"
#include "DictHash.h"
#include "PluginChain.h"
#include "PluginRegistry.h"

#include "plugins/K10A/K10A.h"


void
usage(int argc, char **argv)
{
    fprintf(stderr, "USAGE: %s <options> <plugin_path>\n", argv[0]);
    fprintf(stderr, "\t-n N           Sample N measurements\n");     
}

void
setupParser(wxCmdLineParser &parser)
{
    parser.AddParam(_("Path to plugin files"));
    parser.AddOption(wxT("n"), wxEmptyString, _("Switch to color space N"));
}

int 
main(int argc, char **argv)
{
    Ookala::PluginRegistry     reg;
    Ookala::DictHash          *hash;
    Ookala::Dict              *dict;
    Ookala::IntDictItem       *intItem;
    Ookala::StringDictItem    *stringItem;
    Ookala::PluginChain       *chain;
    Ookala::K10A              *sensor;
    Ookala::Yxy                Yxy;

    int numReadings = 1;
      
    wxInitializer initializer;

    wxString        argStr;
    wxCmdLineParser parser;

    parser.SetCmdLine(argc, argv);

    setupParser(parser);

    if (parser.Parse(false)) {
        usage(argc, argv);
        return 1;
    }
    else {
        fprintf(stderr, "Compiled for sensor: K10A\n");
    }

    if (parser.Found(wxT("n"), &argStr)) {
        numReadings = atoi(argStr.mb_str());
        if (numReadings < 1) numReadings = 1;
    }
 
    std::string pluginDir = (std::string)parser.GetParam().mb_str();
    std::string pluginPath;

    pluginPath = pluginDir + std::string("/libK10A.so");
    if (!reg.loadPlugin(pluginPath.c_str())) {
        fprintf(stderr, "Error loading %s\n", pluginPath.c_str());
        return 1;
    }


    std::vector<Ookala::Plugin *> plugins = reg.queryByName("DictHash");
    if (plugins.empty()) {
        fprintf(stderr, "No DictHash found\n");
        return 1;
    }
    hash = (Ookala::DictHash *)plugins[0];

    // Setup options in a dict, and set the dict in a chain
    dict = hash->newDict("K10A Options");
    
    intItem = (Ookala::IntDictItem *)reg.createDictItem("int");
    intItem->set(1);
    dict->set("K10A::calibrationIdx", intItem);

    stringItem = (Ookala::StringDictItem *)reg.createDictItem("string");
    stringItem->set("lcd");
    dict->set("K10A::displayType", stringItem);

    chain = new Ookala::PluginChain(&reg);
    chain->setDictName("K10A Options");
    
  
    plugins = reg.queryByName("K10A");
    if (plugins.empty()) {
        fprintf(stderr, "No K10A found\n");
        return 1;
    }

    sensor = (Ookala::K10A *)plugins[0];

    sensor->actionTaken(SENSOR_ACTION_DETECT);

    if (sensor->sensors().empty()) {
        fprintf(stderr, "No devices found\n");
        return 1;
    }

    for (int i=0; i<numReadings; ++i) {
        sensor->measureYxy(Yxy, chain);

        printf(" %f %f %f\n", Yxy.Y, Yxy.x, Yxy.y);
    }

    return 0;
}


