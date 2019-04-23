/*
 *          Copyright 2019, Vitali Baumtrok.
 * Distributed under the Boost Software License, Version 1.0.
 *     (See accompanying file LICENSE or copy at
 *        http://www.boost.org/LICENSE_1_0.txt)
 */


#include <stdlib.h>
#include <stdio.h>
#include <X11/Xlib.h>
#include <GL/glx.h>


static const int  canvas_width_init  = 256;
static const int  canvas_height_init = 256;
static int        canvas_width_curr  = 256;
static int        canvas_height_curr = 256;
static GLfloat   *projection_matrix  = NULL;

static Display   *display_ptr        = NULL;
static GLXContext glxContext         = NULL;
static Window     window_id;
static Atom       wm_protocols;
static Atom       wm_delete_window_atom;

static void close_window ( void ) {
	XEvent close_event;

	close_event.xclient.type         = ClientMessage;
	close_event.xclient.message_type = wm_protocols;
	close_event.xclient.format       = 32;
	close_event.xclient.data.l[0]    = wm_delete_window_atom;
	close_event.xclient.data.l[1]    = CurrentTime;

	XSendEvent(display_ptr,window_id,False,NoEventMask,&close_event);
}

static void initialize_projection_matrix ( void ) {
	projection_matrix     = (GLfloat*) malloc(sizeof(GLfloat)*16);
	projection_matrix[0]  = 2.0f / canvas_width_init;
	projection_matrix[1]  = 0;
	projection_matrix[2]  = 0;
	projection_matrix[3]  = 0;

	projection_matrix[4]  = 0;
	projection_matrix[5]  = 2.0f / canvas_height_init;
	projection_matrix[6]  = 0;
	projection_matrix[7]  = 0;

	projection_matrix[8]  = 0;
	projection_matrix[9]  = 0;
	projection_matrix[10] = 2.0f / 255;
	projection_matrix[11] = 0;

	projection_matrix[12] = -1;
	projection_matrix[13] = -1;
	projection_matrix[14] = -1;
	projection_matrix[15] = 1;
}

static void resize_canvas ( void ) {
	const GLfloat ratio_init = ((GLfloat) canvas_width_init ) / canvas_height_init;
	const GLfloat ratio_curr = ((GLfloat) canvas_width_curr ) / canvas_height_curr;
	GLfloat x_scale;
	GLfloat y_scale;

	if ( ratio_init < ratio_curr ) {
		x_scale = ratio_init / ratio_curr;
		y_scale = 1;

	} else {
		x_scale = 1;
		y_scale = ratio_curr / ratio_init;
	}

	projection_matrix[0]  = ( 2.0f / canvas_width_init ) * x_scale;
	projection_matrix[5]  = ( 2.0f / canvas_height_init ) * y_scale;
	projection_matrix[12] = -1 * x_scale;
	projection_matrix[13] = -1 * y_scale;

	glViewport(0,0,canvas_width_curr,canvas_height_curr);
	glLoadMatrixf(projection_matrix);
}

static void update_stencil_buffer ( void ) {
	glStencilFunc(GL_NEVER,1,0xFF);
	glStencilOp(GL_REPLACE,GL_KEEP,GL_KEEP);

	glStencilMask(0xFF);
	glClear(GL_STENCIL_BUFFER_BIT);

	glBegin(GL_TRIANGLE_STRIP);
		glVertex3i(0,canvas_height_init,0);
		glVertex3i(0,0,0);
		glVertex3i(canvas_width_init,canvas_height_init,0);
		glVertex3i(canvas_width_init,0,0);
	glEnd();

	glStencilMask(0x00);
	glStencilFunc(GL_EQUAL,1,0xFF);
}

static void draw_graphics ( void ) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glBegin(GL_TRIANGLES);
		glColor3f(1,0,0);
		glVertex3f(canvas_width_init/2, canvas_height_init, 0);
		glColor3f(0,1,0);
		glVertex3f(0,0,0);
		glColor3f(0,0,1);
		glVertex3f(canvas_width_init, 0, 0);
	glEnd();
	glBegin(GL_TRIANGLES);
		glColor3f(1,1,1);
		glVertex3f(canvas_width_init + 100, canvas_height_init, 10);
		glVertex3f(0, canvas_height_init/2, 10);
		glVertex3f(canvas_width_init + 100, canvas_height_init/2, 10);
	glEnd();
	glFlush();
}

