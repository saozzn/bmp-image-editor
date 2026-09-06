#ifndef GUI_H
#define GUI_H
#include <gtk/gtk.h>
#include "image_proc.h"

#define MAX_UNDO 20
#define IUP_DEFAULT 0
#define IUP_CLOSE -1

typedef GtkWidget Ihandle;
typedef int (*Icallback)(Ihandle*);

void setup_and_run_gui(int argc, char **argv);
#endif
