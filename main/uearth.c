#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "uearth_color_map.h"
#include "esp_attr.h"
#include "watch_state.h"

#define WIDTH_EARTH 240
#define HEIGHT_EARTH 240

#define OFFSET_Y (HEIGHT_EARTH>>1)
#define OFFSET_X (WIDTH_EARTH>>1)

#define MAX_EARTH_X (OFFSET_X-1)
#define MIN_EARTH_X (-OFFSET_X)
#define MAX_EARTH_Y (OFFSET_Y-1)
#define MIN_EARTH_Y (-OFFSET_Y)

#define UFO_FIX_POINT_WDT 14

#define FIX_606_LAST_POINT_BUG


extern const uint8_t world_dat_start[] asm("_binary_WORLD_DAT_start");
extern const uint8_t world_dat_end[] asm("_binary_WORLD_DAT_end");
static int16_t *XYZ;//vertex array
static uint8_t *color;//poly color indexes
static uint8_t *ptrscr;//screen!
static uint8_t *lightscr;
static int16_t Pmatrix[3][3]={{1024,0,0},{0,1024,0},{0,0,1024}};//project matrix
static int32_t Rearth=100;//radius earth in pix
static uint16_t earth_num_pt=0;//num point's
static uint16_t earth_num_poly=0;//num poly
//


void latlon_toXYZ(int16_t *outdata,int16_t lon,int16_t lat)
{
	double lond=(-lon/2880.0)*M_PI*2.0;
	double latd=(lat/720.0)*M_PI*0.5;
	double X=cos(latd)*cos(lond);
	double Y=cos(latd)*sin(lond);
	double Z=sin(latd);
	outdata[0]=X*(1<<UFO_FIX_POINT_WDT);
	outdata[1]=Y*(1<<UFO_FIX_POINT_WDT);
	outdata[2]=Z*(1<<UFO_FIX_POINT_WDT);
}

void earth_unpack()
{
	uint32_t size=world_dat_end-world_dat_start;
	printf("World.dat size = %d\n",size);
	size=size/20;//num poly
	earth_num_poly=size;
	earth_num_pt=size*4;
	uint32_t memsize=size*sizeof(int16_t)*3*4;
	printf("mem for poly = %d\n",memsize);
	printf("mem for color = %d\n",size);
	printf("mem for pscr = %d\n",size*2*4*2);
	XYZ=(int16_t*)malloc(memsize);
	color=(uint8_t*)malloc(size);
	ptrscr=(uint8_t*)malloc(HEIGHT_EARTH*WIDTH_EARTH);
	lightscr=(uint8_t*)malloc(HEIGHT_EARTH*WIDTH_EARTH);
	printf("XYZ = %X\n",(int)XYZ);
	printf("color = %X\n",(int)color);
	printf("ptrscr = %X\n",(int)ptrscr);
	printf("lightscr = %X\n",(int)lightscr);
	//
	int16_t * data=(int16_t *)world_dat_start;
	for (int i=0;i<size;i++)
	{
		color[i]=data[i*10+8];
		if (data[i*10+6]==-1) color[i]=color[i]|128;
		latlon_toXYZ(XYZ+i*4*3,data[i*10+0],data[i*10+1]);
		latlon_toXYZ(XYZ+i*4*3+3,data[i*10+2],data[i*10+3]);
		latlon_toXYZ(XYZ+i*4*3+6,data[i*10+4],data[i*10+5]);
		latlon_toXYZ(XYZ+i*4*3+9,data[i*10+6],data[i*10+7]);
		/*
		printf("poly=%d\n",i);
		printf("%d\t%d\n",data[i*10+0],data[i*10+1]);
		printf("%d\t%d\n",data[i*10+2],data[i*10+3]);
		printf("%d\t%d\n",data[i*10+4],data[i*10+5]);
		if ((color[i]&128)==0)
		printf("%d\t%d\n",data[i*10+6],data[i*10+7]);
		*/
	};
	#ifdef FIX_606_LAST_POINT_BUG
	XYZ[606*4*3+3*3]=  XYZ[608*4*3+0];
	XYZ[606*4*3+3*3+1]=XYZ[608*4*3+1];
	XYZ[606*4*3+3*3+2]=XYZ[608*4*3+2];
	printf("606 last point corrected!\n");
	#endif
	printf("earth is loaded\n");
};

