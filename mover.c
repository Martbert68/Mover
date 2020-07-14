#include <alsa/asoundlib.h>
#include <dirent.h>
#include <pthread.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <jerror.h>
#include <jpeglib.h>
#include <setjmp.h>



#define VX_SIZE 1920 
#define VY_SIZE 1080 
#define HX 960
#define HY 540
#define BWHY 380
#define X_SIZE 1700
#define Y_SIZE 940 
#define XP_SIZE 1280
#define YP_SIZE 920 
#define X_ENV 1180 
#define Y_ENV 512 
#define SMAX 1764000
#define SAVEM 105840000 
#define SLEN 300
#define SWHY 30
#define COLS 96 
#define ROWS 6 
#define CORS COLS*ROWS
#define MIN 5292000
#define ALSA_PCM_NEW_HW_PARAMS_API

/*static float notes[108]={
16.35,17.32,18.35,19.45,20.60,21.83,23.12,24.50,25.96,27.50,29.14,30.87,
32.70,34.65,36.71,38.89,41.20,43.65,46.25,49.00,51.91,55.00,58.27,61.74,
65.41,69.30,73.42,77.78,82.41,87.31,92.50,98.00,103.8,110.0,116.5,123.5,
130.8,138.6,146.8,155.6,164.8,174.6,185.0,196.0,207.7,220.0,233.1,246.9,
261.6,277.2,293.7,311.1,329.6,349.2,370.0,392.0,415.3,440.0,466.2,493.9,
523.3,554.4,587.3,622.3,659.3,698.0,740.0,784.0,830.6,880.0,932.3,987.8,
1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976,
2093, 2217, 2349, 2489, 2637, 2794, 2960, 3136, 3322, 3520, 3729, 3951,
4186, 4435, 4699, 4978, 5274, 5588, 5920, 6272, 6645, 7040, 7459, 7902 }; */

static float notes[96]={
16.35,18.35,20.60,21.83,24.50,27.50,30.87,32.70,36.71,41.20,43.65,49.00,55.00,61.74,
17.32,19.45,23.12,25.96,29.14, 34.65,38.89,46.25,51.91,58.27,
65.41,73.42,82.41,87.31,98.00,110.0,123.5, 130.8,146.8,164.8,174.6,196.0,220.0,246.9,
69.30,77.78,92.50,103.8,116.5, 138.6,155.6,185.0,207.7,233.1,
261.6,293.7,329.6,349.2,392.0,440.0,493.9, 523.3,587.3,659.3,698.0,784.0,880.0,987.8,
277.2,311.1,370.0,415.3,466.2, 554.4,622.3,740.0,830.6,932.3,
1047, 1175, 1319, 1397, 1568, 1760, 1976, 2093, 2349, 2637, 2794, 3136, 3520, 3951,
1109, 1245, 1480, 1661, 1865, 2217, 2489, 2960, 3322, 3729 };

/* jpegs */
int jayit(unsigned char *,int, int, char *);


/* here are our X variables */
Display *display;
XColor    color[100];
int screen,start,sbsz,w,wmax;
Window win,cwin,vwin;
GC gc,cgc,vc;
XImage *x_image;
XImage *cx_image;
unsigned char *x_buffer;
unsigned long black,white;

/* here are our X routines declared! */
void init_x();
void close_x();
void redraw();

// sound
void from_to(int,int,int);
int readwav(char *,int,int,int,int);
void drawwav(int);
void pushwav(int,int,int,int);
void main_osc (int,int);
void *spkr();
void *lfo();
void *explorer();
void *visualizer();
void layout();
void draw_seq();
void do_seq_buttons();
void draw_env(int);
void button(int, int, int);
void s_button(int, int, int);
void seq_button(int, int, int, int);
void tutton(int, int, int, int);
void slider(int,char *);
void sslider(int,char *);
void knob(int, int);
void sknob(int, int);
void setoa();
void box(int,int,int);
void save_wav(int);
void load_chapter(int);
void save_chapter(int);
void p_point (int,int,int,int,int,char *);
void balance(int,int *,int *,int *);

// global variables
short *ring;
short *save;
unsigned long _RGB(int ,int , int );
long verb[ROWS][44100*2*10];
short sample[21][SMAX];
short envelope[ROWS+1][X_ENV];
int ssize[ROWS];
int point[ROWS];
int tnote[ROWS];
int vamp[ROWS];
int vdur[ROWS];
int vvoc[ROWS];
int vocin[ROWS];
int vvow[ROWS];
int wob[ROWS];
int fwob[ROWS];
int goodh[ROWS];
int badh[ROWS];
int baseh[ROWS];
int lfilt[ROWS];
int hfilt[ROWS];
int lpan[ROWS];
int rpan[ROWS];
int pan[ROWS];
int vlpan[ROWS];
int vrpan[ROWS];
int vpan[ROWS];
int phase[ROWS];
float prevl[ROWS];
float prevr[ROWS];
int strigger[ROWS][COLS];
int etrigger[ROWS][COLS];
int atrigger[ROWS][COLS];
int bpm,vp,pages,page,vl,lfo_run,chapter,chap_run,run_on;
int *sasta,*rasta,sp,lp;

void usage ()
{
	printf("usage: mover\n");
	exit (1);
}

int main(int argc,char *argv[])
{
 
	int now,lastv,ahead,loop,off,ring_off,last_ring,next_ring,col,row;
        struct timespec tim, tim2;
	char c;
	pthread_t spkr_id,lfo_id,explorer_id,sequence_id,visualizer_id;

        ring = (short *) malloc(sizeof(short)*SMAX);
	// 10 minutes worth;
	sp=0;
	// visualizer off.
	vl=0;
        save = (short *) malloc(sizeof(short)* SAVEM);
    	rasta=(int *)malloc(sizeof (int) * XP_SIZE*YP_SIZE);
    	sasta=(int *)malloc(sizeof (int) * X_SIZE*Y_SIZE);
	start=0;chapter=1;run_on=1;

	init_x();

	tim.tv_sec = 0;
	tim.tv_nsec = 100L;

	ring_off=0;
	lastv=0;
	last_ring=0;
	ahead=9;
	w=0;
	bpm=120;
	vp=0;
	pages=2;
	page=1;

	for (loop=0;loop<441000*2;loop++){ verb[0][loop]=0;}
	for (loop=0;loop<ROWS;loop++){ vocin[loop]=0;ssize[loop]=88200;vamp[loop]=200;vdur[loop]=11000+(loop*8000);wob[loop]=0;fwob[loop]=10;point[loop]=0;vvoc[loop]=0;vvow[loop]=0;goodh[loop]=0;badh[loop]=0;baseh[loop]=300;lpan[loop]=150;rpan[loop]=150;pan[loop]=150;vlpan[loop]=150;vrpan[loop]=150;vpan[loop]=150;}
	for (loop=0;loop<X_ENV;loop++){ envelope[0][loop]=0; envelope[1][loop]=0; envelope[2][loop]=0; envelope[3][loop]=0; envelope[4][loop]=0; envelope[5][loop]=0; envelope[ROWS][loop]=-1;}


	load_chapter(1);

	pthread_create(&spkr_id, NULL, spkr, NULL);
	pthread_create(&lfo_id, NULL, lfo, NULL);
	pthread_create(&explorer_id, NULL, explorer, NULL);
	pthread_create(&visualizer_id, NULL, visualizer, NULL);

	while (1>0)
	{
		// sleep if no work
        	int sleep;
        	sleep=1;
        	while (sleep==1)
        	{
                now=start;
                if ( now == lastv)
                {
                        nanosleep(&tim , &tim2);
                }else{
                        //printf("now %d last %d diff %d\n",now,lastv,now-lastv);
                        lastv=now;
                        next_ring=now+ahead;
                        if (next_ring>=10){ next_ring=next_ring-10;}
                        //printf("last %d next %d now %d\n",last_ring,next_ring,now);
                        sleep=0;
                }
        }
        from_to(last_ring,next_ring,sbsz/2);
        //printf ("last %d next %d \n",last_ring,next_ring);
        last_ring=next_ring+1;if(last_ring>=10){last_ring=0;}
  }

}

