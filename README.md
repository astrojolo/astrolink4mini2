![AstroLink 4 mini II Banner](https://shop.astrojolo.com/wp-content/uploads/sites/2/2023/01/astrolinks-banner-2023.jpg)

# AstroLink 4 mini II  

![ASCOM & INDI Drivers](https://img.shields.io/badge/Drivers-ASCOM%20%7C%20INDI-blue)  
![Dual Focuser](https://img.shields.io/badge/Focuser-Dual%20Motor-green)  
![PWM Dew Control](https://img.shields.io/badge/Dew_Control-PWM-orange)  
![Multiple Sensors](https://img.shields.io/badge/Sensors-Temp%2FHumidity%2FSky_Brightness-lightgrey)  

---

## Overview  
AstroLink 4 mini II is a versatile controller for astrophotography setups, designed to give you more control and automation in a compact form. It supports **two focusing motors** (e.g. main focuser + guider or mirror-knob), enabling fine adjustments of your imaging system. The device features multiple 12V DC switchable outputs plus a **regulated voltage output** for accessories needing lower voltage. Dew cap heaters or cooling fans are managed via **two PWM regulated outputs**. Several environmental sensors are supported: temperature, humidity, sky brightness, and sky temperature/cloud coverage. The system also includes features for **temperature-based focuser compensation**, keeping your focus sharp as conditions change. You can control the unit using dedicated AstroLink software, or via **ASCOM** and **INDI drivers**, making it compatible with most astronomy workflows. There is built-in monitoring of input voltage, current draw, and other safety features to protect your gear. It’s also designed to work well even in remote or unattended setups.  

---

## 🔧 Key Features  
- Dual motor focuser control (stepper unipolar / bipolar)  
- Multiple 12V DC switchable outputs + regulated 3-10V auxiliary output  
- Two PWM-regulated outputs for dew heaters / fans  
- Environmental sensors: temperature, humidity, sky brightness & optional sky temperature/cloud sensors  
- Temperature-based focuser compensation and focus memory  
- ASCOM & INDI driver support + dedicated control panel  
- Safety monitoring: voltage, current, over-load protection
- QuickCharge USB port
- Compact form factor, reliable operation for remote or long-duration imaging  


## Device
https://shop.astrojolo.com/products-overview/astrolink-4-mini-ii/


# AstroLink 4 mini II driver installation
## Requirements
* INDI http://indilib.org/download.html


### Required packages
```
sudo apt update
sudo apt install git build-essential cmake libindi-dev
```

### INDI driver installation
```
git clone https://github.com/astrojolo/astrolink4mini2
cd astrolink4mini2
mkdir build && cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr ..
make
```
You can install the drivers by running:
```
sudo make install
```
After these steps AstroLink 4 mini II driver will be visible in the Aux devices lists under **Astrojolo** group.
