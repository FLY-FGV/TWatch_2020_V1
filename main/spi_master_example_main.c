/* 
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_system.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_timer.h"


#include "axp202.h"
#include "ft6336.h"
#include "uearth.h"
#include <math.h>
#include <time.h>
#include "watch_state.h"
#include "esp_sleep.h"
#include "state.h"
#include "sun.h"
#include "console_setup.h"

#define LCD_HOST    HSPI_HOST
#define DMA_CHAN    2

#define PIN_NUM_MOSI 19
#define PIN_NUM_CLK  18
#define PIN_NUM_CS    5

#define PIN_NUM_DC   27
#define PIN_NUM_BCKL 12



//To speed up transfers, every SPI transfer sends a bunch of lines. This define specifies how many. More means more memory use,
//but less overhead for setting up / finishing transfers. Make sure 240 is dividable by this.
#define PARALLEL_LINES 60

/*
 The LCD needs a bunch of command/argument values to be initialized. They are stored in this struct.
*/
typedef struct {
    uint8_t cmd;
    uint8_t data[16];
    uint8_t databytes; //No of data in data; bit 7 = delay after set; 0xFF = end of cmds.
} lcd_init_cmd_t;

typedef enum {
    LCD_TYPE_ILI = 1,
    LCD_TYPE_ST,
    LCD_TYPE_MAX,
} type_lcd_t;


struct main_watch_t main_watch_state;

DRAM_ATTR static const lcd_init_cmd_t st_init_cmds2[]={
    {0x01, {0}, 0x80},   // software reset!
    {0x11, {0}, 0x80},   // Sleep out
    {0x13, {0}, 0x80},   // Normal display mode on = ST7789_NORON
  //------------------------------display and color format setting--------------------------------//
    {0x36,{0}, 1},//ST7789_MADCTL);//(1<<5)|(1<<6)   |0-r,pipinleft|
    {0xB6,{0x0A,0x82}, 2},// JLX240 display datasheet
	{0x3A,{0x55}, 0x81},//ST7789_COLMOD
  //--------------------------------ST7789V Frame rate setting----------------------------------//
    {0xB2,{0x0c,0x0c,0x00,0x33,0x33}, 5},//ST7789_PORCTRL
    {0xB7,{0x35}, 1}, //ST7789_GCTRL
  //---------------------------------ST7789V Power setting--------------------------------------//
    {0xBB,{0x28}, 1}, //ST7789_VCOMS JLX240 display datasheet - VCOM SET!
    {0xC0,{0x0C}, 1}, //ST7789_LCMCTRL
    {0xC2,{0x01,0xFF}, 2}, //ST7789_VDVVRHEN
	{0xC3,{0x10}, 1}, //ST7789_VRHS
	{0xC4,{0x20}, 1},//ST7789_VDVSET
	{0xC6,{0x0f}, 1},//ST7789_FRCTR2
	{0xD0,{0xa4,0xa1}, 2},//ST7789_PWCTRL1
  //--------------------------------ST7789V gamma setting---------------------------------------//
    /* Positive Voltage Gamma Control */
    {0xE0, {0xD0, 0x00, 0x02, 0x07, 0x0a, 0x28, 0x32, 0x44, 0x42, 0x06, 0x0e, 0x12, 0x14, 0x17}, 14},
    /* Negative Voltage Gamma Control */
    {0xE1, {0xD0, 0x00, 0x02, 0x07, 0x0a, 0x28, 0x31, 0x54, 0x47, 0x0E, 0x1C, 0x17, 0x1B, 0x1E}, 14},
    {0x21, {0}, 0x80},//ST7789_INVON
    {0x2A, {0,0,0,0xe5}, 4},//ST7789_CASET
    {0x2B, {0,0,1,0x3f}, 0x84},//ST7789_RASET
	{0x29,{0x0}, 0},
    {0, {0}, 0xff},
};

/* Send a command to the LCD. Uses spi_device_polling_transmit, which waits
 * until the transfer is complete.
 *
 * Since command transactions are usually small, they are handled in polling
 * mode for higher speed. The overhead of interrupt transactions is more than
 * just waiting for the transaction to complete.
 */
void lcd_cmd(spi_device_handle_t spi, const uint8_t cmd)
{
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length=8;                     //Command is 8 bits
    t.tx_buffer=&cmd;               //The data is the cmd itself
    t.user=(void*)0;                //D/C needs to be set to 0
    ret=spi_device_polling_transmit(spi, &t);  //Transmit!
    assert(ret==ESP_OK);            //Should have had no issues.
}