void save_wav (int points)
{
	int *fhead,len,chan,sample_rate,bits_pers,byte_rate,ba,size;
        fhead=(int *)malloc(sizeof(int)*11);

        len=60;
        chan=2;
        sample_rate=44100;
        bits_pers=16;
        byte_rate=(sample_rate*chan*bits_pers)/8;
        ba=((chan*bits_pers)/8)+bits_pers*65536;
        size=chan*len*sample_rate;
        size=points;

        fhead[0]=0x46464952;
        fhead[1]=36;
        fhead[2]=0x45564157;
        fhead[3]=0x20746d66;
        fhead[4]=16;
        fhead[5]=65536*chan+1;
        fhead[6]=sample_rate;
        fhead[7]=byte_rate;
        fhead[8]=ba;
        fhead[9]=0x61746164;
        fhead[10]=(size*chan*bits_pers)/8;


    	FILE *record;
        record=fopen("record.wav","wb");
        fwrite(fhead,sizeof(int),11,record);
        fwrite(save,sizeof(short),points,record);
        fclose (record);
        free(fhead);
}

// heres the clever bit
void setoa ()
{
	int bars,bar,row,beat,col;
	static int beats=16;
	bars=COLS/beats;
	for (row=0;row<ROWS;row++)
	{
		for (bar=0;bar<bars;bar++)
		{
			int scount,ecount,dpos,ll,diff;
			int s_pos[beats],e_pos[beats];
			ecount=0; scount=0;
			for (beat=0;beat<beats;beat++)
			{
				s_pos[beat]=0; e_pos[beat]=0;
				col=beat+(bar*beats);
				atrigger[row][col]=0;
				if (strigger[row][col]>0) { s_pos[scount]=col; scount++;}
				if (etrigger[row][col]>0) { e_pos[ecount]=col; ecount++;}
        			//printf ("row %d col %d  bar %d scount %d ecount %d\n",row, col,bar,scount,ecount);
			}
			if (scount==ecount && scount>0)
			{
				for(ll=0;ll<scount;ll++)
				{
					dpos=e_pos[ll]-s_pos[ll];
					dpos=s_pos[ll]+((dpos*(page-1))/(pages-1));
					//lets tend towards our page or perhaps miss
					int chancea,chancee;
					chancea=((pages-page)*100)/(pages-1);
					chancee=((page-1)*100)/(pages-1);
					chancea=chancea+rand()%100;
					chancee=chancee+rand()%100;
					if (chancea>=chancee )
					{
						atrigger[row][dpos]=strigger[row][s_pos[ll]];
					}else {
						atrigger[row][dpos]=etrigger[row][e_pos[ll]];
					}
					if (rand()%1000<100){atrigger[row][dpos]=0;}
				}	
			}
			if (scount>ecount)
			{
				for(ll=0;ll<ecount;ll++)
				{
					dpos=e_pos[ll]-s_pos[ll];
					dpos=s_pos[ll]+((dpos*(page-1))/(pages-1));
					//atrigger[row][dpos]=1;
					//atrigger[row][dpos]=(strigger[row][s_pos[ll]]+etrigger[row][e_pos[ll]])/2;
					atrigger[row][dpos]=(((pages-page)*strigger[row][s_pos[ll]])+(page-1)*etrigger[row][e_pos[ll]])/(pages-1);
				}	
				diff=scount-ecount;
				diff=diff-((diff*(page-1))/(pages-1));
				for(ll=0;ll<diff;ll++)
				{
					dpos=s_pos[ecount+ll];
					//atrigger[row][dpos]=1;
					atrigger[row][dpos]=strigger[row][dpos];
				}
			}	
                        if (scount<ecount)
                        {
                                for(ll=0;ll<scount;ll++)
                                {
                                        dpos=e_pos[ll]-s_pos[ll];
                                        dpos=s_pos[ll]+((dpos*(page-1))/(pages-1));
                                        //atrigger[row][dpos]=1;
					atrigger[row][dpos]=(((pages-page)*strigger[row][s_pos[ll]])+(page-1)*etrigger[row][e_pos[ll]])/(pages-1);
                                }
                                diff=ecount-scount;
                                diff=((diff*(page-1))/(pages-1));
                                for(ll=0;ll<diff;ll++)
                                {
                                        dpos=e_pos[scount+ll];
                                        //atrigger[row][dpos]=1;
					atrigger[row][dpos]=etrigger[row][dpos];
                                }
                        }

		}
	}
}

void layout()
{
	char c[10];
        //wav page 
        button(1,10,10);
        button(2,10,30);
        button(3,10,50);
	XDrawString(display,cwin,cgc,40,60,"Visual",6);
        button(4,10,70);
	XDrawString(display,cwin,cgc,40,80,"Record",6);
        button(5,10,90);
	XDrawString(display,cwin,cgc,40,100,"Along off",9);
        slider(1,"Dur");
        slider(2,"Phase");
        knob(1,0);
        knob(2,0);
	box(90,BWHY,0);

	// Seq page
        draw_seq();
	sprintf(c,"BPM %03d",bpm);
        sslider(250,c);
        sknob(250,(bpm-30)*2);

	// Vis page
	XSetForeground(display,vc,white);
	XFillRectangle(display, vwin, vc, 0, 0, VX_SIZE, VY_SIZE);
	XFlush(display);
}

void draw_env(int row)
{
	int a,first,last,want;
	first=0;last=0;
	for (a=0;a<X_ENV;a++)
	{
		want=envelope[ROWS][a];
		if ( want != -1 )
		{
			if ( first==0){ first=a;}
			last=a;
		}
	}
	for (a=0;a<X_ENV;a++)
	{
		if (a>first && a<last)
		{
			want=envelope[ROWS][a];
			if (want != -1 ){ envelope[row][a]=envelope[ROWS][a];}
			else
			{ envelope[row][a]=envelope[row][a-1]; }
		}else{
		envelope[row][a]=0;}
		//printf ("a %d env %d\n",a,envelope[row][a]);
		envelope[ROWS][a]=-1;
	}
	box(90,BWHY,row);
}	

void draw_seq()
{
	int col,row;
	for (col=0;col<COLS;col++)
	{
		for (row=0;row<ROWS;row++)
		{
			seq_button(row,col,0,strigger[row][col]);
			seq_button(row,col,1,etrigger[row][col]);
			seq_button(row,col,2,atrigger[row][col]);
		}
	}
	do_seq_buttons();
}

void do_seq_buttons()
{
	char pp[30];
	XClearArea(display, win, 0,0,100,220,0 );
	XSetForeground(display,gc,white);
	sprintf(pp,"Page %02d of %02d",page,pages); XDrawString(display,win,gc,5,20,pp,13);
	sprintf(pp,"Chapter %02d of %02d",chapter,4); XDrawString(display,win,gc,5,35,pp,16);
	s_button (99999, 10, 40); XDrawString(display,win,gc,30,55,"Save Chapter",12);
	s_button (99998, 10, 60); XDrawString(display,win,gc,30,75,"Add Page",8);
	s_button (99997, 10, 80); XDrawString(display,win,gc,30,95,"Remove Page",11);
	s_button (99996, 10, 100); if (lfo_run==0){XDrawString(display,win,gc,30,115,"Lfo HALTED",10);}else{XDrawString(display,win,gc,30,115,"Lfo RUN",7);}
	s_button (99995, 10, 120); if (chap_run==0){XDrawString(display,win,gc,30,135,"Chap HALTED",11);}else{XDrawString(display,win,gc,30,135,"Chap RUN",8);}
	s_button (99994, 10, 140); XDrawString(display,win,gc,30,155,"Move Chapter",12);
	s_button (99993, 10, 160); if (run_on==1){ XDrawString(display,win,gc,30,175,"Run on",6);}else{ XDrawString(display,win,gc,30,175,"Restart",7); }
}


