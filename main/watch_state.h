#ifndef _____MAIN_WATCH_STATE___
#define _____MAIN_WATCH_STATE___

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

struct main_watch_t
{
	//current state:
	uint8_t hour;
	uint8_t minute;
	uint8_t sec;
	//
	uint8_t day;
	uint8_t mnt;
	uint8_t year;
	uint8_t dayofweek;
	//
	int16_t current;//+charge,-discharge 0.5mA step
	int16_t BatV;// bat. voltage - 1.1mV step.....
	//setup:
	uint8_t brg;//bright backlight level
	int8_t  TimeZone;//-24...+24, other - time zone unknow
	uint8_t screenorientation;//0-button on left,1-button on top,2-button on right,3-button on bottom side.
	//system:
	uint16_t TimeOutCounter;//time out counter
	//
	QueueHandle_t main_message;//main message queque
	//
	int16_t KLight;
	uint8_t in_sleep;//1=from sleep, =0 - go to sleep
	//point of view:
	float lat;
	float lon;
};

enum msg_type
{
	POINT_DOWN=1,
	POINT_UP=2,
	POINT_MOVE=3,
	GO_SLEEP=4,
	SEND_SCR=5,
	UPDATE_SCR=6
};

#define MAKE_MSG0(A)     ((A<<24))
#define GET_MSG(A)  ((A>>24)&0xFF) 
#define GET_ARG0(A) ((A>>8)&0xFF)
#define GET_ARG1(A) ((A)&0xFF) 

#endif