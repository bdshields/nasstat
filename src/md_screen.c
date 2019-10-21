/*
 * md_screen.c
 *
 *  Created on: 1 Oct. 2019
 *      Author: brandon
 */

#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>

#include "widget_ids.h"
#include "lcd_api.h"
#include "utils.h"
#include "md_screen.h"
#include "config_file.h"


typedef enum _md_state{
    unknown=0,
    init,
    ready,
    busy,
}md_state_t;


#define RAID_STAT(_func) \
    _func(none)   \
    _func(clean    )   \
    _func(degraded )   \
    _func(resync)      \
    _func(rebuild  )   \
    _func(down     )

#define _enum_it(_a) _a,

typedef enum _md_raidStatus{
    RAID_STAT(_enum_it)
}md_raidStatus;

#define _str_it(_a) #_a,

char *md_statStr[] = {
        RAID_STAT(_str_it)
};

#define MD_DISK_STR 32

typedef struct _md_conf{
    char dev_name[MD_DISK_STR];
    int32_t  md_poll;
    int32_t  rd_poll;
    int32_t  ext_poll;
    int      rd_loc; // 0: dev, 1: addr
}md_conf_t;


typedef struct _md_disk{
    char dev[MD_DISK_STR];      // e.g. "sda1"
    char slot[MD_DISK_STR];     // e.g. "0" or "J"
    char bus[MD_DISK_STR];      // e.g. "ata"
    char addr[MD_DISK_STR];     // e.g. "ata2"
    char state[MD_DISK_STR];    // e.g. "in_sync"

}md_disk;

static md_state_t md_state=0;
static md_conf_t  md_conf;
static systime    md_update;
static systime    rd_update;
static systime    ext_update;
static md_raidStatus md_lastStatus;
static int           md_prevSlaves;

static md_disk md_array[MD_ARRAY_SIZE];

void md_initRaidWidgets();
void rd_initSlaveWidgets();
void rd_delSlaveWidgets();

md_raidStatus md_getRaidState(float *progress);
int md_getSlaves();

void md_updateState(char *state, float progress);
void rd_updateState();

int rd_getExtHealth(int device, char *buffer, int length);


/**
 * Initialise the RAID screen
 */
void md_init()
{
    char data[256];

    md_state=init;

    if(config_get_string("md_dev", md_conf.dev_name) == 0)
    {
        strcpy(md_conf.dev_name,"md1");
    }
    if(config_get_int("md_poll", &md_conf.md_poll) == 0)
    {
        md_conf.md_poll = 10;
    }
    if(config_get_int("rd_poll", &md_conf.rd_poll) == 0)
    {
        md_conf.rd_poll = 30;
    }
    if(config_get_int("ext_poll", &md_conf.ext_poll) == 0)
    {
        md_conf.ext_poll = 30;
    }
    md_conf.rd_loc = 0;
    if(config_get_string("rd_loc", data))
    {
        if(strcmp(data,"addr") == 0)
        {
            md_conf.rd_loc = 1;
        }
    }

    md_update = get_systime();
    rd_update = get_systime();
    ext_update = get_systime();

    // Build the display
    lcd_addScreen(RAID_SCR);

    lcd_setScreenBacklight(RAID_SCR, bl_on);

    md_initRaidWidgets();

    md_state = ready;
    md_lastStatus = unknown;
    md_prevSlaves = 0;
}


void md_proc()
{
    if(md_state == ready)
    {
        if(alarm_expired(md_update))
        {
            float progress=0;
            md_raidStatus status;
            int numSlaves;
            // Get raid status
            status = md_getRaidState(&progress);
            // Status changed
            if(status != md_lastStatus)
            {
                // Update status
                md_updateState(md_statStr[status],progress);
                // Expire other timers
                ext_update = set_alarm(0);
                rd_update = set_alarm(0);
                if(status == clean)
                {
                    lcd_setScreenBacklight(RAID_SCR, bl_on);
                }
                else
                {
                    lcd_setScreenBacklight(RAID_SCR, bl_flash);
                }
                md_lastStatus = status;
            }
            else
            {
                if(status == degraded)
                {
                    rd_update = set_alarm(0);
                }
                if(progress > 0)
                {
                    md_updateState(md_statStr[status],progress);
                }
            }


            if(alarm_expired(rd_update))
            {
                // Get slave status
                if(status != down)
                {
                    numSlaves = md_getSlaves();
                }
                else
                {
                    numSlaves = 0;
                }
                if(numSlaves != md_prevSlaves)
                {
                    // Delete slave widgets
                    rd_delSlaveWidgets();
                    if(numSlaves > 0)
                    {
                        // Get slave details
                        rd_initSlaveWidgets();
                    }
                    md_prevSlaves = numSlaves;
                }
                if(numSlaves > 0)
                {
                    rd_updateState();
                }

                rd_update = set_alarm(md_conf.rd_poll * 1000);
            }

            md_update = set_alarm(md_conf.md_poll * 1000);
        }
    }
}

