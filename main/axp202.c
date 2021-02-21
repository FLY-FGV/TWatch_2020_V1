#include <stdio.h>
#include <stdio.h>
#include "driver/i2c.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_sleep.h"
#include "driver/ledc.h"
#include "watch_state.h"

extern struct main_watch_t main_watch_state;

void find()
{
	uint8_t data=0;
	//
	printf("I2C SDA21 SCL22 scaninng...\n");
	for (data=0;data<127;data++)
	{
		i2c_cmd_handle_t cmd = i2c_cmd_link_create();
		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, (data<<1), 1);
		i2c_master_stop(cmd);
		esp_err_t ret = i2c_master_cmd_begin(0, cmd, 50 );
		i2c_cmd_link_delete(cmd);
		if (ret==ESP_OK)
			printf("found: %x\n",data<<1);
	}
}

#define AXP202ADDR 0x6A
#define PCF8563ADDR 0xA2

uint8_t read_REG(uint8_t ADR,uint8_t rnum, uint8_t *out)
{
	uint8_t data=0;
	//
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, ADR, 1);
	i2c_master_write_byte(cmd, rnum, 1);
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, ADR|1, 1);
	i2c_master_read_byte(cmd,&data,  1);
	i2c_master_stop(cmd);
	esp_err_t ret = i2c_master_cmd_begin(0, cmd, 50 );
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

uint8_t write_REG(uint8_t ADR,uint8_t rnum, uint8_t out)
{
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, ADR, 1);
	i2c_master_write_byte(cmd, rnum, 1);
	i2c_master_write_byte(cmd, out, 1);
	i2c_master_stop(cmd);
	esp_err_t ret = i2c_master_cmd_begin(0, cmd, 50 );
	i2c_cmd_link_delete(cmd);
	if (ret==ESP_OK)
		return 0;
	return 1;
}

void get_TimeAndDate(uint8_t *HMS,uint8_t *DMY)
{
	uint8_t DATA[6];
	ESP_ERROR_CHECK(read_REG(PCF8563ADDR,0x2,DATA));//sec
	ESP_ERROR_CHECK(read_REG(PCF8563ADDR,0x3,DATA+1));//min
	ESP_ERROR_CHECK(read_REG(PCF8563ADDR,0x4,DATA+2));//hour's
	//
	ESP_ERROR_CHECK(read_REG(PCF8563ADDR,0x5,DATA+3));//day's
	ESP_ERROR_CHECK(read_REG(PCF8563ADDR,0x7,DATA+4));//mounths
	ESP_ERROR_CHECK(read_REG(PCF8563ADDR,0x8,DATA+5));//year
	//
	HMS[0]=((DATA[2]>>4)&0x3)*10+(DATA[2]&0xF);
	HMS[1]=((DATA[1]>>4)&0x7)*10+(DATA[1]&0xF);
	HMS[2]=((DATA[0]>>4)&0x7)*10+(DATA[0]&0xF);
	//
	DMY[0]=((DATA[3]>>4)&0x3)*10+(DATA[3]&0xF);
	DMY[1]=((DATA[4]>>4)&0x1)*10+(DATA[4]&0xF);
	DMY[2]=((DATA[5]>>4)&0xF)*10+(DATA[5]&0xF);
}

uint8_t get_DayOfWeek()
{
	uint8_t DATA;
	ESP_ERROR_CHECK(read_REG(PCF8563ADDR,0x6,&DATA));
	return DATA&0x7;
}

void set_DayOfWeek(uint8_t nd)
{
	write_REG(PCF8563ADDR,0x6,nd&7);
}

void set_new_Time(uint8_t Hh,uint8_t Mm,uint8_t Ss)
{
	Hh=((Hh/10)<<4)|(Hh%10);
	Mm=((Mm/10)<<4)|(Mm%10);
	Ss=((Ss/10)<<4)|(Ss%10);
	write_REG(PCF8563ADDR,0x0,1<<5);//stop clock
	write_REG(PCF8563ADDR,0x2,Ss);//sec
	write_REG(PCF8563ADDR,0x3,Mm);//min
	write_REG(PCF8563ADDR,0x4,Hh);//hour's
	write_REG(PCF8563ADDR,0x0,0);//start clock
};

void set_new_Date(uint8_t Dd,uint8_t Mm,uint8_t Yy)
{
	Dd=((Dd/10)<<4)|(Dd%10);
	Mm=((Mm/10)<<4)|(Mm%10);
	Yy=((Yy/10)<<4)|(Yy%10);
	write_REG(PCF8563ADDR,0x5,Dd);
	write_REG(PCF8563ADDR,0x7,Mm);
	write_REG(PCF8563ADDR,0x8,Yy);
}

int16_t axp202_getdischargecurrent()
{
	uint8_t data[2];
	ESP_ERROR_CHECK(read_REG(AXP202ADDR,0x7C, data));//high 8 bit
	ESP_ERROR_CHECK(read_REG(AXP202ADDR,0x7D, data+1));//low 5 bit
	return (data[1]&0x1F)|(data[0]<<5);
}
int16_t axp202_getchargecurrent()
{
	uint8_t data[2];
	ESP_ERROR_CHECK(read_REG(AXP202ADDR,0x7A, data));//high 8 bit
	ESP_ERROR_CHECK(read_REG(AXP202ADDR,0x7B, data+1));//low 5 bit
	return (data[1]&0x1F)|(data[0]<<5);
}

