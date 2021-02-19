#ifndef ___MY_UFO_EARTH_COLOR_CODES__
#define ___MY_UFO_EARTH_COLOR_CODES__

#include "esp_attr.h"

#define RGB_G(R,G,B) (uint16_t)(((((G>>2)<<5)|((B>>3)<<11)|(R>>3))<<8)|((((G>>2)<<5)|((B>>3)<<11)|(R>>3))>>8))

//color 0 - jungle
#define U0_R 0x7F
#define U0_G 0xFE
#define U0_B 0x11
//color 6 - jungle
#define U6_R 115
#define U6_G 230
#define U6_B 15
//color 10 - jungle
#define U10_R 104
#define U10_G 209
#define U10_B 14
//color 11 - jungle
#define U11_R 95
#define U11_G 190
#define U11_B 12
//color 1 -farm
#define U1_R 0
#define U1_G 255
#define U1_B 0
//color 2 - farm
#define U2_R 0
#define U2_G 231
#define U2_B 0
//color 3 - farm
#define U3_R 0
#define U3_G 210
#define U3_B 0
//color 4 - farm
#define U4_R 0
#define U4_G 191
#define U4_B 0
//color 5 - mounthain
#define U5_R 116
#define U5_G 116
#define U5_B 116
//color 7 - desert
#define U7_R 226
#define U7_G 187
#define U7_B 86
//color 8 - desert
#define U8_R 205
#define U8_G 170
#define U8_B 78
//color 9 - polar ice
#define U9_R 255
#define U9_G 255
#define U9_B 255
//polar sea & ice
#define U12_R 127
#define U12_G 127
#define U12_B 255
//13-black (fon)
#define BCKG_CODE  13
//
#define U13_R 0
#define U13_G 0
#define U13_B 0
//14-watch arrow
#define ARW_CODE  14
//
#define U14_R 0
#define U14_G 127
#define U14_B 0
//15-ocean
#define OCEAN_CODE 15
//color 15 - ocean on earth
#define U15_R 0
#define U15_G 0
#define U15_B 255
//
extern uint8_t earth_red_code[16];
extern uint8_t earth_blue_code[16];
extern uint8_t earth_green_code[16];
#endif