void seq_button (int row, int col, int type, int val)
{
	if (vl>0){ return;}

	//XLockDisplay(display);
        int xx,yy,big,small,s,xs,ys,yd,xe,ye,x,y,b_num;
	char pp[3];
        xs=110;xe=X_SIZE-10;
        ys=10;ye=Y_SIZE-5;
	yd=((ye-ys)/3)-10;
	ys=ys+((yd+10)*type);
	ye=ys+yd;

	big=16; small=14;

	b_num=col+(row*COLS)+(CORS*type);
	
	if (val>0){
	XSetForeground(display,gc,color[type+1].pixel);}
	else{
	XSetForeground(display,gc,white);}

	if (col%4==0){ 
		s=big;
		x=xs+(((xe-xs)/COLS)*(col));
		y=ys+(((ye-ys)/ROWS)*(row));
	}else{
		s=small;
		x=xs+(((xe-xs)/COLS)*(col));
		y=2+big+ys+(((ye-ys)/ROWS)*(row));
	}
        for (yy=y;yy<y+s;yy++){ for (xx=x;xx<x+s;xx++){
		//printf ("xx %d yy %d bn %d\n",xx,yy,b_num);
                sasta[xx+(yy*X_SIZE)]=b_num;
                XDrawPoint(display, win, gc, xx, yy);
        }}
        XSetForeground(display,gc,black);
	sprintf(pp,"%02d",val);
        XDrawString(display,win,gc,x,y+s,pp,2);
	//XUnlockDisplay(display);
}

void tutton (int b, int trig, int la, int ss)
{
        int xx,yy,big,small,s,xs,ys,xe,ye,xr,yr,x,y;
        xs=100;xe=X_SIZE-20;
        ys=10;ye=Y_SIZE-10;
        big=16; small=8;

        //xr=(xe-xs)/ROWS;yr=b*(ye-ys)/(4*COLS);
        if (b%4==0){
                s=big;
                x=xs+(((xe-xs)/COLS)*(b%COLS));
                y=ys+(((ye-ys)/ROWS)*(b/(COLS)));
        }else{
                s=small;
                x=xs+(((xe-xs)/COLS)*(b%COLS));
                y=2+big+ys+(((ye-ys)/ROWS)*(b/(COLS)));
        }
        for (yy=y;yy<y+s;yy++){ for (xx=x;xx<x+s;xx++){
                XSetForeground(display,gc,color[trig].pixel);
                XDrawPoint(display, win, gc, xx, yy);
        }}
}

void button (int b, int x, int y)
{
        XSetForeground(display,cgc,white);
	int s,xx,yy;
	s=15;
	for (yy=y;yy<y+s;yy++){ for (xx=x;xx<x+s;xx++){
		rasta[xx+(yy*XP_SIZE)]=b;	
                XDrawPoint(display, cwin, cgc, xx, yy);
	}}
	//XFlush(display);
}

void box(int xs,int ys, int r)
{
	int x,y;
	XClearArea(display, cwin, xs,ys,X_ENV,Y_ENV,0 );
	for (x=xs;x<X_ENV+xs;x++){
		XSetForeground(display,cgc,white);
		for (y=ys;y<Y_ENV+ys;y++){
			if (x==xs || y==ys ){ XDrawPoint(display, cwin, cgc, x, y);}
			if (x==X_ENV+xs-1 || y==Y_ENV+ys-1 ){ XDrawPoint(display, cwin, cgc, x, y);}
			rasta[x+(y*XP_SIZE)]=20;	
		}
		XSetForeground(display,cgc,color[3].pixel);
		XDrawPoint(display, cwin, cgc, x, Y_ENV+ys-(envelope[r][x-xs]));
	}
}

void s_button (int b, int x, int y)
{
        int s,xx,yy;
        s=15;
        XSetForeground(display,gc,color[0].pixel);
        for (yy=y;yy<y+s;yy++){ for (xx=x;xx<x+s;xx++){
                sasta[xx+(yy*X_SIZE)]=b;
                XDrawPoint(display, win, gc, xx, yy);
        }}
}


void slider(int num, char *text)
{
        XSetForeground(display,cgc,white);
        int loop,xp,len;
	len=strlen(text);
	xp=150+(num*80);
	XClearArea(display, cwin,xp-(len/2),SWHY-20,30,10,0 );
        XDrawString(display,cwin,cgc,xp-(len/2),SWHY-10,text,len);
        for (loop=SWHY;loop<SWHY+SLEN;loop++)
        {
                XDrawPoint(display, cwin, cgc, xp, loop);
        }
}

void sslider(int yp,char *text)
{
        XSetForeground(display,gc,white);
        int loop,xp,len;
        len=strlen(text);
	xp=30;
        XClearArea(display, win,xp-(len/2),yp-20,8*len,10,0 );
        XDrawString(display,win,gc,xp-(len/2),yp-10,text,len);
        for (loop=yp;loop<yp+SLEN;loop++)
        {
                XDrawPoint(display, win, gc, xp, loop);
        }
}



void knob(int num,int pos)
{
        int xp,x,y,h,w,yval,v;
	yval=SWHY+((pos*SLEN)/300);
        h=4;w=14;
	xp=150+(num*80);
	if (pos>=0 && pos<=SLEN)
	{
        for (y=SWHY-h;y<=SWHY+SLEN+h;y++)
        {
                int mp; mp=XP_SIZE*y;
                for (x=3;x<=w;x++)
                {
                        int x1,x2; x1=xp-x;x2=xp+x;
        		XSetForeground(display,cgc,black);
                        XDrawPoint(display, cwin, cgc, x1, y);
                        XDrawPoint(display, cwin, cgc, x2, y);
                        rasta[mp+x1]=0; rasta[mp+x2]=0;
                }
        }
        //XSetForeground(display,cgc,black);
	v=num+30;
        for (y=yval-h;y<=yval+h;y++)
        {
                int mp; mp=XP_SIZE*y;
                //3 is the 3 pixels either side of the two slider knob parts
                for (x=3;x<=w;x++)
                {
                        int x1,x2; x1=xp-x;x2=xp+x;
        		XSetForeground(display,cgc,white);
                        XDrawPoint(display, cwin, cgc, x1, y);
                        XDrawPoint(display, cwin, cgc, x2, y);
                        rasta[mp+x1]=v; rasta[mp+x2]=v;
                }
        }
	}
}


void sknob(int yp,int pos)
{
        int xp,x,y,h,w,yval;
        yval=yp+pos;
        h=4;w=14;
        xp=30;
        if (pos>=0 && pos<=SLEN)
        {
        for (y=yp-h;y<=yp+SLEN+h;y++)
        {
                int mp; mp=X_SIZE*y;
                for (x=3;x<=w;x++)
                {
                        int x1,x2; x1=xp-x;x2=xp+x;
                        XSetForeground(display,gc,black);
                        XDrawPoint(display, win, gc, x1, y);
                        XDrawPoint(display, win, gc, x2, y);
                        sasta[mp+x1]=0; sasta[mp+x2]=0;
                }
        }
        //XSetForeground(display,cgc,black);
        for (y=yval-h;y<=yval+h;y++)
        {
                int mp; mp=X_SIZE*y;
                //3 is the 3 pixels either side of the two slider knob parts
                for (x=3;x<=w;x++)
                {
                        int x1,x2; x1=xp-x;x2=xp+x;
                        XSetForeground(display,gc,white);
                        XDrawPoint(display, win, gc, x1, y);
                        XDrawPoint(display, win, gc, x2, y);
                        sasta[mp+x1]=99992; sasta[mp+x2]=99992;
                }
        }
        }
}

void balance(int kp, int *adj, int *seta, int *setb)
{
	int remain,forbase;
	*adj=kp;
	remain=300-kp;
	forbase=remain-*seta;
	if (forbase<0){ *seta=*seta+forbase;forbase=0;}
	*setb=forbase;
	printf ("KP %d adj %d seta %d setb %d \n",kp,*adj,*seta,*setb);
}	


