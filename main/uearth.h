#ifndef _UFO_EARTH_UNPACK_
#define _UFO_EARTH_UNPACK_
void earth_init();
void earth_deinit();
//
void earth_proj_to_scr();
void earth_apply_sun();
uint16_t earth_getcolor(int16_t x,int16_t y);
//
void set_line(int16_t y, int16_t xb, int16_t xe, uint8_t color);
void draw_triang(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3,uint8_t color);
//
void earth_setKL(int16_t newKL);
void setRearth(int32_t newR);
int  getRearth();
//
void updateP_setnewpointview(float lat,float lon);
void set_sunA(float Az,float H);
#endif