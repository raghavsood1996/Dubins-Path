#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>

#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/keysymdef.h>
#include <X11/Xatom.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glx.h>


#include "fssimplewindow.h"

#define FS_NUM_XK 65536

extern void FsXCreateKeyMapping(void);
extern int FsXKeySymToFskey(int keysym);
extern char FsXKeySymToChar(int keysym);
extern int FsXFskeyToKeySym(int fskey);

class FsMouseEventLog
{
public:
	int eventType;
	int lb,mb,rb;
	int mx,my;
	unsigned int shift,ctrl;
};

#define NKEYBUF 256
static int nKeyBufUsed=0;
static int keyBuffer[NKEYBUF];
static int nCharBufUsed=0;
static int charBuffer[NKEYBUF];
static int nMosBufUsed=0;
static FsMouseEventLog mosBuffer[NKEYBUF];



static Display *ysXDsp;
static Window ysXWnd;
static Colormap ysXCMap;
static XVisualInfo *ysXVis;
static const int ysXEventMask=(KeyPressMask|KeyReleaseMask|ButtonPressMask|ButtonReleaseMask|PointerMotionMask|ExposureMask|StructureNotifyMask);

static GLXContext ysGlRC;
static int ysGlxCfgSingle[]={GLX_RGBA,GLX_DEPTH_SIZE,16,None};
static int ysGlxCfgDouble[]={GLX_DOUBLEBUFFER,GLX_RGBA,GLX_DEPTH_SIZE,16,None};

static int ysXWid,ysXHei;



static int fsKeyPress[FSKEY_NUM_KEYCODE];
static int exposure=0;
static int lastKnownLb=0,lastKnownMb=0,lastKnownRb=0;


void FsOpenWindow(int x0,int y0,int wid,int hei,int useDoubleBuffer)
{
	const char *title="MainWindow";

	int n;
	char **m,*def;
	XSetWindowAttributes swa;
	Font font;

	int lupX,lupY,sizX,sizY;
	lupX=x0;
	lupY=y0;
	sizX=wid;
	sizY=hei;

	FsXCreateKeyMapping();
	for(n=0; n<FSKEY_NUM_KEYCODE; n++)
	{
		fsKeyPress[n]=0;
	}

	ysXDsp=XOpenDisplay(NULL);

	if(ysXDsp!=NULL)
	{
		if(glXQueryExtension(ysXDsp,NULL,NULL)!=0)
		{
			int tryAlternativeSingleBuffer=0;
			if(useDoubleBuffer!=0)
			{
				ysXVis=glXChooseVisual(ysXDsp,DefaultScreen(ysXDsp),ysGlxCfgDouble);
			}
			else
			{
				ysXVis=glXChooseVisual(ysXDsp,DefaultScreen(ysXDsp),ysGlxCfgSingle);
				if(NULL==ysXVis)
				{
					ysXVis=glXChooseVisual(ysXDsp,DefaultScreen(ysXDsp),ysGlxCfgDouble);
					tryAlternativeSingleBuffer=1;
				}
			}
			if(ysXVis!=NULL)
			{
				ysXCMap=XCreateColormap(ysXDsp,RootWindow(ysXDsp,ysXVis->screen),ysXVis->visual,AllocNone);

				ysGlRC=glXCreateContext(ysXDsp,ysXVis,None,GL_TRUE);
				if(ysGlRC!=NULL)
				{
					swa.colormap=ysXCMap;
					swa.border_pixel=0;
					swa.event_mask=ysXEventMask;

					ysXWnd=XCreateWindow(ysXDsp,RootWindow(ysXDsp,ysXVis->screen),
							  lupX,lupY,sizX,sizY,
					                  1,
							  ysXVis->depth,
					                  InputOutput,
							  ysXVis->visual,
					                  CWEventMask|CWBorderPixel|CWColormap,&swa);


					ysXWid=sizX;
					ysXHei=sizY;

					XStoreName(ysXDsp,ysXWnd,title);


// Should I use XSetWMProperties? titlebar problem.
					XWMHints wmHints;
					wmHints.flags=0;
					wmHints.initial_state=NormalState;
					XSetWMHints(ysXDsp,ysXWnd,&wmHints);


					XSetIconName(ysXDsp,ysXWnd,title);
					XMapWindow(ysXDsp,ysXWnd);

	                                /* printf("Wait Expose Event\n");
					XEvent ev;
					while(XCheckTypedEvent(ysXDsp,Expose,&ev)!=True)
					  {
					    printf("Waiting for create notify\n");
					    sleep(1);
					  }
					printf("Window=%d\n",ev.xexpose.window);
					printf("Window Created\n"); */

					glXMakeCurrent(ysXDsp,ysXWnd,ysGlRC);

					// These lines are needed, or window will not appear >>
				    glClearColor(1.0F,1.0F,1.0F,0.0F);
					glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
					glFlush();
					// glXSwapBuffers(ysXDsp,ysXWnd);
					// These lines are needed, or window will not appear <<

					glEnable(GL_DEPTH_TEST);
					glDepthFunc(GL_LEQUAL);
					glShadeModel(GL_SMOOTH);

					GLfloat dif[]={0.8F,0.8F,0.8F,1.0F};
					GLfloat amb[]={0.4F,0.4F,0.4F,1.0F};
					GLfloat spc[]={0.9F,0.9F,0.9F,1.0F};
					GLfloat shininess[]={50.0,50.0,50.0,0.0};

					glEnable(GL_LIGHTING);
					glEnable(GL_LIGHT0);
					glLightfv(GL_LIGHT0,GL_DIFFUSE,dif);
					glLightfv(GL_LIGHT0,GL_SPECULAR,spc);
					glMaterialfv(GL_FRONT|GL_BACK,GL_SHININESS,shininess);

					glLightModelfv(GL_LIGHT_MODEL_AMBIENT,amb);
					glEnable(GL_COLOR_MATERIAL);
					glEnable(GL_NORMALIZE);

					if(0!=tryAlternativeSingleBuffer)
					{
						glDrawBuffer(GL_FRONT);
					}

				    glClearColor(1.0F,1.0F,1.0F,0.0F);
				    glClearDepth(1.0F);
					glDisable(GL_DEPTH_TEST);

					glViewport(0,0,sizX,sizY);

				    glMatrixMode(GL_PROJECTION);
					glLoadIdentity();
					glOrtho(0,(float)sizX-1,(float)sizY-1,0,-1,1);

					glMatrixMode(GL_MODELVIEW);
					glLoadIdentity();

					glShadeModel(GL_FLAT);
					glPointSize(1);
					glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
					glColor3ub(0,0,0);
				}
				else
				{
					fprintf(stderr,"Cannot create OpenGL context.\n");
					exit(1);
				}
			}
			else
			{
				fprintf(stderr,"Double buffer not supported?\n");
				exit(1);
			}
		}
		else
		{
			fprintf(stderr,"This system doesn't support OpenGL.\n");
			exit(1);
		}
	}
	else
	{
		fprintf(stderr,"Cannot Open Display.\n");
		exit(1);
	}

	return;
}

