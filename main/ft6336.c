#include <stdio.h>
#include "driver/i2c.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include <math.h>
#include "watch_state.h"

static uint8_t read_REG(uint8_t rnum, uint8_t *out)
{
	uint8_t data=0;
	//
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, 0x70, 1);
	i2c_master_write_byte(cmd, rnum, 1);
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, 0x70|1, 1);
	i2c_master_read_byte(cmd,&data,  1);
	i2c_master_stop(cmd);
	esp_err_t ret = i2c_master_cmd_begin(1, cmd, 50 );
	i2c_cmd_link_delete(cmd);
	//
	//printf("ERR:%d R%x: %x\n",ret,rnum,data);
	//
	if (ret==ESP_OK)
	{
		out[0]=data;
		return 0;
	}
	return 1;
}

static uint8_t write_REG(uint8_t rnum, uint8_t out)
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, 0x70, 1);
	i2c_master_write_byte(cmd, rnum, 1);
	i2c_master_write_byte(cmd, out, 1);
	i2c_master_stop(cmd);
	esp_err_t ret = i2c_master_cmd_begin(1, cmd, 50 );
	i2c_cmd_link_delete(cmd);
	if (ret==ESP_OK)
		return 0;
	return 1;
}

static void IRAM_ATTR touch_isr_handler(void* arg);
static void touch_task(void* arg);
static xQueueHandle gpio_evt_queue = NULL;

void init_ft6336()
{
	i2c_config_t conf = {
		.mode = I2C_MODE_MASTER,
		.sda_io_num = 23,         // select GPIO specific to your project
		.sda_pullup_en = GPIO_PULLUP_ENABLE,
		.scl_io_num = 32,         // select GPIO specific to your project
		.scl_pullup_en = GPIO_PULLUP_ENABLE,
		.master.clk_speed = 300000,  // select frequency specific to your project
		// .clk_flags = 0,          /*!< Optional, you can use I2C_SCLK_SRC_FLAG_* flags to choose i2c source clock here. */
	};
	//
	ESP_ERROR_CHECK(i2c_param_config(1,&conf));
	ESP_ERROR_CHECK(i2c_driver_install(1,I2C_MODE_MASTER, 0, 0, 0));
	//
	uint8_t data=0;
	//
	printf("I2C SDA23 SCL32 scaninng...\n");
	for (data=0;data<127;data++)
	{
		i2c_cmd_handle_t cmd = i2c_cmd_link_create();
		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, (data<<1), 1);
		i2c_master_stop(cmd);
		esp_err_t ret = i2c_master_cmd_begin(1, cmd, 50 );
		i2c_cmd_link_delete(cmd);
		if (ret==ESP_OK)
			printf("found: %x\n",data<<1);
	}
	ESP_ERROR_CHECK(write_REG(0x88,10));//set 10Hz report rate
	ESP_ERROR_CHECK(write_REG(0x89,10));//set 10Hz report rate
	//FT6336 Interrupt 	38
	ESP_ERROR_CHECK(write_REG(0xA4,1));//0-polling,1-trigger
	//config interrupt:
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = GPIO_SEL_38;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
    //create a queue to handle gpio event from isr
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    //start gpio task
    xTaskCreate(touch_task, "touch_task", 2048, NULL, 10, NULL);
    //hook isr handler for specific gpio pin
    gpio_isr_handler_add(GPIO_NUM_38, touch_isr_handler, (void*) GPIO_NUM_38);
};

uint16_t get_ft6336_X()
{
	uint8_t dataH,dataL;
	ESP_ERROR_CHECK(read_REG(3,&dataH));
	ESP_ERROR_CHECK(read_REG(4,&dataL));
	return (dataH<<8)|dataL;
};

uint16_t get_ft6336_Y()
{
	uint8_t dataH,dataL;
	ESP_ERROR_CHECK(read_REG(5,&dataH));
	ESP_ERROR_CHECK(read_REG(6,&dataL));
	return ((dataH<<8)|dataL)&0xFFF;
};

static void IRAM_ATTR touch_isr_handler(void* arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

extern struct main_watch_t main_watch_state;

static int curX=0;
static int curY=0;
static int down=0;
int getlastX(){return curX;};
int getlastY(){return curY;};
int getdown(){return down;};

static void touch_task(void* arg)
{
	uint32_t io_num;
	uint32_t msg;
    for(;;)
	{
        if(xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY))
		{
			int newptX=get_ft6336_X();
			int newptY=get_ft6336_Y()&0xFFF;
			int newstate=(newptX&(0x8000|0x4000))>>14;
			newptX=239-(newptX&0x3FFF);
			if (main_watch_state.screenorientation==1)
			{
				int tmp=newptY;
				newptY=239-newptX;
				newptX=tmp;
			};
			if (main_watch_state.screenorientation==2)
			{
				newptY=239-newptY;
				newptX=239-newptX;
			};
			if (main_watch_state.screenorientation==3)
			{
				int tmp=newptY;
				newptY=newptX;
				newptX=239-tmp;
			};
			switch(newstate)
			{
				case 0://down
					//ptX=newptX;ptY=newptY;
					//state=0;
					msg = (POINT_DOWN<<24)|(newptX<<8)|(newptY);
					curX=newptX;
					curY=newptY;
					down=1;
					xQueueSend(main_watch_state.main_message, &msg, 0);
					break;
				case 1://lift up
					//state=-1;
					msg = (POINT_UP<<24)|(newptX<<8)|(newptY);
					down=0;
					xQueueSend(main_watch_state.main_message, &msg, 0);
					break;
				case 2://contact-move
					curX=newptX;
					curY=newptY;
					down=1;
					msg = (POINT_MOVE<<24)|(newptX<<8)|(newptY);
					xQueueSend(main_watch_state.main_message, &msg, 0);
				/*
					if (state==0)
					{
						add_X=add_X+(newptX-ptX);
						add_Y=add_Y+(newptY-ptY);
						ptX=newptX;
						ptY=newptY;
					}
				*/
					break;
			};
        }
    }
}