/**
 * Init main header widgets
 */
void md_initRaidWidgets()
{
    char data[256];
    // Main Raid Device health and status
    lcd_addWidget(MD_DEV, NULL, string);
    lcd_addWidget(MD_STATE, NULL, string);
    lcd_addWidget(MD_PROG, NULL, hbar);

    // Write the device name
    snprintf(data,256,"%s:",md_conf.dev_name);
    lcd_setString(MD_DEV, MD_DEV_POS, data);

}

/**
 * Init widgets for slave devices
 */
void rd_initSlaveWidgets()
{
    int disk_idx;
    char label[64];
    // Main RD widget frame
    lcd_addWidget(SLAVE_FRAME, NULL, frame);
    lcd_setFrame(SLAVE_FRAME, _BOX(1,2,lcd_getWidth(),4), _SPAN(40,3), h, 80);
    disk_idx = 0;
    // Create frame per device
    while(md_array[disk_idx].dev[0] != 0)
    {
        snprintf(label, 64,"%d_FRAME",disk_idx);
        lcd_addWidget(_ID(RAID_SCR,label), SLAVE_FRAME_ID, frame);
        lcd_setFrame(_ID(RAID_SCR,label), _BOX((disk_idx * 10) + 1,1,(disk_idx * 10) + 10,3), _SPAN(10,3), v, 0);

        lcd_addWidget(RD_DISK(disk_idx), label, string);
        lcd_addWidget(RD_STATE(disk_idx), label, string);
        lcd_addWidget(RD_AGE(disk_idx), label, string);


        disk_idx++;
    }
}

/**
 * Delete the widgets for slave devices
 */
void rd_delSlaveWidgets()
{
    lcd_delWidget(SLAVE_FRAME);
}

/**
 * Update main RAID widgets
 */
void md_updateState(char *state, float progress)
{
    int   bar_len;

    bar_len = (int)((lcd_getWidth() - MD_PROG_START) * progress * 5);
    // Set the new value
    lcd_setString(MD_STATE, MD_STATE_POS, state);
    lcd_setBar(MD_PROG, MD_PROG_POS, bar_len);
}

/**
 * Read the Main RAID state
 */
md_raidStatus md_getRaidState(float *progress)
{
    md_raidStatus state = none;
    char filename[1024];
    char contents[256];
    int fd;
    ssize_t size;

    // check degraded state
    snprintf(filename,1024,"/sys/block/%s/md/degraded",md_conf.dev_name);
    fd = open(filename,0);
    if(fd < 0)
    {
        state = down;
        goto end;
    }
    size = read(fd, contents,256);
    close(fd);
    if(size < 1)
    {
        state = down;
        goto end;
    }
    if(contents[0] == '1')
    {
        state = degraded;
    }
    else
    {
        state = clean;
    }
    // Check sync progress
    snprintf(filename,1024,"/sys/block/%s/md/sync_completed",md_conf.dev_name);
    fd = open(filename,0);
    if(fd < 0)
    {
        state = degraded;
        goto end;
    }
    size = read(fd, contents,256);
    close(fd);
    if(size < 1)
    {
        state = degraded;
        goto end;
    }
    if(strncmp(contents,"none",4) == 0)
    {
        // Sync not in progress
        goto end;
    }
    else
    {
        if(state == clean)
        {
            state = resync;
        }
        else
        {
            state = rebuild;
        }
    }
    // get progress
    if(progress != NULL)
    {
        float pos;
        float total;
        char *slash;
        pos = strtof(contents, NULL);
        if(pos == 0.0)
        {
            // No progress yet, just display string
            goto end;
        }
        slash = strchr(contents,'/');
        if(slash != NULL)
        {
            total = strtof(slash + 1, NULL);
            *progress = pos/total;
        }
        else
        {
            *progress = 0;
        }
    }
end:
    return state;
}