int16_t projectX(int16_t *XYZ_v)
{
	int32_t r=XYZ_v[0]*Pmatrix[1][0]+XYZ_v[1]*Pmatrix[1][1]+XYZ_v[2]*Pmatrix[1][2];//10+16(max)=26
	r=r>>10;//
	return (r*Rearth)>>UFO_FIX_POINT_WDT;
}

int16_t projectY(int16_t *XYZ_v)
{
	int32_t r=(XYZ_v[0]*Pmatrix[2][0]+XYZ_v[1]*Pmatrix[2][1]+XYZ_v[2]*Pmatrix[2][2]);
	r=r>>10;
	return (r*Rearth)>>UFO_FIX_POINT_WDT;

}

int32_t projectVisible(int16_t *XYZ_v)
{
	return (XYZ_v[0]*Pmatrix[0][0]+XYZ_v[1]*Pmatrix[0][1]+XYZ_v[2]*Pmatrix[0][2]);//max: 10+16=26 bit
}

#define swap_i16(a,b) {int16_t tmp;tmp=a;a=b;b=tmp;}

int16_t max3i(int16_t m1, int16_t m2, int16_t m3)
{
	if (m2 > m1) m1 = m2;
	if (m3 > m1) m1 = m3;
	return m1;
}
int16_t min3i(int16_t m1, int16_t m2, int16_t m3)
{
	if (m2 < m1) m1 = m2;
	if (m3 < m1) m1 = m3;
	return m1;
}
int16_t max2i(int16_t m1, int16_t m2)
{
	if (m2 > m1) m1 = m2;
	return m1;
}
int16_t min2i(int16_t m1, int16_t m2)
{
	if (m2 < m1) m1 = m2;
	return m1;
}

static inline int16_t clipX(int16_t x){if (x<=MIN_EARTH_X) return MIN_EARTH_X;if (x>=MAX_EARTH_X) return MAX_EARTH_X;return x;};
static inline int16_t clipY(int16_t y){if (y<=MIN_EARTH_Y) return MIN_EARTH_Y;if (y>=MAX_EARTH_Y) return MAX_EARTH_Y;return y;};
static inline void set_line(int16_t y, int16_t xb, int16_t xe, uint8_t color)
{
	if (xb<MIN_EARTH_X && xe<MIN_EARTH_X) return;
	if (xb>MAX_EARTH_X && xe>MAX_EARTH_X) return;
	xb=clipX(xb);
	xe=clipX(xe);
	memset(&ptrscr[xb+OFFSET_X+(y+OFFSET_Y)*WIDTH_EARTH],color,xe-xb+1);
}

//(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t x3, int32_t y3,uint8_t color)
void draw_triang(int16_t x1, int16_t y1, int16_t x2, int16_t y2, int16_t x3, int16_t y3,uint8_t color)
{
	if (y1 == y2 && y1 == y3)
	{
		if (y1>MAX_EARTH_Y) return;
		if (y1<MIN_EARTH_Y) return;
		set_line(y1, min3i(x1, x2, x3), max3i(x1, x2, x3), color);
		return;
	}
	if (y2 > y1) { swap_i16(y1, y2); swap_i16(x1, x2); };
	if (y3 > y1) { swap_i16(y1, y3); swap_i16(x1, x3); };
	//y1 - is upper point
	if (y2<y3) { swap_i16(y2, y3); swap_i16(x2, x3); };
	//y3 - lower point
	//test & clip
	if (y1>MAX_EARTH_Y && y3>MAX_EARTH_Y) return;
	if (y1<MIN_EARTH_Y && y3<MIN_EARTH_Y) return;
	//
	int16_t dyM = y3 - y1;
	int16_t dxM = x3 - x1;
	for (int32_t yi = clipY(y3); yi <= clipY(y1); yi++)
	{
		int32_t x13 = x3+(((yi - y3)*dxM) / dyM);
		int32_t xe = 0;
		if (yi <= y2)
		{
			if (y3 == y2)
				xe = x2;
			else
			{
				xe= x2 + ((yi - y2)*(x3-x2)) / (y3-y2);
			}
			set_line(yi, min2i(x13, xe), max2i(x13, xe), color);
		}
		else
		{
			if (y1 == y2)
				xe = x2;
			else
			{
				xe = x1 + ((yi - y1)*(x1 - x2)) / (y1 - y2);
			}
			set_line(yi, min2i(x13, xe), max2i(x13, xe), color);
		}
	}
}

