/*
 * Copyright (c) 2002, Adam Dunkels.
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution. 
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgement:
 *        This product includes software developed by Adam Dunkels. 
 * 4. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.  
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.  
 *
 * This file is part of the "ctk" console GUI toolkit for cc65
 *
 * $Id: ctk.h,v 1.12 2003/08/09 23:32:37 adamdunkels Exp $
 *
 */

#ifndef __CTK_H__
#define __CTK_H__

#include "ctk-conf.h"
#include "ek.h"

#include "cc.h"

/* Defintions for the CTK widget types. */
#define CTK_WIDGET_SEPARATOR 1
#define CTK_WIDGET_LABEL     2
#define CTK_WIDGET_BUTTON    3
#define CTK_WIDGET_HYPERLINK 4
#define CTK_WIDGET_TEXTENTRY 5
#define CTK_WIDGET_BITMAP    6
#define CTK_WIDGET_ICON      7

struct ctk_widget;

#define CTK_SEPARATOR(x, y, w) \
 NULL, NULL, x, y, CTK_WIDGET_SEPARATOR, w, 1
struct ctk_separator {
  struct ctk_widget *next;
  struct ctk_window *window;
  unsigned char x, y;
  unsigned char type;
  unsigned char w, h;
};

#define CTK_BUTTON(x, y, w, text) \
 NULL, NULL, x, y, CTK_WIDGET_BUTTON, w, 1, text
struct ctk_button {
  struct ctk_widget *next;
  struct ctk_window *window;
  unsigned char x, y;
  unsigned char type;
  unsigned char w, h;
  char *text;
};

#define CTK_LABEL(x, y, w, h, text) \
 NULL, NULL, x, y, CTK_WIDGET_LABEL, w, h, text,
struct ctk_label {
  struct ctk_widget *next;
  struct ctk_window *window;
  unsigned char x, y;
  unsigned char type;
  unsigned char w, h;
  char *text;
};

#define CTK_HYPERLINK(x, y, w, text, url) \
 NULL, NULL, x, y, CTK_WIDGET_HYPERLINK, w, 1, text, url
struct ctk_hyperlink {
  struct ctk_widget *next;
  struct ctk_window *window;
  unsigned char x, y;
  unsigned char type;
  unsigned char w, h;
  char *text;
  char *url;
};

/* Editing modes of the CTK textentry widget. */
#define CTK_TEXTENTRY_NORMAL 0
#define CTK_TEXTENTRY_EDIT   1


#define CTK_TEXTENTRY(x, y, w, h, text, len) \
  NULL, NULL, x, y, CTK_WIDGET_TEXTENTRY, w, h, text, len, \
  CTK_TEXTENTRY_NORMAL, 0, 0
struct ctk_textentry {
  struct ctk_widget *next;
  struct ctk_window *window;
  unsigned char x, y;
  unsigned char type;
  unsigned char w, h;
  char *text;
  unsigned char len;
  unsigned char state;
  unsigned char xpos, ypos;
};


#define CTK_ICON(title, bitmap, textmap) \
 NULL, NULL, 0, 0, CTK_WIDGET_ICON, 2, 4, title, 0, bitmap, textmap
struct ctk_icon {
  struct ctk_widget *next;
  struct ctk_window *window;
  unsigned char x, y;
  unsigned char type;
  unsigned char w, h;
  char *title;
  ek_id_t owner;
  unsigned char *bitmap;
  char *textmap;
};

#define CTK_BITMAP(x, y, w, h, bitmap) \
  NULL, NULL, x, y, CTK_WIDGET_BITMAP, w, h, bitmap,
struct ctk_bitmap {
  struct ctk_widget *next;
  struct ctk_window *window;
  unsigned char x, y;
  unsigned char type;
  unsigned char w, h;
  unsigned char *bitmap;
};

#define CTK_TEXTMAP_NORMAL 0
#define CTK_TEXTMAP_ACTIVE 1

