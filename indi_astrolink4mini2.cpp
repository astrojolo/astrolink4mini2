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

#define ASTROLINK4_LEN 100
#define ASTROLINK4_TIMEOUT 3

//////////////////////////////////////////////////////////////////////
/// Delegates
//////////////////////////////////////////////////////////////////////
std::unique_ptr<IndiAstroLink4mini2> indiFocuserLink(new IndiAstroLink4mini2());

void ISGetProperties(const char *dev)
{
    indiFocuserLink->ISGetProperties(dev);
}
void ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int num)
{
    indiFocuserLink->ISNewSwitch(dev, name, states, names, num);
}
void ISNewText(const char *dev, const char *name, char *texts[], char *names[], int num)
{
    indiFocuserLink->ISNewText(dev, name, texts, names, num);
}
void ISNewNumber(const char *dev, const char *name, double values[], char *names[], int num)
{
    indiFocuserLink->ISNewNumber(dev, name, values, names, num);
}
void ISNewBLOB(const char *dev, const char *name, int sizes[], int blobsizes[], char *blobs[], char *formats[], char *names[], int num)
{
    indiFocuserLink->ISNewBLOB(dev, name, sizes, blobsizes, blobs, formats, names, num);
}
void ISSnoopDevice(XMLEle *root)
{
    indiFocuserLink->ISSnoopDevice(root);
}

//////////////////////////////////////////////////////////////////////
///Constructor
//////////////////////////////////////////////////////////////////////
IndiAstroLink4mini2::IndiAstroLink4mini2() : FI(this), WI(this)
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
            LOG_ERROR("Device not recognized.");
            return false;
        }
        else
        {
            LOGF_ERROR("Serial error: %s", "settimer");
            SetTimer(500);
            return true;
        }
    }
    return false;
}

void IndiAstroLink4mini2::TimerHit()
{
    if (isConnected())
    {
        sensorRead();
        SetTimer(500);
    }
}

//////////////////////////////////////////////////////////////////////
/// Overrides
//////////////////////////////////////////////////////////////////////
bool IndiAstroLink4mini2::initProperties()
{
    INDI::DefaultDevice::initProperties();

    setDriverInterface(AUX_INTERFACE | FOCUSER_INTERFACE | WEATHER_INTERFACE);

    FI::SetCapability(FOCUSER_CAN_ABS_MOVE |
                      FOCUSER_CAN_REL_MOVE |
                      FOCUSER_CAN_REVERSE |
                      FOCUSER_CAN_SYNC |
                      FOCUSER_CAN_ABORT |
                      FOCUSER_HAS_BACKLASH);

    FI::initProperties(FOCUS_TAB);
    WI::initProperties(ENVIRONMENT_TAB, ENVIRONMENT_TAB);

    addDebugControl();
    addSimulationControl();
    addConfigurationControl();

    serialConnection = new Connection::Serial(this);
    serialConnection->registerHandshake([&]()
                                        { return Handshake(); });
    registerConnection(serialConnection);

    serialConnection->setDefaultPort("/dev/ttyUSB0");
    serialConnection->setDefaultBaudRate(serialConnection->B_38400);

    // focuser settings
    IUFillNumber(&FocuserSettingsN[FS_STEP_SIZE], "FS_STEP_SIZE", "Step size [um]", "%.2f", 0, 100, 0.1, 5.0);
    IUFillNumber(&FocuserSettingsN[FS_COMPENSATION], "FS_COMPENSATION", "Compensation [steps/C]", "%.2f", -1000, 1000, 1, 0);
    IUFillNumber(&FocuserSettingsN[FS_COMP_THRESHOLD], "FS_COMP_THRESHOLD", "Compensation threshold [steps]", "%.0f", 1, 1000, 10, 10);
    IUFillNumberVector(&FocuserSettingsNP, FocuserSettingsN, 3, getDeviceName(), "FOCUSER_SETTINGS", "Focuser settings", SETTINGS_TAB, IP_RW, 60, IPS_IDLE);

    IUFillSwitch(&FocuserCompModeS[FS_COMP_AUTO], "FS_COMP_AUTO", "AUTO", ISS_OFF);
    IUFillSwitch(&FocuserCompModeS[FS_COMP_MANUAL], "FS_COMP_MANUAL", "MANUAL", ISS_ON);
    IUFillSwitchVector(&FocuserCompModeSP, FocuserCompModeS, 2, getDeviceName(), "COMP_MODE", "Compensation mode", SETTINGS_TAB, IP_RW, ISR_1OFMANY, 60, IPS_IDLE);

    IUFillSwitch(&FocuserManualS[FS_MANUAL_ON], "FS_MANUAL_ON", "ON", ISS_ON);
    IUFillSwitch(&FocuserManualS[FS_MANUAL_OFF], "FS_MANUAL_OFF", "OFF", ISS_OFF);
    IUFillSwitchVector(&FocuserManualSP, FocuserManualS, 2, getDeviceName(), "MANUAL_CONTROLLER", "Hand controller", SETTINGS_TAB, IP_RW, ISR_1OFMANY, 60, IPS_IDLE);

    // focuser compensation
    IUFillNumber(&CompensationValueN[0], "COMP_VALUE", "Compensation steps", "%.0f", -10000, 10000, 1, 0);
    IUFillNumberVector(&CompensationValueNP, CompensationValueN, 1, getDeviceName(), "COMP_STEPS", "Compensation steps", FOCUS_TAB, IP_RO, 60, IPS_IDLE);

    IUFillSwitch(&CompensateNowS[0], "COMP_NOW", "Compensate now", ISS_OFF);
    IUFillSwitchVector(&CompensateNowSP, CompensateNowS, 1, getDeviceName(), "COMP_NOW", "Compensate now", FOCUS_TAB, IP_RW, ISR_ATMOST1, 60, IPS_IDLE);

    IUFillNumber(&FocusPosMMN[0], "FOC_POS_MM", "Position [mm]", "%.3f", 0.0, 200.0, 0.001, 0.0);
    IUFillNumberVector(&FocusPosMMNP, FocusPosMMN, 1, getDeviceName(), "FOC_POS_MM", "Position [mm]", FOCUS_TAB, IP_RO, 60, IPS_IDLE);

    // Environment Group
    addParameter("WEATHER_TEMPERATURE", "Temperature (C)", -15, 35, 15);
    addParameter("WEATHER_HUMIDITY", "Humidity %", 0, 100, 15);
    addParameter("WEATHER_DEWPOINT", "Dew Point (C)", 0, 100, 15);

    return true;
}