/* Send data to the LCD. Uses spi_device_polling_transmit, which waits until the
 * transfer is complete.
 *
 * Since data transactions are usually small, they are handled in polling
 * mode for higher speed. The overhead of interrupt transactions is more than
 * just waiting for the transaction to complete.
 */
void lcd_data(spi_device_handle_t spi, const uint8_t *data, int len)
{
    esp_err_t ret;
    spi_transaction_t t;
    if (len==0) return;             //no need to send anything
    memset(&t, 0, sizeof(t));       //Zero out the transaction
    t.length=len*8;                 //Len is in bytes, transaction length is in bits.
    t.tx_buffer=data;               //Data
    t.user=(void*)1;                //D/C needs to be set to 1
    ret=spi_device_polling_transmit(spi, &t);  //Transmit!
    assert(ret==ESP_OK);            //Should have had no issues.
}

//This function is called (in irq context!) just before a transmission starts. It will
//set the D/C line to the value indicated in the user field.
void lcd_spi_pre_transfer_callback(spi_transaction_t *t)
{
    int dc=(int)t->user;
    gpio_set_level(PIN_NUM_DC, dc);
}

//Initialize the display
void lcd_init(spi_device_handle_t spi)
{
    int cmd=0;
    const lcd_init_cmd_t* lcd_init_cmds;
	//init st7789v
    printf("LCD ST7789V initialization.\n");
    lcd_init_cmds = st_init_cmds2;
    //Send all the commands
    while (lcd_init_cmds[cmd].databytes!=0xff)
	{
        lcd_cmd(spi,  lcd_init_cmds[cmd].cmd);
        lcd_data(spi, lcd_init_cmds[cmd].data, lcd_init_cmds[cmd].databytes&0x1F);
        if (lcd_init_cmds[cmd].databytes&0x80)
		{
            vTaskDelay(100 / portTICK_RATE_MS);
        }
        cmd++;
    }
	printf("init - done!\n");
}


/* To send a set of lines we have to send a command, 2 data bytes, another command, 2 more data bytes and another command
 * before sending the line data itself; a total of 6 transactions. (We can't put all of this in just one transaction
 * because the D/C line needs to be toggled in the middle.)
 * This routine queues these commands up as interrupt transactions so they get
 * sent faster (compared to calling spi_device_transmit several times), and at
 * the mean while the lines for next transactions can get calculated.
 */
static void send_lines(spi_device_handle_t spi, int ypos, uint16_t *linedata)
{
    esp_err_t ret;
    int x;
    //Transaction descriptors. Declared static so they're not allocated on the stack; we need this memory even when this
    //function is finished because the SPI driver needs access to it even while we're already calculating the next line.
    static spi_transaction_t trans[6];

    //In theory, it's better to initialize trans and data only once and hang on to the initialized
    //variables. We allocate them on the stack, so we need to re-init them each call.
    for (x=0; x<6; x++) {
        memset(&trans[x], 0, sizeof(spi_transaction_t));
        if ((x&1)==0) {
            //Even transfers are commands
            trans[x].length=8;
            trans[x].user=(void*)0;
        } else {
            //Odd transfers are data
            trans[x].length=8*4;
            trans[x].user=(void*)1;
        }
        trans[x].flags=SPI_TRANS_USE_TXDATA;
    }
    trans[0].tx_data[0]=0x2A;           //Column Address Set
    trans[1].tx_data[0]=0;              //Start Col High
    trans[1].tx_data[1]=0;              //Start Col Low
    trans[1].tx_data[2]=(239)>>8;       //End Col High
    trans[1].tx_data[3]=(239)&0xff;     //End Col Low
    trans[2].tx_data[0]=0x2B;           //Page address set
    trans[3].tx_data[0]=ypos>>8;        //Start page high
    trans[3].tx_data[1]=ypos&0xff;      //start page low
    trans[3].tx_data[2]=(ypos+PARALLEL_LINES)>>8;    //end page high
    trans[3].tx_data[3]=(ypos+PARALLEL_LINES)&0xff;  //end page low
    trans[4].tx_data[0]=0x2C;           //memory write
    trans[5].tx_buffer=linedata;        //finally send the line data
    trans[5].length=240*2*8*PARALLEL_LINES;          //Data length, in bits
    trans[5].flags=0; //undo SPI_TRANS_USE_TXDATA flag

    //Queue all transactions.
    for (x=0; x<6; x++) {
        ret=spi_device_queue_trans(spi, &trans[x], portMAX_DELAY);
        assert(ret==ESP_OK);
    }

    //When we are here, the SPI driver is busy (in the background) getting the transactions sent. That happens
    //mostly using DMA, so the CPU doesn't have much to do here. We're not going to wait for the transaction to
    //finish because we may as well spend the time calculating the next line. When that is done, we can call
    //send_line_finish, which will wait for the transfers to be done and check their status.
}
//

