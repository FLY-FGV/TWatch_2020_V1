#include <stdio.h>
#include <stdlib.h>
#include "uearth.h"
#include "uearth_color_map.h"
#include "watch_state.h"
//digit width:   6*size
//digit height: 11*size
void draw_seg(int x,int y,int size,int snum)
{
	//size   - size*4 - size     //     0
	//size*4 ---------- size*4   //  5     1
	//       - size*4 - size     //     6
	//size*4 ---------- size*4   //  4     2
	//       - size*4 - size     //     3    7
	int16_t RcX[4]={0,0,0,0};
	int16_t RcY[4]={0,0,0,0};
	switch(snum)
	{
		case 0:
			RcX[3]=RcX[0]=x+size;RcX[1]=RcX[2]=x+size*5;
			RcY[0]=RcY[1]=y;RcY[2]=RcY[3]=size+y;
			break;
		case 3:
			RcX[3]=RcX[0]=x+size;RcX[1]=RcX[2]=x+size*5;
			RcY[0]=RcY[1]=y+size*10;RcY[2]=RcY[3]=size*11+y;
			break;
		case 6:
			RcX[3]=RcX[0]=x+size;RcX[1]=RcX[2]=x+size*5;
			RcY[0]=RcY[1]=y+size*5;RcY[2]=RcY[3]=size*6+y;
			break;
		case 1:
			RcX[3]=RcX[0]=x+size*5;RcX[1]=RcX[2]=x+size*6;
			RcY[0]=RcY[1]=y+size*1;RcY[2]=RcY[3]=size*5+y;
			break;
		case 2:
			RcX[3]=RcX[0]=x+size*5;RcX[1]=RcX[2]=x+size*6;
			RcY[0]=RcY[1]=y+size*6;RcY[2]=RcY[3]=size*10+y;
			break;
		case 5:
			RcX[3]=RcX[0]=x+size*0;RcX[1]=RcX[2]=x+size*1;
			RcY[0]=RcY[1]=y+size*1;RcY[2]=RcY[3]=size*5+y;
			break;
		case 4:
			RcX[3]=RcX[0]=x+size*0;RcX[1]=RcX[2]=x+size*1;
			RcY[0]=RcY[1]=y+size*6;RcY[2]=RcY[3]=size*10+y;
			break;
		case 7://dot segment
			RcX[3]=RcX[0]=x+size;RcX[1]=RcX[2]=x+size*2;
			RcY[0]=RcY[1]=y+size*10;RcY[2]=RcY[3]=size*11+y;
			break;
		case 8://up point of :
			RcX[3]=RcX[0]=x+size;RcX[1]=RcX[2]=x+size*2;
			RcY[0]=RcY[1]=y+size*3;RcY[2]=RcY[3]=size*4+y;
			break;
		case 9://dow poind of :
			RcX[3]=RcX[0]=x+size;RcX[1]=RcX[2]=x+size*2;
			RcY[0]=RcY[1]=y+size*7;RcY[2]=RcY[3]=size*8+y;
			break;
		case 10://slahs symbol
			RcX[0]=RcX[1]=x+size*4;RcX[2]=RcX[3]=x+size;
			RcY[0]=RcY[1]=y;RcY[2]=RcY[3]=size*11+y;
			break;
	}
	draw_triang(RcX[0],RcY[0],RcX[1],RcY[1],RcX[2],RcY[2],ARW_CODE);
	draw_triang(RcX[2],RcY[2],RcX[3],RcY[3],RcX[0],RcY[0],ARW_CODE);
}