#define CTK_TEXTMAP(x, y, w, h, textmap, flags) \
 NULL, NULL, x, y, CTK_WIDGET_LABEL, w, h, text, flags, CTK_TEXTMAP_NORMAL
struct ctk_textmap {
  struct ctk_widget *next;
  struct ctk_window *window;
  unsigned char x, y;
  unsigned char type;
  unsigned char w, h;
  char *textmap;
  unsigned char flags;
  unsigned char state;
};

  

struct ctk_widget_button {
  char *text;
};

struct ctk_widget_label {
  char *text;
};

struct ctk_widget_hyperlink {
  char *text;
  char *url;
};

struct ctk_widget_textentry {
  char *text;
  unsigned char len;
  unsigned char state;
  unsigned char xpos, ypos;
};

struct ctk_widget_icon {
  char *title;
  ek_id_t owner;
  unsigned char *bitmap;
  char *textmap;
};

struct ctk_widget_bitmap {
  unsigned char *bitmap;
};

struct ctk_widget {
  struct ctk_widget *next;
  struct ctk_window *window;
  unsigned char x, y;
  unsigned char type;
  unsigned char w, h;
  union {
    struct ctk_widget_label label;
    struct ctk_widget_button button;
    struct ctk_widget_hyperlink hyperlink;
    struct ctk_widget_textentry textentry;
    struct ctk_widget_icon icon;
    struct ctk_widget_bitmap bitmap;
  } widget;
};

struct ctk_desktop;

/* Definition of the CTK window structure. */
struct ctk_window {
  struct ctk_window *next, *prev;
  struct ctk_desktop *desktop;
  
  ek_id_t owner;
  
  char *title;
  unsigned char titlelen;

#if CTK_CONF_WINDOWCLOSE
  struct ctk_button closebutton;
#else /* CTK_CONF_WINDOWCLOSE */
  struct ctk_label closebutton;
#endif /* CTK_CONF_WINDOWCLOSE */
  
#if CTK_CONF_WINDOWMOVE
  struct ctk_button titlebutton;
#else /* CTK_CONF_WINDOWMOVE */
  struct ctk_label titlebutton;
#endif /* CTK_CONF_WINDOWMOVE */

  unsigned char x, y;
  unsigned char w, h;


  struct ctk_widget *inactive;
  struct ctk_widget *active;
  struct ctk_widget *focused;
};

struct ctk_menuitem {
  char *title;
  unsigned char titlelen;
};

struct ctk_menu {
  struct ctk_menu *next;
  char *title;
  unsigned char titlelen;
#if CC_UNSIGNED_CHAR_BUGS
  unsigned int nitems;
  unsigned int active;
#else /* CC_UNSIGNED_CHAR_BUGS */
  unsigned char nitems;
  unsigned char active;
#endif /* CC_UNSIGNED_CHAR_BUGS */
  struct ctk_menuitem items[CTK_CONF_MAXMENUITEMS];
};

struct ctk_menus {
  struct ctk_menu *menus;
  struct ctk_menu *open;
  struct ctk_menu *desktopmenu;
};

/* Definition of the CTK desktop structure. */
struct ctk_desktop {
  char *name;
  
  struct ctk_window desktop_window;
  struct ctk_window *windows;
  struct ctk_window *dialog;
  
#if CTK_CONF_MENUS
  struct ctk_menus menus;
  struct ctk_menu *lastmenu;
  struct ctk_menu desktopmenu;
#endif /* CTK_CONF_MENUS */

  unsigned char height, width;
  
#define CTK_REDRAW_NONE         0
#define CTK_REDRAW_ALL          1
#define CTK_REDRAW_WINDOWS      2
#define CTK_REDRAW_WIDGETS      4
#define CTK_REDRAW_MENUS        8
#define CTK_REDRAW_PART        16

#ifndef CTK_CONF_MAX_REDRAWWIDGETS
#define CTK_CONF_MAX_REDRAWWIDGETS 8
#endif /* CTK_CONF_MAX_REDRAWWIDGETS */
#ifndef CTK_CONF_MAX_REDRAWWINDOWS
#define CTK_CONF_MAX_REDRAWWINDOWS 8
#endif /* CTK_CONF_MAX_REDRAWWINDOWS */
  
  