static void send_line_finish(spi_device_handle_t spi)
{
    spi_transaction_t *rtrans;
    esp_err_t ret;
    //Wait for all 6 transactions to be done and get back the results.
    for (int x=0; x<6; x++) {
        ret=spi_device_get_trans_result(spi, &rtrans, portMAX_DELAY);
        assert(ret==ESP_OK);
        //We could inspect rtrans now if we received any info back. The LCD is treated as write-only, though.
    }
}

//1sec timer callback
static void vTimerCallback( TimerHandle_t xTimer )
{
	uint32_t msg=MAKE_MSG0(UPDATE_SCR);
	xQueueSend(main_watch_state.main_message, &msg, 0);
	if ((axp202_getpowerstatus()&0x30)!=0x30)
		main_watch_state.TimeOutCounter++;
	else
		main_watch_state.TimeOutCounter=0;
	if (main_watch_state.TimeOutCounter>=20)
	{
		main_watch_state.in_sleep=1;
		msg = MAKE_MSG0(GO_SLEEP);
		xQueueSend(main_watch_state.main_message, &msg, 0);
		main_watch_state.TimeOutCounter=0;
	}
}

static uint32_t getlastmovemsg(uint32_t m)
{
	uint32_t msg;
	if (xQueuePeek(main_watch_state.main_message, &msg, 0)==pdFALSE)
		return m;
	while(GET_MSG(msg)==POINT_MOVE)
	{
		xQueueReceive(main_watch_state.main_message, &m, 0);
		if (xQueuePeek(main_watch_state.main_message, &msg, 0)==pdFALSE) break;
	};
	return m;
}