void FsGetWindowSize(int &wid,int &hei)
{
	wid=ysXWid;
	hei=ysXHei;
}

void FsPollDevice(void)
{
	int i,fsKey;
	char chr;
	KeySym ks;
	XEvent ev;

	fsKey=FSKEY_NULL;

	while(XCheckWindowEvent(ysXDsp,ysXWnd,KeyPressMask|KeyReleaseMask,&ev)==True)
	{
		int i;
		KeySym *keySymMap;
		int keysyms_per_keycode_return;
		keySymMap=XGetKeyboardMapping(ysXDsp,ev.xkey.keycode,1,&keysyms_per_keycode_return);
		//	 	printf("NumKeySym=%d\n",keysyms_per_keycode_return);
		//	printf("%s %s\n",XKeysymToString(keySymMap[0]),XKeysymToString(keySymMap[1]));
		//	printf("%d %d %d %d %d %d %d %d\n",
		//	 (ev.xkey.state&LockMask)!=0,
		//	 (ev.xkey.state&ShiftMask)!=0,
		//	 (ev.xkey.state&ControlMask)!=0,
		//	 (ev.xkey.state&Mod1Mask)!=0,  // Alt
		//	 (ev.xkey.state&Mod2Mask)!=0,  // Num Lock
		//	 (ev.xkey.state&Mod3Mask)!=0,
		//	 (ev.xkey.state&Mod4Mask)!=0,  // Windows key
		//	 (ev.xkey.state&Mod5Mask)!=0);

		if(0!=(ev.xkey.state&ControlMask) || 0!=(ev.xkey.state&Mod1Mask))
		  {
		    chr=0;
		  }
		  else if((ev.xkey.state&LockMask)==0 && (ev.xkey.state&ShiftMask)==0)
		{
		    chr=FsXKeySymToChar(keySymMap[0]); // mapXKtoChar[keySymMap[0]];
		}
		else if((ev.xkey.state&LockMask)==0 && (ev.xkey.state&ShiftMask)!=0)
		{
		    chr=FsXKeySymToChar(keySymMap[1]); // mapXKtoChar[keySymMap[1]];
		}
		else if((ev.xkey.state&ShiftMask)==0 && (ev.xkey.state&LockMask)!=0)
		{
			chr=FsXKeySymToChar(keySymMap[0]); // mapXKtoChar[keySymMap[0]];
			if('a'<=chr && chr<='z')
			{
				chr=chr+('A'-'a');
			}
		}
		else if((ev.xkey.state&ShiftMask)!=0 && (ev.xkey.state&LockMask)!=0)
		{
			chr=FsXKeySymToChar(keySymMap[1]); // mapXKtoChar[keySymMap[1]];
			if('a'<=chr && chr<='z')
			{
				chr=chr+('A'-'a');
			}
		}

		// Memo:
		// XK code is so badly designed.  XK_KP_Divide, XK_KP_Multiply,
		// XK_KP_Subtract, XK_KP_Add, should not be altered to
		// XK_XF86_Next_VMode or like that.  Other XK_KP_ code
		// can be altered by Mod2Mask.


		// following keys should be flipped based on Num Lock mask.  Apparently mod2mask is num lock by standard.
		// XK_KP_Space
		// XK_KP_Tab
		// XK_KP_Enter
		// XK_KP_F1
		// XK_KP_F2
		// XK_KP_F3
		// XK_KP_F4
		// XK_KP_Home
		// XK_KP_Left
		// XK_KP_Up
		// XK_KP_Right
		// XK_KP_Down
		// XK_KP_Prior
		// XK_KP_Page_Up
		// XK_KP_Next
		// XK_KP_Page_Down
		// XK_KP_End
		// XK_KP_Begin
		// XK_KP_Insert
		// XK_KP_Delete
		// XK_KP_Equal
		// XK_KP_Multiply
		// XK_KP_Add
		// XK_KP_Separator
		// XK_KP_Subtract
		// XK_KP_Decimal
		// XK_KP_Divide

		// XK_KP_0
		// XK_KP_1
		// XK_KP_2
		// XK_KP_3
		// XK_KP_4
		// XK_KP_5
		// XK_KP_6
		// XK_KP_7
		// XK_KP_8
		// XK_KP_9

		ks=XKeycodeToKeysym(ysXDsp,ev.xkey.keycode,0);
		if(XK_a<=ks && ks<=XK_z)
		{
			ks=ks+XK_A-XK_a;
		}
		if(ks==XK_Alt_R)
		{
			ks=XK_Alt_L;
		}
		if(ks==XK_Shift_R)
		{
			ks=XK_Shift_L;
		}
		if(ks==XK_Control_R)
		{
			ks=XK_Control_L;
		}

		if(0<=ks && ks<FS_NUM_XK)
		{
			fsKey=FsXKeySymToFskey(ks); // mapXKtoFSKEY[ks];

			// 2005/03/29 >>
			if(fsKey==0)
			{
				KeyCode kcode;
				kcode=XKeysymToKeycode(ysXDsp,ks);
				if(kcode!=0)
				{
					ks=XKeycodeToKeysym(ysXDsp,kcode,0);
					if(XK_a<=ks && ks<=XK_z)
					{
						ks=ks+XK_A-XK_a;
					}
					if(ks==XK_Alt_R)
					{
						ks=XK_Alt_L;
					}
					if(ks==XK_Shift_R)
					{
						ks=XK_Shift_L;
					}
					if(ks==XK_Control_R)
					{
						ks=XK_Control_L;
					}

					if(0<=ks && ks<FS_NUM_XK)
					{
						fsKey=FsXKeySymToFskey(ks); // mapXKtoFSKEY[ks];
					}
				}
			}
			// 2005/03/29 <<

			if(ev.type==KeyPress && fsKey!=0)
			{
				fsKeyPress[fsKey]=1;
				if(ev.xkey.window==ysXWnd) // 2005/04/08
				{
					if(nKeyBufUsed<NKEYBUF)
					{
						keyBuffer[nKeyBufUsed++]=fsKey;
					}
					if(chr!=0 && nCharBufUsed<NKEYBUF)
					{
						charBuffer[nCharBufUsed++]=chr;
					}
				}
			}
			else
			{
				fsKeyPress[fsKey]=0;
			}
		}
	}

	while(XCheckWindowEvent(ysXDsp,ysXWnd,ButtonPressMask|ButtonReleaseMask|PointerMotionMask,&ev)==True)
	{
		if(ButtonPress==ev.type || ButtonRelease==ev.type)
		{
			fsKey=FSKEY_NULL;
			if(ev.xbutton.button==Button4)
			{
				fsKey=FSKEY_WHEELUP;
			}
			else if(ev.xbutton.button==Button5)
			{
				fsKey=FSKEY_WHEELDOWN;
			}

			if(FSKEY_NULL!=fsKey)
			{
				if(ev.type==ButtonPress)
				{
					fsKeyPress[fsKey]=1;
					if(ev.xbutton.window==ysXWnd)
					{
						if(nKeyBufUsed<NKEYBUF)
						{
							keyBuffer[nKeyBufUsed++]=fsKey;
						}
					}

				}
				else if(ev.type==ButtonRelease)
				{
					fsKeyPress[fsKey]=0;
				}
			}
			else if(NKEYBUF>nMosBufUsed)
			{
				mosBuffer[nMosBufUsed].eventType=FSMOUSEEVENT_NONE;
				if(ev.type==ButtonPress)
				{
					switch(ev.xbutton.button)
					{
					case Button1:
						mosBuffer[nMosBufUsed].eventType=FSMOUSEEVENT_LBUTTONDOWN;
						lastKnownLb=1;
						break;
					case Button2:
						mosBuffer[nMosBufUsed].eventType=FSMOUSEEVENT_MBUTTONDOWN;
						lastKnownMb=1;
						break;
					case Button3:
						mosBuffer[nMosBufUsed].eventType=FSMOUSEEVENT_RBUTTONDOWN;
						lastKnownRb=1;
						break;
					}
				}
				else if(ev.type==ButtonRelease)
				{
					switch(ev.xbutton.button)
					{
					case Button1:
						mosBuffer[nMosBufUsed].eventType=FSMOUSEEVENT_LBUTTONUP;
						lastKnownLb=0;
						break;
					case Button2:
						mosBuffer[nMosBufUsed].eventType=FSMOUSEEVENT_MBUTTONUP;
						lastKnownMb=0;
						break;
					case Button3:
						mosBuffer[nMosBufUsed].eventType=FSMOUSEEVENT_RBUTTONUP;
						lastKnownRb=0;
						break;
					}
				}

				mosBuffer[nMosBufUsed].mx=ev.xbutton.x;
				mosBuffer[nMosBufUsed].my=ev.xbutton.y;
				// Turned out these button states are highly unreliable.
				// It may come with (state&Button1Mask)==0 on ButtonPress event of Button 1.
				// Confirmed this problem in VirtualBox.  This silly flaw may not occur in 
				// real environment.
				// mosBuffer[nMosBufUsed].lb=(0!=(ev.xbutton.state & Button1Mask));
				// mosBuffer[nMosBufUsed].mb=(0!=(ev.xbutton.state & Button2Mask));
				// mosBuffer[nMosBufUsed].rb=(0!=(ev.xbutton.state & Button3Mask));
				mosBuffer[nMosBufUsed].lb=lastKnownLb;
				mosBuffer[nMosBufUsed].mb=lastKnownMb;
				mosBuffer[nMosBufUsed].rb=lastKnownRb;
				mosBuffer[nMosBufUsed].shift=(0!=(ev.xbutton.state & ShiftMask));
				mosBuffer[nMosBufUsed].ctrl=(0!=(ev.xbutton.state & ControlMask));

				nMosBufUsed++;
			}
		}
		else if(ev.type==MotionNotify)
		{
			int mx=ev.xbutton.x;
			int my=ev.xbutton.y;
			int lb=lastKnownLb; // XButtonEvent.state turns out to be highly unreliable  (0!=(ev.xbutton.state & Button1Mask));
			int mb=lastKnownMb; // (0!=(ev.xbutton.state & Button2Mask));
			int rb=lastKnownRb; // (0!=(ev.xbutton.state & Button3Mask));
			int shift=(0!=(ev.xbutton.state & ShiftMask));
			int ctrl=(0!=(ev.xbutton.state & ControlMask));

			if(0<nMosBufUsed &&
			   mosBuffer[nMosBufUsed-1].eventType==FSMOUSEEVENT_MOVE &&
			   mosBuffer[nMosBufUsed-1].lb==lb &&
			   mosBuffer[nMosBufUsed-1].mb==mb &&
			   mosBuffer[nMosBufUsed-1].rb==rb &&
			   mosBuffer[nMosBufUsed-1].shift==shift &&
			   mosBuffer[nMosBufUsed-1].ctrl==ctrl)
			{
				mosBuffer[nMosBufUsed-1].mx=mx;
				mosBuffer[nMosBufUsed-1].my=my;
			}

			if(NKEYBUF>nMosBufUsed)
			{
				mosBuffer[nMosBufUsed].mx=mx;
				mosBuffer[nMosBufUsed].my=my;
				mosBuffer[nMosBufUsed].lb=lb;
				mosBuffer[nMosBufUsed].mb=mb;
				mosBuffer[nMosBufUsed].rb=rb;
				mosBuffer[nMosBufUsed].shift=shift;
				mosBuffer[nMosBufUsed].ctrl=ctrl;

				mosBuffer[nMosBufUsed].eventType=FSMOUSEEVENT_MOVE;

				nMosBufUsed++;
			}
		}
	}

	if(XCheckTypedWindowEvent(ysXDsp,ysXWnd,ConfigureNotify,&ev)==True)
	{
		ysXWid=ev.xconfigure.width;
		ysXHei=ev.xconfigure.height;
	}

	if(XCheckWindowEvent(ysXDsp,ysXWnd,ExposureMask,&ev)==True)
	{
		exposure=1;
	}

	if(XCheckTypedWindowEvent(ysXDsp,ysXWnd,DestroyNotify,&ev)==True)
	{
		exit(1);
	}

	return;
}