void draw_dig(int x,int y,int size,int d)
{
	switch(d)
	{
		case 0:
			draw_seg(x,y,size,0);
			draw_seg(x,y,size,1);
			draw_seg(x,y,size,2);
			draw_seg(x,y,size,3);
			draw_seg(x,y,size,4);
			draw_seg(x,y,size,5);
			break;
		case 1:
			draw_seg(x,y,size,1);
			draw_seg(x,y,size,2);
			break;
		case 2:
			draw_seg(x,y,size,0);
			draw_seg(x,y,size,1);
			draw_seg(x,y,size,6);
			draw_seg(x,y,size,4);
			draw_seg(x,y,size,3);
			break;
		case 3:
			draw_seg(x,y,size,0);
			draw_seg(x,y,size,1);
			draw_seg(x,y,size,6);
			draw_seg(x,y,size,2);
			draw_seg(x,y,size,3);
			break;
		case 4:
			draw_seg(x,y,size,1);
			draw_seg(x,y,size,2);
			draw_seg(x,y,size,6);
			draw_seg(x,y,size,5);
			break;
		case 5:
			draw_seg(x,y,size,0);
			draw_seg(x,y,size,5);
			draw_seg(x,y,size,6);
			draw_seg(x,y,size,2);
			draw_seg(x,y,size,3);
			break;
		case 6:
			draw_seg(x,y,size,0);
			draw_seg(x,y,size,5);
			draw_seg(x,y,size,6);
			draw_seg(x,y,size,2);
			draw_seg(x,y,size,3);
			draw_seg(x,y,size,4);
			break;
		case 7:
			draw_seg(x,y,size,0);
			draw_seg(x,y,size,1);
			draw_seg(x,y,size,2);
			break;
		case 8:
			draw_seg(x,y,size,0);
			draw_seg(x,y,size,1);
			draw_seg(x,y,size,2);
			draw_seg(x,y,size,3);
			draw_seg(x,y,size,4);
			draw_seg(x,y,size,5);
			draw_seg(x,y,size,6);
			break;
		case 9:
			draw_seg(x,y,size,0);
			draw_seg(x,y,size,1);
			draw_seg(x,y,size,2);
			draw_seg(x,y,size,3);
			draw_seg(x,y,size,5);
			draw_seg(x,y,size,6);
			break;
	};
}
                  //   0     1     2     3     4     5     6     7     8     9    10    11    12    13    14  15
				  //  30    29    28    27    26    25    24    23    22    21    20    19    18    17    16
				  //        31    32    33    34    35    36    37    38    39    40    41    42    43    44  45
				  //        46    47    48    49    50    51    52    53    54    55    56    57    58    59
int32_t CS6D[16]= { 1024, 1018, 1002,  974,  935,  887,  828,  761,  685,  602,  512,  416,  316,  213,  107, 0};//COS 0...90
int32_t getC6D(uint8_t a)
{
	a=a%60;
	if (a<=15) return CS6D[a];
	if (a<=30) return -CS6D[30-a];
	if (a<=45) return -CS6D[a-30];
	return CS6D[60-a];
};
int32_t getS6D(uint8_t a)
{
	return getC6D(a+15);
}
//
#define sizeS 2
void draw_date(int x,int y,int DD,int MM,int YY)
{
	draw_dig(x,             y,sizeS,DD/10);
	draw_dig(x+sizeS*7     ,y,sizeS,DD%10);
	draw_seg(x+sizeS*13    ,y,sizeS,10);
	draw_dig(x+sizeS*19    ,y,sizeS,MM/10);
	draw_dig(x+sizeS*(7+19),y,sizeS,MM%10);
	draw_seg(x+sizeS*(26+6),y,sizeS,10);
	draw_dig(x+sizeS*38    ,y,sizeS,YY/10);
	draw_dig(x+sizeS*(7+38),y,sizeS,YY%10);

}
void draw_time(int x,int y,int H,int M,int s)
{
	H=H%24;
	draw_dig(x,             y,sizeS,H/10);
	draw_dig(x+sizeS*7     ,y,sizeS,H%10);
	draw_dig(x+sizeS*16    ,y,sizeS,M/10);
	draw_dig(x+sizeS*(7+16),y,sizeS,M%10);
	draw_dig(x+sizeS*32    ,y,sizeS,s/10);
	draw_dig(x+sizeS*(7+32),y,sizeS,s%10);
	//
	draw_seg(x+sizeS*13    ,y,sizeS,8);
	draw_seg(x+sizeS*13    ,y,sizeS,9);
	draw_seg(x+sizeS*29    ,y,sizeS,8);
	draw_seg(x+sizeS*29    ,y,sizeS,9);
}

