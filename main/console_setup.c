#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <nvs_flash.h>
#include "driver/ledc.h"
#include <math.h>
#include "watch_state.h"
#include "sun.h"
#include "uearth.h"
#include "axp202.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

extern struct main_watch_t main_watch_state;

void setbright(uint8_t brg)
{
	ledc_set_duty(LEDC_HIGH_SPEED_MODE,0, brg);
	ledc_update_duty(LEDC_HIGH_SPEED_MODE,0);
}

void read_nvs()
{
	esp_err_t err = nvs_flash_init();
    if (err==ESP_ERR_NVS_NO_FREE_PAGES)
    {
		nvs_flash_erase();
		err=nvs_flash_init();
    }
    if (err==ESP_OK)
    {
		printf("nvs init - is OK!\n");
		nvs_handle nvs_h;
		err=nvs_open("watchsetup",NVS_READWRITE,&nvs_h);
		if (err==ESP_OK)
		{
			nvs_get_u8(nvs_h, "BR",  &main_watch_state.brg);
			nvs_get_i16(nvs_h, "KL", &main_watch_state.KLight);
			nvs_get_u8(nvs_h, "SO",  &main_watch_state.screenorientation);
			nvs_get_i8(nvs_h, "TZ",  &main_watch_state.TimeZone);
			nvs_close(nvs_h);
		}
	}
}

void write_nvs()
{
	nvs_handle nvs_h;
	esp_err_t err=nvs_open("watchsetup",NVS_READWRITE,&nvs_h);
	//printf("update setup in nvs!\n");
	if (err==ESP_OK)
	{
		err=nvs_set_u8( nvs_h, "BR",  main_watch_state.brg);
		err=err|nvs_set_i16(nvs_h, "KL", main_watch_state.KLight);
		err=err|nvs_set_u8( nvs_h, "SO", main_watch_state.screenorientation);
		err=err|nvs_set_i8( nvs_h, "TZ", main_watch_state.TimeZone);
		nvs_commit(nvs_h);
		nvs_close(nvs_h);
	};
	if (err==ESP_OK)
		printf("saved to NVS - done!\n");
	else
		printf("error to save data in NVS!\n");
}

