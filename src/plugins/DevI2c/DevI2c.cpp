// --------------------------------------------------------------------------
// $Id: DevI2c.cpp 135 2008-12-19 00:49:58Z omcf $
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

#include <linux/i2c-dev.h>		// Requires i2c Dev pkg (user-space)

#ifndef I2C_SLAVE
#include <linux/i2c.h>			// Possibly defined here if necessary
#endif

#include "Dict.h"
#include "DevI2c.h"

//static uint32_t eventId = 0;

// ----------------------------------

BEGIN_PLUGIN_REGISTER(1)
    PLUGIN_REGISTER(0, Ookala::DevI2c)
END_PLUGIN_REGISTER


// -----------------------------------

Ookala::DevI2c::DevI2c():
    Ddc()
{
    setName("DevI2c");
}

// -----------------------------------

Ookala::DevI2c::DevI2c(const DevI2c &src):
    Ddc(src)
{
    // Duplicating file descriptors here would mean that 
    // we'd close the same fd multiple times. Which probably
    // isn't good. So just don't copy things over for now
    // and re-enumerate if need be.
    enumerate();
}

// -----------------------------------
//
// virtual
Ookala::DevI2c::~DevI2c()
{
printf("DevI2c::~DevI2c() - closing %d fds\n", (int)mFds.size());
    // Close all the device we have open.
    for (std::map<uint32_t, int>::iterator i=mFds.begin();
            i != mFds.end(); ++i) {
        close((*i).second);
    }
    mFds.clear();
}

// ----------------------------
//
Ookala::DevI2c &
Ookala::DevI2c::operator=(const DevI2c &src)
{
    if (this != &src) {
        Ddc::operator=(src);

        enumerate();
    }

    return *this;
}



// -----------------------------------
//
// virtual
bool
Ookala::DevI2c::enumerate()
{
    int         fd;
    uint32_t    realId;
    struct stat statbuf;

    // Now, walk all the devices that aren't open already 
    // and open them, adding them to mFds + mFilenames
    for (uint32_t idx=0; idx<16; ++idx) {
        char filename[4096];

        // Test if a given device exists.
        sprintf(filename, "/dev/i2c-%d", idx);
        if (stat(filename, &statbuf) != 0) {
            break;
        }
    
        // Test if we've already opened it
        bool opened = false;
        for (std::map<uint32_t, std::string>::iterator file =
                mFilenames.begin(); file != mFilenames.end(); ++file) {
            if ((*file).second == std::string(filename)) {
                opened = true;
                break;
            }
        }
        if (opened) {
            continue;
        }

        // If we've gotten this far, add the filename and fd to
        // our list, but don't check the EDID yet. We'll do that
        // in the next loop.
        fd = open(filename, O_RDWR);
        if (fd < 0) {
            continue;
        }
        realId = mNextKey;
        mNextKey++;

        mFds[realId]       = fd;
        mFilenames[realId] = std::string(filename);
    }

    // Walk over everything we have open, and see if they 
    // all seem to still be alive (checking EDIDs..)
    std::vector<uint32_t> badDevIds;

    for (std::map<uint32_t, int>::iterator file = mFds.begin();
                file != mFds.end(); ++file) {

        struct Edid1_3 edid;

        realId = (*file).first;
        fd     = (*file).second;
        
        usleep(50000);
        if (ioctl(fd, I2C_SLAVE, 0x50) < 0) {
            continue;
        }

        // Try a few times to grab edid
        bool foundEdid = false;
        for (int i=0; i<5; i++) {
            uint8_t        rawEdid[256], writeBuf[64];

            // Write the data offset
            writeBuf[0] = 0;
            usleep(50000);

            if (write(fd, &writeBuf, 1) != 1) {
                continue;
            }

            // Read the response at that offset

            usleep(50000);
            if (read(fd, rawEdid, 128) != 128) {
                continue;
            }

            if (parseEdid1_3(rawEdid, 128, &edid) == true) {
                foundEdid = true;
                break;
            }
        }

        // Put the device in a state for ddc/ci commands
        if (ioctl(fd, I2C_SLAVE, 0x37) < 0) {
            foundEdid = false;
        }

        if (!foundEdid) {
            badDevIds.push_back(realId);
        } else {
            addEdid(realId, edid);
        }      
    }

    // For everyone who seems dead, close 'em (and remove their
    // EDIDs)
    for (std::vector<uint32_t>::iterator theId = badDevIds.begin();
                theId != badDevIds.end(); ++theId) {

        std::map<uint32_t, int>::iterator theDev = mFds.find(*theId);

        if (theDev == mFds.end()) continue;

        fd = (*theDev).second;
        close(fd);
        
        mFds.erase(*theId);
        mFilenames.erase(*theId);
        clearEdid(*theId);
    }

    // Double check consistancy between fds and edids
    if (mFilenames.size() != mFds.size()) {
        fprintf(stderr, "WARNING: Inconsistancy between mFilenames + mFds\n");
    }
    if (getEdidKeys().size() != mFds.size()) {
        fprintf(stderr, "WARNING: Inconsistancy between mEdidData + mFds\n");
    }
   
    return true;
}


// -----------------------------------
//
// virtual
std::vector<uint32_t>
Ookala::DevI2c::devices()
{
    std::vector<uint32_t> devs;

    for (std::map<uint32_t, int>::iterator theFd = mFds.begin();
            theFd != mFds.end(); ++theFd) {
        devs.push_back((*theFd).first);
    }

    return devs;
}


