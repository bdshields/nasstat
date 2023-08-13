/*
 * lcd_api.c
 *
 *  Created on: 16 Sep. 2019
 *      Author: brandon
 *
 *
 *      http://lcdproc.sourceforge.net/docs/lcdproc-0-5-5-dev.html#language
 */

#define lcdd_port   13666
#define lcdd_addr   "127.0.0.1"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include "lcd_api.h"
#include "utils.h"
#include "config_file.h"

#define LCD_TIMEOUT 10

typedef enum _thing_state{
    none =0,
    init,
    idle,
    add_widget,
}thing_state;


#define w_name(_a) #_a,

char *widget_name[] = {
    _widget_filler(w_name)
};

#define bl_name(_a) #_a,

char *scr_bl_name[] = {
    _scr_bl_filler(bl_name)
};

#define pri_name(_a) #_a,

char *scr_pri_name[] = {
        _scr_priority_filler(pri_name)
};


char *scrollname[]={"v","h","m"};


typedef struct _lcdd_connection{
    thing_state state;
    int socket;
    int width;
    int height;
}lcdd_connection;

lcdd_connection lcd_server;

void lcd_sendMessage(char *message, ...);
void lcd_waitResponse();


int lcd_init()
{
    struct sockaddr_in name;
    char ip_addr[128];
    int32_t ip_port;

    if(config_get_string("lcdd_addr", ip_addr) == 0)
    {
        strcpy(ip_addr,lcdd_addr);
    }
    if(config_get_int("lcdd_port", &ip_port) == 0)
    {
        ip_port = lcdd_port;
    }


    memset(&lcd_server, 0, sizeof(lcd_server));
    lcd_server.socket = socket (PF_INET, SOCK_STREAM, 0);
    name.sin_family = AF_INET;
    name.sin_port = htons (ip_port);
    inet_aton(ip_addr,&name.sin_addr);
    
    if(connect(lcd_server.socket, (struct sockaddr *) &name, sizeof(name)) != 0)
    {
        NOTICE("connect Failed\n");
        goto fail;

    }
    fcntl (lcd_server.socket, F_SETFL, O_NONBLOCK);
    
    lcd_server.state = init;
    lcd_sendMessage("hello\n");
    return 1;

fail:
    lcd_server.state = none;
    return 0;
}

int lcd_getWidth()
{
    if(lcd_server.state == idle)
    {
        return lcd_server.width;
    }
    else
    {
        return 0;
    }
}

int lcd_getheight()
{
    if(lcd_server.state == idle)
    {
        return lcd_server.height;
    }
    else
    {
        return 0;
    }
}

void lcd_addScreen(char *name)
{
    if(lcd_server.state == idle)
    {
        lcd_server.state = add_widget;
        lcd_sendMessage("screen_add %s\n",name);
        lcd_waitResponse();
    }
    else
    {
        WARN("Can't create screen \"%s\", busy\n",name);
    }
}

void lcd_setScreenBacklight(char *name, scr_bl mode)
{
    if(lcd_server.state == idle)
    {
        lcd_server.state = add_widget;
        lcd_sendMessage("screen_set %s -backlight %s\n",name,scr_bl_name[mode]);
        lcd_waitResponse();
    }
    else
    {
        WARN("Can't set screen backlight mode \"%s\", busy\n", scr_bl_name[mode]);
    }

}

void lcd_setScreenPriority(char *name, scr_priority priority)
{
    if(lcd_server.state == idle)
    {
        lcd_server.state = add_widget;
        lcd_sendMessage("screen_set %s -priority %s\n",name,scr_pri_name[priority]);
        lcd_waitResponse();
    }
    else
    {
        WARN("Can't set screen priority \"%s\", busy\n", scr_pri_name[priority]);
    }

}

void lcd_addWidget(widget_id id, char *frame, widget_type type)
{
    if(lcd_server.state == idle)
    {
        lcd_server.state = add_widget;
        if(frame != NULL)
        {
            lcd_sendMessage("widget_add %s %s %s -in %s\n",id.screen,id.widget, widget_name[type], frame);
        }
        else
        {
            lcd_sendMessage("widget_add %s %s %s\n",id.screen,id.widget, widget_name[type]);
        }
        lcd_waitResponse();
    }
    else
    {
        WARN("Can't create %s widget \"%s\", busy\n", widget_name[type],id.widget);
    }
}

