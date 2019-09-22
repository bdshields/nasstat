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
#define lcdd_addr   "192.168.0.10"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include <stdio.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>


#define MAX_SCREENS 1
#define MAX_WIDGETS 10

typedef enum _thing_state{
    none =0,
    init,
    created,
    defined,
    deleted
}thing_state;

typedef enum _widget_type{
    string,
    
}widget_type;

typedef struct _lcd_widget{
    thing_state state;
    widget_type type;
}lcd_widget;

typedef struct _lcd_screen{
    thing_state state;
    thing_state lcd_widget[MAX_WIDGETS];
    
}lcd_screen;

typedef struct _lcdd_connection{
    thing_state state;
    int socket;
    int width;
    int height;
    int current_screen;
    int current_widget;
    lcd_screen screen[MAX_SCREENS];

}lcdd_connection;

lcdd_connection lcd_server;

void lcd_sendMessage(char *message, ...);


int lcd_init()
{
    struct sockaddr_in name;

    memset(&lcd_server, 0, sizeof(lcd_server));
    lcd_server.current_screen == -1;
    lcd_server.current_widget == -1;
    lcd_server.socket = socket (PF_INET, SOCK_STREAM, 0);
    name.sin_family = AF_INET;
    name.sin_port = htons (lcdd_port);
    inet_aton(lcdd_addr,&name.sin_addr);
    
    if(connect(lcd_server.socket, (struct sockaddr *) &name, sizeof(name)) != 0)
    {
        printf("connect Failed\n");
        goto fail;

    }
    fcntl (lcd_server.socket, F_SETFL, O_NONBLOCK);
    
    lcd_server.state = init;
    lcd_sendMessage("hello\n");
    printf("Connected\n");
    return 1;

fail:
    lcd_server.state = none;
    return 0;
}

void lcd_add_screen(int id)
{
    if(id >= MAX_SCREENS)
    {
        printf("Bad screen id %d (%d)",id,MAX_SCREENS);
        return;
    }
    if(lcd_server.screen[id].state == none)
    {
        lcd_sendMessage("screen_add %d\n",id);
        lcd_server.screen[id].state = init;
        lcd_server.current_screen = id;
        lcd_server.current_widget = -1;
    }
    else
    {
        printf("Screen already created %d",id);
    }
}

void lcd_addWidget(int scr, int frame, int id, widget_type type)
{
    if(id >= MAX_WIDGETS)
    {
        printf("Bad widget id %d (%d)",id,MAX_SCREENS);
        return;
    }
    if(lcd_server.screen[scr].state == created)
    {
        if(lcd_server.screen[scr].widget_state[id].state == none)
        {
            if(frame > -1)
            {
                lcd_sendMessage("widget_add %d %d string in %d\n",scr,id,frame);
            }
            else
            {
                lcd_sendMessage("widget_add %d %d string\n",scr,id);
            }
            lcd_server.screen[scr].widget_state[id].state = init;
            lcd_server.screen[scr].widget_state[id].type = type;
            lcd_server.current_screen = scr;
            lcd_server.current_widget = id;
        }
        else
        {
            printf("Screen already created %d",id);
        }
    }
}

void lcd_setWidget(int scr, int id, char *string)
{
    if(lcd_server.screen[scr].widget_state[id].state == created)
    {
        lcd_sendMessage("widget_set %d %d 1 1 %s\n",scr,id, string);
    }
}

void lcd_poll()
{
    char buffer[1024];
    int  length;
    char *param;
    length = recv(lcd_server.socket, buffer, 1024,0);
    if(length > 0)
    {
        // debugging
        buffer[length] = 0;
        printf("%s",buffer);

        if(strncmp(buffer,"connect",7) == 0)
        {
            // Parse connect message
            param = strstr(buffer,"lcd");
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
            printf("LCD is %d x %d",lcd_server.width, lcd_server.height);
            lcd_server.state = created;
            
            lcd_add_screen(0);
        }
        else if(strncmp(buffer,"success",7) == 0)
        {
            if(lcd_server.current_widget == -1)
            {
                if(lcd_server.current_screen >= 0)
                {
                    lcd_server.screen[lcd_server.current_screen].state = created;
                    lcd_server.current_screen = -1;
                    lcd_server.current_widget = -1;
                    lcd_addWidget(0,0);
                }
            }
            else if(lcd_server.current_screen >= 0)
            {
                lcd_server.screen[lcd_server.current_screen].widget_state[lcd_server.current_widget].state = created;
                lcd_server.current_screen = -1;
                lcd_server.current_widget = -1;
                lcd_setWidget(0, 0, "\"Hello World\"");
            }
        }
        
finish:
        ;
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
        printf("%s", send_buffer);
        send(lcd_server.socket,send_buffer, strlen(send_buffer), 0);
    }
}

void lcd_close()
{
    printf("Closing\n");
    if(lcd_server.state != none)
    {
        shutdown(lcd_server.socket, 2);
    }
}