void *explorer()
{
	XEvent event;           /* the XEvent declaration !!! */
    struct dirent *dir;
    char *ptr,c;
    char wname[100];
    char *wnames[100];
    int wcount, loop, slen, into,dats,my_row,als;
    wcount=0; into=0;my_row=0;als=0;
    // we have filenames
	int x_point,y_point,file_point,click,do_wav;
	char lot[3];
	file_point=0;
	click=0;
    for (loop=0;loop<(XP_SIZE* YP_SIZE);loop++){ rasta[loop]=0;}
    for (loop=0;loop<(X_SIZE* Y_SIZE);loop++){ sasta[loop]=100001;}
	XSetForeground(display,cgc,white);
	sprintf(lot,"%02d",my_row); XDrawString(display,cwin,cgc,50,20,lot,2);
    layout();
    layout();
    layout();
    layout();
	int kp,rec;
    while (1>0)
	{
	XNextEvent(display, &event);
	if (event.xany.window==cwin){ 
		if (event.type==ButtonPress ) {
			x_point=event.xbutton.x; y_point=event.xbutton.y;
			click=rasta[x_point+(XP_SIZE*y_point)];
			if (click==1) {
				my_row++;if (my_row>5){my_row=0;}
				XClearArea(display, cwin, 50,0,50,20,0 );
				XSetForeground(display,cgc,white);
				sprintf(lot,"%02d",my_row); XDrawString(display,cwin,cgc,50,20,lot,2);
	                        if (into==0) { knob(1,ssize[my_row]/4410); knob(2,phase[my_row]);}
                                if (into==1){ knob(1,vamp[my_row]);  knob(2,vdur[my_row]/2205);}
                                if (into==2){ knob(1,vvoc[my_row]);  knob(2,vvow[my_row]);}
                                if (into==3){ knob(1,wob[my_row]);   knob(2,fwob[my_row]);}
                                if (into==4){ knob(1,goodh[my_row]); knob(2,badh[my_row]);}
                                if (into==5){ knob(1,lfilt[my_row]); knob(2,hfilt[my_row]);}
                                if (into==6){ knob(1,pan[my_row]);   knob(2,vpan[my_row]);}

				box(90,BWHY,my_row);
			}
			if (click==2) {
				into++; if (into>6){into=0;}
				XClearArea(display, cwin, 50,20,150,20,0 );
				XSetForeground(display,cgc,white);
				if (into==0) {slider(1,"Dur"); slider(2,"Phase");}
				if (into==1) {slider(1,"Vll"); slider(2,"Vtt");}
				if (into==2) {slider(1,"Voc"); slider(2,"Vow");}
				if (into==3) {slider(1,"Wod"); slider(2,"Wof");}
				if (into==4) {slider(1,"Gud"); slider(2,"Bad");}
				if (into==5) {slider(1,"LFl"); slider(2,"HFl");}
				if (into==6) {slider(1,"Pan"); slider(2,"VPan");}
				if (into==0) { knob(1,ssize[my_row]/4410); knob(2,phase[my_row]);}
				if (into==1){ knob(1,vamp[my_row]);  knob(2,vdur[my_row]/2205);}
				if (into==2){ knob(1,vvoc[my_row]);  knob(2,vvow[my_row]);}
				if (into==3){ knob(1,wob[my_row]);   knob(2,fwob[my_row]);}
				if (into==4){ knob(1,goodh[my_row]); knob(2,badh[my_row]);}
				if (into==5){ knob(1,lfilt[my_row]); knob(2,hfilt[my_row]);}
				if (into==6){ knob(1,pan[my_row]);   knob(2,vpan[my_row]);}
			}
			if (click==3) {
				XClearArea(display, cwin, 40,40,200,30,0 );
				XSetForeground(display,cgc,white);
				if (vl==0) {vl=1; XDrawString(display,cwin,cgc,40,60,"Visualing",9);}
				else if (vl==1) {vl=2; XDrawString(display,cwin,cgc,40,60,"Visl+JPEG",9);}
				else { vl=0; XDrawString(display,cwin,cgc,40,60,"Visual",6); }
			}
			if (click==4) { 
				XClearArea(display, cwin, 40,65,200,30,0 );
				XSetForeground(display,cgc,white);
				if (rec==0){
					XDrawString(display,cwin,cgc,40,80,"Recording",9);
					// rewind visualisation.
					rec=1; sp=0;lp=0;
				} else {
					rec=0;
					save_wav(sp);
					XDrawString(display,cwin,cgc,40,80,"Record",6);
				}
			}
			if (click==5) { 
				XSetForeground(display,cgc,white);
				XClearArea(display, cwin, 40,85,100,30,0 );
				als++;if(als>3){als=0;}
				if (als==0){
					XDrawString(display,cwin,cgc,40,100,"Along off",9);
				}
				if (als==1){
					XDrawString(display,cwin,cgc,40,100,"Along up",8);
				}
				if (als==2){
					XDrawString(display,cwin,cgc,40,100,"Along down",10);
				}
				if (als==3){
					XDrawString(display,cwin,cgc,40,100,"Wibble Wob",10);
				}
				vocin[my_row]=als;

			}
		}
		if (event.type==ButtonRelease ) { 
				if (do_wav==1){draw_env(my_row);do_wav=0;}
       		}
		if (event.type==MotionNotify){
			x_point=event.xbutton.x; y_point=event.xbutton.y;
			click=rasta[x_point+(XP_SIZE*y_point)];
			if(click==31)   {
				kp=(y_point-SWHY);
				if (kp<1){kp=1;}
				if (kp>300){kp=300;}
				knob(1,kp);
				if (into==0){ ssize[my_row]=4410*kp;}
				if (into==1){ vamp[my_row]=kp;}
				if (into==2){ vvoc[my_row]=kp;}
				if (into==3){ wob[my_row]=kp;}
				if (into==4){ balance(kp,goodh+my_row,badh+my_row,baseh+my_row);printf("good %d bad %d base %d\n",goodh[my_row],badh[my_row],baseh[my_row]);}
				if (into==5){ lfilt[my_row]=kp;}
				if (into==6){ pan[my_row]=kp;lpan[my_row]=kp;rpan[my_row]=300-kp;}
			}
			if(click==32)   {
				kp=(y_point-SWHY);
				if (kp<1){kp=1;}
				if (kp>300){kp=300;}
				knob(2,kp);
				if (into==0){ phase[my_row]=kp;}
				if (into==1){ vdur[my_row]=kp*2205;}
				if (into==2){ vvow[my_row]=kp;}
				if (into==3){ fwob[my_row]=kp;}
				if (into==4){ balance(kp,badh+my_row,goodh+my_row,baseh+my_row);}
				if (into==5){ hfilt[my_row]=kp;}
				if (into==6){ vpan[my_row]=kp;vlpan[my_row]=kp;vrpan[my_row]=300-kp;}
			}
			if (click==20){
				XSetForeground(display,cgc,white);
                		XDrawPoint(display, cwin, cgc, x_point, y_point);
				envelope[ROWS][x_point-90]=(Y_ENV-y_point+BWHY);
				do_wav=1;
			}
		}
	} else {
		if (event.type==ButtonPress ) {
			x_point=event.xbutton.x; y_point=event.xbutton.y;
			click=sasta[x_point+(X_SIZE*y_point)];
			if (click<90000){
				int bp,col,row,type,pea;
                		bp=sasta[x_point+(y_point*X_SIZE)];
				type=bp/(CORS);
				bp=bp-(type*CORS);
				row=bp/(COLS);
				col=bp-(row*COLS);	
				if (type<2)
				{
					if (type==0){pea=strigger[row][col];}
					if (type==1){pea=etrigger[row][col];}
        				switch (event.xbutton.button) {
          				case 1:
            					//printf("Left Click\n");
						if (pea==24){ pea=0; }else{pea++;}	
            					break;
          				case 2:
            					//printf("Middle Click\n");
						pea=0; 	
            					break;
          				case 3:
            					//printf("Right Click\n");
						if (pea==0){ pea=24; }else{pea--;}	
            					break;
          				case 4:
            					//printf("Scroll UP\n");
						if (pea==24){ pea=0; }else{pea++;}	
            					break;
          				case 5:
            					//printf("Scroll Down\n");
						if (pea==0){ pea=24; }else{pea--;}	
            					break;
        				}	
					seq_button(row,col,type,pea);
					if (type==0){strigger[row][col]=pea;}
					if (type==1){etrigger[row][col]=pea;}
				}
			}
			if (click==99999) {
				// save the sequences.
				save_chapter(chapter);
			}
			if (click==99998) {
				pages++;
				do_seq_buttons();
			}
			if (click==99997) {
				if (pages>2){pages--;
					if (page>pages){page=pages;}
					do_seq_buttons();
				}
			}
			if (click==99996) {
				if (lfo_run==0){lfo_run=1;}else{lfo_run=0;}
				do_seq_buttons();
			}
			if (click==99995){
				if (chap_run==0){chap_run=1;}else{chap_run=0;}
				do_seq_buttons();
			}
			if (click==99994){
				if (chapter<4){chapter++;}else{chapter=1;}
				load_chapter(chapter); page=1;
				setoa();
				draw_seq();
			}
			if (click==99993){
				if (run_on==0){ run_on=1;}else{run_on=0;}
				do_seq_buttons();
			}
			if (click==99992){ 
				kp=(y_point-250);
				sknob(250,kp);
			}
		}
		if (event.type==MotionNotify){
			if (click==99992){ 
				char c[10];
				y_point=event.xbutton.y;
				kp=(y_point-250);
				if (kp>0 && kp<=300)
				{
				sknob(250,kp);
				bpm=30+(kp/2);
				sprintf(c,"BPM %03d",bpm);
				sslider(250,c);
				}
			}
		}
	} 
	}
}

