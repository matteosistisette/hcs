/*
 * This object allows you to see what is currently in the canvas-local path.
 */

#include "m_pd.h"
#include "s_stuff.h"
#include "g_canvas.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* WARNING: KLUDGE!  */
/*
 * this struct is not publically defined (its in g_canvas.c) so I need to
 * include this here.  Its from Pd 0.41-test03 2006-11-19. */
struct _canvasenvironment
{
    t_symbol *ce_dir;      /* directory patch lives in */
    int ce_argc;           /* number of "$" arguments */
    t_atom *ce_argv;       /* array of "$" arguments */
    int ce_dollarzero;     /* value of "$0" */
    t_namelist *ce_path;   /* search path */
};

//static char *version = "$Revision: 1.2 $";

#define DEBUG(x)
//#define DEBUG(x) x 

/*------------------------------------------------------------------------------
 *  CLASS DEF
 */
t_class *ce_path_class;

typedef struct _ce_path
{
    t_object            x_obj;
    t_canvasenvironment *x_canvasenvironment;
    t_namelist          *x_current;
    t_outlet            *x_data_outlet;
    t_outlet            *x_status_outlet;
} t_ce_path;

static void ce_path_output(t_ce_path* x)
{
    DEBUG(post("ce_path_output"););
    char buffer[FILENAME_MAX];

/* TODO: think about using x->x_current->nl_next so that if [ce_path] is at
 * the end of its list, and another element gets added to the local
 * namespace, [ce_path] will output the new element on the next bang. */
    if(x->x_current)
    {
        strncpy(buffer, x->x_current->nl_string, FILENAME_MAX);
        outlet_symbol( x->x_data_outlet, gensym(buffer));
        x->x_current = x->x_current->nl_next;
    }
    else 
        outlet_bang(x->x_status_outlet);
}


static void ce_path_rewind(t_ce_path* x) 
{
    x->x_current = x->x_canvasenvironment->ce_path;
}


static void *ce_path_new(void)
{
    t_ce_path *x = (t_ce_path *)pd_new(ce_path_class);

    x->x_data_outlet = outlet_new(&x->x_obj, &s_symbol);
    x->x_status_outlet = outlet_new(&x->x_obj, 0);

    x->x_canvasenvironment = canvas_getenv(canvas_getcurrent());
    ce_path_rewind(x);
    
    return (x);
}

void ce_path_setup(void)
{
    ce_path_class = class_new(gensym("ce_path"), (t_newmethod)ce_path_new,
                             NULL,
                             sizeof(t_ce_path), 
                             CLASS_DEFAULT, 
                             0, 
                             0);
    /* add inlet atom methods */
    class_addbang(ce_path_class, (t_method) ce_path_output);
    
    /* add inlet selector methods */
    class_addmethod(ce_path_class, (t_method) ce_path_rewind,
                    gensym("rewind"), 0);
}