// -----------------------------------
//
// Prepend a header to the payload, and append a checksum.
// Then pass the message over the wire.
//
// virtual
bool
Ookala::DevI2c::sendI2cPayload(uint8_t *buf, uint32_t bufLen, uint32_t devId /* = 0 */)
{
    int      checksum;
    uint32_t realId;
    uint8_t *pkt  = NULL;
    bool     ret  = true;
    int      addr = 0x37;
 
    if (!validDevId(devId, realId)) {
        setErrorString("Invalid device ID.");
        return false;
    }

    pkt = new uint8_t[3+bufLen];

    pkt[0] = 0x51;
    pkt[1] = 0x80 | bufLen;
    memcpy(&pkt[2], buf, bufLen);

    checksum = (addr << 1);
    for (uint32_t i=0; i<2+bufLen; i++) {
        checksum = checksum ^ pkt[i];
    }
    pkt[2+bufLen] = checksum;

    ret = sendI2cMsg(pkt, 3+bufLen, realId);

    if (mDebug) {
        FILE *fid = fopen(mDebugFile.c_str(), "w");
        if (fid) {
            uint8_t c = 0x6E;
            fwrite(&c, 1, 1, fid);
            fwrite(pkt, 3+bufLen,1, fid);
            fclose(fid);
        }
    }

    delete[] pkt;

    return ret;
}

// -----------------------------------
//
// Pull in the whole message from the wire, then rip off
// the header and checksum.
//
// virtual
bool
Ookala::DevI2c::recvI2cPayload(
                       uint8_t *buf, uint32_t &bufLen,
                       bool strictSizeChecking /*=true*/,
                       uint32_t devId          /* = 0 */)
{
    int      checksum;
    uint32_t realId;
    uint8_t *pkt = NULL;

    if (!validDevId(devId, realId)) {
        setErrorString("Invalid device ID");
        return false;
    }

    pkt = new uint8_t[3+(bufLen)];
   
    if (recvI2cMsg(pkt, 3+(bufLen), realId) == false) {
        //mErrorString.assign("Message was unexpected size");
        delete[] pkt;
        return false;
    }
   
    if (mDebug) {
        FILE *fid = fopen(mDebugFile.c_str(), "w");
        if (fid) {
            uint8_t c = 0x6F;
            fwrite(&c, 1, 1, fid);
            fwrite(pkt, 3+(bufLen), 1, fid);
            fclose(fid);
        }
    }

    // Check that the number of bytes we're expecting is what we get
    if ((pkt[1] & (~0x80)) != static_cast<int>(bufLen)) {

        if (strictSizeChecking) {
            char buf[1024];

            sprintf(buf, "%d bytes recv, %d expected", 
                        static_cast<int>((pkt[1] & (~0x80))), bufLen);
            setErrorString( std::string("Unexpected number of bytes: ") + 
                                    std::string(buf));
            return false;
        }  

        bufLen = (pkt[1] & (~0x80));
    }
        
    // Copy the sucka
    memcpy(buf, &pkt[2], bufLen);

    // Recompute checksum of the data
    checksum = 0x50;
    for (uint32_t i=0; i<2+(bufLen); i++) {
        checksum = checksum ^ pkt[i];
    }

    if (checksum != pkt[2+(bufLen)]) {
        setErrorString("Checksum does not match.");
        delete[] pkt;
        return false;
    }

    delete[] pkt;

    return true;
}

// -----------------------------------
//
// Take a whole message and send it over the wire.
//
// virtual 
bool 
Ookala::DevI2c::sendI2cMsg(uint8_t *buf, uint32_t bufLen,
                                     uint32_t devId /* = 0 */)
{
    int      fd, retVal;
    uint32_t realId;
    uint32_t sleepUsec =  40000;

    if (!validDevId(devId, realId)) {
        setErrorString("Invalid device ID.");
        return false;
    }

    // Grab the fd we want to use
    std::map<uint32_t, int>::iterator theFd = mFds.find(realId);
    if (theFd == mFds.end()) {
        setErrorString("Invalid device ID.");
        return false;
    }

    fd = (*theFd).second;

    retVal = write(fd, buf, bufLen);
    if (retVal != (int)bufLen) {
        // We should wait ~40ms between events.
        usleep(sleepUsec);

        setErrorString("write() error");
        return false;
    }

    // We should wait ~40ms between events.
    usleep(sleepUsec);


    return true;
}

// -----------------------------------
//
// Read in a whole message from the wire.
//
// virtual 
bool 
Ookala::DevI2c::recvI2cMsg(uint8_t *buf, uint32_t bufLen,
                                         uint32_t devId /* = 0 */)
{
    int      fd;
    uint32_t realId;
    int      addr      = 0x37;
    uint32_t sleepUsec =  40000;

    if (!validDevId(devId, realId)) {
        setErrorString("Invalid device ID/");
        return false;
    }

    // Grab the fd we want to use
    std::map<uint32_t, int>::iterator theFd = mFds.find(realId);
    if (theFd == mFds.end()) {
        setErrorString("Invalid device ID.");
        return false;
    }

    fd = (*theFd).second;

    if (read(fd, buf, bufLen) != (ssize_t) bufLen) {
        setErrorString("read() error.");
        usleep(sleepUsec);
        return false;
    }
    
    // We should wait ~40ms between events.
    usleep(sleepUsec);

    // Check that the first frame matches
    if (buf[0] != (addr << 1)) {
        fprintf(stderr, 
                "DevI2c::recvI2cMsg() ERROR: First field of recv msg didn't match\n");
        setErrorString("First field of recv'd msg doesn't match address.");
        return false;
    }

    return true;
}

// -------------------------------------
//
// Enumerate displays. Perhaps this should happe on preRun?
//
// virtual protected
bool
Ookala::DevI2c::_run(PluginChain *chain)
{
    return enumerate();
}



