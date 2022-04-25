/*******************************************************************************
 Copyright(c) 2022 astrojolo.com
 .
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License version 2 as published by the Free Software Foundation.
 .
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.
 .
 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
*******************************************************************************/
#include "indi_astrolink4mini2.h"

#include "indicom.h"

#define VERSION_MAJOR 0
#define VERSION_MINOR 2

#define ASTROLINK4_LEN 200
#define ASTROLINK4_TIMEOUT 3

#define POLLTIME 500

//////////////////////////////////////////////////////////////////////
/// Delegates
//////////////////////////////////////////////////////////////////////
std::unique_ptr<IndiAstroLink4mini2> indiFocuserLink(new IndiAstroLink4mini2());

//////////////////////////////////////////////////////////////////////
///Constructor
//////////////////////////////////////////////////////////////////////
IndiAstroLink4mini2::IndiAstroLink4mini2()
{
    setVersion(VERSION_MAJOR, VERSION_MINOR);
}

const char *IndiAstroLink4mini2::getDefaultName()
{
    return (char *)"AstroLink 4 mini II";
}

//////////////////////////////////////////////////////////////////////
/// Communication
//////////////////////////////////////////////////////////////////////
bool IndiAstroLink4mini2::Handshake()
{
    PortFD = serialConnection->getPortFD();

    char res[ASTROLINK4_LEN] = {0};
    if (sendCommand("#", res))
    {
        if (strncmp(res, "#:AstroLink4mini", 16) != 0)
        {
            DEBUG(INDI::Logger::DBG_ERROR, "Device not recognized.");
            return false;
        }
        else
        {
            SetTimer(POLLTIME);
            // Require update
            FocuserSettingsNP.s = IPS_BUSY;
            return true;
        }
    }
    return false;
}

void IndiAstroLink4mini2::TimerHit()
{
    if (isConnected())
    {
        SetTimer(POLLTIME);
    }
}

//////////////////////////////////////////////////////////////////////
/// Overrides
//////////////////////////////////////////////////////////////////////
bool IndiAstroLink4mini2::initProperties()
{
    INDI::DefaultDevice::initProperties();

    setDriverInterface(AUX_INTERFACE);

    addDebugControl();
    addSimulationControl();
    addConfigurationControl();

    serialConnection = new Connection::Serial(this);
    serialConnection->registerHandshake([&]()
                                        { return Handshake(); });
    registerConnection(serialConnection);

    serialConnection->setDefaultPort("/dev/ttyUSB0");
    serialConnection->setDefaultBaudRate(serialConnection->B_38400);

    return true;
}

bool IndiAstroLink4mini2::updateProperties()
{
    // Call parent update properties first
    INDI::DefaultDevice::updateProperties();

    if (isConnected())
    {
         //
    }
    else
    {
        //
    }

    return true;
}

bool IndiAstroLink4mini2::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{   
    return INDI::DefaultDevice::ISNewNumber(dev, name, values, names, n);
}

bool IndiAstroLink4mini2::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    return INDI::DefaultDevice::ISNewSwitch(dev, name, states, names, n);
}

bool IndiAstroLink4mini2::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    return INDI::DefaultDevice::ISNewText(dev, name, texts, names, n);
}

bool IndiAstroLink4mini2::saveConfigItems(FILE *fp)
{
    INDI::DefaultDevice::saveConfigItems(fp);
    return true;
}


//////////////////////////////////////////////////////////////////////
/// Serial commands
//////////////////////////////////////////////////////////////////////
bool IndiAstroLink4mini2::sendCommand(const char *cmd, char *res)
{
    int nbytes_read = 0, nbytes_written = 0, tty_rc = 0;
    char command[ASTROLINK4_LEN];

    if (isSimulation())
    {
        if(strncmp(cmd, "#", 1) == 0) sprintf(res, "%s\n", "#:AstroLink4mini");
        if(strncmp(cmd, "q", 1) == 0) sprintf(res, "%s\n", "q:AL4MII:1234:0:5678:0:3.14:1:23.12:45:9.11:1:19.19:35:80:1:0:1:12.11:7.62:20.01:132.11:33:0:0:0:1:-10.1:7.7:1:19.19:35:8.22:1:1:18.11");
        if(strncmp(cmd, "p", 1) == 0) sprintf(res, "%s\n", "p:1234");
        if(strncmp(cmd, "i", 1) == 0) sprintf(res, "%s\n", "i:0");
        if(strncmp(cmd, "u", 1) == 0) sprintf(res, "%s\n", "u:1:1:80:120:30:50:200:800:200:800:0:2:10000:80000:0:0:50:18:30:15:5:10:10:0:1:0:0:0:0:0:0:0:40:90:10:1100:14000:10000:100:0");
        if(strncmp(cmd, "A", 1) == 0) sprintf(res, "%s\n", "A:4.5.0 mini II");
        if(strncmp(cmd, "R", 1) == 0) sprintf(res, "%s\n", "R:");
        if(strncmp(cmd, "C", 1) == 0) sprintf(res, "%s\n", "C:");
        if(strncmp(cmd, "B", 1) == 0) sprintf(res, "%s\n", "B:");
        if(strncmp(cmd, "H", 1) == 0) sprintf(res, "%s\n", "H:");
        if(strncmp(cmd, "P", 1) == 0) sprintf(res, "%s\n", "P:");
        if(strncmp(cmd, "U", 1) == 0) sprintf(res, "%s\n", "U:");
        if(strncmp(cmd, "S", 1) == 0) sprintf(res, "%s\n", "S:");
    }
    else
    {
        tcflush(PortFD, TCIOFLUSH);
        sprintf(command, "%s\n", cmd);
        DEBUGF(INDI::Logger::DBG_DEBUG, "CMD %s", cmd);
        if ((tty_rc = tty_write_string(PortFD, command, &nbytes_written)) != TTY_OK)
            return false;

         if (!res)
        {
            tcflush(PortFD, TCIOFLUSH);
            return true;
        }

        if ((tty_rc = tty_nread_section(PortFD, res, ASTROLINK4_LEN, stopChar, ASTROLINK4_TIMEOUT, &nbytes_read)) != TTY_OK || nbytes_read == 1)
            return false;

        tcflush(PortFD, TCIOFLUSH);
        res[nbytes_read - 1] = '\0';
        DEBUGF(INDI::Logger::DBG_DEBUG, "RES %s", res);
        if (tty_rc != TTY_OK)
        {
            char errorMessage[MAXRBUF];
            tty_error_msg(tty_rc, errorMessage, MAXRBUF);
            LOGF_ERROR("Serial error: %s", errorMessage);
            return false;
        }
    }
    return (cmd[0] == res[0]);
}
