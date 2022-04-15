# astrolink4mini2
INDI driver for AstroLink 4 mini II device

# Installing INDI server and libraries
To start you need to download and install INDI environment. See [INDI page](http://indilib.org/download.html) for details. 

Then AstroLink 4 mini II INDI driver needs to be fetched and installed:

```
git clone https://github.com/astrojolo/astrolink4mini2.git
cd astrolink4mini2
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr ..
make
sudo make install
```

Then indiserver needs to be started with AstroLink 4 mini II drivers:

```
indiserver -v indi_astrolink4mini2
```

Now AstroLink 4 mini II can be used with any software that supports INDI drivers, like KStars with Ekos.