unsigned long _RGB(int r,int g, int b)
{
    return b + (g<<8) + (r<<16);
}

void p_point (int x , int y, int r, int g, int b, char *jfile)
{
	XSetForeground(display, vc, _RGB(r,g,b));
       	XDrawPoint(display, vwin, vc, x, y);
	jfile[(x*3)+(3*y*VX_SIZE)]=r;
	jfile[(x*3)+(3*y*VX_SIZE)+1]=g;
	jfile[(x*3)+(3*y*VX_SIZE)+2]=b;
}

void *visualizer()
{
	int np,dp,x,lv,count;
	float theta,dtheta,lasx,lasy,y,dy,lasxx,lasyy;
       struct timespec tim, tim2;
       char fname[20];
       char *jfile;

       jfile=(unsigned char *)malloc(sizeof(unsigned char)*3*VX_SIZE*VY_SIZE);
       tim.tv_sec = 0;
       tim.tv_nsec = 1000000L;
       dp=8810;
       dtheta=0.1;
       lp=0;y=200;dy=1.8;theta=1;lasx=2;lasy=1.5;count=0;lasxx=3.5;lasyy=2.5;

	while (1>0)
	{
		while (sp<lp+dp)
		{
               		nanosleep(&tim , &tim2);
		}
		np=sp;
		if (vl>0)
		{
		for (x=lp;x<np;x+=2)
		{
			int rpoint,gpoint,bpoint,xp,yp,ys,r,g,b;
			rpoint=save[x];
			bpoint=save[x+1];
			if (rpoint <0){ rpoint=-rpoint;}
			if (bpoint <0){ bpoint=-bpoint;}
			gpoint=bpoint-rpoint;
			if (gpoint <0){ gpoint=-gpoint;}
  			r=rpoint;g=gpoint;b=bpoint;
			ys=y+(rpoint/160);
			// the child */
			xp=HX+(2*ys*(cos(lasx*theta)*sin(lasxx*theta)));
			yp=HY+(ys*(sin(lasy*theta)*cos(lasyy*theta)));
			p_point(xp,yp,r,g,b,jfile);
			//vu 
		/*	p_point(0,(rpoint*VY_SIZE)/37268,r,128,128,jfile);
			p_point(1,(rpoint*VY_SIZE)/37268,r,128,128,jfile);
			p_point(3,(bpoint*VY_SIZE)/37268,128,128,b,jfile);
			p_point(4,(bpoint*VY_SIZE)/37268,128,128,b,jfile); */

			theta=theta+dtheta;
			if (theta>=2*M_PI){ 
				theta=0;y+=dy;
				if(y>=(VY_SIZE/3) || y<150){dy=-dy;
  				XSetForeground(display, vc, _RGB(r,g,b));
				XFillRectangle(display, vwin, vc, 0, 0, VX_SIZE, VY_SIZE);
				int loop;
				for (loop=0;loop<VY_SIZE*VX_SIZE*3;loop+=3)
				{
					jfile[loop]=r;
					jfile[loop+1]=g;
					jfile[loop+2]=b;
				}
				lasx=(float)rpoint/3276;
				lasy+=2.3;
				lasyy=(float)bpoint/3276;
				lasxx+=1.5;
				} 
				dtheta=1/((float)(y));
			}
		}
		lp=np;
		printf ("checking %d \n",vl);
		if (vl==2){
			sprintf(fname,"./jpegs/vis%04d.jpg",count);
			jayit(jfile,VX_SIZE,VY_SIZE,fname);
			count++;
			printf ("jaying %d \n",count);
		}else{ count=0;}
		}
	}
}


void *lfo ()
{
	FILE *trigf;
	int sleeps,dats,row,col,pd,vwdir,vdir,lfdir,hfdir;
	long tc;
	struct timespec ts;
       struct timespec tim, tim2;
       tim.tv_sec = 0;
       tim.tv_nsec = 1000000L;
       lfo_run=1;
       row=0;col=0;
       tc=0;vdir=4;lfdir=5;hfdir=6;vwdir=7;
	uint64_t prev_time_value, time_value, time_diff, tick;

	for(row=0;row<ROWS;row++){ balance(goodh[row],goodh+row,badh+row,baseh+row);}
	for (row=0;row<ROWS;row++){ for (col=0;col<COLS;col++){ atrigger[row][col]=strigger[row][col];}}	


	//tenths of ms per tick;
	pd=1;

       	prev_time_value=(uint64_t) (ts.tv_sec * 1000000 + ts.tv_nsec / 1000000);
	while (1>0)
	{
		XKeyEvent event;
		sleeps=0;
		time_diff = 0;
		tick=(uint64_t)(150000/bpm);
		while (time_diff<tick)
		{
        	//time_value=(uint64_t) (ts.tv_sec * 1000000 + ts.tv_nsec / 100000);
        	time_value=(uint64_t) (ts.tv_sec * 1 + ts.tv_nsec / 100000);
		clock_gettime (CLOCK_MONOTONIC, &ts) ;
		if ( time_value < prev_time_value) { prev_time_value=prev_time_value-10000; }
		time_diff = time_value - prev_time_value;
               	nanosleep(&tim , &tim2);
		sleeps ++;
		}
       		prev_time_value=(uint64_t) (ts.tv_sec * 1 + ts.tv_nsec / 100000);
		tc++;
		// this is a hack to keep the x server alive. I dont like it but it works
		event.type=KeyPress; event.keycode=XK_A; event.display=display; event.root=win;
        	XSendEvent(display,win,True,KeyPressMask,(XEvent *)&event);
        	XSendEvent(display,win,True,KeyPressMask,(XEvent *)&event);
		if (lfo_run==1)
		{
		if (tc%COLS==0){ 
			page+=pd;
			if (page>pages) { 
				// running chapters dont go back.
				if (chap_run==1){ 
					chapter++; if (chapter>4){chapter=1;}
					page=1;
					load_chapter(chapter);
					draw_seq();
				} else {
					page-=2; pd=-pd; }
			}
			if (page<1) { page=2; pd=-pd; }
			do_seq_buttons();
			setoa();
		}
		col=tc%COLS;
		vvow[3]+=vwdir;if (vvow[3]>288){vwdir=-vwdir;}if (vvow[3]<20){vwdir=-vwdir;}
		vvoc[3]+=vdir;if (vvoc[3]>298){vdir=-vdir;}if (vvoc[3]<2){vdir=-vdir;}
		hfilt[3]+=hfdir;if (hfilt[3]>260){hfdir=-hfdir;}if (hfilt[3]<50){hfdir=-hfdir;}
		for (row=0;row<ROWS;row++)
		{
			int bb,bc;
			if (col==0)
			{
				seq_button(row,COLS-1,2,atrigger[row][COLS-1]);
			}else{
				seq_button(row,col-1,2,atrigger[row][col-1]);
			}
			seq_button(row,col,2,1);
			//if (atrigger[row][col]>0 ){point[row]=1;
			if (atrigger[row][col]>0 ){
				if (run_on==0)
				{
					//point[row]=1+((rand()%ssize[row])/3);
					point[row]=1;
				}else{
					if (point[row]==0){
						point[row]=1;
					}
				}

				//printf ("firing row %d col %d len %d tnote[row] %d freq %f\n",row,col,ssize[row],tnote[row],notes[tnote[row]]);
				if (row==0) {tnote[row]=23+atrigger[row][col];}
				if (row==1)  {tnote[row]=35+atrigger[row][col];}
				if (row==2)  {tnote[row]=47+atrigger[row][col];}
				if (row==3)  {tnote[row]=47+atrigger[row][col];}
				if (row==4)  {tnote[row]=59+atrigger[row][col];}
				if (row==5) {tnote[row]=71+atrigger[row][col];}
				}
		}
		}
	}	
}


