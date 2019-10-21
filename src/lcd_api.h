/*
 * lcd_api.h
 *
 *  Created on: 16 Sep. 2019
 *      Author: brandon
 */

#ifndef SRC_LCD_API_H_
#define SRC_LCD_API_H_

#include "string.h"

// Basic init
int lcd_init();
void lcd_close();
void lcd_poll();
int lcd_busy();
int lcd_ready();


int lcd_getWidth();
int lcd_getheight();


#define _widget_filler(_func) \
_func(string) \
_func(title) \
_func(hbar) \
_func(vbar) \
_func(icon) \
_func(scroller) \
_func(frame) \
_func(num)

#define w_enum(_a) _a,

typedef enum _widget_type{
    _widget_filler(w_enum)
}widget_type;

typedef enum _scrolldir{
    v,
    h,
    m
}scrolldir;

#define _scr_bl_filler(_func) \
_func(on) \
_func(off) \
_func(toggle) \
_func(open) \
_func(blink) \
_func(flash)

#define bl_enum(_a) bl_##_a,

typedef enum _scr_bl{
    _scr_bl_filler(bl_enum)
}scr_bl;

typedef struct _lcdbox{
    int left;
    int top;
    int right;
    int bottom;
}lcdbox;

typedef struct _lcdspan{
    int width;
    int height;
}lcdspan;

typedef struct _lcdpos{
    int x;
    int y;
}lcdpos;

typedef struct _widget_id{
    char *screen;
    char *widget;
}widget_id;


#define _BOX(_left,_top,_right,_bottom) (lcdbox){.left=_left, .top=_top,.right=_right,.bottom=_bottom}
#define _SPAN(_width,_height) (lcdspan){.width=_width,.height=_height}
#define _POS(_x,_y) (lcdpos){.x=_x, .y=_y}
#define _ID(_scr, _widget) (widget_id){.screen=_scr, .widget=_widget}

static inline char * _id(int value, char *suffix)
{
    static char name[128];
    snprintf(name,128,"%d%s",value,suffix);
    return name;
}

void lcd_addScreen(char *name);
void lcd_setScreenBacklight(char *name, scr_bl mode);


void lcd_addWidget(widget_id id, char * frame, widget_type type);
void lcd_delWidget(widget_id id);

void lcd_setString(widget_id id, lcdpos pos, char *string);
void lcd_setBar(widget_id id, lcdpos pos, int length);
void lcd_setFrame(widget_id id, lcdbox box, lcdspan span, scrolldir dir, int speed);


#endif /* SRC_LCD_API_H_ */
