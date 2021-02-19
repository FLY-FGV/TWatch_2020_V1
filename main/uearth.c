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

DRAM_ATTR uint8_t earth_red_code[16]=  {U0_R,U1_R,U2_R,U3_R, U4_R,U5_R,U6_R,U7_R, U8_R,U9_R,U10_R,U11_R, U12_R,U13_R,U14_R,U15_R};
DRAM_ATTR uint8_t earth_blue_code[16]= {U0_B,U1_B,U2_B,U3_B, U4_B,U5_B,U6_B,U7_B, U8_B,U9_B,U10_B,U11_B, U12_B,U13_G,U14_B,U15_B};
DRAM_ATTR uint8_t earth_green_code[16]={U0_G,U1_G,U2_G,U3_G, U4_G,U5_G,U6_G,U7_G, U8_G,U9_G,U10_G,U11_G, U12_G,U13_B,U14_G,U15_G};

extern const uint8_t world_dat_start[] asm("_binary_WORLD_DAT_start");
extern const uint8_t world_dat_end[] asm("_binary_WORLD_DAT_end");
static int16_t *XYZ;//vertex array
static uint8_t *color;//poly color indexes
static uint8_t *ptrscr;//screen buff
static uint8_t *lightscr;//light buff
static int16_t Pmatrix[3][3]={{1024,0,0},{0,1024,0},{0,0,1024}};//project matrix
static int16_t sunV[3]={0,1024,0};//sun light beam vector
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

void earth_init()
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
	};
	#ifdef FIX_606_LAST_POINT_BUG
	XYZ[606*4*3+3*3]=  XYZ[608*4*3+0];
	XYZ[606*4*3+3*3+1]=XYZ[608*4*3+1];
	XYZ[606*4*3+3*3+2]=XYZ[608*4*3+2];
	printf("606 last point corrected!\n");
	#endif
	printf("earth is loaded\n");
};

void earth_deinit()
{
	free(XYZ);XYZ=NULL;
	free(color);color=NULL;
	free(ptrscr);ptrscr=NULL;
	free(lightscr);lightscr=NULL;
}

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
void set_line(int16_t y, int16_t xb, int16_t xe, uint8_t color)
{
	if (xb<MIN_EARTH_X && xe<MIN_EARTH_X) return;
	if (xb>MAX_EARTH_X && xe>MAX_EARTH_X) return;
	xb=clipX(xb);
	xe=clipX(xe);
	memset(&ptrscr[xb+OFFSET_X+(y+OFFSET_Y)*WIDTH_EARTH],color,xe-xb+1);
}

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

void set_sunA(float Az,float H)
{
	sunV[2]=1024*sin(H);
	sunV[1]=1024*cos(Az)*cos(H);
	sunV[0]=1024*sin(Az)*cos(H);
};
//
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
//
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

static int16_t K_light=1;
void earth_setKL(int16_t newKL)
{
	K_light=newKL;
}
IRAM_ATTR uint16_t earth_getcolor(int16_t x,int16_t y)
{
	uint8_t c0=ptrscr[x+y*WIDTH_EARTH];
	if (c0==BCKG_CODE) return RGB_G(U13_R,U13_G,U13_B);
	if (c0==ARW_CODE) return RGB_G(U14_R,U14_G,U14_B);
	//
	uint16_t intens=lightscr[x+y*WIDTH_EARTH]+K_light;
	uint32_t R_o=(earth_red_code[c0]*intens)/(255+K_light);
	uint32_t G_o=(earth_green_code[c0]*intens)/(255+K_light);
	uint32_t B_o=(earth_blue_code[c0]*intens)/(255+K_light);
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