void save_chapter (int chapter)
{
	FILE *trigf;
	char fname[20];
	int dats,a,b;
	sprintf(fname,"chapter%02d.dat",chapter);

        if ((trigf = fopen(fname, "wb")) == NULL) { exit (1);}
        dats=fwrite(strigger,sizeof(int),ROWS*COLS,trigf);
        dats=fwrite(etrigger,sizeof(int),ROWS*COLS,trigf);
        for (a=0;a<=ROWS;a++){ for (b=0;b<X_ENV;b++){ dats=fwrite(&envelope[a][b],sizeof(short),1,trigf); }}
        dats=fwrite(vamp,sizeof(int),ROWS,trigf);
        dats=fwrite(vdur,sizeof(int),ROWS,trigf);
        dats=fwrite(vvoc,sizeof(int),ROWS,trigf);
        dats=fwrite(wob,sizeof(int),ROWS,trigf);
        dats=fwrite(fwob,sizeof(int),ROWS,trigf);
        dats=fwrite(ssize,sizeof(int),ROWS,trigf);
        dats=fwrite(vvow,sizeof(int),ROWS,trigf);
        dats=fwrite(goodh,sizeof(int),ROWS,trigf);
        dats=fwrite(badh,sizeof(int),ROWS,trigf);
        dats=fwrite(hfilt,sizeof(int),ROWS,trigf);
        dats=fwrite(lfilt,sizeof(int),ROWS,trigf);
        dats=fwrite(&bpm,sizeof(int),1,trigf);
        fclose(trigf);
        printf ("saved Chapter %d\n",chapter);
}

void load_chapter (int chapter)
{
	FILE *trigf;
	char fname[20];
	int dats;
	sprintf(fname,"chapter%02d.dat",chapter);
        if ((trigf = fopen(fname, "rb")) == NULL) {
                fprintf(stderr, "can't open file making it\n");
                if ((trigf = fopen(fname, "wb")) == NULL) {
                        exit (1);
                }
                int loopa,a,b;
                dats=fwrite(strigger,sizeof(int),ROWS*COLS,trigf);
                dats=fwrite(etrigger,sizeof(int),ROWS*COLS,trigf);
                for (a=0;a<=ROWS;a++){ for (b=0;b<X_ENV;b++){ dats=fwrite(&envelope[a][b],sizeof(short),1,trigf); }}
                dats=fwrite(vamp,sizeof(int),ROWS,trigf);
                dats=fwrite(vdur,sizeof(int),ROWS,trigf);
                dats=fwrite(vvoc,sizeof(int),ROWS,trigf);
                dats=fwrite(wob,sizeof(int),ROWS,trigf);
                dats=fwrite(fwob,sizeof(int),ROWS,trigf);
                dats=fwrite(ssize,sizeof(int),ROWS,trigf);
                dats=fwrite(vvow,sizeof(int),ROWS,trigf);
                dats=fwrite(goodh,sizeof(int),ROWS,trigf);
                dats=fwrite(badh,sizeof(int),ROWS,trigf);
                dats=fwrite(hfilt,sizeof(int),ROWS,trigf);
                dats=fwrite(lfilt,sizeof(int),ROWS,trigf);
                dats=fwrite(&bpm,sizeof(int),1,trigf);
        }else{
                dats=fread(strigger,sizeof(int),ROWS*COLS,trigf);
                dats=fread(etrigger,sizeof(int),ROWS*COLS,trigf);
                int a,b;
                for (a=0;a<=ROWS;a++){ for (b=0;b<X_ENV;b++){ dats=fread(&envelope[a][b],sizeof(short),1,trigf); }}
                dats=fread(vamp,sizeof(int),ROWS,trigf);
                dats=fread(vdur,sizeof(int),ROWS,trigf);
                dats=fread(vvoc,sizeof(int),ROWS,trigf);
                dats=fread(wob,sizeof(int),ROWS,trigf);
                dats=fread(fwob,sizeof(int),ROWS,trigf);
                dats=fread(ssize,sizeof(int),ROWS,trigf);
                dats=fread(vvow,sizeof(int),ROWS,trigf);
                dats=fread(goodh,sizeof(int),ROWS,trigf);
                dats=fread(badh,sizeof(int),ROWS,trigf);
                dats=fread(hfilt,sizeof(int),ROWS,trigf);
                dats=fread(lfilt,sizeof(int),ROWS,trigf);
                dats=fread(&bpm,sizeof(int),1,trigf);
        }
        fclose(trigf);
	setoa();
}	

void pushwav (int loc,int start, int end, int speed)
{
	int along,max,points;
	points=ssize[20];
	max=(speed*points)/150; if (max>SMAX){max=SMAX;}
	for (along=0;along<max;along+=2)
	{
		int sp;
		sp=(((along*150)/speed)/2)*2;
		sample[loc][along]=sample[20][sp];
		sample[loc][along+1]=sample[20][sp+1];
	}
	ssize[loc]=max;
}	

void drawwav (int sz)
{
	//printf("Size %d\n",ssize[20]);
	int along,xs,xe,len,points;
	xs=300;xe=X_SIZE-10;
	len=xe-xs;
	points=ssize[20];
	XClearArea(display, cwin, xs,0,len,YP_SIZE,0 );
	for (along=0;along<points;along++)
	{
		int l,r,p;
		p=300*(along/(2*sz));
		if (p<points)
		{
		l=(128+(sample[20][p])/256);
		r=300+(128+(sample[20][p+1])/256);
                XDrawPoint(display, cwin, cgc, ((len*along)/points)+xs, l);
                XDrawPoint(display, cwin, cgc, ((len*along)/points)+xs, r);
		}
	}
}