void project_poly3(int ip)
{
	int i=ip<<2;
	//
	int32_t vis=projectVisible(XYZ+i*3)+projectVisible(XYZ+i*3+3)+projectVisible(XYZ+i*3+6);
	if (vis<0) return;
	//
	int16_t x1 = projectX(XYZ+i*3);//
	int16_t y1 = projectY(XYZ+i*3);
	int16_t x2 = projectX(XYZ+i*3+3);//
	int16_t y2 = projectY(XYZ+i*3+3);
	int16_t x3 = projectX(XYZ+i*3+6);//
	int16_t y3 = projectY(XYZ+i*3+6);//
	//
	draw_triang(x1,y1,x2,y2,x3,y3,color[ip]&0xf);
};

void project_poly4(int ip)
{
	int i=ip<<2;
	//
	int32_t vis13=projectVisible(XYZ+i*3)+projectVisible(XYZ+i*3+6);
	int32_t vis123=vis13+projectVisible(XYZ+i*3+3);
	int32_t vis134=vis13+projectVisible(XYZ+i*3+9);
	if (vis123<0 && vis134<0) return;
	//
	int16_t x1 = projectX(XYZ+i*3);//
	int16_t y1 = projectY(XYZ+i*3);
	int16_t x2 = projectX(XYZ+i*3+3);//
	int16_t y2 = projectY(XYZ+i*3+3);
	int16_t x3 = projectX(XYZ+i*3+6);//
	int16_t y3 = projectY(XYZ+i*3+6);//
	int16_t x4 = projectX(XYZ+i*3+9);//
	int16_t y4 = projectY(XYZ+i*3+9);//
	//
	if (vis123>=0) draw_triang(x1,y1,x2,y2,x3,y3,color[ip]&0xf);
	if (vis134>=0) draw_triang(x1,y1,x3,y3,x4,y4,color[ip]&0xf);
};

void draw_sph(int32_t R)
{
	int32_t delta = 1 - 2 * R;
	int32_t error = 0;
	int16_t y = R;
	int16_t x = 0;
	while (y >= 0)
	{
		error = 2 * (delta + y) - 1;
		if ((delta < 0) && (error <= 0))
		{
			delta += 2 * ++x + 1;
			continue;
		}
		if ((delta > 0) && (error > 0))
		{
			if (y<=MAX_EARTH_Y)
			{
				set_line(y, -x+1, x-1, OCEAN_CODE);
				if (y!=0 && -y>=MIN_EARTH_Y) set_line(-y, -x+1, x-1, OCEAN_CODE);
			}
			delta -= 2 * --y + 1;
			continue;
		}
		if (y<=MAX_EARTH_Y)
		{
			set_line(y, -x+1, x-1, OCEAN_CODE);
			if (y!=0 && -y>=MIN_EARTH_Y) set_line(-y, -x+1, x-1, OCEAN_CODE);
		}
		delta += 2 * (++x - y--);
	}
}
uint16_t sqrt_newton(uint32_t x)  // Hardware algorithm [GLS]
{
   uint32_t m, y, b;
   uint16_t m16, y16, b16, x16;
   if (x & 0xffff0000)
   {
      if (x & 0xff000000)
      {
         m = 0x40000000;
      }
      else
      {
         m = 0x00400000;
      }
      y = 0;
      do               // Do 12-16 times.
      {
         b = y | m;
         y >>= 1;
         if (x >= b)
         {
            x -= b;
            y |=  m;
         }
         m >>= 2;
      } 
      while (m);
      return y;
   }
   else
   {
      if(x & 0x0000ff00)
      {
         m16 = 0x4000;
      }
      else
      {
         m16 = 0x0040;
      }
         y16 = 0;
         x16 = x;
      do              // Do 4-8 times.
      {
         b16 = y16 | m16;
         y16 >>=  1;
         if (x16 >= b16)
         {
            x16 -= b16;
            y16 |= m16;
         }
         m16 >>= 2;
      }
      while (m16);
      return y16;
   }
}
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
	draw_dow(-120+(2*7)*sizeS,-117+11*sizeS,p->dayofweek);
	draw_current(-120,120-sizeS*11-4,p->current);
	drawvoltage(120-sizeS*8*5,120-sizeS*11-4,p->BatV);
};

