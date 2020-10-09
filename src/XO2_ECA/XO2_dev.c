/*
 *  COPYRIGHT (c) 2012 by Lattice Semiconductor Corporation
 *  Copyright (c) 2018-2020 CETITEC GmbH
 *
 * All rights reserved. All use of this software and documentation is
 * subject to the License Agreement located in the file LICENSE.
 */

/** @file XO2_dev.c
 * Database of needed parameters per XO2 device type for use in programming
 * the Configuration and UFM sectors.
 */
#define XO2DEVLIST_DECLARATION

#include "XO2_dev.h"


/**
 * Database of XO2 device parameters needed for accessing, erasing and programming
 * the Configuration and UFM sectors in different sized XO2 parts.
 */
const XO2DevInfo_t XO2DevList[LATTICE_XO2_NUM_DEVS] =
{
	// Name            Cfg pgs
	//                  |     UFM pgs
	//                  |       |    Cfg erase time
	//                  |       |     |    UFM erase time
	//                  |       |     |     |      Trefresh time
	//                  |       |     |     |       |          Device ID Code (HE/ZE)
	//                  |       |     |     |       |          |           Device ID Code (HC)
	{"MachXO2-256",    575,     0,   700,    0,     1, 0x012B0043, 0x012B8043},
	{"MachXO2-640",   1152,   191,  1100,  600,     1, 0x012B1043, 0x012B9043},
	{"MachXO2-640U",  2175,   512,  1400,  700,     1, 0x012B2043, 0x012BA043},
	{"MachXO2-1200",  2175,   512,  1400,  700,     1, 0x012B2043, 0x012BA043},
	{"MachXO2-1200U", 3200,   639,  1900,  900,     2, 0x012B3043, 0x012BB043},
	{"MachXO2-2000",  3200,   639,  1900,  900,     2, 0x012B3043, 0x012BB043},
	{"MachXO2-2000U", 5760,   767,  3100, 1000,     3, 0x012B4043, 0x012BC043},
	{"MachXO2-4000",  5760,   767,  3100, 1000,     3, 0x012B4043, 0x012BC043},
	{"MachXO2-7000",  9216,  2046,  4800, 1600,     4, 0x012B5043, 0x012BD043}
};