void console_arg(void *param)
{
	char s[256];
	while(1)
	{
		gpio_set_level(4, 0);
		if (gets(s)==0)
		{
			vTaskDelay(10);
			continue;
		}
		gpio_set_level(4, 1);
		if (s[0]=='L' && s[1]=='=')
		{
			main_watch_state.brg=atol(s+2);
			ledc_set_duty(LEDC_HIGH_SPEED_MODE,0, main_watch_state.brg);
			ledc_update_duty(LEDC_HIGH_SPEED_MODE,0);
			printf("BL=%d\n",main_watch_state.brg);
			write_nvs();
			continue;
		}
		if (s[0]=='R' && s[1]=='=')
		{
			setRearth(atol(s+2));
			printf("Re=%d\n",getRearth());
			continue;
		}
		if (s[0]=='T' && s[1]=='Z'  && s[2]=='=')
		{
			main_watch_state.TimeZone=atol(s+3);
			printf("TZ=%d\n",main_watch_state.TimeZone);
			write_nvs();
			continue;
		}
		if (s[0]=='D' && s[1]=='=')
		{
			//D=DD:MM:YY
			s[4]=0;s[7]=0;
			uint8_t Dd=atol(s+2);
			uint8_t Mm=atol(s+5);
			uint8_t Yy=atol(s+8);
			printf("set new date: %2.2d:%2.2d:%4.4d ?\n",Dd,Mm,Yy+2000);
			printf("type 'y':");
			while(gets(s)==0) vTaskDelay(10);
			if (s[0]=='y')
			{
				set_new_Date(Dd,Mm,Yy);
				printf("new date has been setted!\n");
			}
			else
				printf("abort operation (set new date)!\n");
			continue;
		}
		if (s[0]=='T' && s[1]=='=')
		{
			//0123456789ABCDEF
			//T=HH:MM:SS
			s[4]=0;s[7]=0;
			uint8_t Hh=atol(s+2);
			uint8_t Mm=atol(s+5);
			uint8_t Ss=atol(s+8);
			printf("set new time: %2.2d:%2.2d:%2.2d ?\n",Hh,Mm,Ss);
			printf("type 'y':");
			while(gets(s)==0) vTaskDelay(10);
			if (s[0]=='y')
			{
				set_new_Time(Hh,Mm,Ss);
				printf("new time has been setted!\n");
			}
			else
				printf("abort operation (set new time)!\n");
			continue;
		}
		if (s[0]=='W' && s[1]=='=')
		{
			uint8_t Nd=atol(s+2);
			set_DayOfWeek(Nd);
			printf("setted new day of week=%d\n",Nd);
			continue;
		}
		if (s[0]=='O' && s[1]=='=')
		{
			uint8_t Nd=atol(s+2);
			main_watch_state.screenorientation=Nd;
			printf("setted new screen orientation=%d\n",Nd);
			write_nvs();
			continue;
		}
		if (s[0]=='K' && s[1]=='=')
		{
			main_watch_state.KLight=atol(s+2);
			earth_setKL(main_watch_state.KLight);
			printf("setted new KL=%d\n",main_watch_state.KLight);
			write_nvs();
			continue;
		}
		if (s[0]=='l' && s[1]=='a' && s[2]=='t' && s[3]=='=')
		{
			main_watch_state.lat=-(M_PI/180.0)*atof(s+4);
			earth_setnewpointview(main_watch_state.lat,main_watch_state.lon);
			uint32_t msg=MAKE_MSG0(UPDATE_SCR);
			xQueueSend(main_watch_state.main_message, &msg, 0);
			continue;
		}
		if (s[0]=='l' && s[1]=='o' && s[2]=='n' && s[3]=='=')
		{
			main_watch_state.lon=-(M_PI/180.0)*atof(s+4);
			earth_setnewpointview(main_watch_state.lat,main_watch_state.lon);
			uint32_t msg=MAKE_MSG0(UPDATE_SCR);
			xQueueSend(main_watch_state.main_message, &msg, 0);
			continue;
		}
		if (strcmp(s,"prn")==0)
		{
			uint32_t msg=MAKE_MSG0(SEND_SCR);
			xQueueSend(main_watch_state.main_message, &msg, 0);
			continue;
		}
		if(strcmp(s,"reboot")==0)
		{
			ESP_ERROR_CHECK(-1);
		}
		if(strcmp(s,"help")==0||strcmp(s,"?")==0)
		{
			printf("UFO Watch V1.0\n");
			printf("\nstatus:\n");
			printf("DATE=%2.2d/%2.2d/%4.4d\n",main_watch_state.day,main_watch_state.mnt,main_watch_state.year+2000);
			printf("TIME=%2.2d:%2.2d:%2.2d\n",main_watch_state.hour,main_watch_state.minute,main_watch_state.sec);
			char *MDN[8]={"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday",""};
			printf("DAYOFWEEK=%s",MDN[main_watch_state.dayofweek]);
			printf("[%d]\n",main_watch_state.dayofweek);
			printf("TIMEZONE=%d\n",main_watch_state.TimeZone);
			printf("BrightLevel=%d\n",main_watch_state.brg);
			printf("KL=%d\n",main_watch_state.KLight);
			printf("SORIENT=%d\n",main_watch_state.screenorientation);
			printf("POINTVIEW(N>0,E>0): lat=%f lon=%f\n",-main_watch_state.lat*180.0/M_PI,-main_watch_state.lon*180.0/M_PI);
			printf("\ncommand list:\n");
			printf("help - print this help\n");
			printf("reboot - reboot watch\n");
			printf("prn - out to console screen buffer in hex (2byte to pixel = 240lines*960char)\n");
			printf("lat=*.*** - set latitude point of view in degrees\n");
			printf("lon=*.*** - set longitude point of view in degrees\n");
			printf("K=*** - set ambiglight coef. *** - number from 0 (black on dark side) to 255 (half bright on dark side)\n");
			printf("L=*** - set backlight level *** - number from 0 to 255\n");
			printf("R=**** - set earth radius *** - radius earth in pix\n");
			printf("TZ=** - set time zone. ** - number -24...24 - offset in hours from UCT. Example: 'TZ=3'- MSK time, 'TZ=5' - EKB time, e.t.c.\n");
			printf("O=* - orientation screen (0,1,2,3)\n");
			printf("D=**/**/**** - set date\n");
			printf("T=**:**:** - set time\n");
			printf("W=* - set day of week:\n");
			for (int k=0;k<7;k++) printf("   %d - %s\n",k,MDN[k]);
			continue;
		}
		printf("Bad command. Type: help or ?\n");
	}
}