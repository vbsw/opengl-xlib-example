/*
 *          Copyright 2019, Vitali Baumtrok.
 * Distributed under the Boost Software License, Version 1.0.
 *     (See accompanying file LICENSE or copy at
 *        http://www.boost.org/LICENSE_1_0.txt)
 */


#include <stdio.h>
#include <X11/Xlib.h>
#include <GL/glx.h>


static Display   *display_ptr   = NULL;
static const int  window_width  = 256;
static const int  window_height = 256;
static Window     window_id;
static Atom       wm_protocols;
static Atom       wm_delete_window_atom;
static GLXContext glxContext    = NULL;

static void close_window ( void ) {
	XEvent close_event;

	close_event.xclient.type         = ClientMessage;
	close_event.xclient.message_type = wm_protocols;
	close_event.xclient.format       = 32;
	close_event.xclient.data.l[0]    = wm_delete_window_atom;
	close_event.xclient.data.l[1]    = CurrentTime;

	XSendEvent(display_ptr,window_id,False,NoEventMask,&close_event);
}

static void draw_graphics ( const int width, const int height ) {
	glViewport(0,0,width,height);
	glClear(GL_COLOR_BUFFER_BIT);
	glBegin(GL_TRIANGLES);
	glColor3f(1.0f, 0.0f, 0.0f);
	glVertex2i(0, 1);
	glColor3f(0.0f, 1.0f, 0.0f);
	glVertex2i(-1, -1);
	glColor3f(0.0f, 0.0f, 1.0f);
	glVertex2i(1, -1);
	glEnd();
	glFlush();
}

static void process_event ( void ) {
	XEvent xevent;
	XWindowAttributes window_attributes;

	XNextEvent(display_ptr,&xevent);

	if (xevent.type==KeyPress) {

		/* printf("key pressed %d\n", (int) xevent.xkey.keycode); */
		/* ESC key */
		if ((int) xevent.xkey.keycode == 9) {
			close_window();
		}

	} else if ( xevent.type==Expose
	         && xevent.xexpose.count==0 ) {

		XGetWindowAttributes(display_ptr,window_id,&window_attributes);
		draw_graphics(window_attributes.width,window_attributes.height);

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
	GLint        visual_attributes[5] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, None };
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
		set_window_attributes.event_mask = ExposureMask|KeyPressMask|SubstructureNotifyMask;
		window_id                        = XCreateWindow(display_ptr,
		                                                 window_root_id,
		                                                 0,
		                                                 0,
		                                                 window_width,
		                                                 window_height,
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
			glXMakeCurrent(display_ptr,window_id,glxContext);

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