bool IndiAstroLink4mini2::updateProperties()
{
    // Call parent update properties first
    INDI::DefaultDevice::updateProperties();

    if (isConnected())
    {
        defineProperty(&FocusPosMMNP);
        FI::updateProperties();
        WI::updateProperties();
        defineProperty(&FocuserSettingsNP);
        defineProperty(&FocuserCompModeSP);
        defineProperty(&FocuserManualSP);
        defineProperty(&CompensationValueNP);
        defineProperty(&CompensateNowSP);
    }
    else
    {
        deleteProperty(FocuserSettingsNP.name);
        deleteProperty(CompensateNowSP.name);
        deleteProperty(CompensationValueNP.name);
        deleteProperty(FocuserCompModeSP.name);
        deleteProperty(FocuserManualSP.name);
        deleteProperty(FocusPosMMNP.name);
        FI::updateProperties();
        WI::updateProperties();
    }

    return true;
}

bool IndiAstroLink4mini2::ISNewNumber(const char *dev, const char *name, double values[], char *names[], int n)
{
    if (dev && !strcmp(dev, getDeviceName()))
    {
        char cmd[ASTROLINK4_LEN] = {0};
        char res[ASTROLINK4_LEN] = {0};

        // Focuser settings
        if (!strcmp(name, FocuserSettingsNP.name))
        {
            bool allOk = true;
            std::map<int, std::string> updates;
            updates[U_FOC1_STEP] = doubleToStr(values[FS_STEP_SIZE] * 100.0);
            updates[U_FOC_COMP_CYCLE] = "30"; // cycle [s]
            updates[U_FOC1_COMPSTEPS] = doubleToStr(values[FS_COMPENSATION] * 100.0);
            updates[U_FOC1_COMPTRIGGER] = doubleToStr(values[FS_COMP_THRESHOLD]);
            allOk = allOk && updateSettings("u", "U", updates);
            updates.clear();
            if (allOk)
            {
                FocuserSettingsNP.s = IPS_BUSY;
                IUUpdateNumber(&FocuserSettingsNP, values, names, n);
                IDSetNumber(&FocuserSettingsNP, nullptr);
                LOG_INFO(values[FS_COMPENSATION] > 0 ? "Temperature compensation is enabled." : "Temperature compensation is disabled.");
                return true;
            }
            FocuserSettingsNP.s = IPS_ALERT;
            return true;
        }

        if (strstr(name, "FOCUS_"))
            return FI::processNumber(dev, name, values, names, n);
        if (strstr(name, "WEATHER_"))
            return WI::processNumber(dev, name, values, names, n);
    }

    return INDI::DefaultDevice::ISNewNumber(dev, name, values, names, n);
}