int readwav( char *fname, int loc, int start, int end,int rate)
{
	FILE *infile;
	int len,chan,sample_rate,bits_pers,byte_rate,ba,size,*fhead,max,alloc,begin,finish,ramp,points;
	ramp=300;
	len=20;
        chan=2;
        sample_rate=44100;
        bits_pers=16;
        byte_rate=(sample_rate*chan*bits_pers)/8;
        ba=((chan*bits_pers)/8)+bits_pers*65536;
	start=start*2;
	end=end*2;

	short *gotwav;
        gotwav=(short *)malloc(sizeof(short)*SMAX);

        size=chan*len*sample_rate;
        //wave=(short *)malloc(sizeof(short)*size);
        fhead=(int *)malloc(sizeof(int)*11);

        if ((infile = fopen(fname, "rb")) == NULL) { fprintf(stderr, "can't open file\n"); exit(1);}
	/*
        fhead[0]=0x46464952;
        fhead[1]=36;
        fhead[2]=0x45564157;
        fhead[3]=0x20746d66;
        fhead[4]=16;
        fhead[5]=65536*chan+1;
        fhead[6]=sample_rate;
        fhead[7]=byte_rate;
        fhead[8]=ba;
        fhead[9]=0x61746164;
        fhead[10]=(size*chan*bits_pers)/8; */

        points=fread(fhead,sizeof(int),11,infile);
        points=fread(gotwav,sizeof(short),size,infile);
	if (end>=points){ end=points-1;}


	// normalize
	int spoints=((end-start)*rate)/100;
	max=0;
	for (alloc=start;alloc<end;alloc++)
	{
		// could be a big negative
		int pixel;
		pixel=gotwav[alloc];
		if (pixel<0){ pixel=-pixel;}
		if (pixel>max){ max=pixel;}
	}
	for (alloc=start;alloc<end;alloc++)
	{
		gotwav[alloc]=(32767*gotwav[alloc])/max;
	}

	// we need to get rid of the click.
	begin=gotwav[start];
	// ramp up.
	for (alloc=0;alloc<ramp;alloc++)
	{
		sample[loc][alloc]=(begin*alloc)/ramp;
	}
	for (alloc=0;alloc<spoints;alloc++)
	{
		sample[loc][ramp+alloc]=gotwav[start+((100*alloc)/rate)];
	}
	// ramp out
	finish=gotwav[end-1];
	for (alloc=0;alloc<ramp;alloc++)
	{
		sample[loc][ramp+spoints+alloc]=(finish*(ramp-alloc))/ramp;
	}

        fclose(infile);
	free(fhead);
	free(gotwav);
	ssize[loc]=spoints+(ramp*2);
	return spoints+(ramp*2);
}


void from_to (int from, int to ,int size)
{
	int a;
	to++;
	//loop round
	if (to<from)
	{
		main_osc(from*size,(10*size));
		main_osc(0,(to*size));
	}else {
		main_osc(from*size,(to*size));
	}
}

void main_osc (int from, int to)
{
	int a,vpr[ROWS];
	for (a=from;a<to;a+=2)
	{
		int s;
		long l,r;
		l=0;r=0;

		vp+=2;
                if (vp>=881000){vp=0;}



		for (s=0;s<ROWS;s++)
		{
			int p;
			float f,sir,sil,qu,qi,dl,dr,along,blong,gdl,gdr,bdl,bdr,bal,bar,valong;
			long amp;
			long lm,rm;
			lm=0;rm=0;
			p=point[s];
			if (p>0)
			{
				float fphase;
				along=(float)p/(float)ssize[s];
				blong=1-along;

				fphase=(along*(float)phase[s]);
				f=notes[tnote[s]];
				amp=envelope[s][(p*(X_ENV-1))/ssize[s]];

				if (vocin[s]==0){ valong=1;}
				if (vocin[s]==1){ valong=along;}
				if (vocin[s]==2){ valong=blong;}
				if (vocin[s]==3){ valong=(1+sin(fwob[s]*p*M_PI/441000))/2;}

				qu=((float)vvoc[s]/300);
				qi=((float)vvow[s])/300;
				//printf("along %f\n",along);
				//
				//
				//DONT add PHI on change this to a rolling number.
				//

				bal=sin(((M_PI*((f-((wob[s]*sin((fwob[s]*p*M_PI)/441000))/50))*p))/44100)+fphase);
				bar=sin(((M_PI*((f+((wob[s]*sin((fwob[s]*p*M_PI)/441000))/50))*p))/44100)-fphase);
				gdl=sin(((M_PI*(((3*f)-((wob[s]*sin((fwob[s]*p*M_PI)/441000))/50))*p))/44100)-(fphase/2));
				gdr=sin(((M_PI*(((3*f)+((wob[s]*sin((fwob[s]*p*M_PI)/441000))/50))*p))/44100)+(fphase/2));
				bdl=sin((M_PI*(((2.08*f)-((wob[s]*sin((fwob[s]*p*M_PI)/441000))/50))*p))/44100);
				bdr=sin((M_PI*(((2.09*f)+((wob[s]*sin((fwob[s]*p*M_PI)/441000))/50))*p))/44100);

				if (bal<qi && bal>0){bal=0;} if (-bal<qi && bal<0){bal=0;} if (bal>qu){bal=1;} if (-bal>qu){bal=-1;}
				if (bar<qi && bar>0){bar=0;} if (-bar<qi && bar<0){bal=0;} if (bar>qu){bar=1;} if (-bar>qu){bar=-1;}

				if (gdl<qi && gdl>0){gdl=0;} if (-gdl<qi && gdl<0){gdl=0;} if (gdl>qu){gdl=1;} if (-gdl>qu){gdl=-1;}
				if (gdr<qi && gdr>0){gdr=0;} if (-gdr<qi && gdr<0){bal=0;} if (gdr>qu){gdr=1;} if (-gdr>qu){gdr=-1;}

				if (bdl<qi && bdl>0){bdl=0;} if (-bdl<qi && bdl<0){bal=0;} if (bdl>qu){bdl=1;} if (-bdl>qu){bdl=-1;}
				if (bdr<qi && bdr>0){bdr=0;} if (-bdr<qi && bdr<0){bdr=0;} if (bdr>qu){bdr=1;} if (-bdr>qu){bdr=-1;}


				sil=(bal*(float)baseh[s])+(gdl*(float)goodh[s])+(bdl*(float)badh[s]);
				sir=(bar*(float)baseh[s])+(gdr*(float)goodh[s])+(bdr*(float)badh[s]);

				//filter low pass
				dl=sil-prevl[s];
				dr=sir-prevr[s];
				sil=sil-((valong*lfilt[s]*dl)/300); 
				sir=sir-((valong*lfilt[s]*dr)/300); 

				//filter high pass
				
				dl=sil-prevl[s];
				dr=sir-prevr[s];
				sil=prevl[s]+((valong*hfilt[s]*dl)/300); 
				sir=prevr[s]+((valong*hfilt[s]*dr)/300); 

				prevl[s]=sil;prevr[s]=sir; 
				//amp =512 pan=300 */
				lm=(amp*sil*lpan[s]);
				rm=(amp*sir*rpan[s]);

				point[s]+=2;
				if (point[s]>=ssize[s]){ point[s]=0; }
			}

                        vpr[s]=vp+vdur[s]; if (vpr[s]>=881000){vpr[s]=vpr[s]-881000;}

			l=l+lm+verb[s][vp];
			r=r+rm+verb[s][vp+1]; 
			//printf ("vprs %d\n",vpr[s]);
			long lv,rv;
			lv=(vamp[s]*(lm+verb[s][vp]))/300;
			rv=(vamp[s]*(rm+verb[s][vp+1]))/300;
			verb[s][vpr[s]]=((lv*vlpan[s])+(rv*vrpan[s]))/300; 
			verb[s][vpr[s]+1]=((rv*vlpan[s])+(lv*vrpan[s]))/300; 
		}
		// pan is 300 amplitude is 512 times too big for a short. plan also or 4 sounds at once. 512*4*300 
		ring[a]=l/6144;
		ring[a+1]=r/6144;
		// this is the recording
		if (sp>=SAVEM){ sp=0;lp=0;}
		save[sp]=ring[a];
		save[sp+1]=ring[a+1];
		sp+=2;
		// the verb pointer.
	}
}