static void process_event ( void ) {
	XEvent xevent;
	XNextEvent(display_ptr,&xevent);

	/* printf("event type %d\n", (int) xevent.type); */

	/* input */
	if (xevent.type==KeyPress) {

		/* printf("key pressed %d\n", (int) xevent.xkey.keycode); */

		/* ESC key */
		if ((int) xevent.xkey.keycode == 9) {
			close_window();
		}

	/* resize */
	} else if ( xevent.type == ConfigureNotify ) {

		if ( xevent.xconfigure.width  != canvas_width_curr
		  || xevent.xconfigure.height != canvas_height_curr ) {

			canvas_width_curr  = xevent.xconfigure.width;
			canvas_height_curr = xevent.xconfigure.height;

			resize_canvas();
			update_stencil_buffer();
		}

	/* redraw */
	} else if ( xevent.type          == Expose
	         && xevent.xexpose.count == 0 ) {

		draw_graphics();

	/* close window */
	} else if ( xevent.type                 == ClientMessage
	         && xevent.xclient.message_type == wm_protocols
	         && xevent.xclient.data.l[0]    == wm_delete_window_atom ) {

		glXMakeCurrent(display_ptr, None, NULL);
		glXDestroyContext(display_ptr, glxContext);
		XDestroyWindow(display_ptr,window_id);
		XCloseDisplay(display_ptr);
		display_ptr = NULL;
	}
}

void create_window ( void ) {
	int          screen_id            = 0;
	Screen      *screen_ptr           = NULL;
	XVisualInfo *visual_info_ptr      = NULL;
	GLint        visual_attributes[5] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_STENCIL_SIZE, 8, None };
	Window       window_root_id;

	display_ptr     = XOpenDisplay(NULL);
	screen_id       = DefaultScreen(display_ptr);
	screen_ptr      = ScreenOfDisplay(display_ptr,screen_id);
	window_root_id  = RootWindowOfScreen(screen_ptr);
	visual_info_ptr = glXChooseVisual(display_ptr,screen_id,visual_attributes);

	if (visual_info_ptr != NULL) {
		XSetWindowAttributes set_window_attributes;

		Colormap colormap                = XCreateColormap(display_ptr,window_root_id,visual_info_ptr->visual,AllocNone);
		set_window_attributes.colormap   = colormap;
		set_window_attributes.event_mask = ExposureMask|KeyPressMask|StructureNotifyMask;
		window_id                        = XCreateWindow(display_ptr,
		                                                 window_root_id,
		                                                 0,
		                                                 0,
		                                                 canvas_width_init,
		                                                 canvas_height_init,
		                                                 0,
		                                                 visual_info_ptr->depth,
		                                                 InputOutput,
		                                                 visual_info_ptr->visual,
		                                                 CWColormap|CWEventMask,
		                                                 &set_window_attributes);

		wm_protocols          = XInternAtom(display_ptr,"WM_PROTOCOLS",False);
		wm_delete_window_atom = XInternAtom(display_ptr,"WM_DELETE_WINDOW",False);

		XSetWMProtocols(display_ptr,window_id,&wm_delete_window_atom,1);
		XMapWindow(display_ptr,window_id);
		XStoreName(display_ptr,window_id,"Example");

		glxContext = glXCreateContext(display_ptr,visual_info_ptr,NULL,GL_TRUE);

		if (glxContext != NULL) {
			initialize_projection_matrix();
			glXMakeCurrent(display_ptr,window_id,glxContext);
			glEnable(GL_DEPTH_TEST);
			glEnable(GL_STENCIL_TEST);
			resize_canvas();
			update_stencil_buffer();

		} else {
			printf("error: couldn't create OpenGL context\n");
			XDestroyWindow(display_ptr,window_id);
			XCloseDisplay(display_ptr);
			display_ptr = NULL;
		}
		XFree(visual_info_ptr);

	} else {
		printf("error: visual setting not available\n");
		XCloseDisplay(display_ptr);
		display_ptr = NULL;
	}
}

int main ( int argc, char **argv ) {
	create_window();

	while (display_ptr != NULL) {
		process_event();
	}
	return 0;
}