bool IndiAstroLink4mini2::ISNewSwitch(const char *dev, const char *name, ISState *states, char *names[], int n)
{
    if (dev && !strcmp(dev, getDeviceName()))
    {
        char cmd[ASTROLINK4_LEN] = {0};
        char res[ASTROLINK4_LEN] = {0};

        // compensate now
        if (!strcmp(name, CompensateNowSP.name))
        {
            sprintf(cmd, "S:%d", static_cast<uint16_t>(FocuserSettingsN[FS_COMP_THRESHOLD].value));
            bool allOk = sendCommand(cmd, res);
            CompensateNowSP.s = allOk ? IPS_BUSY : IPS_ALERT;
            if (allOk)
                IUUpdateSwitch(&CompensateNowSP, states, names, n);

            IDSetSwitch(&CompensateNowSP, nullptr);
            return true;
        }

        // Manual mode
        if (!strcmp(name, FocuserManualSP.name))
        {
            sprintf(cmd, "F:%s", (strcmp(FocuserManualS[0].name, names[0])) ? "0" : "1");
            if (sendCommand(cmd, res))
            {
                FocuserManualSP.s = IPS_BUSY;
                IUUpdateSwitch(&FocuserManualSP, states, names, n);
                IDSetSwitch(&FocuserManualSP, nullptr);
                return true;
            }
            FocuserManualSP.s = IPS_ALERT;
            return true;
        }

        // Focuser compensation mode
        if (!strcmp(name, FocuserCompModeSP.name))
        {
            std::string value = "0";
            if (!strcmp(FocuserCompModeS[FS_COMP_AUTO].name, names[0]))
                value = "1";
            if (updateSettings("u", "U", U_FOC1_COMPAUTO, value.c_str()))
            {
                FocuserCompModeSP.s = IPS_BUSY;
                IUUpdateSwitch(&FocuserCompModeSP, states, names, n);
                IDSetSwitch(&FocuserCompModeSP, nullptr);
                return true;
            }
            FocuserCompModeSP.s = IPS_ALERT;
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
    INDI::DefaultDevice::saveConfigItems(fp);
    FI::saveConfigItems(fp);

    return true;
}

//////////////////////////////////////////////////////////////////////
/// Focuser interface
//////////////////////////////////////////////////////////////////////
IPState IndiAstroLink4mini2::MoveAbsFocuser(uint32_t targetTicks)
{
    int32_t backlash = 0;
    if (backlashEnabled)
    {
        if ((targetTicks > FocusAbsPosN[0].value) == (backlashSteps > 0))
        {
            if ((targetTicks + backlash) < 0 || (targetTicks + backlash) > FocusMaxPosN[0].value)
            {
                backlash = 0;
            }
            else
            {
                backlash = backlashSteps;
                requireBacklashReturn = true;
            }
        }
    }
    char cmd[ASTROLINK4_LEN] = {0}, res[ASTROLINK4_LEN] = {0};
    snprintf(cmd, ASTROLINK4_LEN, "R:0:%u", targetTicks + backlash);
    return (sendCommand(cmd, res)) ? IPS_BUSY : IPS_ALERT;
}

IPState IndiAstroLink4mini2::MoveRelFocuser(FocusDirection dir, uint32_t ticks)
{
    return MoveAbsFocuser(dir == FOCUS_INWARD ? FocusAbsPosN[0].value - ticks : FocusAbsPosN[0].value + ticks);
}

bool IndiAstroLink4mini2::AbortFocuser()
{
    char res[ASTROLINK4_LEN] = {0};
    return (sendCommand("H", res));
}

bool IndiAstroLink4mini2::ReverseFocuser(bool enabled)
{
    return updateSettings("u", "U", U_FOC1_REV, (enabled) ? "1" : "0");
}

bool IndiAstroLink4mini2::SyncFocuser(uint32_t ticks)
{
    char cmd[ASTROLINK4_LEN] = {0}, res[ASTROLINK4_LEN] = {0};
    snprintf(cmd, ASTROLINK4_LEN, "P:%u", ticks);
    return sendCommand(cmd, res);
}

bool IndiAstroLink4mini2::SetFocuserMaxPosition(uint32_t ticks)
{
    if (updateSettings("u", "U", U_FOC1_MAX, std::to_string(ticks).c_str()))
    {
        FocuserSettingsNP.s = IPS_BUSY;
        return true;
    }
    else
    {
        return false;
    }
}

bool IndiAstroLink4mini2::SetFocuserBacklash(int32_t steps)
{
    backlashSteps = steps;
    return true;
}

bool IndiAstroLink4mini2::SetFocuserBacklashEnabled(bool enabled)
{
    backlashEnabled = enabled;
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
        LOGF_DEBUG("CMD %s", command);
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
        LOGF_DEBUG("RES %s", res);
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

//////////////////////////////////////////////////////////////////////
/// Sensors
//////////////////////////////////////////////////////////////////////
bool IndiAstroLink4mini2::sensorRead()
{
    char res[ASTROLINK4_LEN] = {0};
    if (sendCommand("q", res))
    {
        LOGF_ERROR("Serial error: %s", res);
        std::vector<std::string> result = split(res, ":");
        result.erase(result.begin());

        float focuserPosition = std::stod(result[Q_FOC1_POS]);
        FocusAbsPosN[0].value = focuserPosition;
        FocusPosMMN[0].value = focuserPosition * FocuserSettingsN[FS_STEP_SIZE].value / 1000.0;
        float stepsToGo = std::stod(result[Q_FOC1_TO_GO]);
        if (stepsToGo == 0)
        {
            if (requireBacklashReturn)
            {
                requireBacklashReturn = false;
                MoveAbsFocuser(focuserPosition - backlashSteps);
            }
            FocusAbsPosNP.s = FocusRelPosNP.s = FocusPosMMNP.s = IPS_OK;
            IDSetNumber(&FocusRelPosNP, nullptr);
        }
        else
        {
            FocusAbsPosNP.s = FocusRelPosNP.s = FocusPosMMNP.s = IPS_BUSY;
        }
        IDSetNumber(&FocusPosMMNP, nullptr);
        IDSetNumber(&FocusAbsPosNP, nullptr);

        if (result.size() > 5)
        {
            if (std::stod(result[Q_SENS1_PRESENT]) > 0)
            {
                setParameterValue("WEATHER_TEMPERATURE", std::stod(result[Q_SENS1_TEMP]));
                setParameterValue("WEATHER_HUMIDITY", std::stod(result[Q_SENS1_HUM]));
                setParameterValue("WEATHER_DEWPOINT", std::stod(result[Q_SENS1_DEW]));
                ParametersNP.s = IPS_OK;
                IDSetNumber(&ParametersNP, nullptr);
            }
            else
            {
                ParametersNP.s = IPS_IDLE;
            }

            CompensationValueN[0].value = std::stod(result[Q_FOC1_COMP]);
            CompensateNowSP.s = CompensationValueNP.s = (CompensationValueN[0].value > 0) ? IPS_OK : IPS_IDLE;
            CompensateNowS[0].s = (CompensationValueN[0].value != 0) ? ISS_OFF : ISS_ON;
            IDSetNumber(&CompensationValueNP, nullptr);
            IDSetSwitch(&CompensateNowSP, nullptr);
        }
    }

    // update settings data if was changed
    if (FocuserSettingsNP.s != IPS_OK || FocuserCompModeSP.s != IPS_OK)
    {
        if (sendCommand("u", res))
        {
            std::vector<std::string> result = split(res, ":");

            FocuserSettingsN[FS_STEP_SIZE].value = std::stod(result[U_FOC1_STEP]) / 100.0;
            FocuserSettingsN[FS_COMPENSATION].value = std::stod(result[U_FOC1_COMPSTEPS]) / 100.0;
            FocuserSettingsN[FS_COMP_THRESHOLD].value = std::stod(result[U_FOC1_COMPTRIGGER]);
            FocusMaxPosN[0].value = std::stod(result[U_FOC1_MAX]);
            FocuserSettingsNP.s = IPS_OK;

            FocuserCompModeS[FS_COMP_MANUAL].s = (std::stod(result[U_FOC1_COMPAUTO]) == 0) ? ISS_ON : ISS_OFF;
            FocuserCompModeS[FS_COMP_AUTO].s = (std::stod(result[U_FOC1_COMPAUTO]) > 0) ? ISS_ON : ISS_OFF;
            FocuserCompModeSP.s = IPS_OK;

            IDSetSwitch(&FocuserCompModeSP, nullptr);
            IDSetNumber(&FocuserSettingsNP, nullptr);
            IDSetNumber(&FocusMaxPosNP, nullptr);
        }
    }

    if (FocuserManualSP.s != IPS_OK)
    {
        if (sendCommand("f", res))
        {
            std::vector<std::string> result = split(res, ":");
            FocuserManualS[FS_MANUAL_OFF].s = (std::stod(result[1]) == 0) ? ISS_ON : ISS_OFF;
            FocuserManualS[FS_MANUAL_ON].s = (std::stod(result[1]) > 0) ? ISS_ON : ISS_OFF;
            FocuserManualSP.s = IPS_OK;
            IDSetSwitch(&FocuserManualSP, nullptr);
        }
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
