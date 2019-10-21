/*
 * widget_ids.h
 *
 *  Created on: 26 Sep. 2019
 *      Author: brandon
 */

#ifndef SRC_WIDGET_IDS_H_
#define SRC_WIDGET_IDS_H_

#include "lcd_api.h"

// Define Screen IDs
#define RAID_SCR "RAID_SRC"


// Define Widget IDs
#define MD_DEV      _ID(RAID_SCR,"MD_DEV")
#define MD_DEV_POS  _POS(1,1)

#define MD_STATE    _ID(RAID_SCR,"MD_STAT")
#define MD_STATE_START 8
#define MD_STATE_POS  _POS(MD_STATE_START,1)

#define MD_PROG    _ID(RAID_SCR,"MD_PROG")
#define MD_PROG_START (MD_STATE_START + 8)
#define MD_PROG_POS  _POS(MD_PROG_START,1)


#define SLAVE_FRAME_ID "SLAVE_FRAME"
#define SLAVE_FRAME    _ID(RAID_SCR,SLAVE_FRAME_ID)

#define RD_FRAME(_a) _ID(RAID_SCR,_id(_a,"_FRAME"))
#define RD_DISK(_a) _ID(RAID_SCR,_id(_a,"_DEV"))
#define RD_STATE(_a) _ID(RAID_SCR,_id(_a,"_STATE"))
#define RD_AGE(_a) _ID(RAID_SCR,_id(_a,"_AGE"))

#define RD_DISK_POS  _POS(1,1)
#define RD_STATE_POS  _POS(1,2)
#define RD_AGE_POS  _POS(1,3)



#endif /* SRC_WIDGET_IDS_H_ */