void FsCloseWindow(void)
{
	XCloseDisplay(ysXDsp);
}

void FsSleep(int ms)
{
	if(ms>0)
	{
		fd_set set;
		struct timeval wait;
		wait.tv_sec=ms/1000;
		wait.tv_usec=(ms%1000)*1000;
		FD_ZERO(&set);
		select(0,&set,NULL,NULL,&wait);
	}
}

int FsPassedTime(void)
{
	static unsigned int lastTime=0;
	unsigned int clk,passed;
	timeval tm;

	gettimeofday(&tm,NULL);
	clk=(tm.tv_sec*1000+tm.tv_usec/1000)&0x7fffffff;

	if(clk-lastTime<0)
	{
		lastTime=clk;
	}
	passed=clk-lastTime;

	lastTime=clk;

	return passed;
}

void FsGetMouseState(int &lb,int &mb,int &rb,int &mx,int &my)
{
	Window r,c;
	int xInRoot,yInRoot;
	unsigned int mask;

	XQueryPointer(ysXDsp,ysXWnd,&r,&c,&xInRoot,&yInRoot,&mx,&my,&mask);
	
	/* These masks are seriouly unreliable.  It could still report zero after ButtonPress event 
	   is issued.  Therefore, it causes inconsistency and unusable.  Flaw confirmed in VirtualBox.
	lb=((mask & Button1Mask) ? 1 : 0);
	mb=((mask & Button2Mask) ? 1 : 0);
	rb=((mask & Button3Mask) ? 1 : 0); */

	lb=lastKnownLb;
	mb=lastKnownMb;
	rb=lastKnownRb;
}

