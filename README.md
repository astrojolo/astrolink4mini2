## Device
https://shop.astrojolo.com/astrolink-4-controllers/


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