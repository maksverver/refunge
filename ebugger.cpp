#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <GL/gl.h>
#include <GL/glu.h>

#include <FTGL/FTGL.h>
#include <FTGL/FTGLPolygonFont.h>
#include <FTGL/FTLibrary.h>

#include <FL/gl.h>
#include <FL/Fl.H>
#include <FL/Fl_Gl_Window.H>
#include <FL/Fl_Box.H>

#include "interpreter.h"

#define FONT_FILE "cour.ttf"
#define FONT_SIZE 12
#define CELL_SIZE 16.0f

#define PI 3.1415926535897932384626
#define RAD2DEG(x) (x*180.0f/PI)

float zoom_speed = .1;
float rot_speed = 0.33;
float move_speed = .1;

bool fullscreen = false;
bool clearmode = false;

float camera_z = 0.0f;
float camera_rotz = PI/4;
float camera_dist = 4.0f;
float camera_z_offset = 0.0f;

float cyl_radius = 0.0f;

FTFont *font = 0;

Interpreter *initial=0;
Interpreter *ci=0;

class Color {
	float r,g,b,a;

public:
  void select() const {
		glColor4f(r,g,b,a);
	}

	Color(int _r, int _g, int _b) {
		a = 1.0f;
		r = _r/255.0;
		g = _g/255.0;
		b = _b/255.0;
	}

	Color(float _r, float _g, float _b):r(_r),g(_g), b(_b),a(1.0) {}
	Color(float _r, float _g, float _b, float _a):r(_r),g(_g), b(_b),a(_a) {}
};

const float DIRECTION_ROT[4] = {-90.0f, -180.0f, 90.0f, 0.0f};

const int COLORS_SIZE = 12;
const Color COLORS[COLORS_SIZE] = {
    Color(255,0,0), Color(0,240,0), Color(0,0,255),
    Color(240,240,0), Color(240,0,240), Color(0,240,240),
    Color(240,128,0), Color(240,0,128), Color(0,240,128),
    Color(128,240,0), Color(128,0,240), Color(0,128,240) 
};

int current_color = 0;

const Color asciiColor = Color(1.0f,1.0f,1.0f);
const Color hexColor = Color(.5f,.5f,1.0f);

class DebugWindow : public Fl_Gl_Window {
  void draw();
	void drawCell(char c);
	void drawCursor(Cursor *c);
	void worldToCell(int r, int c);
	void init();

	int handle(int input);
	int handle_key();

public:
  DebugWindow(int x, int y, int w, int h, const char *label)
	  : Fl_Gl_Window(x,y,w,h,label) {
		//TODO: constructor code
	}
};

void DebugWindow::init() {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, double(w()) / h(), 1.0, 5000.0);
	glMatrixMode(GL_MODELVIEW);

	//clear modes
	glClearColor(0,0,0,0);
	glClearDepth(1);

	//depth buffer
	//glDepthFunc(GL_LESS);
	glDepthFunc(GL_LEQUAL);

	//fog setup
	float fog_color[] = {0,0,0,1};
	glFogfv(GL_FOG_COLOR, fog_color);
	glFogi(GL_FOG_MODE, GL_LINEAR);
	glFogf(GL_FOG_DENSITY, 0.01);
	glFogf(GL_FOG_START, 2*cyl_radius);
	glFogf(GL_FOG_END  , 8*cyl_radius); 

	//font setup
	font = new FTGLPolygonFont(FONT_FILE);
	if(font->Error()) {
		printf("Unable to op font file '%s'\n",FONT_FILE);
	} else {
		if(!font->FaceSize(FONT_SIZE)) {
			printf("Unable to set font face size %d\n",FONT_SIZE);
		}

		font->UseDisplayList(true);
	}


	//enable wanted features
	glEnable(GL_FOG);
	glEnable(GL_DEPTH_TEST);
}

void axis(float w) {
	glBegin(GL_LINES);
	//x
	glColor3f(1,w,w);
	glVertex3f(-1,0,0);
	glVertex3f(.75,0,0);
	glColor3f(1,1,1);
	glVertex3f(.75,0,0);
	glVertex3f(1,0,0);
	//y
	glColor3f(w,1,w);
	glVertex3f(0,-1,0);
	glVertex3f(0,.75,0);
	glColor3f(1,1,1);
	glVertex3f(0,.75,0);
	glVertex3f(0,1,0);
	//z
	glColor3f(w,w,1);
	glVertex3f(0,0,-1);
	glVertex3f(0,0,.75);
	glColor3f(1,1,1);
	glVertex3f(0,0,.75);
	glVertex3f(0,0,1);
	glEnd();
}