int sunV[3]={0,1024,0};
void set_sunA(float Az,float H)
{
	sunV[2]=1024*sin(H);
	sunV[1]=1024*cos(Az)*cos(H);
	sunV[0]=1024*sin(Az)*cos(H);
};
/*
//old version very stupid calc!
void earth_apply_sun()
{
	int32_t R2E=Rearth*Rearth;
	int32_t s_X=(sunV[0]*Pmatrix[0][0]+sunV[1]*Pmatrix[0][1]+sunV[2]*Pmatrix[0][2])>>10;
	int32_t s_Y=(sunV[0]*Pmatrix[1][0]+sunV[1]*Pmatrix[1][1]+sunV[2]*Pmatrix[1][2])>>10;
	int32_t s_Z=(sunV[0]*Pmatrix[2][0]+sunV[1]*Pmatrix[2][1]+sunV[2]*Pmatrix[2][2])>>10;
	for (int iy=0;iy<HEIGHT_EARTH;iy++)
	{
		for (int ix=0;ix<WIDTH_EARTH;ix++)
		{
			uint8_t cor_c=ptrscr[ix+iy*WIDTH_EARTH];
			if (cor_c==BCKG_CODE) continue;
			int32_t Z=iy-OFFSET_Y;
			int32_t Y=ix-OFFSET_X;
			int32_t X=R2E-Z*Z-Y*Y;
			if (X<0) continue;
			X=sqrt_newton(X);
			int32_t P=(Z*s_Z+Y*s_Y+X*s_X)/Rearth;
			if (P<0) P=0;
			ptrscr[ix+iy*WIDTH_EARTH]=cor_c|((P>>2)&0xF0);
		};
	};
}
*/
IRAM_ATTR void earth_apply_sun()
{
	memset(lightscr,0,WIDTH_EARTH*HEIGHT_EARTH);
	int32_t s_X=(sunV[0]*Pmatrix[0][0]+sunV[1]*Pmatrix[0][1]+sunV[2]*Pmatrix[0][2])>>10;
	int32_t s_Y=(sunV[0]*Pmatrix[1][0]+sunV[1]*Pmatrix[1][1]+sunV[2]*Pmatrix[1][2])>>10;
	int32_t s_Z=(sunV[0]*Pmatrix[2][0]+sunV[1]*Pmatrix[2][1]+sunV[2]*Pmatrix[2][2])>>10;
	for (int iy=0;iy<(HEIGHT_EARTH>>1);iy++)
	{
		//Z - is y
		//Y - is x
		//X - arrow to user
		int32_t R2E=Rearth*Rearth-iy*iy;
		if (R2E<0) continue;
		for (int ix=0;ix<(WIDTH_EARTH>>1);ix++)
		{
			int32_t Xu=R2E-ix*ix;
			if (Xu<0) continue;
			Xu=sqrt_newton(Xu);
			int32_t P=(iy*s_Z+ix*s_Y+Xu*s_X)/Rearth;
			if (P>0)
			{
				uint32_t Adr=OFFSET_X+ix+(iy+OFFSET_Y)*WIDTH_EARTH;
				//ptrscr[Adr]=ptrscr[Adr]|((P>>2)&0xF0);
				lightscr[Adr]=P>>2;//8bit's!
			};
			if (iy>0)
			{
				P=(-iy*s_Z+ix*s_Y+Xu*s_X)/Rearth;
				if (P>0)
				{
					uint32_t Adr=OFFSET_X+ix+(-iy+OFFSET_Y)*WIDTH_EARTH;
					//ptrscr[Adr]=ptrscr[Adr]|((P>>2)&0xF0);
					lightscr[Adr]=P>>2;//8bit's!
				}
			}
			if (ix>0)
			{
				P=(iy*s_Z-ix*s_Y+Xu*s_X)/Rearth;
				if (P>0)
				{
					uint32_t Adr=OFFSET_X-ix+(iy+OFFSET_Y)*WIDTH_EARTH;
					//ptrscr[Adr]=ptrscr[Adr]|((P>>2)&0xF0);
					lightscr[Adr]=P>>2;//8bit's!
				};
				if (iy>0)
				{
					P=(-iy*s_Z-ix*s_Y+Xu*s_X)/Rearth;
					if (P>0)
					{
						uint32_t Adr=OFFSET_X-ix+(-iy+OFFSET_Y)*WIDTH_EARTH;
						//ptrscr[Adr]=ptrscr[Adr]|((P>>2)&0xF0);
						lightscr[Adr]=P>>2;//8bit's!
					}
				};
			};
		};
	};
}

