# TWatch_2020_V1
my project for ttgo-watch2020-v1 based on ESP-IDF (v 4.1.1-drity)
# TWatch_UFO94
Simple watch based on T-Watch TTGO 2020 V1.
The screen displays the clock, date, battery level and current consumption.
A globe from the game X-COM: UFO Defense (file world.dat: https://www.ufopaedia.org/index.php/WORLD.DAT) is drawn in the background. Globe lighting is calculated based on the current time and date. You can rotate the ball with the touchscreen. After 20 seconds of inactivity (no touching the touchscreen), the watch goes into sleep mode. Entering / exiting sleep mode - pressing the button.
Configuration (setting date, time, backlight level, etc.) is performed via the PC console.
Screen watch:

![Screen watch](https://github.com/FLY-FGV/TWatch_2020_V1/blob/main/img/4.png)

Configure from termite:

![Screen term](https://github.com/FLY-FGV/TWatch_2020_V1/blob/main/img/termite.png)

Files in project:

spi_master_example_main.c - main cycle, init spi etc

axp202.c - auxiliary functions for working with clock PCF8563 and power AXP202

ft6336.c - auxiliary functions for working with touchscreen

uearth.c - earth project & draw function.

WORLD.DAT - globe geo data file from X-COM: UFO Defense

1) in setup call earth_unpack() once.  This function prepare internal buffer's and unpack coordinates from lat,lon to 3D - x,y,z.

2) in main cycle:

call updateP_setnewpointview(lat in radian,lon in radian) to set view point earth

call set_sunA(Azimuth in rad,Height in rad) to set vector of sun light

call setRearth(Radius in pix) to change or set new radius of earth

call earth_proj_to_scr() to project earth to internal buffer

if need apply sun light call apply_sun() to calculate light

in cycle writes to screen call uint16_t earth_getcolor(int16_t x,int16_t y) to get color codes of rendered image.

Most computes in earth.c make to fixed point. Base projection matrix is 1024. Power of 2 base 3D coodinates earth polygones defined by UFO_FIX_POINT_WDT.
WORLD.DAT describes only ground. Sea and ocean draw before as sphere witch solid color. 
Codes screen orientation:

Screen orientation code:

![Screen term](https://github.com/FLY-FGV/TWatch_2020_V1/blob/main/img/scr_orientation.png)