static void display_earth(void *param)
{
	spi_device_handle_t spi=param;
    uint16_t *lines[2];
	#define LINE_BUF_SIZE 240*PARALLEL_LINES*sizeof(uint16_t)
    //Allocate memory for the pixel buffers
    for (int i=0; i<2; i++)
	{
        lines[i]=heap_caps_malloc(LINE_BUF_SIZE, MALLOC_CAP_DMA);
        assert(lines[i]!=NULL);
    }
	memset(lines[0],0,LINE_BUF_SIZE);
	memset(lines[1],0,LINE_BUF_SIZE);
	//
	int sending_line=-1;
    int calc_line=0;
	for (int y=0; y<240; y+=PARALLEL_LINES)
	{
            //Finish up the sending process of the previous line, if any
            if (sending_line!=-1) send_line_finish(spi);
            //Swap sending_line and calc_line
            sending_line=calc_line;
			if (calc_line==1) calc_line=0;else calc_line=1;
            //calc_line=(calc_line==1)?0:1;
            //Send the line we currently calculated.
            send_lines(spi, y, lines[sending_line]);
            //The line set is queued up for sending now; the actual sending happens in the
            //background. We can go on to calculate the next line set as long as we do not
            //touch line[sending_line]; the SPI sending process is still reading from that.
    }
	//
	send_line_finish(spi);
	sending_line=-1;
    calc_line=0;
	//
	earth_init();
	earth_setKL(main_watch_state.KLight);
	//
	setbright(main_watch_state.brg);
	//main cycle:
	uint32_t msg;
	int16_t X=0,Y=0;
	earth_setnewpointview(main_watch_state.lat,main_watch_state.lon);
	main_watch_state.TimeOutCounter=0;
	while(1)
	{
		if(xQueueReceive(main_watch_state.main_message, &msg, 0)==pdFALSE)
		{
			vTaskDelay(1);
			continue;
		}
		if (GET_MSG(msg)==UPDATE_SCR)
		{
			if (xQueuePeek(main_watch_state.main_message, &msg, 0)==pdTRUE)
				xQueueReceive(main_watch_state.main_message, &msg, 0);
		};
		//get msg....
		if (GET_MSG(msg)==GO_SLEEP)
		{
			//turn off backlight
			setbright(0);
			disable_all_ldo();
			//go sleep screen
			vTaskDelay(20);
			lcd_cmd(spi,0x10);
			//go sleep
			esp_light_sleep_start();
			main_watch_state.TimeOutCounter=0;
			//on screen
			lcd_cmd(spi,0x11);
			vTaskDelay(1);
			//turn on backlight
			enable_ldo2();
			setbright(main_watch_state.brg);
		};
		if (GET_MSG(msg)==SEND_SCR)
		{
			printf("image:\n");
			for (int iy=0;iy<240;iy++)
			{
				for (int ix=0;ix<240;ix++)
					printf("%4.4X",earth_getcolor(ix,iy));
				printf("\n");
			}
			printf("end img\n");
			main_watch_state.TimeOutCounter=0;
		};
		if (GET_MSG(msg)==POINT_DOWN)
		{
			X=GET_ARG0(msg);
			Y=GET_ARG1(msg);
			main_watch_state.TimeOutCounter=0;
			continue;
		};
		if (GET_MSG(msg)==POINT_MOVE)
		{
			//have any other message
			msg=getlastmovemsg(msg);
			main_watch_state.TimeOutCounter=0;
			int16_t dx=GET_ARG0(msg)-X;
			int16_t dy=GET_ARG1(msg)-Y;
			if (dx==0 && dy==0) continue;
			if (cos(main_watch_state.lat)>=0.0)
				main_watch_state.lon=main_watch_state.lon+(dx*M_PI/240.0);
			else
				main_watch_state.lon=main_watch_state.lon-(dx*M_PI/240.0);
			main_watch_state.lat=main_watch_state.lat+(dy*M_PI/240.0);
			earth_setnewpointview(main_watch_state.lat,main_watch_state.lon);
			X=GET_ARG0(msg);
			Y=GET_ARG1(msg);
		}
		//get data from sensor's:
		{
			uint8_t hour_min_sec[3];
			uint8_t day_mnt_yar[3];
			get_TimeAndDate(hour_min_sec,day_mnt_yar);
			main_watch_state.hour   =hour_min_sec[0];
			main_watch_state.minute =hour_min_sec[1];
			main_watch_state.sec    =hour_min_sec[2];
			main_watch_state.day  =day_mnt_yar[0];
			main_watch_state.mnt  =day_mnt_yar[1];
			main_watch_state.year =day_mnt_yar[2];
			main_watch_state.dayofweek=get_DayOfWeek();
		}
		//расчет дробной части дня по гринвичу
		float dr=getsun_D(main_watch_state.hour-main_watch_state.TimeZone,main_watch_state.minute,main_watch_state.sec);
		//рассчет угола падения солнечных лучей
		float angle_sunH=getsun_H(main_watch_state.day,main_watch_state.mnt,main_watch_state.year,dr);
		//расчет угла поворота земли
		float angle_sunAz=M_PI*1.5-M_PI*2.0*dr;
		//задаем вектор падения солнечных лучей на землю:
		earth_set_sunA(angle_sunAz,angle_sunH);
		//опрос и инициализация данных по батарее
		int16_t dcur=axp202_getdischargecurrent();
		int16_t ccur=axp202_getchargecurrent();
		if (ccur>dcur)
			main_watch_state.current=ccur;
		else
			main_watch_state.current=-dcur;
		main_watch_state.BatV=axp202_getBatV();
		//
		//int32_t T0=esp_timer_get_time();
		earth_proj_to_scr();
		//int32_t T1=esp_timer_get_time();
		if (main_watch_state.TimeZone<=24 && main_watch_state.TimeZone>=-24) earth_apply_sun();
		//int32_t T2=esp_timer_get_time();
		draw_state(	&main_watch_state);
		//int32_t T3=esp_timer_get_time();
		//
		//printf("Tprojc=%d\n",T1-T0);
		//printf("Tlight=%d\n",T2-T1);
		//printf("Tstate=%d\n",T3-T2);
		//
		//int32_t T4=esp_timer_get_time();
		for (int y=0; y<240; y+=PARALLEL_LINES)
		{
			//pretty_effect_calc_lines(lines[calc_line], y, frame, PARALLEL_LINES);
			for (int x=0;x<240;x++)
				for (int yi=0;yi<PARALLEL_LINES;yi++)
				{
					switch(main_watch_state.screenorientation)
					{
					case 1:
						lines[calc_line][x+yi*240]=earth_getcolor(239-y-yi,x);
						break;
					case 2:
						lines[calc_line][x+yi*240]=earth_getcolor(239-x,239-y-yi);
						break;
					case 3:
						lines[calc_line][x+yi*240]=earth_getcolor(y+yi,239-x);
						break;
					default:
						lines[calc_line][x+yi*240]=earth_getcolor(x,y+yi);
						break;
					};
				}
			//Finish up the sending process of the previous line, if any
			if (sending_line!=-1) send_line_finish(spi);
			//Swap sending_line and calc_line
			sending_line=calc_line;
			if (calc_line==1) calc_line=0;else calc_line=1;
			//Send the line we currently calculated.
			send_lines(spi, y, lines[sending_line]);
			//The line set is queued up for sending now; the actual sending happens in the
			//background. We can go on to calculate the next line set as long as we do not
			//touch line[sending_line]; the SPI sending process is still reading from that.
		};
		//int32_t T5=esp_timer_get_time();
		//printf("Tsend=%d\n",T5-T4);
	};
};

