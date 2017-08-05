#include <opora.h>

#define MY_PI 512
#define USE_SVPWM
#define abs(a) ((a>0)?a:-a)

extern const int32_t cos_tb[1024];

//#define Ki 1
//#define Kp 0

struct pi_reg_state{
	int32_t ki;
	int32_t kp;
	int32_t a;
	int32_t y;	
};

int32_t mycos(int32_t a)
{
	return cos_tb[1023&a];
}

int32_t mysin(int32_t a)
{
	return cos_tb[1023&(a+3*MY_PI/2)];
}

void update(struct pi_reg_state *s, int32_t e, int32_t fs)
{
	int32_t a = s->a;
	int32_t d = s->ki*e;
	
	// will accumulator grow up?
	if(fs) if( ((a>0)&&(d>0))||((a<0)&&(d<0)) ) d = 0;

	a += d;
	s->y = e*s->kp + a;
	s->a = a;
	
	//s->y = 1024*e + s->a;
	//s->a = s->y - 782*e;
}

int32_t dot3(int32_t *a, int32_t *b)
{
	return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

void abc_to_dq(int32_t *abc, int32_t *dq, int32_t angle)
{
	int32_t ct[3] = {	cos_tb[angle], 
						cos_tb[1023&(angle+(4*512)/3)], 
						cos_tb[1023&(angle+(2*512)/3)] };
	int32_t st[3] = {	cos_tb[1023&(3*512/2+angle)], 
						cos_tb[1023&(3*512/2+angle+(4*512)/3)], 
						cos_tb[1023&(3*512/2+angle+2*512/3)] };

	dq[0] = (dot3(abc, ct)) >> 10;	
	dq[1] = (-dot3(abc, st)) >> 10;
}

void dq_to_abc(int32_t *abc, int32_t *dq, int32_t angle)
{
	abc[0] = (dq[0]*cos_tb[angle] 				- dq[1]*cos_tb[1023&(angle+3*512/2)]) >> 20;
	abc[1] = (dq[0]*cos_tb[1023&(angle+4*512/3)] - dq[1]*cos_tb[1023&(angle+4*512/3+3*512/2)]) >> 20;
	abc[2] = (dq[0]*cos_tb[1023&(angle+2*512/3)] - dq[1]*cos_tb[1023&(angle+2*512/3+3*512/2)]) >> 20;
}

// calc magnitude and angle of vector using CORDIC algorithm
void cord_atan(int32_t *v, int32_t *ang, int32_t *mag)
{
	const int32_t AngTable[] = {128, 76, 40, 20, 10, 5, 3, 1};
	const int32_t kc[] = {724,  648, 628,  623,  623,  622,  622,  622};
	int32_t SumAngle = 0; 
	int i = 0;
	int x, y, x1, y1;
	int ns = 0;

	x = abs(v[0]);
	y = v[1];

	for(i = 0; i < 8; i++)
	{		
		ns++;
		
		x1 = x;
		y1 = y;
			
		if(y > 0){
			x = x1 + (y1 >> i); 
			y = y1 - (x1 >> i); 
			SumAngle = SumAngle + AngTable[i]; 
		}else{
			x = x1 - (y1 >> i); 
			y = y1 + (x1 >> i); 
			SumAngle = SumAngle - AngTable[i]; 
		}
		if(y == 0) break;
	}
	
	if(v[0] < 0) SumAngle = MY_PI-SumAngle;		
	if(SumAngle < 0) SumAngle += 2*MY_PI;	
	
	*ang = SumAngle;
	*mag = (kc[ns-1]*x) >> 10;
}

int32_t sinpwm(int32_t *abc, int32_t *dq, int32_t phase)
{
	int32_t fs = 0;
	int32_t mag;
	int32_t ang;
	cord_atan(dq, &ang, &mag);
	
	mag = mag >> 10;	
	if(mag > 500) {
		mag = 500;
		fs = 1;
	}
	else fs = 0;
	
	dq_to_abc(abc, dq, phase);
	
	return fs;	
}

int32_t svpwm(int32_t *abc, int32_t *dq, int32_t phase)
{
	int32_t fs = 0;
	int32_t mag;
	int32_t ang;
	cord_atan(dq, &ang, &mag);
	
	mag = mag >> 10;
	int32_t phi = 1023&(phase + ang);
	
	if(mag > 500) {
		mag = 500;
		fs = 1;
	}
	else fs = 0;
			
	int32_t ns = (phi*6) >> 10;	 // get the sector number
	int32_t r1;
	int32_t r2;
	
	switch(ns){
		case 0:
		r1 = mag*mysin(7*MY_PI/3-phi) >> 10;
		r2 = mag*mysin(phi) >> 10;
		
		abc[0] = r1+r2;
		abc[1] = -r1+r2;
		abc[2] = -r1-r2;			
		break;
		
		case 1:
		phi -= MY_PI/3;
		r1 = mag*mysin(7*MY_PI/3-phi) >> 10;
		r2 = mag*mysin(phi) >> 10;
		
		abc[0] = r1-r2;
		abc[1] = r1+r2;
		abc[2] = -r1-r2;		
		break;
		
		case 2:
		phi -= 2*MY_PI/3;
		r1 = mag*mysin(7*MY_PI/3-phi) >> 10;
		r2 = mag*mysin(phi) >> 10;
		
		abc[0] = -r1-r2;
		abc[1] = r1+r2;
		abc[2] = -r1+r2;
		break;
		
		case 3:
		phi -= 3*MY_PI/3;
		r1 = mag*mysin(7*MY_PI/3-phi) >> 10;
		r2 = mag*mysin(phi) >> 10;
		
		abc[0] = -r1-r2;
		abc[1] = r1-r2;
		abc[2] = r1+r2;		
		break;
		
		case 4:
		phi -= 4*MY_PI/3;
		r1 = mag*mysin(7*MY_PI/3-phi) >> 10;
		r2 = mag*mysin(phi) >> 10;
		
		abc[0] = -r1+r2;
		abc[1] = -r1-r2;
		abc[2] = r1+r2;	
		break;
		
		case 5:
		phi -= 5*MY_PI/3;
		r1 = mag*mysin(7*MY_PI/3-phi) >> 10;
		r2 = mag*mysin(phi) >> 10;
		
		abc[0] = +r1+r2;
		abc[1] = -r1-r2;
		abc[2] = r1-r2;
		break;
	}
	
	return fs;
}

int32_t get_speed(int32_t enc, int32_t *pos)
{
	int32_t denc;
	static int32_t enc1 = 0;
	static int32_t enc2 = 0;
	int32_t rate = 60*(120000000/5/1024/8);
	
	denc = (enc-enc2);
	enc2 = enc1;
	enc1 = enc;
	if(abs(denc) > 1000){
		if(denc < 0) denc += 4096;
		else denc -= 4096;
	}		
	
	*pos += denc;
	
	return ((denc>>1)*rate)>>12;
} 