int16_t axp202_getBatV()
{
	uint8_t data[2];
	ESP_ERROR_CHECK(read_REG(AXP202ADDR,0x78, data));//high 8 bit
	ESP_ERROR_CHECK(read_REG(AXP202ADDR,0x79, data+1));//low 5 bit
	return (data[1]&0x1F)|(data[0]<<5);
}

uint8_t axp202_getpowerstatus()
{
	uint8_t data;
	ESP_ERROR_CHECK(read_REG(AXP202ADDR,0x0, &data));//high 8 bit
	return data;
}

static void power_task(void* arg)
{
	main_watch_state.in_sleep=0;
    for(;;)
	{
		vTaskDelay(10/portTICK_RATE_MS);
		if (gpio_get_level(GPIO_NUM_35)==0)
		{
			uint8_t data;
			ESP_ERROR_CHECK(read_REG(AXP202ADDR,0x4A, &data));
			//printf("PEK:%d!\n",data);
			ESP_ERROR_CHECK(write_REG(AXP202ADDR,0x4A,data));
			if (main_watch_state.in_sleep==0)
			{
				main_watch_state.in_sleep=1;
				uint32_t msg = GO_SLEEP<<24;
				xQueueSend(main_watch_state.main_message, &msg, 0);
			}
			else
			{
				main_watch_state.in_sleep=0;
			}
		};
	}
}

void disable_ldo2()
{
	uint8_t data;
	ESP_ERROR_CHECK(read_REG(AXP202ADDR,0x12, &data));
	ESP_ERROR_CHECK(write_REG(AXP202ADDR,0x12,(data&0b11111011)|2));
};

void  enable_ldo2()
{
	uint8_t data;
	ESP_ERROR_CHECK(read_REG(AXP202ADDR,0x12, &data));
	ESP_ERROR_CHECK(write_REG(AXP202ADDR,0x12,data|0x4));
};

void disable_all_ldo()
{
	ESP_ERROR_CHECK(write_REG(AXP202ADDR,0x12,2));
}

void init_axp202()
{
	i2c_config_t conf = {
		.mode = I2C_MODE_MASTER,
		.sda_io_num = 21,         // select GPIO specific to your project
		.sda_pullup_en = GPIO_PULLUP_ENABLE,
		.scl_io_num = 22,         // select GPIO specific to your project
		.scl_pullup_en = GPIO_PULLUP_ENABLE,
		.master.clk_speed = 300000,  // select frequency specific to your project
		// .clk_flags = 0,          /*!< Optional, you can use I2C_SCLK_SRC_FLAG_* flags to choose i2c source clock here. */
	};
	//
	ESP_ERROR_CHECK(i2c_param_config(0,&conf));
	ESP_ERROR_CHECK(i2c_driver_install(0,I2C_MODE_MASTER, 0, 0, 0));
	//
	find();
	//power-up sequence:
	uint8_t data;
	ESP_ERROR_CHECK(read_REG(AXP202ADDR,0x36, &data));ESP_ERROR_CHECK(write_REG(AXP202ADDR,0x36,data&0xfc));
	ESP_ERROR_CHECK(read_REG(AXP202ADDR,0x32, &data));ESP_ERROR_CHECK(write_REG(AXP202ADDR,0x32,data&0xcf));
	ESP_ERROR_CHECK(read_REG(AXP202ADDR,0x12, &data));ESP_ERROR_CHECK(write_REG(AXP202ADDR,0x12,data&0xfe));
	ESP_ERROR_CHECK(read_REG(AXP202ADDR,0x33, &data));ESP_ERROR_CHECK(write_REG(AXP202ADDR,0x33,data&0xf0));
	ESP_ERROR_CHECK(read_REG(AXP202ADDR,0x28, &data));ESP_ERROR_CHECK(write_REG(AXP202ADDR,0x28,data|0xf0));
	ESP_ERROR_CHECK(read_REG(AXP202ADDR,0x12, &data));ESP_ERROR_CHECK(write_REG(AXP202ADDR,0x12,0x6));//0x56=== 0101 0110 LDO3+DC2 LDO2+DC3
	ESP_ERROR_CHECK(write_REG(AXP202ADDR,0x31,3));//set power off = 2.9V
	ESP_ERROR_CHECK(write_REG(AXP202ADDR,0x40,0));//dis irq40
	ESP_ERROR_CHECK(write_REG(AXP202ADDR,0x41,0));//dis irq41
	ESP_ERROR_CHECK(write_REG(AXP202ADDR,0x42,2));//dis irq42. enable only pek short
	ESP_ERROR_CHECK(read_REG(AXP202ADDR,0x4A, &data));ESP_ERROR_CHECK(write_REG(AXP202ADDR,0x4A,data));//reset irq request from reg42
	ESP_ERROR_CHECK(write_REG(AXP202ADDR,0x43,0));//dis irq43
	//
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = GPIO_SEL_35;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);
	xTaskCreate(power_task, "power_task", 2048, NULL, 10, NULL);
	//
	printf("power init - done!\n");
	//
	gpio_wakeup_enable(GPIO_NUM_35,GPIO_INTR_LOW_LEVEL);
	esp_sleep_enable_gpio_wakeup();
};