  unsigned char redraw;
  
  struct ctk_widget *redraw_widgets[CTK_CONF_MAX_REDRAWWIDGETS];
  unsigned char redraw_widgetptr;

  struct ctk_window *redraw_windows[CTK_CONF_MAX_REDRAWWINDOWS];
  unsigned char redraw_windowptr;

  
  unsigned char redraw_y1, redraw_y2;
};


/* Global CTK modes. */
#define CTK_MODE_NORMAL      0
#define CTK_MODE_WINDOWMOVE  1
#define CTK_MODE_SCREENSAVER 2
#define CTK_MODE_EXTERNAL    3

/* General ctk functions. */
void ctk_init(void);
void ctk_mode_set(unsigned char mode);
unsigned char ctk_mode_get(void);
/*void ctk_redraw(void);*/

/* Functions for manipulating windows. */
void ctk_window_new(struct ctk_window *window,
		    unsigned char w, unsigned char h,
		    char *title);
void ctk_window_clear(struct ctk_window *w);
void ctk_window_open(struct ctk_window *w);
#define ctk_window_move(w,xpos,ypos) do {(w)->x=xpos; (w)->y=ypos;}while(0)
void ctk_window_close(struct ctk_window *w);
void ctk_window_redraw(struct ctk_window *w);
#define ctk_window_isopen(w) ((w)->next != NULL)


/* Functions for manipulating dialogs. */
void ctk_dialog_new(struct ctk_window *window,
		    unsigned char w, unsigned char h);
void ctk_dialog_open(struct ctk_window *d);
void ctk_dialog_close(void);

/* Functions for manipulating menus. */
void ctk_menu_new(struct ctk_menu *menu, char *title);
void ctk_menu_add(struct ctk_menu *menu);
void ctk_menu_remove(struct ctk_menu *menu);
unsigned char ctk_menuitem_add(struct ctk_menu *menu, char *name);

/* Functions for icons. */
#define CTK_ICON_ADD(icon, id) ctk_icon_add((struct ctk_widget *)icon, id)
void ctk_icon_add(struct ctk_widget *icon, ek_id_t id);

/* Functions for manipulating widgets. */
#define CTK_WIDGET_ADD(win, widg) \
 ctk_widget_add(win, (struct ctk_widget *)widg)
void ctk_widget_add(struct ctk_window *window,
		    struct ctk_widget *widget);

#define CTK_WIDGET_FOCUS(win, widg) \
  (win)->focused = (struct ctk_widget *)(widg)
#define CTK_WIDGET_REDRAW(widg) \
 ctk_widget_redraw((struct ctk_widget *)widg)
void ctk_widget_redraw(struct ctk_widget *w);

#define CTK_WIDGET_TYPE(w) ((w)->type)

#define CTK_WIDGET_SET_XPOS(w, xpos) ((struct ctk_widget *)(w))->x = (xpos)
#define CTK_WIDGET_SET_YPOS(w, ypos) ((struct ctk_widget *)(w))->y = (ypos)

#define CTK_WIDGET_SET_WIDTH(widget, width) do { \
    ((struct ctk_widget *)(widget))->w = (width); } while(0)

#define CTK_WIDGET_XPOS(w) (((struct ctk_widget *)(w))->x)
#define CTK_WIDGET_YPOS(w) (((struct ctk_widget *)(w))->y)

#define ctk_textentry_set_height(w, height) \
                           (w)->widget.textentry.h = (height)
#define ctk_label_set_height(w, height) \
                           (w)->widget.label.h = (height)
#define ctk_label_set_text(l, t) (l)->text = (t)

