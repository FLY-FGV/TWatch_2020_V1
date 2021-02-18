#ifndef _UFO_EARTH_UNPACK_
#define _UFO_EARTH_UNPACK_
void earth_unpack();
void earth_proj_to_scr();
void earth_apply_sun();
uint16_t earth_getcolor(int16_t x,int16_t y);
//
void setRearth(int32_t newR);
int  getRearth();
//
void updateP_setnewpointview(float lat,float lon);
void set_sunA(float Az,float H);
void draw_state(void *ptrstate);
#endif