void draw_dow(int x,int y,int dow)
{
	if (dow<0) return;
	if (dow>=7) return;
	uint16_t DOW_IMG[7][16]={	{0x0000,0x7800,0x4400,0x4478,0x7880,0x4480,0x4480,0x4480, 0x7878,0x00,0x00,0x00,0x00,0x00,0x00,0x00},// 0-Âñ
								{0x0000,0x7C00,0x4400,0x4488,0x4488,0x44F8,0x4488,0x4488, 0x4488,0x00,0x00,0x00,0x00,0x00,0x00,0x00},// 1-Ïí
								{0x0000,0x7800,0x4400,0x44F8,0x7820,0x4420,0x4420,0x4420, 0x7820,0x00,0x00,0x00,0x00,0x00,0x00,0x00},// 2-Âò
								{0x0000,0x3C00,0x4000,0x40F0,0x4088,0x4088,0x4088,0x4088, 0x3CF0,0x80,0x80,0x80,0x00,0x00,0x00,0x00},// 3-Ñð
								{0x0000,0x4400,0x4400,0x44F8,0x4420,0x3C20,0x0420,0x0420, 0x0420,0x00,0x00,0x00,0x00,0x00,0x00,0x00},// 4-×ò
								{0x0000,0x7C00,0x4400,0x44F8,0x4420,0x4420,0x4420,0x4420, 0x4420,0x00,0x00,0x00,0x00,0x00,0x00,0x00},// 5-Ïò
								{0x0000,0x3C78,0x4080,0x40F0,0x4088,0x4088,0x4088,0x4088, 0x3C70,0x00,0x00,0x00,0x00,0x00,0x00,0x00}};//6-Ñá
	for (int iy=0;iy<16;iy++)
	{
		for (int ix=0;ix<16;ix++)
			if (DOW_IMG[dow][iy]&(0x8000>>ix))
			{
				for (int k=0;k<=sizeS;k++)
					set_line(y+iy*sizeS+k,ix*sizeS+x,ix*sizeS+x+sizeS,ARW_CODE);
			};
	};
};