void DebugWindow::worldToCell( int r, int c) {
	float rho = (2*PI / interpreter_size(ci).width) * c;

	//rotate zodanig dat de y-as op de celrichting ligt
	glRotatef(RAD2DEG(rho)+90,0,0,1);

	//klap assesn zodat een cell x,y als oppervlak heeft
	glRotatef(90,1,0,0); 

	//Schuif assen naar beneden
	glTranslatef(0, -r*CELL_SIZE - .5*CELL_SIZE, 0);
}

void DebugWindow::draw() {
	if(!valid()) init();
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	glLoadIdentity();
	current_color=0;
	float ex = camera_dist*cos(camera_rotz);
	float ey = camera_dist*sin(camera_rotz);
	float ez = camera_z + camera_z_offset;

	float tx = 0;
	float ty = 0;
	float tz = camera_z;

	gluLookAt(ex,ey,ez,  tx,ty,tz,  0,0,1);

	Size size = interpreter_size(ci);

	for(int r=0;r<size.height;r++) {
		int w = size.width;
		while(w>0 && !interpreter_get(ci,r,w-1)) w--;
		for(int c=0;c<w;c++) {
			glPushMatrix();
			worldToCell(r, c);
			glTranslatef(0,0,cyl_radius);
			drawCell(interpreter_get(ci,r,c));
			glPopMatrix();
		}
	}

	Cursor *cursor = ci->cursors;
	while(cursor) {
		drawCursor(cursor);
		cursor = cursor->next;
	}
}

void DebugWindow::drawCursor(Cursor *c) {
	COLORS[current_color].select();
	float cs = CELL_SIZE/2;
	float bw = cs/4;

	//render cursor
	glPushMatrix();
	worldToCell(c->ir,c->ic);
	glTranslatef(0,0,cyl_radius-0.1);
	glRotatef(DIRECTION_ROT[c->id],0,0,1);
	glBegin(GL_TRIANGLES);
	
	glVertex2f(-cs, -cs + cs/2);
	glVertex2f(  0,  cs       );
	glVertex2f( cs, -cs + cs/2);
	
	glEnd();
	glPopMatrix();

	//render datapointer
	glPushMatrix();
	worldToCell(c->dr,c->dc);
	glTranslatef(0,0,cyl_radius-0.1);
	glBegin(GL_QUADS);
	glVertex2f(-cs   ,-cs);
	glVertex2f(-cs   , cs);
	glVertex2f(-cs+bw, cs);
	glVertex2f(-cs+bw,-cs);

	glVertex2f(-cs, cs);
	glVertex2f( cs, cs);
	glVertex2f( cs, cs-bw);
	glVertex2f(-cs, cs-bw);

	glVertex2f( cs,-cs);
	glVertex2f(cs-bw, -cs);
	glVertex2f(cs-bw,  cs);
	glVertex2f( cs,  cs);

	glVertex2f(-cs,-cs);
	glVertex2f(-cs,-cs+bw);
	glVertex2f( cs,-cs+bw);
	glVertex2f( cs,-cs);
	glEnd();
	glPopMatrix();

	//render connector
	float rho = (2*PI / interpreter_size(ci).width);
	float sx,sy,sz;
	float ex,ey,ez;
	sx = cyl_radius*cos(rho * c->ic);
	sy = cyl_radius*sin(rho * c->ic);
	sz = -(cs*2) * c->ir - cs;
	ex = cyl_radius*cos(rho * c->dc);
	ey = cyl_radius*sin(rho * c->dc);
	ez = -(cs*2) * c->dr - cs;

	#define STEPS 40
  float L1 = sqrt(pow(sx-ex,2)+pow(sy-ey,2));

	float Xx = ex+sx;
	float Xy = ey+sy;
	float dX = sqrt(Xx*Xx + Xy*Xy);
	Xx/=dX;
	Xy/=dX;
	
	float Lb = sqrt(cyl_radius*cyl_radius -  (L1/2)*(L1/2));
	float alpha = atan2f(Lb, L1/2);
	float beta = PI/2 - alpha;
	float La = (L1/2)*tanf(beta);
	float L2 = La + Lb;
	float Lr = sqrt((L1/2)*(L1/2) + La*La);
	float Cx = Xx*L2;
	float Cy = Xy*L2;

	float theta = (PI/2-beta) *2;
	float startTheta = atan2f(sy-Cy, sx-Cx);
	
	float dt = theta/STEPS;
	float hS = atan2(sy,sx);
	float hE = atan2(ey,ex);
	float Ry = sin(hS-hE);
	if(Ry<0) dt*=-1;
	float t = startTheta;

	float dz = (ez-sz)/STEPS;
	float z = sz;

	COLORS[current_color].select();
	glBegin(GL_LINE_STRIP);
	for(int i=0;i<STEPS+1;i++) {
		glVertex3f(Lr*cos(t)+Cx, Lr*sin(t)+Cy,z);
		t+=dt;
		z+=dz;
	}
	glEnd();


	current_color = (current_color+1) % COLORS_SIZE;
}