void *spkr() {
  //lock him.
  long loops;
  int rc;
  int size;
  snd_pcm_t *handle;
  snd_pcm_hw_params_t *params;
  unsigned int val;
  int dir;
  snd_pcm_uframes_t frames;
  char *buffer;
  start=0;

  /* Open PCM device for playback. */
  rc = snd_pcm_open(&handle, "default",
                    SND_PCM_STREAM_PLAYBACK, 0);
  if (rc < 0) {
    fprintf(stderr,
            "unable to open pcm device: %s\n",
            snd_strerror(rc));
    exit(1);
  }
  /* Allocate a hardware parameters object. */
  snd_pcm_hw_params_alloca(&params);
  /* Fill it in with default values. */
  snd_pcm_hw_params_any(handle, params);
  /* Set the desired hardware parameters. */
  /* Interleaved mode */
  snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
  /* Signed 16-bit little-endian format */
  snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);
  /* Two channels (stereo) */
  snd_pcm_hw_params_set_channels(handle, params, 2);
  /* 44100 bits/second sampling rate (CD quality) */
  val = 44100;
  snd_pcm_hw_params_set_rate_near(handle, params, &val, &dir);
  /* Set period size to 32 frames. */
  frames = 32;
  snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);
  /* Write the parameters to the aux */
  rc = snd_pcm_hw_params(handle, params);
  if (rc < 0) {
    fprintf(stderr,
            "unable to set hw parameters: %s\n",
            snd_strerror(rc));
    exit(1);
  }

  /* Use a buffer large enough to hold one period */
  snd_pcm_hw_params_get_period_size(params, &frames, &dir);
  size = frames * 4; /* 2 bytes/sample, 2 channels */
  sbsz= size;
  buffer = (char *) malloc(size);

  /* We want to loop for 5 seconds */
  snd_pcm_hw_params_get_period_time(params, &val, &dir);
  /* 5 seconds in microseconds divided by
   * period time */

  int ring_point,a;
  ring_point=0;

  while (1 > 0) {
    rc = snd_pcm_writei(handle, ring+ring_point, frames);
    if (rc == -EPIPE) {
      /* EPIPE means underrun */
      fprintf(stderr, "underrun occurred\n");
      snd_pcm_prepare(handle);
    } else if (rc < 0) {
      fprintf(stderr,
              "error from writei: %s\n",
              snd_strerror(rc));
    }  else if (rc != (int)frames) {
      fprintf(stderr,
              "short write, write %d frames\n", rc);
    }
	a=start+1;
	if (a>=10){ start=0;}else{start=a;}
    ring_point=(start*(size/2));
  }

  snd_pcm_drain(handle);
  snd_pcm_close(handle);
  free(buffer);

  return 0;
}

void init_x()
{
/* get the colors black and white (see section for details) */
	XInitThreads();
        //x_buffer=(unsigned char *)malloc(sizeof(unsigned char)*4*VX_SIZE*VY_SIZE);
        display=XOpenDisplay((char *)0);
        screen=DefaultScreen(display);
        black=BlackPixel(display,screen),
        white=WhitePixel(display,screen);
        win=XCreateSimpleWindow(display,DefaultRootWindow(display),0,0, X_SIZE, Y_SIZE, 5, white,black);
        cwin=XCreateSimpleWindow(display,DefaultRootWindow(display),0,0, XP_SIZE, YP_SIZE, 5, white,black);
        vwin=XCreateSimpleWindow(display,DefaultRootWindow(display),0,0, VX_SIZE, VY_SIZE, 5, white,black);
        XSetStandardProperties(display,win,"seq","seq",None,NULL,0,NULL);
        XSetStandardProperties(display,cwin,"Xplore","Xplore",None,NULL,0,NULL);
        XSetStandardProperties(display,vwin,"Vis","Vis",None,NULL,0,NULL);
        XSelectInput(display, cwin, ExposureMask|ButtonPressMask|KeyPressMask|ButtonReleaseMask|ButtonMotionMask);
        XSelectInput(display, win, ExposureMask|ButtonPressMask|KeyPressMask|ButtonReleaseMask|ButtonMotionMask);
        //XSelectInput(display, vwin, ExposureMask|ButtonPressMask|KeyPressMask|ButtonReleaseMask|ButtonMotionMask);
        vc=XCreateGC(display, vwin, 0,0);
        gc=XCreateGC(display, win, 0,0);
        cgc=XCreateGC(display, cwin, 0,0);
        XSetBackground(display,gc,black); XSetForeground(display,gc,white);
        XSetBackground(display,cgc,black); XSetForeground(display,cgc,white);
        XSetBackground(display,vc,white); XSetForeground(display,cgc,white);
        XClearWindow(display, win); XClearWindow(display, cwin); XClearWindow(display, vwin); 
        XMapRaised(display, vwin); XMapRaised(display, cwin); XMapRaised(display, win); 
        XMoveWindow(display, win,0,0);
        XMoveWindow(display, cwin,100,100);
        XMoveWindow(display, vwin,300,300);
        Visual *visual=DefaultVisual(display, 0);
        //x_image=XCreateImage(display, visual, DefaultDepth(display,DefaultScreen(display)), ZPixmap, 0, x_buffer, VX_SIZE, VY_SIZE, 32, 0);
        //cx_image=XCreateImage(display, visual, DefaultDepth(display,DefaultScreen(display)), ZPixmap, 0, cx_buffer, CX_SIZE, CY_SIZE, 32, 0);
        Colormap cmap;
        cmap = DefaultColormap(display, screen);
        color[0].red = 65535; color[0].green = 65535; color[0].blue = 65535;
        color[1].red = 65535; color[1].green = 0; color[1].blue = 0;
        color[2].red = 0; color[2].green = 0; color[2].blue = 65535;
        color[3].red = 0; color[3].green = 65535; color[3].blue = 0;
        color[4].red = 0; color[4].green = 65535; color[4].blue = 65535;
        color[5].red = 65535; color[5].green = 65535; color[5].blue = 0;
        XAllocColor(display, cmap, &color[0]);
        XAllocColor(display, cmap, &color[1]);
        XAllocColor(display, cmap, &color[2]);
        XAllocColor(display, cmap, &color[3]);
        XAllocColor(display, cmap, &color[4]);
        XAllocColor(display, cmap, &color[5]);
}

void close_x() {
        XFreeGC(display, gc);
        XDestroyWindow(display,win);
        XCloseDisplay(display);
        exit(1);
}

void redraw() {
        XClearWindow(display, win);
}

int jayit(unsigned char *screen,int image_width, int image_height, char *name)
{

int row_stride,ex,why,cmp,div,set;
unsigned char *image,**row_pointer,*cr,*cg,*cb;
row_pointer=(unsigned char **)malloc(1);

struct jpeg_compress_struct cinfo;
struct jpeg_error_mgr jerr;
FILE * outfile;         /* target file */
cinfo.err = jpeg_std_error(&jerr);
jpeg_create_compress(&cinfo);
if ((outfile = fopen(name, "wb")) == NULL) {
        fprintf(stderr, "can't open file\n");
        exit(1);
}
jpeg_stdio_dest(&cinfo, outfile);
cinfo.image_width = image_width;        /* image width and height, in pixels */
cinfo.image_height = image_height;
cinfo.input_components = 3;             /* # of color components per pixel */
cinfo.in_color_space = JCS_RGB;         /* colorspace of input image */
jpeg_set_defaults(&cinfo);
jpeg_set_quality(&cinfo,100,TRUE); /* limit to baseline-JPEG values */
jpeg_start_compress(&cinfo, TRUE);

  row_stride = image_width * 3; /* JSAMPLEs per row in image_buffer */

  while (cinfo.next_scanline < cinfo.image_height) {
    /* jpeg_write_scanlines expects an array of pointers to scanlines.
     * Here the array is only one element long, but you could pass
     * more than one scanline at a time if that's more convenient.
     */
    row_pointer[0] = & screen[cinfo.next_scanline * row_stride];
    (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
  }
jpeg_finish_compress(&cinfo);
fclose(outfile);
jpeg_destroy_compress(&cinfo);
}

