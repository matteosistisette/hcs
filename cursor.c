#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <m_pd.h>
#include <g_canvas.h>

static t_symbol *button_symbol;
static t_symbol *motion_symbol;
static t_symbol *wheel_symbol;

static t_class *cursor_class;

typedef struct _cursor
{
    t_object x_obj;
    t_symbol *receive_symbol;
    t_symbol *current_cursor;
    t_canvas *parent_canvas;
    t_outlet *data_outlet;
    t_outlet *status_outlet;
    t_int optionc;
    char *optionv[];
} t_cursor;

static void cursor_float(t_cursor *x, t_float f)
{
    if(f > 0)
    {
        sys_vgui("bind all <ButtonPress> {+pd [concat %s button %%b 1 \\;]}\n",
            x->receive_symbol->s_name);
        sys_vgui("bind all <ButtonRelease> {+pd [concat %s button %%b 0 \\;]}\n",
            x->receive_symbol->s_name);
        sys_vgui("bind all <Motion> {+pd [concat %s motion %%x %%y \\;]}\n",
            x->receive_symbol->s_name);
        sys_vgui("bind all <MouseWheel> {+pd [concat %s wheel %%D \\;]}\n",
            x->receive_symbol->s_name);
    }
    else
    {
    }
}

static void cursor_cursor(t_cursor *x, t_symbol *s)
{
    post("setting cursor: %s", s->s_name);
    x->current_cursor = s;
    post("set parent%lx [winfo parent .x%lx.c]; $parent%lx configure -cursor %s \n",
             x, x->parent_canvas, x, s->s_name);
    sys_vgui("set parent%lx [winfo parent .x%lx.c]; $parent%lx configure -cursor %s \n",
             x, x->parent_canvas, x, s->s_name);
}

static void cursor_button_callback(t_cursor *x, t_float button, t_float state)
{
    t_atom output_atoms[2];
    
    SETFLOAT(output_atoms, button);
    SETFLOAT(output_atoms + 1, state);
    outlet_anything(x->data_outlet, button_symbol, 2, output_atoms);
}

static void cursor_motion_callback(t_cursor *x, t_float x_position, t_float y_position)
{
    t_atom output_atoms[2];
    
    SETFLOAT(output_atoms, x_position);
    SETFLOAT(output_atoms + 1, y_position);
    outlet_anything(x->data_outlet, motion_symbol, 2, output_atoms);
}

static void cursor_wheel_callback(t_cursor *x, t_float f)
{
    t_atom output_atom;
    
    SETFLOAT(&output_atom, f);
    outlet_anything(x->data_outlet, wheel_symbol, 1, &output_atom);
}

static void cursor_free(t_cursor *x)
{
    pd_unbind(&x->x_obj.ob_pd, x->receive_symbol);
}

static void *cursor_new(t_symbol *s, int argc, t_atom *argv)
{
    char buf[MAXPDSTRING];
    t_cursor *x = (t_cursor *)pd_new(cursor_class);

    x->parent_canvas = canvas_getcurrent();

    sprintf(buf, "#%lx", (t_int)x);
    x->receive_symbol = gensym(buf);
    pd_bind(&x->x_obj.ob_pd, x->receive_symbol);
	x->data_outlet = outlet_new(&x->x_obj, 0);
	x->status_outlet = outlet_new(&x->x_obj, 0);

    return(x);
}

void cursor_setup(void)
{
    cursor_class = class_new(gensym("cursor"),
        (t_newmethod)cursor_new, (t_method)cursor_free,
        sizeof(t_cursor), 0, 0);

    class_addfloat(cursor_class, (t_method)cursor_float);

    class_addmethod(cursor_class, (t_method)cursor_cursor, 
                    gensym("cursor"), A_DEFSYMBOL, 0);
    class_addmethod(cursor_class, (t_method)cursor_button_callback, 
                    gensym("button"), A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(cursor_class, (t_method)cursor_motion_callback, 
                    gensym("motion"), A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(cursor_class, (t_method)cursor_wheel_callback, 
                    gensym("wheel"), A_DEFFLOAT, 0);

    button_symbol = gensym("button");
    motion_symbol = gensym("motion");
    wheel_symbol = gensym("wheel");
}
