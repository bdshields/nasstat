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
#define lcdd_addr   "localhost"

#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>

typedef struct _lcdd_connection{
    int socket;

}lcdd_connection;

lcdd_connection lcd_server;

int lcd_init()
{
    struct sockaddr_in name;

    lcd_server.socket = socket (PF_INET, SOCK_STREAM, 0);
    name.sin_family = AF_INET;
    name.sin_port = htons (lcdd_port);
    inet_aton(lcdd_addr,&name);
    if(bind (lcd_server.socket, (struct sockaddr *) &name, sizeof (name)) != 0)
    {
        // Failed to set port, try again in 1 second
        printf("bind Failed\n");
        goto fail;
    }
    if(connect(lcd_server.socket, (struct sockaddr *) &name, lcdd_port) != 0)
    {
        printf("connect Failed\n");
        goto fail;

    }
    return 1;

fail:
    return 0;
}

void lcd_close()
{
    shutdown(lcd_server.socket, 2);
}
