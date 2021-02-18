# TWatch_2020_V1
my c project for esp32 based on ESP-IDF
# TWatch_UFO94
Simple watch based on T-Watch TTGO 2020 V1.
The screen displays the clock, date, battery level and current consumption.
A globe from the game X-COM: UFO Defense (file world.dat: https://www.ufopaedia.org/index.php/WORLD.DAT) is drawn in the background. Globe lighting is calculated based on the current time and date. You can rotate the ball with the touchscreen. After 20 seconds of inactivity (no touching the touchscreen), the watch goes into sleep mode. Entering / exiting sleep mode - pressing the button.
Configuration (setting date, time, backlight level, etc.) is performed via the PC console.
Screen watch:

![Screen watch](https://github.com/FLY-FGV/TWatch_2020_V1/blob/main/img/4.png)
Configure from termite:

![Screen term]https://github.com/FLY-FGV/TWatch_2020_V1/blob/main/img/termite.png

Files in project:
spi_master_example_main.c - main cycle, init spi etc
axp202.c - auxiliary functions for working with clock PCF8563 and power AXP202
ft6336.c - auxiliary functions for working with touchscreen
uearth.c - earth project & draw function.

1) in setup call earth_unpack() once.  This function prepare buffer's and unpack coordinates from lat,lon to 3D - x,y,z.
2) in main cycle:
call updateP_setnewpointview(lat,lon) to set view point earth
call set_sunA(Azimuth,Height) to set vector of sun light
call setRearth(Radius) to change or set new radius of earth
call earth_proj_to_scr() to project earth to internal buffer
if need apply sun light call apply_sun()
in cycle writes to screen call uint16_t earth_getcolor(int16_t x,int16_t y) to get color codes of rendered image.