int FsGetMouseEvent(int &lb,int &mb,int &rb,int &mx,int &my)
{
	if(0<nMosBufUsed)
	{
		int eventType=mosBuffer[0].eventType;
		mx=mosBuffer[0].mx;
		my=mosBuffer[0].my;
		lb=mosBuffer[0].lb;
		mb=mosBuffer[0].mb;
		rb=mosBuffer[0].rb;

		int i;
		for(i=0; i<nMosBufUsed-1; i++)
		{
			mosBuffer[i]=mosBuffer[i+1];
		}
		nMosBufUsed--;

		return eventType;
	}
	else
	{
		FsGetMouseState(lb,mb,rb,mx,my);
		return FSMOUSEEVENT_NONE;
	}
}

void FsSwapBuffers(void)
{
	glFlush();
	glXSwapBuffers(ysXDsp,ysXWnd);
}

int FsInkey(void)
{
	if(nKeyBufUsed>0)
	{
		int i,keyCode;
		keyCode=keyBuffer[0];
		nKeyBufUsed--;
		for(i=0; i<nKeyBufUsed; i++)
		{
			keyBuffer[i]=keyBuffer[i+1];
		}
		return keyCode;
	}
	return 0;
}

int FsInkeyChar(void)
{
	if(nCharBufUsed>0)
	{
		int i,asciiCode;
		asciiCode=charBuffer[0];
		nCharBufUsed--;
		for(i=0; i<nCharBufUsed; i++)
		{
			charBuffer[i]=charBuffer[i+1];
		}
		return asciiCode;
	}
	return 0;
}

int FsGetKeyState(int fsKeyCode)
{
	if(0<fsKeyCode && fsKeyCode<FSKEY_NUM_KEYCODE)
	{
		return fsKeyPress[fsKeyCode];
	}
	return 0;
}

int FsCheckWindowExposure(void)
{
	int ret;
	ret=exposure;
	exposure=0;
	return ret;
}
