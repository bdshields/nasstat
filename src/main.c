/*
 * main.c
 *
 *  Created on: 16 Sep. 2019
 *      Author: brandon
 */


#include <signal.h>

void clear_down();
void sig_handler(int sig);

int running;

int main(int argc, char *argv[])
{
    // validate parameters

    // initialise the config file

    // init the lcd api

    // init signals
    signal (SIGTERM,sig_handler);
    signal (SIGINT,sig_handler);
    signal (SIGHUP,sig_handler);


    // Main loop
    running = 1;
    while(running)
    {

    }
    clear_down();
}

void sig_handler(int sig)
{
    switch(sig)
    {
    case SIGTERM:   // Default kill signal
    case SIGINT:    // Control-C signal
    case SIGHUP:    // Hang up signal (close terminal)
        clear_down();
        exit (0);
        break;
    }
}

void clear_down()
{
    // Close the lcd connection
}