void app_main(void)
{
    esp_err_t ret;
    spi_device_handle_t spi;
    spi_bus_config_t buscfg={
        .miso_io_num=-1,
        .mosi_io_num=PIN_NUM_MOSI,
        .sclk_io_num=PIN_NUM_CLK,
        .quadwp_io_num=-1,
        .quadhd_io_num=-1,
        .max_transfer_sz=PARALLEL_LINES*320*2+8
    };
    spi_device_interface_config_t devcfg={
        .clock_speed_hz=26*1000*1000,           //Clock out at 26 MHz
        .mode=0,                                //SPI mode 0
        .spics_io_num=PIN_NUM_CS,               //CS pin
        .queue_size=7,                          //We want to be able to queue 7 transactions at a time
        .pre_cb=lcd_spi_pre_transfer_callback,  //Specify pre-transfer callback to handle D/C line
    };
	//init backlight:
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_TIMER_8_BIT,  // resolution of PWM duty
        .freq_hz = 12000,                     // frequency of PWM signal
        .speed_mode = LEDC_HIGH_SPEED_MODE,   // timer mode
        .timer_num = LEDC_TIMER_0,            // timer index
        .clk_cfg = LEDC_USE_APB_CLK,          // Auto select the source clock
    };
    // Set configuration of timer0 for high speed channels
	ledc_channel_config_t ledc_channel=        {
            .channel    = 0,
            .duty       = 0,
            .gpio_num   = PIN_NUM_BCKL,
            .speed_mode = LEDC_HIGH_SPEED_MODE,
            .hpoint     = 0,
            .timer_sel  = LEDC_TIMER_0,
        };
		
	ledc_channel_config(&ledc_channel);
	ledc_timer_config(&ledc_timer);
    gpio_set_direction(PIN_NUM_DC, GPIO_MODE_DEF_OUTPUT);
	//set default value:
	main_watch_state.main_message=xQueueCreate(10, sizeof(uint32_t));
	main_watch_state.main_event_bits=xEventGroupCreate();
	setRearth(90);
	main_watch_state.TimeZone=5;
	main_watch_state.brg=127;
	main_watch_state.lon=-((60.0+(36.0/60.0))*M_PI)/180.0;
	main_watch_state.lat=-((56.0+(51.0/60.0))*M_PI)/180.0;
	main_watch_state.screenorientation=0;
	main_watch_state.KLight=10;
	//read setting's:
	read_nvs();
	//install gpio isr service
    gpio_install_isr_service(0);
	//power ON!!!!
	init_axp202();
	//init i2c - ft6336
	init_ft6336();
    //Initialize the SPI bus
    ret=spi_bus_initialize(LCD_HOST, &buscfg, DMA_CHAN);
    ESP_ERROR_CHECK(ret);
    //Attach the LCD to the SPI bus
    ret=spi_bus_add_device(LCD_HOST, &devcfg, &spi);
    ESP_ERROR_CHECK(ret);
    //Initialize the LCD
    lcd_init(spi);
    //send first draw message
	uint32_t msg=MAKE_MSG0(UPDATE_SCR);
	xQueueSend(main_watch_state.main_message, &msg, 0);
	//start task's
    xTaskCreate(display_earth,"spi_tsk",4096,spi,3,NULL);
	xTaskCreate(console_arg,"console",4096,NULL,3,NULL);
	//start 1-sec draw timer
	TimerHandle_t xTimer = xTimerCreate("Timer",1000/portTICK_RATE_MS,pdTRUE,( void * ) 0,vTimerCallback);
	xTimerStart( xTimer, 0 );
}