#define ctk_button_set_text(b, t) (b)->text = (t)

#define ctk_bitmap_set_bitmap(b, m) (b)->bitmap = (m)

#define CTK_BUTTON_NEW(widg, xpos, ypos, width, buttontext) \
 (widg)->window = NULL; \
 (widg)->next = NULL; \
 (widg)->type = CTK_WIDGET_BUTTON; \
 (widg)->x = (xpos); \
 (widg)->y = (ypos); \
 (widg)->w = (width); \
 (widg)->h = 1; \
 (widg)->text = (buttontext)

#define CTK_LABEL_NEW(widg, xpos, ypos, width, height, labeltext) \
 (widg)->window = NULL; \
 (widg)->next = NULL; \
 (widg)->type = CTK_WIDGET_LABEL; \
 (widg)->x = (xpos); \
 (widg)->y = (ypos); \
 (widg)->w = (width); \
 (widg)->h = (height); \
 (widg)->text = (labeltext)

#define CTK_BITMAP_NEW(widg, xpos, ypos, width, height, bmap) \
 (widg)->window = NULL; \
 (widg)->next = NULL; \
 (widg)->type = CTK_WIDGET_BITMAP; \
 (widg)->x = (xpos); \
 (widg)->y = (ypos); \
 (widg)->w = (width); \
 (widg)->h = (height); \
 (widg)->bitmap = (bmap)

#define CTK_TEXTENTRY_NEW(widg, xxpos, yypos, width, height, textptr, textlen) \
 (widg)->window = NULL; \
 (widg)->next = NULL; \
 (widg)->type = CTK_WIDGET_TEXTENTRY; \
 (widg)->x = (xxpos); \
 (widg)->y = (yypos); \
 (widg)->w = (width); \
 (widg)->h = (height); \
 (widg)->text = (textptr); \
 (widg)->len = (textlen); \
 (widg)->state = CTK_TEXTENTRY_NORMAL; \
 (widg)->xpos = 0; \
 (widg)->ypos = 0



#define CTK_HYPERLINK_NEW(widg, xpos, ypos, width, linktext, linkurl) \
 (widg)->window = NULL; \
 (widg)->next = NULL; \
 (widg)->type = CTK_WIDGET_HYPERLINK; \
 (widg)->x = (xpos); \
 (widg)->y = (ypos); \
 (widg)->w = (width); \
 (widg)->h = 1; \
 (widg)->text = (linktext); \
 (widg)->url = (linkurl)

/* Desktop interface. */
void ctk_desktop_redraw(struct ctk_desktop *d);
unsigned char ctk_desktop_width(struct ctk_desktop *d);
unsigned char ctk_desktop_height(struct ctk_desktop *d);

/* Signals. */
extern ek_signal_t ctk_signal_keypress,
  ctk_signal_widget_activate,
  ctk_signal_widget_select,
  ctk_signal_timer,
  ctk_signal_menu_activate,
  ctk_signal_window_close,
  ctk_signal_pointer_move,
  ctk_signal_pointer_button;

#if CTK_CONF_SCREENSAVER
extern ek_signal_t ctk_signal_screensaver_stop,
  ctk_signal_screensaver_start;

extern unsigned short ctk_screensaver_timeout;
#define CTK_SCREENSAVER_SET_TIMEOUT(t) ctk_screensaver_timeout = (t)
#define CTK_SCREENSAVER_TIMEOUT() ctk_screensaver_timeout
#endif /* CTK_CONF_SCREENSAVER */

/* These should no longer be used: */
extern ek_signal_t ctk_signal_button_activate,
  ctk_signal_button_hover,
  ctk_signal_hyperlink_activate,
  ctk_signal_hyperlink_hover;

/* Focus flags */
#define CTK_FOCUS_NONE     0
#define CTK_FOCUS_WIDGET   1
#define CTK_FOCUS_WINDOW   2
#define CTK_FOCUS_DIALOG   4



#endif /* __CTK_H__ */
