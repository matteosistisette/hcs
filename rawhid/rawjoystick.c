/* Copyright 2003 Hans-Christoph Steiner <hans@eds.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
/* 
 * $Id: rawjoystick.c,v 1.4 2003-10-20 16:17:52 eighthave Exp $
 *
 * TODO
 * -make work with multiple joysticks (using SDL_JoyHatEvent.which)
 * -figure out why it takes so long for [rawjoystick] to start
 * -get throttle and twist working 
 * -use SDL_PumpEvent rather than SDL_PollEvent in _read
 */
static char *version = "$Revision: 1.4 $";

#include <SDL/SDL.h>
#include <m_pd.h>

#define DEBUG(x)
/* #define DEBUG(x) x */

/* total number of axes and buttons supported by this object */
/* each axis gets a fixed outlet */
#define RAWJOYSTICK_AXES     6
#define RAWJOYSTICK_BUTTONS  9

/* this is the outlet number for the hat switch */
#define RAWJOYSTICK_HATX     3
#define RAWJOYSTICK_HATY     4


/*------------------------------------------------------------------------------
 *  CLASS DEF
 */
static t_class *rawjoystick_class;

typedef struct _rawjoystick  {
  t_object            x_obj;
  SDL_Joystick        *x_joystick;
  t_int               x_devnum;
  int                 read_ok;
  int                 started;
  t_outlet            *x_axis_out[RAWJOYSTICK_AXES];
  t_outlet            *x_button_num_out;
  t_outlet            *x_button_val_out;
  t_clock             *x_clock;
  double              x_delaytime;
  int                 x_buttons;
  int                 x_hats;
  int                 x_axes;
} t_rawjoystick;

/*------------------------------------------------------------------------------
  */

static int rawjoystick_close(t_rawjoystick *x)  {
    DEBUG(post("rawjoystick_CLOSE"));

    if ( SDL_JoystickOpened(x->x_devnum) ) {
		 SDL_JoystickClose(x->x_joystick);
      return 1;
    }
    else {	 
		 post("ERROR: joystick not closed!!");
		 return 0;
    }
}

static int rawjoystick_open(t_rawjoystick *x)  {
  rawjoystick_close(x);

  DEBUG(post("rawjoystick_OPEN"));
  
  /* open device */
  SDL_JoystickEventState(SDL_ENABLE);
  x->x_joystick = SDL_JoystickOpen(x->x_devnum);

  /* test if device open */
  /* get name of device */  
  if ( SDL_JoystickOpened(x->x_devnum) ) {
	  post ("Configuring %s",SDL_JoystickName(x->x_devnum));
  }
  else {	 
	  post("ERROR: joystick not opened!!");
	  return 0;
  }

  x->x_axes = SDL_JoystickNumAxes(x->x_joystick);
  x->x_hats = SDL_JoystickNumHats(x->x_joystick);
  x->x_buttons = SDL_JoystickNumButtons(x->x_joystick);

  post ("   device has %i axes, %i hats, and %i buttons.\n",x->x_axes,x->x_hats,x->x_buttons);
  post ("WARNING * WARNING * WARNING * WARNING * WARNING * WARNING * WARNING");
  post ("This object is under development!  The interface will change!");
  post ("This means inlets, outlets, messages, etc. are not fixed!");
  post ("WARNING * WARNING * WARNING * WARNING * WARNING * WARNING * WARNING");
    
  return 1;
}

/* read the joystick, called through clock */
static void *rawjoystick_read(t_rawjoystick *x) {	
	SDL_Event           event; 
	
/* 	DEBUG(post("rawjoystick_READ"));    */
	
	if ( ! SDL_JoystickOpened(x->x_devnum) ) {
		post ("Joystick not open, you need to open it first.");
		return 0;
	}
	
/*   DEBUG(post("Joystick read: %s",SDL_JoystickName(x->x_devnum));) */

  if ( SDL_PollEvent(&event) ) {
	  DEBUG(post("SDL_Event.type: %i",event.type);)
	  DEBUG(post("SDL_JoyAxisEvent.value: %i",event.jaxis.value);)
	  DEBUG(post("SDL_JoyButtonEvent.value: %i",event.jbutton.state);)
	  switch (event.type) {
		  case SDL_JOYAXISMOTION:
/* 
 * It might be a good idea to make a for{;;} loop to output the values of
 * all of the axes everytime in right-to-left order.
 */
			  outlet_float (x->x_axis_out[event.jaxis.axis], event.jaxis.value);	
			  break;
		  case SDL_JOYHATMOTION:
			  /* this object only supports the first hat switch */
			  if (event.jhat.hat == 0) {
				  /* X axis */
				  if (event.jhat.value & SDL_HAT_LEFT) {
					  outlet_float (x->x_axis_out[RAWJOYSTICK_HATX], -1);	
				  } else if (event.jhat.value & SDL_HAT_RIGHT) {
					  outlet_float (x->x_axis_out[RAWJOYSTICK_HATX], 1);	
				  } else {
					  outlet_float (x->x_axis_out[RAWJOYSTICK_HATX], 0);	
				  }
				  /* Y axis */
				  if (event.jhat.value & SDL_HAT_UP) {
					  outlet_float (x->x_axis_out[RAWJOYSTICK_HATY], -1);	
				  } else if (event.jhat.value & SDL_HAT_DOWN) {
					  outlet_float (x->x_axis_out[RAWJOYSTICK_HATY], 1);	
				  } else {
					  outlet_float (x->x_axis_out[RAWJOYSTICK_HATY], 0);	
				  }
			  }
			  break;
		  case SDL_JOYBUTTONDOWN:
			  outlet_float (x->x_button_val_out, 1);
			  outlet_float (x->x_button_num_out, (float)event.jbutton.button);
			  break;
		  case SDL_JOYBUTTONUP:
			  outlet_float (x->x_button_val_out, 0);
			  outlet_float (x->x_button_num_out, (float)event.jbutton.button);
			  break;
		  default:
			  DEBUG(post("Unhandled event."));
	  }
  }

  if (x->started) 
	  clock_delay(x->x_clock, x->x_delaytime);

  return NULL;
}

