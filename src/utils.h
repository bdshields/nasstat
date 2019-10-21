/*
 * utils.h
 *
 *  Created on: 5 May 2019
 *      Author: brandon
 */

#ifndef SRC_UTILS_H_
#define SRC_UTILS_H_

#include <sys/time.h>
#include <stdint.h>

typedef struct timeval systime;
systime get_systime(void);
systime set_alarm(uint32_t milliseconds);
systime cancel_alarm(systime *alarm);

uint16_t alarm_expired(systime alarm);

void frame_sleep(uint32_t ms);

void debugSet(int level);
void _debug(int level,char *format,...);
#define DEBUG(...) _debug(2,__VA_ARGS__)
#define WARN(...) _debug(1,__VA_ARGS__)
#define NOTICE(...) _debug(0,__VA_ARGS__)


#endif /* SRC_UTILS_H_ */
