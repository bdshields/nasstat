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


int lcd_init()
{

    return 1;
}