/* Actions */

static void rawjoystick_bang(t_rawjoystick* x)  {
    DEBUG(post("rawjoystick_bang"));
}

static void rawjoystick_float(t_rawjoystick* x)  {
    DEBUG(post("rawjoystick_float"));
}

void rawjoystick_start(t_rawjoystick* x)
{
  DEBUG(post("rawjoystick_START"));

  if ( ( SDL_JoystickOpened(x->x_devnum) ) && ( ! x->started ) ) {
    x->started = 1;
	 clock_delay(x->x_clock, 0);
  }
}


void rawjoystick_stop(t_rawjoystick* x)  {
  DEBUG(post("rawjoystick_STOP");)
	  
  if ( ( SDL_JoystickOpened(x->x_devnum) ) && ( x->started ) ) {
	  x->started = 0;
	  clock_unset(x->x_clock);
  }
}

void rawjoystick_delay(t_rawjoystick* x, t_float f)  {
	DEBUG(post("rawjoystick_DELAY %f",f);)
	  
	  x->x_delaytime = f;
}


/* Misc setup functions */


static void rawjoystick_free(t_rawjoystick* x) {
  DEBUG(post("rawjoystick_free"));
    
  rawjoystick_stop(x);
  
  if ( SDL_JoystickOpened(x->x_devnum)) 
    SDL_JoystickClose(x->x_joystick);
  
  SDL_Quit();
  clock_free(x->x_clock);
}

static void *rawjoystick_new(t_float argument) {
  int i,joystickNumber;
  t_rawjoystick *x = (t_rawjoystick *)pd_new(rawjoystick_class);

  DEBUG(post("rawjoystick_NEW"));
  post("rawHID(e) rawjoystick %s, <hans@eds.org>", version);

  /* init vars */
  x->x_devnum = 0;
  x->read_ok = 1;
  x->started = 0;
  x->x_delaytime = 5;
  
  x->x_clock = clock_new(x, (t_method)rawjoystick_read);

  /* INIT SDL using joystick layer  */  
  /* Note: Video is required to start Event Loop !! */
  if ( SDL_Init(SDL_INIT_VIDEO|SDL_INIT_JOYSTICK) < 0 ) { 
	  post("Could not initialize SDL: %s\n", SDL_GetError());
	  // exit(-1);
	  return (0);	/* changed by olafmatt */
  }    

  post("%i joysticks were found:", SDL_NumJoysticks() );
  
  for( i=0; i < SDL_NumJoysticks(); i++ ) {
    post("    %s", SDL_JoystickName(i));
  }  

  joystickNumber = (int)argument;

  if ( (joystickNumber >= 0) && (joystickNumber < SDL_NumJoysticks() ) )
    x->x_devnum = joystickNumber;
  else 
    post("Joystick %i does not exist!",joystickNumber);
  
  /* create outlets for each axis */
  for (i = 0; i < RAWJOYSTICK_AXES; i++) 
    x->x_axis_out[i] = outlet_new(&x->x_obj, &s_float);
  
  /* create outlets for buttons */
  x->x_button_num_out = outlet_new(&x->x_obj, &s_float);
  x->x_button_val_out = outlet_new(&x->x_obj, &s_float);
  
  /* Open the device and save settings */
  if ( ! rawjoystick_open(x) ) return x;
  
  return (x);
}


void rawjoystick_setup(void)
{
  DEBUG(post("rawjoystick_setup");)
  rawjoystick_class = class_new(gensym("rawjoystick"), 
			     (t_newmethod)rawjoystick_new, 
			     (t_method)rawjoystick_free,
			     sizeof(t_rawjoystick),0,A_DEFFLOAT,0);

  /* add inlet datatype methods */
  class_addfloat(rawjoystick_class,(t_method) rawjoystick_float);
  class_addbang(rawjoystick_class,(t_method) rawjoystick_bang);

  /* add inlet message methods */
  class_addmethod(rawjoystick_class,(t_method) rawjoystick_open,gensym("open"),0);
  class_addmethod(rawjoystick_class,(t_method) rawjoystick_close,gensym("close"),0);

  class_addmethod(rawjoystick_class,(t_method) rawjoystick_start,gensym("start"),0);
  class_addmethod(rawjoystick_class,(t_method) rawjoystick_stop,gensym("stop"),0);

  class_addmethod(rawjoystick_class,(t_method) rawjoystick_read,gensym("read"),0);

  class_addmethod(rawjoystick_class,(t_method) rawjoystick_delay,gensym("delay"),A_FLOAT,0);
}