IRAM_ATTR void earth_proj_to_scr()
{
	if ((Rearth*Rearth)>=(((WIDTH_EARTH*WIDTH_EARTH)>>2)+((HEIGHT_EARTH*HEIGHT_EARTH)>>2)))
	{
		memset(ptrscr,OCEAN_CODE,WIDTH_EARTH*HEIGHT_EARTH);
	}
	else
	{
		memset(ptrscr,BCKG_CODE,WIDTH_EARTH*HEIGHT_EARTH);
		draw_sph(Rearth-1);
	};
	for (int ip=0;ip<earth_num_poly;ip++)
	{
		if (color[ip]&128)
			project_poly3(ip);
		else
			project_poly4(ip);
	}
}

extern struct main_watch_t main_watch_state;
IRAM_ATTR uint16_t earth_getcolor(int16_t x,int16_t y)
{
	uint8_t c0=ptrscr[x+y*WIDTH_EARTH];
	if (c0==BCKG_CODE) return RGB_G(U13_R,U13_G,U13_B);
	if (c0==ARW_CODE) return RGB_G(U14_R,U14_G,U14_B);
	//return RGB_G(earth_red_code[c0],earth_green_code[c0],earth_blue_code[c0]);
	uint16_t intens=lightscr[x+y*WIDTH_EARTH]+main_watch_state.KLight;
	uint32_t R_o=(earth_red_code[c0]*intens)/(255+main_watch_state.KLight);
	uint32_t G_o=(earth_green_code[c0]*intens)/(255+main_watch_state.KLight);
	uint32_t B_o=(earth_blue_code[c0]*intens)/(255+main_watch_state.KLight);
	return RGB_G(R_o,G_o,B_o);
}

void setRearth(int32_t newR)
{
	if (newR>=2 && newR<=32767)
		Rearth=newR;
	else
	{
		printf("UFO Earth: bad radius (attempt set R=%d)!\n",newR);
	}
}
int  getRearth() {return Rearth;};
//
void updateP_setnewpointview(float lat,float lon)
{
	float clat=cos(lat);
	float slat=sin(lat);
	float clon=cos(lon);
	float slon=sin(lon);
	//
	float oxx=clon*clat;
	float oxy=slon*clat;
	float oxz=slat;
	//
	float ozx=-slat*clon;
	float ozy=-slat*slon;
	float ozz=clat;
	//oy = ox*oz
	float oyx=oxy*ozz-oxz*ozy;
	float oyy=oxz*ozx-oxx*ozz;
	float oyz=oxx*ozy-oxy*ozx;
	//
	Pmatrix[0][0]=1024*oxx;
	Pmatrix[0][1]=1024*oxy;
	Pmatrix[0][2]=1024*oxz;
	Pmatrix[1][0]=1024*oyx;
	Pmatrix[1][1]=1024*oyy;
	Pmatrix[1][2]=1024*oyz;
	Pmatrix[2][0]=1024*ozx;
	Pmatrix[2][1]=1024*ozy;
	Pmatrix[2][2]=1024*ozz;
};