#include <math.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

float getsun_D(int8_t H,uint8_t M,uint8_t S)
{
	return (H*3600.0+M*60.0+S)/(3600.0*24.0);
}

float getsun_H(uint8_t d,uint8_t m,uint16_t y,float dayrem)
{
	uint8_t DMM[12]={31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	float numday=0;
	for (int i=0;i<(m-1);i++)
		numday=numday+DMM[i];
	//numday=numday+main_watch_state.day-173;
	//return=-(23.44*M_PI/180.0)*cos(numday*M_PI*2.0/365.0);
	numday=numday+d+dayrem;
	return asin((M_PI/180.0)*(0.39779*cos((M_PI/180.0)*0.98565*(numday+10.0))+1.914*sin((M_PI/180.0)*0.98565*(numday-2.0))));
}