void lcd_delWidget(widget_id id)
{
    if(lcd_server.state == idle)
    {
        lcd_server.state = add_widget;
        lcd_sendMessage("widget_del %s %s\n",id.screen,id.widget);
        lcd_waitResponse();
    }
    else
    {
        WARN("Can't delete widget \"%s\", busy\n", id.widget);
    }
}

void lcd_setString(widget_id id, lcdpos pos, char *string)
{
    if(lcd_server.state == idle)
    {
        lcd_server.state = add_widget;
        lcd_sendMessage("widget_set %s %s %d %d \"%s\"\n",id.screen,id.widget, pos.x, pos.y, string);
        lcd_waitResponse();
    }
    else
    {
        WARN("Can't set widget \"%s\", busy\n", id.widget);
    }
}

void lcd_setBar(widget_id id, lcdpos pos, int length)
{
    if(lcd_server.state == idle)
    {
        lcd_server.state = add_widget;
        lcd_sendMessage("widget_set %s %s %d %d %d\n",id.screen,id.widget, pos.x, pos.y, length);
        lcd_waitResponse();
    }
    else
    {
        WARN("Can't set widget \"%s\", busy\n", id.widget);
    }
}

void lcd_setFrame(widget_id id, lcdbox box, lcdspan span, scrolldir dir, int speed)
{
    if(lcd_server.state == idle)
    {
        lcd_server.state = add_widget;
        lcd_sendMessage("widget_set %s %s %d %d %d %d %d %d %s %d\n",id.screen,id.widget, box.left, box.top, box.right, box.bottom, span.width, span.height, scrollname[dir], speed);
        lcd_waitResponse();
    }
    else
    {
        WARN("Can't set widget \"%s\", busy\n", id.widget);
    }
}


void lcd_poll()
{
    char buffer[1024];
    char *message;
    char *end;
    int  length;
    char *param;
    length = recv(lcd_server.socket, buffer, 1024,0);
    message = buffer;
    while(length > 0)
    {
        end = strchr(message,'\n');
        if(end)
        {
            *end=0;
            end ++;
            length -= end - message;
        }
        else
        {
            // I hope there's no partial message
            message[length] = 0;
            length = 0;
            printf("Partial message\n");
        }
        // debugging
        DEBUG("%s\n",message);

        if(strncmp(message,"connect",7) == 0)
        {
            // Parse connect message
            param = strstr(message,"lcd");
            if(param == NULL)
            {
                goto finish;
            }
            param = strstr(param,"wid");
            if(param == NULL)
            {
                goto finish;
            }
            lcd_server.width = strtol(param + 4,NULL,10);
            param = strstr(param,"hgt");
            if(param == NULL)
            {
                goto finish;
            }
            lcd_server.height = strtol(param + 4,NULL,10);
            DEBUG("LCD is %d x %d\n",lcd_server.width, lcd_server.height);
            lcd_server.state = idle;
            
        }
        else if(strncmp(message,"success",7) == 0)
        {
            if(lcd_server.state == add_widget)
            {
                lcd_server.state = idle;
            }
        }
        else if(strncmp(message,"huh?",4) == 0)
        {
            // an error occured
            lcd_server.state = idle;
        }
        
finish:
        message = end;;
    }
}

int lcd_busy()
{
    if(lcd_server.state != idle)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

int lcd_ready()
{
    if(lcd_server.state != idle)
    {
        return 0;
    }
    else
    {
        return 1;
    }

}

void lcd_sendMessage(char *message, ...)
{
    char send_buffer[1024];
    if(lcd_server.state != none)
    {
        va_list ap;
        
        va_start (ap, message);
        vsnprintf(send_buffer,1024,message,ap);
        va_end(ap);
        DEBUG("%s", send_buffer);
        send(lcd_server.socket,send_buffer, strlen(send_buffer), 0);
    }
}

void lcd_waitResponse()
{
    systime timeout;
    lcd_poll();
    timeout = set_alarm(LCD_TIMEOUT);
    while(!alarm_expired(timeout))
    {
        lcd_poll();
        if(lcd_server.state == idle)
        {
            break;
        }
        frame_sleep(5);
    }
    // Timeout, set to idle
    lcd_server.state = idle;
}

void lcd_close()
{
    printf("Closing\n");
    if(lcd_server.state != none)
    {
        shutdown(lcd_server.socket, 2);
    }
}