static int dev_selector(const struct dirent *dir)
{
    if(strncmp(dir->d_name,"dev-",4) == 0)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

/**
 *  Populates a table of all currently attached devices
 */
int md_getSlaves()
{
    int fd;
    char contents[1024];
    char *path;
    ssize_t size;
    int num_entries;
    int counter;
    char filename[1024];
    struct dirent **eps;



    snprintf(filename,1024,"/sys/block/%s/md/",md_conf.dev_name);
    num_entries = scandir(filename,&eps,dev_selector,alphasort);
    for(counter = 0; counter<num_entries; counter++)
    {
        // Device Name
        strcpy(md_array[counter].dev, eps[counter]->d_name + 4);

        // Get raid slot
        snprintf(filename,1024,"/sys/block/%s/md/%s/slot",md_conf.dev_name,eps[counter]->d_name);
        fd = open(filename,0);
        if(fd < 0)
        {
            continue;
        }
        size = read(fd, contents,1024);
        close(fd);
        if(size < 1)
        {
            continue;
        }
        contents[size-1] = 0;
        if(strcmp(contents,"journal")==0)
        {
            strcpy(md_array[counter].slot,"J");
        }
        else if(strcmp(contents,"none")==0)
        {
            strcpy(md_array[counter].slot,"X");
        }
        else
        {
            strcpy(md_array[counter].slot,contents);
        }

        // Get host type
        snprintf(filename,1024,"/sys/block/%s/md/%s/block",md_conf.dev_name,eps[counter]->d_name);
        size = readlink(filename, contents, 1024);
        if(size < 1)
        {
            continue;
        }
        if((path = strstr(contents,"/nvme/")) != NULL)
        {
            // NVME device
            path+= strlen("/nvme/");
            path[strlen("nvme0")] = 0;
            strcpy(md_array[counter].bus, path);
            // Get position on bus
            path+= strlen("nvme0/");
            path[strlen("nvme0n1")] = 0;
        }
        else if((path = strstr(contents,"/ata")) != NULL)
        {
            // ATA device
            strcpy(md_array[counter].bus, "ata");
            // Get position on bus
            path++;
            path[strlen("ata1")] = 0;
        }
        else if((path = strstr(contents,"/usb")) != NULL)
        {
            char *c;
            // USB device
            path++;
            path[strlen("usb1")] = 0;
            strcpy(md_array[counter].bus, path);
            // Get position on bus
            path += strlen("usb1/");
            path = strstr(path,"/host");
            if(path == NULL)
            {
                continue;
            }
            path[0]=0;
            do{
                path--;
            }while(path[0] != '/');
            path ++;
            c = strchr(path,':');
            if(c != NULL)
            {
                *c = 0;
            }
        }
        strcpy(md_array[counter].addr, path);
    }
    if(num_entries > -1)
    {
        free(eps);
    }

    // Add a marker to indicate end of the list
    md_array[counter].dev[0] = 0;

    return num_entries;
}

/**
 * Update the Slave Widgets
 */
void rd_updateState()
{
    int disk_idx;
    int fd;
    char contents[1024];
    char *path;
    ssize_t size;
    int num_entries;
    int counter;
    char filename[1024];

    int runExt = 0;

    if(alarm_expired(ext_update))
    {
        runExt = 1;
        ext_update = set_alarm(md_conf.ext_poll * 1000);
    }

    disk_idx = 0;
    // Create frame per device
    while(md_array[disk_idx].dev[0] != 0)
    {
        if(md_conf.rd_loc == 1)
        {
            snprintf(contents, 1024,"%s:%s",md_array[disk_idx].slot,md_array[disk_idx].addr);
        }
        else
        {
            snprintf(contents, 1024,"%s:%s",md_array[disk_idx].slot,md_array[disk_idx].dev);
        }
        lcd_setString(RD_DISK(disk_idx), RD_DISK_POS, contents);

        if(runExt)
        {
            if(rd_getExtHealth(disk_idx, contents, 1024))
            {
                // limit response
                contents[9]='\0';
                lcd_setString(RD_AGE(disk_idx), RD_AGE_POS, contents);
            }
            else
            {
                lcd_setString(RD_AGE(disk_idx), RD_AGE_POS, md_array[disk_idx].addr);
            }
        }

        // Get raid status
        snprintf(filename,1024,"/sys/block/%s/md/dev-%s/state",md_conf.dev_name,md_array[disk_idx].dev);
        fd = open(filename,0);
        if(fd < 0)
        {
            strcpy(contents,"removed");
            goto next;
        }
        size = read(fd, contents,1024);
        close(fd);
        if(size < 1)
        {
            strcpy(contents,"removed");
            goto next;
        }
        contents[size-1] = 0;
next:
        lcd_setString(RD_STATE(disk_idx), RD_STATE_POS, contents);
        disk_idx++;
    }
}

/**
 * Execute external health script
 */
int rd_getExtHealth(int device, char *buffer, int length)
{
    char cmd[1024];
    char cmdline[1024];
    ssize_t size;
    FILE *ps;
    int cmdResult;
    int dataCount;

    if(config_get_string("md_cmd", cmd) == 0)
    {
        return 0;
    }

    snprintf(cmdline, 1024,"%s %s", cmd, md_array[device].dev );
    DEBUG("Executing: %s\n",cmdline);

    ps = popen(cmdline,"r");
    if(ps == NULL)
    {
        WARN("Failed to execute: %s\n", cmdline);
        return 0;
    }

    dataCount = fread(buffer, 1, 1024, ps);

    while(dataCount > 0)
    {
        if(isspace(buffer[dataCount - 1]))
        {
            dataCount--;
        }
        else
        {
            break;
        }
    }

    buffer[dataCount]='\0';

    DEBUG("Read %d bytes:\n%s\n", dataCount, buffer);

    cmdResult = pclose(ps);
    DEBUG("Command finished with result: %d\n", cmdResult);

    if((cmdResult == 0) && (dataCount > 0))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}