void draw_current(int x,int y,int curent_mA_x2)
{
	if (curent_mA_x2<0)
	{
		int16_t mAdr=(-curent_mA_x2)/2;
		int16_t mAdr100=mAdr/100;
		int16_t mAdr10=(mAdr-mAdr100*100)/10;
		int16_t mAdr1=  mAdr-mAdr100*100-mAdr10*10;
		draw_seg(x,          y,sizeS,6);
		draw_dig(x+sizeS*7,  y,sizeS,mAdr100);
		draw_dig(x+sizeS*14, y,sizeS,mAdr10);
		draw_dig(x+sizeS*21, y,sizeS,mAdr1);
		//
		draw_seg(x+sizeS*27, y,sizeS,7);
		//
		int16_t mAdr0=(-curent_mA_x2-(mAdr100*100+mAdr10*10+mAdr1)*2)*5;
		draw_dig(x+sizeS*29, y,sizeS,mAdr0);
	}
	else
	{
		int16_t mAdr=curent_mA_x2/2;
		int16_t mAdr100=mAdr/100;
		int16_t mAdr10=(mAdr-mAdr100*100)/10;
		int16_t mAdr1=  mAdr-mAdr100*100-mAdr10*10;
		draw_dig(x+sizeS*7,  y,sizeS,mAdr100);
		draw_dig(x+sizeS*14, y,sizeS,mAdr10);
		draw_dig(x+sizeS*21, y,sizeS,mAdr1);
		//
		draw_seg(x+sizeS*27, y,sizeS,7);
		//
		int16_t mAdr0=(curent_mA_x2-(mAdr100*100+mAdr10*10+mAdr1)*2)*5;
		draw_dig(x+sizeS*29, y,sizeS,mAdr0);
	};
}
void drawvoltage(int x,int y,int BatV)
{
	int16_t V=(BatV*11)/20;// V=step to 1mV
	int16_t V1_0=V/1000;
	V=V-V1_0*1000;
	int16_t V_100=V/100;	V=V-V_100*100;
	int16_t V_10=V/10;		V=V-V_10*10;
	int16_t V_1=V;
	draw_dig(x+sizeS*7,  y,sizeS,V1_0);
	draw_dig(x+sizeS*16, y,sizeS,V_100);
	draw_dig(x+sizeS*23, y,sizeS,V_10);
	draw_dig(x+sizeS*30, y,sizeS,V_1);
	draw_seg(x+sizeS*13, y,sizeS,7);
}
//
void draw_state(void *ptrstate)
{
	struct main_watch_t *p=ptrstate;
	//
	int H=p->hour;
	int M=p->minute;
	int s=p->sec;
	//int DD,int MM,int YY,int16_t curent_mA_x2,int16_t BatV;
	int32_t COS_A[12]={1024, 887, 512,    0,-512,-887,-1024,-887,-512,   0, 512, 887};
	int32_t SIN_A[12]={   0,-512,-887,-1024,-887,-512,    0, 512, 887,1024, 887, 512};
	//
	int16_t WathR=90;
	int16_t RcX[4]={-2,-2,2,2};
	int16_t RcY[4]={WathR+4,WathR+10,WathR+10,WathR+4};
	int16_t PcX[4],PcY[4];
	//clock table
	for (int cw=0;cw<12;cw++)
	{
		for (int i=0;i<4;i++)
		{
			PcX[i]=(RcX[i]*COS_A[cw]+RcY[i]*SIN_A[cw])>>10;
			PcY[i]=(-RcX[i]*SIN_A[cw]+RcY[i]*COS_A[cw])>>10;
		}
		//draw 0,1,2
		//draw 2,1,3
		draw_triang(PcX[0],PcY[0],PcX[1],PcY[1],PcX[2],PcY[2],ARW_CODE);
		draw_triang(PcX[0],PcY[0],PcX[3],PcY[3],PcX[2],PcY[2],ARW_CODE);
	};
	//sec arrow:
	{
		RcX[1]=0;
		RcX[0]=-4;RcX[2]=4;
		RcY[0]=WathR-30;
		RcY[2]=WathR-30;
		RcY[1]=WathR-10;
		//
		int32_t sinM=getS6D((s+30)%60);
		int32_t cosM=getC6D((s+30)%60);
		for (int i=0;i<3;i++)
		{
			PcX[i]=(RcX[i]*cosM+RcY[i]*sinM)>>10;
			PcY[i]=(-RcX[i]*sinM+RcY[i]*cosM)>>10;
		}
		draw_triang(PcX[0],PcY[0],PcX[1],PcY[1],PcX[2],PcY[2],ARW_CODE);
	}
	//min arrow:
	{
		RcX[1]=RcX[0]=-2;
		RcX[2]=RcX[3]=2;
		RcY[2]=RcY[1]=WathR*5/9;
		RcY[0]=RcY[3]=-20;
		//
		int32_t sinM=getS6D((M+30)%60);
		int32_t cosM=getC6D((M+30)%60);
		for (int i=0;i<4;i++)
		{
			PcX[i]=(RcX[i]*cosM+RcY[i]*sinM)>>10;
			PcY[i]=(-RcX[i]*sinM+RcY[i]*cosM)>>10;
		}
		draw_triang(PcX[0],PcY[0],PcX[1],PcY[1],PcX[2],PcY[2],ARW_CODE);
		draw_triang(PcX[0],PcY[0],PcX[3],PcY[3],PcX[2],PcY[2],ARW_CODE);
	}
	//H arrow:
	{
		RcX[1]=RcX[0]=-4;
		RcX[2]=RcX[3]=4;
		RcY[2]=RcY[1]=WathR*3/9;
		RcY[0]=RcY[3]=-20;
		//
		int Ht=(H*5+(M/12))%60;
		int32_t sinM=getS6D((Ht+30)%60);
		int32_t cosM=getC6D((Ht+30)%60);
		for (int i=0;i<4;i++)
		{
			PcX[i]=(RcX[i]*cosM+RcY[i]*sinM)>>10;
			PcY[i]=(-RcX[i]*sinM+RcY[i]*cosM)>>10;
		}
		draw_triang(PcX[0],PcY[0],PcX[1],PcY[1],PcX[2],PcY[2],ARW_CODE);
		draw_triang(PcX[0],PcY[0],PcX[3],PcY[3],PcX[2],PcY[2],ARW_CODE);
	}
	set_line(0,-1,1,BCKG_CODE);
	set_line(1,-1,1,BCKG_CODE);
	set_line(-1,-1,1,BCKG_CODE);
	set_line(0,0,0,ARW_CODE);
	//draw status:
	draw_date(-117,-117,p->day,p->mnt,p->year);
	draw_time(20,-117,H,M,s);
	draw_dow(-120+(2*7)*sizeS,-117+13*sizeS,p->dayofweek);
	draw_current(-120,120-sizeS*11-4,p->current);
	drawvoltage(120-sizeS*8*5,120-sizeS*11-4,p->BatV);
};