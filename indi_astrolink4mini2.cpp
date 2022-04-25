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
IndiAstroLink4mini2::IndiAstroLink4mini2() : FI(this)
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
            // FocuserSettingsNP.s = IPS_BUSY;
            return true;
        }
    }
    return false;
}

void IndiAstroLink4mini2::TimerHit()
{
    if (isConnected())
    {
        readDevice();
        SetTimer(POLLTIME);
    }
}

//////////////////////////////////////////////////////////////////////
/// Overrides
//////////////////////////////////////////////////////////////////////
bool IndiAstroLink4mini2::initProperties()
{
    INDI::DefaultDevice::initProperties();

    setDriverInterface(AUX_INTERFACE | FOCUSER_INTERFACE);

    FI::SetCapability(FOCUSER_CAN_ABS_MOVE |
                      FOCUSER_CAN_REL_MOVE |
                      FOCUSER_CAN_REVERSE |
                      FOCUSER_CAN_SYNC |
                      FOCUSER_CAN_ABORT |
                      FOCUSER_HAS_BACKLASH);

    FI::initProperties(FOCUS_TAB);    

    addDebugControl();
    addSimulationControl();
    addConfigurationControl();

    serialConnection = new Connection::Serial(this);
    serialConnection->registerHandshake([&]()
                                        { return Handshake(); });
    registerConnection(serialConnection);

    serialConnection->setDefaultPort("/dev/ttyUSB0");
    serialConnection->setDefaultBaudRate(serialConnection->B_38400);

    IUFillSwitch(&FocuserSelectS[0], "FOC_SEL_1", "Focuser 1", ISS_ON);
    IUFillSwitch(&FocuserSelectS[1], "FOC_SEL_2", "Focuser 2", ISS_OFF);
    IUFillSwitchVector(&FocuserSelectSP, FocuserSelectS, 2, getDeviceName(), "FOCUSER_SELECT", "Focuser select", FOC_SETTINGS_TAB, IP_RW, ISR_1OFMANY, 60, IPS_IDLE);


    return true;
}

bool IndiAstroLink4mini2::updateProperties()
{
    // Call parent update properties first
    INDI::DefaultDevice::updateProperties();

    if (isConnected())
    {
         FI::updateProperties();
         defineProperty(&FocuserSelectSP);
    }
    else
    {
        deleteProperty(FocuserSelectSP.name);
        FI::updateProperties();
    }

    return true;
}

bool IndiAstroLink4mini2::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{   
    if (dev && !strcmp(dev, getDeviceName()))
    {    
        if (strstr(name, "FOCUS"))
            return FI::processNumber(dev, name, values, names, n);    
    }
    return INDI::DefaultDevice::ISNewNumber(dev, name, values, names, n);
}

bool IndiAstroLink4mini2::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (dev && !strcmp(dev, getDeviceName()))
    {
        // Stepper select
        if (!strcmp(name, FocuserSelectSP.name))
        {
            //focuserIndex = (strcmp(FocuserSelectS[0].name, names[0])) ? 1 : 0;
            FocuserSelectSP.s = FocusMaxPosNP.s = FocusReverseSP.s = FocusAbsPosNP.s = IPS_BUSY;
            IUUpdateSwitch(&FocuserSelectSP, states, names, n);
            IDSetSwitch(&FocuserSelectSP, nullptr);
            IDSetSwitch(&FocusReverseSP, nullptr);
            IDSetNumber(&FocusMaxPosNP, nullptr);
            IDSetNumber(&FocusAbsPosNP, nullptr);
            return true;
        }

        if (strstr(name, "FOCUS"))
            return FI::processSwitch(dev, name, states, names, n);  
    }  
    return INDI::DefaultDevice::ISNewSwitch(dev, name, states, names, n);
}

bool IndiAstroLink4mini2::ISNewText(const char *dev, const char *name, char *texts[], char *names[], int n)
{
    return INDI::DefaultDevice::ISNewText(dev, name, texts, names, n);
}

bool IndiAstroLink4mini2::saveConfigItems(FILE *fp)
{
    IUSaveConfigSwitch(fp, &FocuserSelectSP);
    FI::saveConfigItems(fp);
    INDI::DefaultDevice::saveConfigItems(fp);
    return true;
}

//////////////////////////////////////////////////////////////////////
/// Focuser interface
//////////////////////////////////////////////////////////////////////
IPState IndiAstroLink4mini2::MoveAbsFocuser(uint32_t targetTicks)
{
    char cmd[ASTROLINK4_LEN] = {0}, res[ASTROLINK4_LEN] = {0};
    snprintf(cmd, ASTROLINK4_LEN, "R:%i:%u", getFocuserIndex(),  targetTicks);
    return (sendCommand(cmd, res)) ? IPS_BUSY : IPS_ALERT;
}

IPState IndiAstroLink4mini2::MoveRelFocuser(FocusDirection dir, uint32_t ticks)
{
    return MoveAbsFocuser(dir == FOCUS_INWARD ? FocusAbsPosN[0].value - ticks : FocusAbsPosN[0].value + ticks);
}

bool IndiAstroLink4mini2::AbortFocuser()
{
    char cmd[ASTROLINK4_LEN] = {0}, res[ASTROLINK4_LEN] = {0};
    snprintf(cmd, ASTROLINK4_LEN, "H:%i", getFocuserIndex());
    return (sendCommand(cmd, res));
}

bool IndiAstroLink4mini2::ReverseFocuser(bool enabled)
{
    int index = getFocuserIndex() > 0 ? U_FOC2_REV : U_FOC1_REV;
    if (updateSettings("u", "U", index, (enabled) ? "1" : "0"))
    {
        FocusReverseSP.s = IPS_BUSY;
        return true;
    }
    else
    {
        return false;
    }
}

