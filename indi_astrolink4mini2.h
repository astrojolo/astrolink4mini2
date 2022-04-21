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

#ifndef ASTROLINK4_H
#define ASTROLINK4_H

#include <string>
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <memory>
#include <regex>
#include <cstring>
#include <map>
#include <sstream>

#include <defaultdevice.h>
#include <indifocuserinterface.h>
#include <indiweatherinterface.h>
#include <connectionplugins/connectionserial.h>

#define Q_DEVICE_CODE       0
#define Q_FOC1_POS		    1
#define Q_FOC1_TO_GO		2
#define Q_FOC2_POS          3
#define Q_FOC2_TO_GO        4
#define Q_ITOT              5
#define Q_SENS1_PRESENT     6
#define Q_SENS1_TEMP        7
#define Q_SENS1_HUM         8
#define Q_SENS1_DEW         9
#define Q_SENS2_PRESENT     10
#define Q_SENS2_TEMP        11
#define Q_PWM1              12
#define Q_PWM2              13
#define Q_OUT1              14
#define Q_OUT2              15
#define Q_OUT3              16
#define Q_VIN               17
#define Q_VREG              18
#define Q_AH                19
#define Q_WH                20
#define Q_FOC1_COMP         21
#define Q_FOC2_COMP         22
#define Q_OVERTYPE          23
#define Q_OVERVALUE         24
#define Q_MLX_TEMP          25
#define Q_MLX_AUX           26
#define Q_SENS2_HUM         30
#define Q_SENS2_DEW         31
#define Q_SBM_PRESENT       32
#define Q_SBM               33

#define U_BUZZER            0
#define U_MANUAL            1
#define U_FOC1_CUR          2
#define U_FOC2_CUR          3
#define U_FOC1_HOLD         4
#define U_FOC2_HOLD         5
#define U_FOC1_SPEED        6
#define U_FOC2_SPEED        7
#define U_FOC1_ACC          8
#define U_FOC2_ACC          9
#define U_FOC1_MODE         10
#define U_FOC2_MODE         11
#define U_FOC1_MAX          12
#define U_FOC2_MAX          13
#define U_FOC1_REV          14
#define U_FOC2_REV          15
#define U_FOC1_STEP         16
#define U_FOC2_STEP         17
#define U_FOC1_COMPSTEPS    18
#define U_FOC2_COMPSTEPS    19
#define U_FOC_COMP_CYCLE    20
#define U_FOC1_COMPTRIGGER  21
#define U_FOC2_COMPTRIGGER  22
#define U_FOC1_COMPAUTO     23
#define U_FOC2_COMPAUTO     24
#define U_PWM_PRESC         25
#define U_OUT1_DEF          26
#define U_OUT2_DEF          27
#define U_OUT3_DEF          28
#define U_PWM1_DEF          29
#define U_PWM2_DEF          30
#define U_HUM_SENSOR        31
#define U_HUM_START         32
#define U_HUM_FULL          33
#define U_TEMP_PRESET       34
#define U_VREF              35
#define U_OVERVOLTAGE       36
#define U_OVERCURRENT       37
#define U_OVERTIME          38
#define U_COMPSENSOR        39

#define POLLTIME            500

namespace Connection
{
class Serial;
}

class IndiAstroLink4mini2 : public INDI::DefaultDevice, public INDI::FocuserInterface, public INDI::WeatherInterface
{

public:
    IndiAstroLink4mini2();
    virtual bool initProperties();
    virtual bool updateProperties();
	
    virtual bool ISNewNumber (const char *dev, const char *name, double values[], char *names[], int n);
    virtual bool ISNewSwitch (const char *dev, const char *name, ISState *states, char *names[], int n);
    virtual bool ISNewText(const char * dev, const char * name, char * texts[], char * names[], int n);
	
protected:
    virtual const char *getDefaultName();
    virtual void TimerHit();
    virtual bool saveConfigItems(FILE *fp);
    virtual bool sendCommand(const char * cmd, char * res);

    // Focuser Overrides
    virtual IPState MoveAbsFocuser(uint32_t targetTicks) override;
    virtual IPState MoveRelFocuser(FocusDirection dir, uint32_t ticks) override;
    virtual bool AbortFocuser() override;
    virtual bool ReverseFocuser(bool enabled) override;
    virtual bool SyncFocuser(uint32_t ticks) override;

    virtual bool SetFocuserBacklash(int32_t steps) override;
    virtual bool SetFocuserBacklashEnabled(bool enabled) override;
    virtual bool SetFocuserMaxPosition(uint32_t ticks) override;

    // Weather Overrides
    virtual IPState updateWeather() override
    {
        return IPS_OK;
    }

	
private:
    virtual bool Handshake();
    int PortFD = -1;
    Connection::Serial *serialConnection { nullptr };
    bool updateSettings(const char * getCom, const char * setCom, int index, const char * value);
    bool updateSettings(const char * getCom, const char * setCom, std::map<int, std::string> values);
    std::vector<std::string> split(const std::string &input, const std::string &regex);
    std::string doubleToStr(double val);
    bool sensorRead();
    int32_t calculateBacklash(uint32_t targetTicks);
    char stopChar { 0xA };	// new line
    bool backlashEnabled = false;
    int32_t backlashSteps = 0;
    bool requireBacklashReturn = false;

    INumber FocusPosMMN[1];
    INumberVectorProperty FocusPosMMNP;

    INumber CompensationValueN[1];
    INumberVectorProperty CompensationValueNP;
    ISwitch CompensateNowS[1];
    ISwitchVectorProperty CompensateNowSP;

    INumber FocuserSettingsN[5];
    INumberVectorProperty FocuserSettingsNP;
    enum
    {
        FS_STEP_SIZE, FS_COMPENSATION, FS_COMP_THRESHOLD
    };

    ISwitch FocuserCompModeS[2];
    ISwitchVectorProperty FocuserCompModeSP;
    enum
    {
    FS_COMP_AUTO, FS_COMP_MANUAL
    };
    
    ISwitch FocuserManualS[2];
    ISwitchVectorProperty FocuserManualSP;
    enum
    {
    FS_MANUAL_ON, FS_MANUAL_OFF
    };

    ISwitch BuzzerS[1];
    ISwitchVectorProperty BuzzerSP;
    
    static constexpr const char *ENVIRONMENT_TAB {"Environment"};
    static constexpr const char *SETTINGS_TAB {"Settings"};
};

#endif