void DebugWindow::drawCell(char c) {
	
	/*
	float a = CELL_SIZE/2 - 0.1;
	glColor3f(.5,.5,.5);
	glBegin(GL_LINE_STRIP);
	glVertex3f(-a,-a, 0);
	glVertex3f(-a, a, 0);
	glVertex3f( a, a, 0);
	glVertex3f( a,-a, 0);
	glVertex3f(-a,-a, 0);
	glEnd();
	*/
	/*
	glBegin(GL_QUADS);
	glVertex3f( a,-a, 0);
	glVertex3f(-a,-a, 0);
	glVertex3f(-a, a, 0);
	glVertex3f( a, a, 0);
	glEnd();
	*/
  
	glPushMatrix();
	glTranslatef(0,0,0.01);
	char buffer[10] = {0};
	if(c>31 && c<127) {
		asciiColor.select();
		sprintf(buffer,"%c",c);
	} else {
		hexColor.select();
		sprintf(buffer,"%02X",((unsigned)c)&0xff);
	}
	float th = font->LineHeight();
	float tw = font->Advance(buffer);
	glTranslatef(-tw/2,-th/6,0);
	font->Render(buffer);
	glPopMatrix();
}

int DebugWindow::handle_key() {
	bool dirty = false;

	switch(Fl::event_key()) {
		case FL_Up:
			camera_z+=move_speed;
			if(camera_z>0) camera_z=0;
			dirty=true;
			break;
		case FL_Down:
			camera_z-=move_speed;
			dirty=true;
			break;
		case FL_Left:
			camera_rotz -= rot_speed;
			if(camera_rotz<0) camera_rotz+=2*PI;
			dirty=true;
			break;
		case FL_Right:
			camera_rotz += rot_speed;
			if(camera_rotz>2*PI) camera_rotz-=2*PI;
			dirty=true;
			break;
		case FL_Page_Up:
			camera_dist -= zoom_speed;
			if(camera_dist<0) camera_dist=0;
			dirty=true;
			break;
		case FL_Page_Down:
			camera_dist += zoom_speed;
			dirty=true;
			break;
		case FL_Home:
			camera_z_offset+=move_speed;
			dirty=true;
			break;
		case FL_End:
			camera_z_offset-=move_speed;
			dirty=true;
			break;
		case ' ':
			{
				int in=0,out=0;
				if(interpreter_needs_input(ci)) {
					in = fgetc(stdin);
				}
			  if(interpreter_step(ci,in,&out) & I_OUTPUT) {
					fputc(out,stdout);
					fflush(stdout);
				}
			}
			dirty=true;
			break;
		default:
			break;
	}

	if(dirty) {
		redraw();
		return 1;
	}	else {
		return 0;
	}
}

int DebugWindow::handle(int input) {
	switch(input) {
	case FL_FOCUS:
	case FL_UNFOCUS:
		return 1;

	case FL_KEYBOARD:
		return handle_key();

	default:
  	return Fl_Gl_Window::handle(input);
	}
}

int run_debugger() {
	DebugWindow *window = new DebugWindow(100,100,600,400,"3Debug");

	window->end();
	if(fullscreen) window->fullscreen();


	Size size = interpreter_size(ci);
	float theta = 2*PI/size.width;
	cyl_radius = CELL_SIZE / (2*tanf(theta/2));
	camera_dist = cyl_radius + 80;

	rot_speed = (2*PI) / size.width * rot_speed;
	move_speed = cyl_radius * move_speed;
	zoom_speed = cyl_radius * zoom_speed;

	window->show();

	return Fl::run();
}

int main(int argc, char **argv) {
	char nul=0; //nul-replacement-char
	char ch;

	while ((ch = getopt(argc, argv, "f*c:")) != -1)	{
		switch (ch)	{
		case 'c':
			if (strlen(optarg) != 1) {
				printf("-c expects a single character, not \"%s\"\n", optarg);
				return 1;
			}
			nul = *optarg;
			break;
		case 'f':
		  fullscreen = !fullscreen;
			break;
		case '*':
			clearmode = !clearmode;
			break;
		}
	}

	if (argc - optind != 1) {
		printf("Usage: %s [-cx] [-f] [-*] <program>\n", argv[0]);
		return argc != 1;
	}

	if(FTLibrary::Instance().Error()) {
		printf("Could not load FreeType library\n");
		return FTLibrary::Instance().Error();
	}

	initial = interpreter_from_source(argv[optind],nul);

	if(!initial) {
		printf("Could not create interpreter\n");
		return 1;
	}

	if(clearmode) {
		interpreter_add_flags(initial, F_CLEAR_MODE);
	}

	ci = interpreter_clone(initial);

	int ret = run_debugger();

	if(font) delete font;
	interpreter_destroy(ci);
	interpreter_destroy(initial);

	return ret;
}