bool IndiAstroLink4mini2::SyncFocuser(uint32_t ticks)
{
    char cmd[ASTROLINK4_LEN] = {0}, res[ASTROLINK4_LEN] = {0};
    snprintf(cmd, ASTROLINK4_LEN, "P:%i:%u", getFocuserIndex(), ticks);
    if (sendCommand(cmd, res))
    {
        FocusAbsPosNP.s = IPS_BUSY;
        return true;
    }
    else
    {
        return false;
    }
    
}

bool IndiAstroLink4mini2::SetFocuserMaxPosition(uint32_t ticks)
{
    int index = getFocuserIndex() > 0 ? U_FOC2_MAX : U_FOC1_MAX;
    if (updateSettings("u", "U", index, std::to_string(ticks).c_str()))
    {
        FocusMaxPosNP.s = IPS_BUSY;
        return true;
    }
    else
    {
        return false;
    }
}

bool IndiAstroLink4mini2::SetFocuserBacklash(int32_t steps)
{
    //backlashSteps = steps;
    return true;
}

bool IndiAstroLink4mini2::SetFocuserBacklashEnabled(bool enabled)
{
    //backlashEnabled = enabled;
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

bool IndiAstroLink4mini2::readDevice()
{
    char res[ASTROLINK4_LEN] = {0};
    if (sendCommand("q", res))
    {
        std::vector<std::string> result = split(res, ":");
        result.erase(result.begin());

        //DEBUGF(INDI::Logger::DBG_SESSION, "Selected %i", selectedFocuser);
        
        int focuserPosition = std::stoi(result[getFocuserIndex() == 1 ? Q_FOC2_POS : Q_FOC1_POS]);
        int stepsToGo = std::stod(result[getFocuserIndex() == 1 ? Q_FOC2_TO_GO : Q_FOC1_TO_GO]);
        FocusAbsPosN[0].value = focuserPosition;
        if (stepsToGo == 0)
        {
            FocusAbsPosNP.s = FocusRelPosNP.s = IPS_OK;
            IDSetNumber(&FocusRelPosNP, nullptr);
        }
        else
        {
            FocusAbsPosNP.s = FocusRelPosNP.s = IPS_BUSY;
        }        
        IDSetNumber(&FocusAbsPosNP, nullptr);
    }

    // update settings data if was changed
    if(FocusMaxPosNP.s != IPS_OK || FocusReverseSP.s != IPS_OK || FocuserSelectSP.s != IPS_OK)
    {
        if (sendCommand("u", res))
        {
            std::vector<std::string> result = split(res, ":");
            if (FocusMaxPosNP.s != IPS_OK)
            {
                int index = getFocuserIndex() > 0 ? U_FOC2_MAX : U_FOC1_MAX;
                FocusMaxPosN[0].value = std::stod(result[index]);
                FocusMaxPosNP.s = IPS_OK;
                IDSetNumber(&FocusMaxPosNP, nullptr);
            }    
            if (FocusReverseSP.s != IPS_OK)
            {
                int index = getFocuserIndex() > 0 ? U_FOC2_REV : U_FOC1_REV;
                FocusReverseS[0].s = (std::stoi(result[index]) > 0) ? ISS_ON : ISS_OFF;
                FocusReverseS[1].s = (std::stoi(result[index]) == 0) ? ISS_ON : ISS_OFF;
                FocusReverseSP.s = IPS_OK;
                IDSetSwitch(&FocusReverseSP, nullptr);
            }     
        }
        FocuserSelectSP.s = IPS_OK;
        IDSetSwitch(&FocuserSelectSP, nullptr);
    }
  

    return true;
}

//////////////////////////////////////////////////////////////////////
/// Helper functions
//////////////////////////////////////////////////////////////////////
std::vector<std::string> IndiAstroLink4mini2::split(const std::string &input, const std::string &regex)
{
    // passing -1 as the submatch index parameter performs splitting
    std::regex re(regex);
    std::sregex_token_iterator
        first{input.begin(), input.end(), re, -1},
        last;
    return {first, last};
}

std::string IndiAstroLink4mini2::doubleToStr(double val)
{
    char buf[10];
    sprintf(buf, "%.0f", val);
    return std::string(buf);
}

bool IndiAstroLink4mini2::updateSettings(const char *getCom, const char *setCom, int index, const char *value)
{
    std::map<int, std::string> values;
    values[index] = value;
    return updateSettings(getCom, setCom, values);
}

bool IndiAstroLink4mini2::updateSettings(const char *getCom, const char *setCom, std::map<int, std::string> values)
{
    char cmd[ASTROLINK4_LEN] = {0}, res[ASTROLINK4_LEN] = {0};
    snprintf(cmd, ASTROLINK4_LEN, "%s", getCom);
    if (sendCommand(cmd, res))
    {
        std::string concatSettings = "";
        std::vector<std::string> result = split(res, ":");
        if (result.size() >= values.size())
        {
            result[0] = setCom;
            for (std::map<int, std::string>::iterator it = values.begin(); it != values.end(); ++it)
                result[it->first] = it->second;

            for (const auto &piece : result)
                concatSettings += piece + ":";

            snprintf(cmd, ASTROLINK4_LEN, "%s", concatSettings.c_str());
            if (sendCommand(cmd, res))
                return true;
        }
    }
    return false;
}

int IndiAstroLink4mini2::getFocuserIndex()
{
    return FocuserSelectS[0].s == ISS_ON ? 0 : 1;
}

