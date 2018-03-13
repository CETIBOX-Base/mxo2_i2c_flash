/*
 *  COPYRIGHT (c) 2012 by Lattice Semiconductor Corporation
 *  Copyright (c) 2018 CETiTEC GmbH
 *
 * All rights reserved. All use of this software and documentation is
 * subject to the License Agreement located in the file LICENSE.
 */

/** @file XO2_cmds.c
 * The commands provided here allow low-level access to the operations of the
 * XO2 configuration logic. The functions are C implementations of the published
 * XO2 configuration commands and their operands and usage.  The commands
 * usually need to be used in groups.  For example, some commands require that
 * the device be in configuration mode in order to issue a command. The XO2
 * API's provide routines that bundle together the necessary sequence of
 * commands to implement the higher-level function.  @see XO2_api.c
 *
 * <p>
 * All functions take a pointer to an XO2 device structure.  This structure
 * provides the list of access functions and other information, such as I2C
 * slave address.  The XO2 command functions do not assume a particular access
 * method (i.e. they do not care if connected to XO2 by I2C or SPI or other
 * method).
 */

#include <stdio.h>
#include <stropts.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>

#include "XO2_cmds.h"

static int XO2_read(XO2Handle_t *pXO2, uint8_t reg, uint32_t args,
					unsigned len, uint8_t *data)
{
	unsigned char cmd[4];
	struct i2c_rdwr_ioctl_data i2c_req;
	struct i2c_msg i2c_msgs[2];
	int status;

	cmd[0] = reg;
	cmd[1] = args>>16; // arg0
	cmd[2] = args>>8;  // arg1
	cmd[3] = args;     // arg2
	i2c_msgs[0].addr = pXO2->addr;
	i2c_msgs[0].flags = 0;
	i2c_msgs[0].len = 4;
	i2c_msgs[0].buf = cmd;

	i2c_msgs[1].addr = pXO2->addr;
	i2c_msgs[1].flags = I2C_M_RD ;
	i2c_msgs[1].len = len;
	i2c_msgs[1].buf = data;

	i2c_req.msgs = i2c_msgs;
	i2c_req.nmsgs = 2;

	status = ioctl(pXO2->i2cfd, I2C_RDWR, &i2c_req);

	if (status != -1) {
		return OK;
	} else {
		return ERROR;
	}
}

static int XO2_write(XO2Handle_t *pXO2, uint8_t reg, uint32_t args,
					 unsigned len, uint8_t *data)
{
	uint8_t buf[32];
	struct i2c_rdwr_ioctl_data i2c_req;
	struct i2c_msg i2c_msgs[1];
	int status;

	if ((data == NULL && len != 0) || len > 28) {
		return ERROR;
	}

	buf[0] = reg;
	buf[1] = args>>16; // arg0
	buf[2] = args>>8;  // arg1
	buf[3] = args;     // arg2
	if (len > 0) {
		memcpy(buf+4, data, len);
	}
	i2c_msgs[0].addr = pXO2->addr;
	i2c_msgs[0].flags = 0;
	i2c_msgs[0].len = 4+len;
	i2c_msgs[0].buf = buf;

	i2c_req.msgs = i2c_msgs;
	i2c_req.nmsgs = 1;

	status = ioctl(pXO2->i2cfd, I2C_RDWR, &i2c_req);

	if (status != -1) {
		return OK;
	} else {
		return ERROR;
	}
}
/**
 * Read the 4 byte Device ID from the XO2 Configuration logic block.
 * This function assembles the command sequence that allows reading the XO2 Device ID
 * from the configuration logic.  The command is first written to the XO2, then a read
 * of 4 bytes is perfromed to return the value.
 *
 * @param pXO2 pointer to the XO2 device to access
 * @param pVal pointer to the 32 bit integer to return the ID value in
 * @return 0 if successful, error code on error
 *
 */
int XO2ECAcmd_readDevID(XO2Handle_t *pXO2, unsigned int *pVal)
{
	unsigned char data[4];
	int status;

#ifdef DEBUG_ECA
	printf("XO2ECAcmd_readDevID()\n");
#endif

	status = XO2_read(pXO2, 0xE0, 0, 4, data);

#ifdef DEBUG_ECA
	printf("\tstatus=%d  data=%x %x %x %x\n", status, data[0], data[1], data[2], data[3]);
#endif

	if (status == OK) {
		*pVal = (data[0]<<24) | (data[1]<<16) | (data[2]<<8) | data[3];
	}
	return status;
}



/**
 * Read the 4 byte USERCODE from the XO2 Configuration logic block.
 * This function assembles the command sequence that allows reading the XO2 USERCODE
 * value from the configuration Flash sector.  The command is first written to the XO2, then a read
 * of 4 bytes is perfromed to return the value.
 *
 *
 * @param pXO2 pointer to the XO2 device to access
 * @param pVal pointer to the 32 bit integer to return the USERCODE value in
 * @return OK if successful, ERROR if failed to read
 * @note
 *
 */
int XO2ECAcmd_readUserCode(XO2Handle_t *pXO2, unsigned int *pVal)
{
	unsigned char data[4];
	int status;

#ifdef DEBUG_ECA
	printf("XO2ECAcmd_readUserCode()\n");
#endif

	status = XO2_read(pXO2, 0xC0, 0, 4, data);

#ifdef DEBUG_ECA
	printf("\tstatus=%d  data=%x %x %x %x\n", status, data[0], data[1], data[2], data[3]);
#endif
	if (status == OK)
	{
		*pVal = (data[0]<<24) | (data[1]<<16) | (data[2]<<8) | data[3];
		return OK;
	} else {
		return ERROR;
	}
}




/**
 * Set the 4 byte USERCODE in the XO2 Configuration logic block.
 * This function assembles the command sequence that allows programming the XO2 USERCODE
 * value.  The command is written to the XO2, along with 4 bytes to program into the USERCODE
 * area of flash.
 *
 * @param pXO2 pointer to the XO2 device to access
 * @param val new USERCODE value
 * @return OK if successful, ERROR if failed to write
 *
 * @note This command is only useful if the USERCODE contents is previously all 0's.
 * The USERCODE is cleared when the Cfg sector is erased.  So there is no way to
 * individually clear just the USERCODE and reprogram with a new value using this command.
 * Its usefulness is questionable, seeing as the USERCODE is set when programming the
 * Cfg sector anyway.
 */
int XO2ECAcmd_setUserCode(XO2Handle_t *pXO2, unsigned int val)
{
	unsigned char data[4];
	int status;

#ifdef DEBUG_ECA
	printf("XO2ECAcmd_setUserCode()\n");
#endif

	if (pXO2->cfgEn == false)
	{
		return(ERR_XO2_NOT_IN_CFG_MODE);
	}

	data[0] = (unsigned char)(val>>24);
	data[1] = (unsigned char)(val>>16);
	data[2] = (unsigned char)(val>>8);
	data[3] = (unsigned char)(val);

	status = XO2_write(pXO2, 0xC2, 0, 4, data);

#ifdef DEBUG_ECA
	printf("\tstatus=%d\n", status);
#endif
	if (status == OK)
	{
		return(OK);
	}
	else
	{
		return(ERROR);
	}
}


/**
 * Read the 8 byte (64 bit) TraceID from the XO2 Feature Row.
 * This function assembles the command sequence that allows reading the XO2 TraceID
 * value from the Feature Row Flash sector.  The command is first written to the XO2, then a read
 * of 8 bytes is perfromed to return the value.
 * The TraceID is set in the Global Settings of Spreadsheet view.
 * The first byte read back (pVal[0]) can be set by the user in Spreadsheet view.
 * The remaining 7 bytes are unique for each silicon die.
 *
 * @param pXO2 pointer to the XO2 device to access
 * @param pVal pointer to the 8 byte array to return the TraceID value in
 * @return OK if successful, ERROR if failed to read
 *
 */
int XO2ECAcmd_readTraceID(XO2Handle_t *pXO2, unsigned char *pVal)
{
	unsigned char data[8];
	int status;
	int i;

#ifdef DEBUG_ECA
	printf("XO2ECAcmd_readTraceID()\n");
#endif

	status = XO2_read(pXO2, 0x19, 0, 8, data);

#ifdef DEBUG_ECA
	printf("\tstatus=%d  data=", status);
	for (i = 0; i < 7; i++)
		printf("  %x", data[i]);
	printf("  %x\n", data[7]);
#endif
	if (status == OK)
	{
		for (i = 0; i < 8; i++)
			pVal[i] = data[i];
		return(OK);
	}
	else
	{
		return(ERROR);
	}
}



/**
 * Enable access to Configuration Logic in Transparent mode or Offline mode.
 * This function issues one of the Enable Configuration Interface commands depending on
 * the value of mode.  Transparent mode allows the XO2 to continue operating in user mode
 * while access to the config logic is performed.  Offline mode halts all user logic, and
 * tri-states I/Os, while access is occurring.
 *
 * @param pXO2 pointer to the XO2 device to access
 * @param mode specify TRANSPARENT_MODE or OFFLINE_MODE
 * @return OK if successful, ERROR if failed to read
 *
 */
int XO2ECAcmd_openCfgIF(XO2Handle_t *pXO2, XO2CfgMode_t mode)
{
	unsigned char cmd;
	int status;


#ifdef DEBUG_ECA
	if (mode == TRANSPARENT_MODE)
		printf("XO2ECAcmd_openCfgIF(Transparent_MODE)\n");
	else
		printf("XO2ECAcmd_openCfgIF(Offline_MODE)\n");
#endif

	if (mode == TRANSPARENT_MODE) {
		cmd = 0x74;
	} else if (mode == OFFLINE_MODE) {
		cmd = 0xC6;
	} else {
		return ERROR;
	}

	status = XO2_write(pXO2, cmd, 0x080000, 0, NULL);

	// Wait till not busy - we have entered Config mode
	if (status == OK)
		status = XO2ECAcmd_waitStatusBusy(pXO2);

#ifdef DEBUG_ECA
		printf("\tstatus=%d\n", status);
#endif
	if (status == OK)
	{
		pXO2->cfgEn = true;
		return(OK);
	}
	else
	{
		pXO2->cfgEn = false;
		return(ERROR);
	}
}


/**
 * Disable access to Configuration Logic Interface.
 * This function issues the Disable Configuration Interface command and
 * registers that the interface is no longer available for certain commands
 * that require it to be enabled.
 *
 * @param pXO2 pointer to the XO2 device to access
 * @return OK if successful, ERROR if failed to read
 *
 */
int XO2ECAcmd_closeCfgIF(XO2Handle_t *pXO2)
{
	int status;

#ifdef DEBUG_ECA
	printf("XO2ECAcmd_closeCfgIF()\n");
#endif

	status = XO2_write(pXO2, 0x26, 0, 0, NULL);

#ifdef DEBUG_ECA
	printf("\tstatus=%d\n", status);
#endif
	if (status == OK)
	{
		pXO2->cfgEn = false;
		return(OK);
	}
	else
	{
		return(ERROR);
	}
}


/**
 * Issue the Refresh command that updates the SRAM from Flash and boots the XO2.
 *
 * @param pXO2 pointer to the XO2 device to access
 * @return OK if successful, ERROR code if failed to write
 *
 */
int XO2ECAcmd_Refresh(XO2Handle_t *pXO2)
{
	int status;
	unsigned int sr;

#ifdef DEBUG_ECA
	printf("XO2ECAcmd_Refresh()\n");
#endif
	status = XO2_write(pXO2, 0x79, 0, 0, NULL);

	usleep(XO2DevList[pXO2->devType].Trefresh*1000);

	if (XO2ECAcmd_readStatusReg(pXO2, &sr) != OK)
		return(ERROR);

#ifdef DEBUG_ECA
	printf("\tstatus=%d   sr=%x\n", status, sr);
#endif
	// Verify that only DONE bit is definitely set and not FAIL or BUSY or ISC_ENABLED
	if ((sr & 0x3f00) == 0x0100)
	{
		pXO2->cfgEn = false;
		return(OK);
	}
	else
	{
		return(ERROR);
	}
}


/**
 * Issue the Done command that updates the Program DONE bit.
 * Typically used after programming the Cfg Flash and before
 * closing access to the configuration interface.
 *
 * @param pXO2 pointer to the XO2 device to access
 * @return OK if successful, ERROR code if failed to write
 *
 */
int XO2ECAcmd_setDone(XO2Handle_t *pXO2)
{
	int status;
	unsigned int sr;

#ifdef DEBUG_ECA
	printf("XO2ECAcmd_setDone()\n");
#endif

	if (pXO2->cfgEn == false)
	{
		return(ERR_XO2_NOT_IN_CFG_MODE);
	}

	status = XO2_write(pXO2, 0x5E, 0, 0, NULL);

// TODO: This delay time may be excessive

	if (status == OK)
	{
		// Wait 10 msec for Done
		usleep(10000);
	}
	else
	{
		return(ERROR);
	}

	if (XO2ECAcmd_readStatusReg(pXO2, &sr) != OK)
		return(ERROR);

	// Verify that DONE bit is definitely set and not FAIL or BUSY
	if ((sr & 0x3100) == 0x0100)
	{
		return(OK);
	}
	else
	{
		return(ERROR);
	}
}





/**
 * Read the 4 byte Status Register from the XO2 Configuration logic block.
 * This function assembles the command sequence that allows reading the XO2 Status Register.
 * The command is first written to the XO2, then a read of 4 bytes is perfromed to return the value.
 *
 * @param pXO2 pointer to the XO2 device to access
 * @param pVal pointer to the 32 bit integer to return the Status Register value in.
 * @return OK if successful, ERROR if failed to read.
 *
 */
int XO2ECAcmd_readStatusReg(XO2Handle_t *pXO2, unsigned int *pVal)
{
	unsigned char data[4];
	int status;

#ifdef DEBUG_ECA
	printf("XO2ECAcmd_readStatusReg()\n");
#endif

	status = XO2_read(pXO2, 0x3C, 0, 4, data);

#ifdef DEBUG_ECA
	printf("\tstatus=%d  data=%x %x %x %x\n", status, data[0], data[1], data[2], data[3]);
#endif
	if (status == OK)
	{
		*pVal = (data[0]<<24) | (data[1]<<16) | (data[2]<<8) | data[3];
		return(OK);
	}
	else
	{
		return(ERROR);
	}
}



/**
 * Wait for the Status register to report no longer busy.
 * Read the 4 byte Status Register from the XO2 Configuration logic block and check bit 12
 * to see if BUSY.  Also check bit 13 for FAIL indication.  Return error if an error
 * condition is detected.  Also return if exceed polling loop timeout.
 * This function assembles the command sequence that allows reading the XO2 Status Register.
 * The command is first written to the XO2, then a read of 4 bytes is perfromed to return the value.
 *
 * @param pXO2 pointer to the XO2 device to access
 * @return OK if no longer Busy and can proceed, ERROR if failed to read.
 *
 */
int XO2ECAcmd_waitStatusBusy(XO2Handle_t *pXO2)
{
	unsigned char data[4];
	int status;
	int loop;

#ifdef DEBUG_ECA
	printf("XO2ECAcmd_waitStatusBusy()\n");
#endif

	loop = XO2ECA_CMD_LOOP_TIMEOUT;
	do
	{
		status = XO2_read(pXO2, 0x3C, 0, 4, data);

		if (status != OK)
			return(ERROR);

		if (data[2] & 0x20)  // FAIL bit set
			return(ERROR);

		if (data[2] & 0x10)
		{
			// Still busy so wait another msec and loop again, if not timed out
			--loop;
			usleep(1000); // delay 1 msec
		}

	} while(loop && (data[2] & 0x10));

	if (loop)
		return(OK);
	else
		return(ERROR);   // timed out waiting for BUSY to clear
}



/**
 * Read the Busy Flag bit from the XO2 Configuration logic block.
 * This function assembles the command sequence that allows reading the XO2 Busy Flag.
 * The command is first written to the XO2, then a read of 1 byte is perfromed to return the value.
 *
 * @param pXO2 pointer to the XO2 device to access
 * @param pVal pointer to the 8 bit integer to return the Busy Flag bit value in.
 * @return OK if successful, ERROR if failed to read.
 *
 */
int XO2ECAcmd_readBusyFlag(XO2Handle_t *pXO2, unsigned char *pVal)
{
	unsigned char data;
	int status;

#ifdef DEBUG_ECA
	printf("XO2ECAcmd_readBusyFlag()\n");
#endif

	status = XO2_read(pXO2, 0xF0, 0, 1, &data);

#ifdef DEBUG_ECA
	printf("\tstatus=%d  data=%x\n", status, data);
#endif
	if (status == OK)
	{
		*pVal = data;
		return(OK);
	}
	else
	{
		return(ERROR);
	}
}




/**
 * Wait for the Busy Flag to be cleared.
 * Read the 1 byte from the Busy Flag resgister and check if not 0
 * to see if still BUSY.  Return error if an error
 * condition is detected.  Also return if exceed polling loop timeout.
 * This function assembles the command sequence that allows reading the XO2 Busy Flag Register.
 * The command is first written to the XO2, then a read of 1 byte is perfromed to obtain the value.
 *
 * @param pXO2 pointer to the XO2 device to access
 * @return OK if no longer Busy and can proceed, ERROR if failed to read.
 *
 */
int XO2ECAcmd_waitBusyFlag(XO2Handle_t *pXO2)
{
	unsigned char data[1];
	int status;
	int loop;

#ifdef DEBUG_ECA
	printf("XO2ECAcmd_waitBusyFlag()\n");
#endif

	loop = XO2ECA_CMD_LOOP_TIMEOUT;
	do
	{
		status = XO2_read(pXO2, 0xF0, 0, 1, data);

		if (status != OK)
			return(ERROR);

		if (data[0])
		{
			// Still busy so wait another msec
			--loop;
			usleep(1000);   // delay 1 msec
		}

	} while(loop && data[0]);

	if (loop)
		return(OK);
	else
		return(ERROR);   // timed out waiting for BUSY to clear
}


/**
 * Send the Bypass command.
 * This function assembles the command sequence that allows writing the XO2 Bypass command.
 * This command is typically called after closing the Configuration Interface.
 *
 * @param pXO2 pointer to the XO2 device to access
 * @return OK if successful, ERROR if failed to read.
 *
 */
int XO2ECAcmd_Bypass(XO2Handle_t *pXO2)
{
	unsigned char cmd[1];
	int status;
	struct i2c_rdwr_ioctl_data i2c_req;
	struct i2c_msg i2c_msgs[1];

#ifdef DEBUG_ECA
	printf("XO2ECAcmd_Bypass()\n");
#endif

	cmd[0] = 0xFF;  // Bypass opcode - supposedly does not have arguements, just command byte
//	cmd[1] = 0x00;  // arg0
//	cmd[2] = 0x00;  // arg1
//	cmd[3] = 0x00;  // arg2
	i2c_msgs[0].addr = pXO2->addr;
	i2c_msgs[0].flags = 0;
	i2c_msgs[0].len = 1;
	i2c_msgs[0].buf = cmd;

	i2c_req.msgs = i2c_msgs;
	i2c_req.nmsgs = 1;

	status = ioctl(pXO2->i2cfd, I2C_RDWR, &i2c_req);

	if (status != -1) {
		status = OK;
	} else {
		status = ERROR;
	}


#ifdef DEBUG_ECA
	printf("\tstatus=%d\n", status);
#endif
	if (status == OK)
	{
		return(OK);
	}
	else
	{
		return(ERROR);
	}
}


/**
 * Set the current Page Address in either Cfg Flash or UFM.
 * The specific page is set for the next read/write operation.
 * This is probably only useful for the UFM since skipping around in the configuration
 * sector is very unlikely.
 *
 * @param pXO2 pointer to the XO2 device to access
 * @param mode which address to update: CFG_SECTOR or UFM_SECTOR
 * @param pageNum the page number to set address pointer to
 * @return OK if successful, ERROR if failed to set address.
 *
 */
int XO2ECAcmd_SetPage(XO2Handle_t *pXO2, XO2SectorMode_t mode, unsigned int pageNum)
{
	unsigned char cmd[4];
	int status;

#ifdef DEBUG_ECA
	printf("XO2ECAcmd_SetPage()\n");
#endif

	if (pXO2->cfgEn == false)
	{
#ifdef DEBUG_ECA
		printf("\tERR_XO2_NOT_IN_CFG_MODE\n");
#endif
		return(ERR_XO2_NOT_IN_CFG_MODE);
	}

	if ((mode == UFM_SECTOR) && (pageNum > XO2DevList[pXO2->devType].UFMpages))
	{
#ifdef DEBUG_ECA
		printf("\tERR_XO2_EXCEEDS_UFM_SIZE\n");
#endif
		return(ERR_XO2_EXCEEDS_UFM_SIZE );
	}

	if ((mode == CFG_SECTOR) && (pageNum > XO2DevList[pXO2->devType].Cfgpages))
	{
#ifdef DEBUG_ECA
		printf("\tERR_XO2_EXCEEDS_CFG_SIZE\n");
#endif
		return(ERR_XO2_EXCEEDS_CFG_SIZE );
	}

	if (mode == CFG_SECTOR)
		cmd[0] = 0x00;  // page[0] = 0=Config, 1=UFM
	else
		cmd[0] = 0x40;  // page[0] = 0=Config, 1=UFM
	cmd[1] = 0x00;  // page[1]
	cmd[2] = (unsigned char)(pageNum>>8);  // page[2] = page number MSB
	cmd[3] = (unsigned char)pageNum;       // page[3] = page number LSB

	status = XO2_write(pXO2, 0xB4, 0, 4, cmd);

#ifdef DEBUG_ECA
	printf("\tstatus=%d\n", status);
#endif
	if (status == OK)
	{
		return(OK);
	}
	else
	{
		return(ERROR);
	}
}


/**
 * Erase any/all entire sectors of the XO2 Flash memory.
 * Erase sectors based on the bitmap of parameter mode passed in.
 * <ul>
 * <li> 8 = UFM
 * <li> 4 = CFG
 * <li> 2 = Feature Row
 * <li> 1 = SRAM
 * </ul>
 *
 * @param pXO2 pointer to the XO2 device to access
 * @param mode bit map of what sector contents to erase
 * @return OK if successful, ERROR if failed.
 *
 * @note Erases bits to a value of 0.  Any bit that is a 0 can then be programmed to a 1.
 *
 */
int XO2ECAcmd_EraseFlash(XO2Handle_t *pXO2, unsigned char mode)
{
	int status;

#ifdef DEBUG_ECA
	printf("XO2ECAcmd_EraseFlash()\n");
#endif

	if (pXO2->cfgEn == false)
	{
#ifdef DEBUG_ECA
		printf("\tERR_XO2_NOT_IN_CFG_MODE\n");
#endif
		return(ERR_XO2_NOT_IN_CFG_MODE);
	}

	mode = mode & 0x0f;

	status = XO2_write(pXO2, 0x0E, mode<<16, 0, NULL);

	if (status == OK)
	{
		// Must wait an amount of time, based on device size, for largest flash sector to erase.
		if (mode & XO2ECA_CMD_ERASE_CFG)
			usleep(XO2DevList[pXO2->devType].CfgErase*1000);  // longest
		else if (mode & XO2ECA_CMD_ERASE_UFM)
			usleep(XO2DevList[pXO2->devType].UFMErase*1000);  // medium
		else
			usleep(50000);	// SRAM & Feature Row = shortest

		status = XO2ECAcmd_waitStatusBusy(pXO2);
	}

#ifdef DEBUG_ECA
	printf("\tstatus=%d\n", status);
#endif
	if (status == OK)
	{
		return(OK);
	}
	else
	{
		return(ERROR);
	}
}


//==============================================================================
//==============================================================================
//==============================================================================
//                          C O N F I G     F L A S H      C O M M A N D S
//==============================================================================
//==============================================================================
//==============================================================================
/**
 * Reset the Address Regsiter to point to the first Config Flash page (sector 0, page 0).
 * @param pXO2 pointer to the XO2 device to operate on
 *
 * @return OK is address reset.  Error if failed.
 *
 */
int XO2ECAcmd_CfgResetAddr(XO2Handle_t *pXO2)
{
	int status;

#ifdef DEBUG_ECA
	printf("XO2ECAcmd_CfgResetAddr()\n");
#endif

	if (pXO2->cfgEn == false)
	{
#ifdef DEBUG_ECA
		printf("\tERR_XO2_NOT_IN_CFG_MODE\n");
#endif
		return(ERR_XO2_NOT_IN_CFG_MODE);
	}

	status = XO2_write(pXO2, 0x46, 0, 0, NULL);

#ifdef DEBUG_ECA
	printf("\tstatus=%d\n", status);
#endif
	if (status == OK)
	{
		return(OK);
	}
	else
	{
		return(ERROR);
	}

}


/**
 * Read the next page (16 bytes) from the Config Flash memory.
 * Page address can be set using SetAddress command.
 *
 * @param pXO2 pointer to the XO2 device to access
 * @param pBuf pointer to the 16 byte array to return the Config page bytes in.
 * @return OK if successful, ERROR if failed to read.
 *
 * @note There is no advantage to reading multiple pages since the interface
 * is most likely so slow.  The I2C only runs at 100kHz.
 *
 * @note The number of pages read is not comparted against the total pages in the
 * device.  Reading too far may have unexpected results.
 */
int XO2ECAcmd_CfgReadPage(XO2Handle_t *pXO2, unsigned char *pBuf)
{
	unsigned char data[XO2_FLASH_PAGE_SIZE];
	int status;
	int i;

#ifdef DEBUG_ECA
	printf("XO2ECAcmd_CfgReadPage()\n");
#endif

	if (pXO2->cfgEn == false)
	{
#ifdef DEBUG_ECA
		printf("\tERR_XO2_NOT_IN_CFG_MODE\n");
#endif
		return(ERR_XO2_NOT_IN_CFG_MODE);
	}

	status = XO2_read(pXO2, 0x73, 0x000001, XO2_FLASH_PAGE_SIZE, data);

#ifdef DEBUG_ECA
	printf("\tstatus=%d  data=", status);
	for (i = 0; i < XO2_FLASH_PAGE_SIZE-1; i++)
		printf("  %x", data[i]);
	printf("  %x\n", data[XO2_FLASH_PAGE_SIZE-1]);
#endif
	if (status == OK)
	{
		for (i = 0; i < XO2_FLASH_PAGE_SIZE; i++)
			pBuf[i] = data[i];
		return(OK);
	}
	else
	{
		return(ERROR);
	}
}


/**
 * Write a page (16 bytes) into the current UFM memory page.
 * Page address can be set using SetAddress command.
 * Page address advances to next page after programming is completed.
 *
 * @param pXO2 pointer to the XO2 device to access
 * @param pBuf pointer to the 16 byte array to write into the UFM page.
 * @return OK if successful, ERROR if failed to read.
 *
 * @note Programming must be done on a page basis.  Pages must be erased to 0's first.
 * @see XO2ECAcmd_UFMErase
 *
 * @note The number of pages written is not compared against the total pages in the
 * device.  Writing too far may have unexpected results.
 */
int XO2ECAcmd_CfgWritePage(XO2Handle_t *pXO2, unsigned char *pBuf)
{
	int status;

#ifdef DEBUG_ECA
	printf("XO2ECAcmd_CfgWritePage()\n");
#endif

	if (pXO2->cfgEn == false)
	{
#ifdef DEBUG_ECA
		printf("\tERR_XO2_NOT_IN_CFG_MODE\n");
#endif
		return(ERR_XO2_NOT_IN_CFG_MODE);
	}


	status = XO2_write(pXO2, 0x70, 0x000001, 16, pBuf);

	if (status == OK)
	{
		// Must wait 200 usec for a page to program.  This is a constant for all
		// devices (see XO2 datasheet)
		usleep(200);
		status = XO2ECAcmd_waitStatusBusy(pXO2);
	}

#ifdef DEBUG_ECA
	printf("\tstatus=%d\n", status);
#endif
	if (status == OK)
	{
		return(OK);
	}
	else
	{
		return(ERROR);
	}
}

/**
 * Erase the entire sector of the Configuration Flash memory.
 * This is a convience function to erase all Config contents to 0.  You can not erase on a page basis.
 * The entire sector is cleared.
 * The erase takes up to a few seconds.  The time to wait is device specific.
 * It is important that the correct device is selected in the pXO2 structure.
 *
 * @param pXO2 pointer to the XO2 device to access
 * @return OK if successful, ERROR if failed.
 *
 * @note Erases bits to a value of 0.  Any bit that is a 0 can then be programmed to a 1.
 *
 */
int XO2ECAcmd_CfgErase(XO2Handle_t *pXO2)
{

#ifdef DEBUG_ECA
	printf("XO2ECAcmd_CfgErase()\n");
#endif

	return(XO2ECAcmd_EraseFlash(pXO2, XO2ECA_CMD_ERASE_CFG));
}

//==============================================================================
//==============================================================================
//==============================================================================
//                            U F M      C O M M A N D S
//==============================================================================
//==============================================================================
//==============================================================================
/**
 * Reset the Address Regsiter to point to the first UFM page (sector 1, page 0).
 * @param pXO2 pointer to the XO2 device to operate on
 *
 * @return OK if successful. ERROR if failed.
 *
 */
int XO2ECAcmd_UFMResetAddr(XO2Handle_t *pXO2)
{
	unsigned char cmd[4];
	int status;

#ifdef DEBUG_ECA
	printf("XO2ECAcmd_UFMResetAddr()\n");
#endif

	if (pXO2->cfgEn == false)
	{
#ifdef DEBUG_ECA
		printf("\tERR_XO2_NOT_IN_CFG_MODE\n");
#endif
		return(ERR_XO2_NOT_IN_CFG_MODE);
	}

	if (pXO2->devType == MachXO2_256)
	{
#ifdef DEBUG_ECA
		printf("\tERR_XO2_NO_UFM\n");
#endif
		return(ERR_XO2_NO_UFM);
	}

	status = XO2_write(pXO2, 0x47, 0, 0, NULL);

#ifdef DEBUG_ECA
	printf("\tstatus=%d\n", status);
#endif
	if (status == OK)
	{
		return(OK);
	}
	else
	{
		return(ERROR);
	}

}


/**
 * Read the next page (16 bytes) from the UFM memory.
 * Page address can be set using SetAddress command.
 *
 * @param pXO2 pointer to the XO2 device to access
 * @param pBuf pointer to the 16 byte array to return the UFM page bytes in.
 * @return OK if successful, ERROR if failed to read.
 *
 * @note There is no advantage to reading multiple pages since the interface
 * is most likely so slow.  The I2C only runs at 100kHz.
 *
 */
int XO2ECAcmd_UFMReadPage(XO2Handle_t *pXO2, unsigned char *pBuf)
{
	unsigned char data[16];
	int status;
	int i;

#ifdef DEBUG_ECA
	printf("XO2ECAcmd_UFMReadPage()\n");
#endif

	if (pXO2->cfgEn == false)
	{
#ifdef DEBUG_ECA
		printf("\tERR_XO2_NOT_IN_CFG_MODE\n");
#endif
		return(ERR_XO2_NOT_IN_CFG_MODE);
	}

	if (pXO2->devType == MachXO2_256)
	{
#ifdef DEBUG_ECA
		printf("\tERR_XO2_NO_UFM\n");
#endif
		return(ERR_XO2_NO_UFM);
	}

	status = XO2_read(pXO2, 0xCA, 0x000001, 16, data);

#ifdef DEBUG_ECA
	printf("\tstatus=%d  data=", status);
	for (i = 0; i < 15; i++)
		printf("  %x", data[i]);
	printf("  %x\n", data[15]);
#endif
	if (status == OK)
	{
		for (i = 0; i < 16; i++)
			pBuf[i] = data[i];
		return(OK);
	}
	else
	{
		return(ERROR);
	}
}

/**
 * Write a page (16 bytes) into the current UFM memory page.
 * Page address can be set using SetAddress command.
 * Page address advances to next page after programming is completed.
 *
 * @param pXO2 pointer to the XO2 device to access
 * @param pBuf pointer to the 16 byte array to write into the UFM page.
 * @return OK if successful, ERROR if failed to read.
 *
 * @note Programming must be done on a page basis.  Pages must be erased to 0's first.
 * @see XO2ECAcmd_UFMErase
 *
 */
int XO2ECAcmd_UFMWritePage(XO2Handle_t *pXO2, unsigned char *pBuf)
{
	int status;

#ifdef DEBUG_ECA
	printf("XO2ECAcmd_UFMWritePage()_1\n");
#endif

	if (pXO2->cfgEn == false)
	{
#ifdef DEBUG_ECA
		printf("\tERR_XO2_NOT_IN_CFG_MODE\n");
#endif
		return(ERR_XO2_NOT_IN_CFG_MODE);
	}

	if (pXO2->devType == MachXO2_256)
	{
#ifdef DEBUG_ECA
		printf("\tERR_XO2_NO_UFM\n");
#endif
		return(ERR_XO2_NO_UFM);
	}

	status = XO2_write(pXO2, 0xC9, 0x000001, 16, pBuf);

	if (status == OK)
	{
		// Must wait 200 usec for a page to program.  This is a constant for all devices (see XO2 datasheet)
		usleep(200);
		status = XO2ECAcmd_waitStatusBusy(pXO2);
	}

#ifdef DEBUG_ECA
	printf("\tstatus=%d\r\n", status);
#endif
	if (status == OK)
	{
		return(OK);
	}
	else
	{
		return(ERROR);
	}
}

/**
 * Erase the entire sector of the UFM memory.
 * This is a convience function to erase all UFM contents to 0.  You can not erase on a page basis.
 * The entire sector is cleared.  Therefore save any data first, erase, then
 * reprogram, putting the saved data back along with any new data.
 * The erase takes up to a few hundered milliseconds.  The time to wait is device specific.
 * It is important that the correct device is selected in the pXO2 structure.
 *
 * @param pXO2 pointer to the XO2 device to access
 * @return OK if successful, ERROR if failed.
 *
 * @note Erases bits to a value of 0.  Any bit that is a 0 can then be programmed to a 1.
 * @note The routine does not poll for completion, but rather waits the maximum time
 * for an erase as specified in the data sheet.
 */
int XO2ECAcmd_UFMErase(XO2Handle_t *pXO2)
{

#ifdef DEBUG_ECA
	printf("XO2ECAcmd_UFMErase()\n");
#endif

	return(XO2ECAcmd_EraseFlash(pXO2, XO2ECA_CMD_ERASE_UFM));
}


//==============================================================================
//==============================================================================
//==============================================================================
//                            F E A T U R E    R O W     C O M M A N D S
//==============================================================================
//==============================================================================
//==============================================================================


/**
 * Erase the Feature Row contents.
 * This is a convience function to erase just the feature row bits to 0.
 * You must reprogram the Feature Row with FeatureWrite() or the
 * XO2 part may be in an unusable state.
 *
 * @param pXO2 pointer to the XO2 device to access
 * @return OK if successful, ERROR if failed.
 *
 * @note Erases bits to a value of 0.  Any bit that is a 0 can then be programmed to a 1.
 *
 */
int XO2ECAcmd_FeatureRowErase(XO2Handle_t *pXO2)
{

#ifdef DEBUG_ECA
	printf("XO2ECAcmd_FeatureErase()\n");
#endif

	return(XO2ECAcmd_EraseFlash(pXO2, XO2ECA_CMD_ERASE_FTROW));
}


/**
 * Set the Feature Row.
 * This function assembles the command sequence that allows programming the XO2 FEATURE
 * bits and the FEABITS in the Feature Row.  The 8 FEATURE bytes and 2 FEABITS bytes
 * must be properly formatted or possible
 * lock-up of the XO2 could occur.  Only the values obtained from properly parsing the JEDEC
 * file should be used.
 *
 * @param pXO2 pointer to the XO2 device to access
 * @param pFeature pointer to the Feature Row structure containing the encoded data to write
 * into the Feature and FEABITS fields in the Feature Row.
 * @return OK if successful, ERROR if failed to write
 *
 * @note The Feature Row must first be erased
 */
int XO2ECAcmd_FeatureRowWrite(XO2Handle_t *pXO2, XO2FeatureRow_t *pFeature)
{
	int status;

#ifdef DEBUG_ECA
	printf("XO2ECAcmd_FeatureWrite()\n");
#endif

	if (pXO2->cfgEn == false)
	{
		return(ERR_XO2_NOT_IN_CFG_MODE);
	}

	status = XO2_write(pXO2, 0xE4, 0, 8, pFeature->feature);

	if (status != OK)
		return(ERROR);

	// Must wait 200 usec for a page to program.  This is a constant for all
	// devices (see XO2 datasheet)
	usleep(200);

	status = XO2_write(pXO2, 0xF8, 0, 2, pFeature->feabits);

	if (status == OK)
	{
		// Must wait 200 usec for a page to program.  This is a constant for all
		// devices (see XO2 datasheet)
	    usleep(200);
		status = XO2ECAcmd_waitStatusBusy(pXO2);
	}

	if (status == OK)
	{
		return(OK);
	}
	else
	{
		return(ERROR);
	}
}



/**
 * Read the Feature Row contents.
 * This function assembles the command sequence that allows reading back the Feature Row
 * contents.  The FEATURE bytes and FEABITS fields are returned in a XO2 specific stucture.
 * Uses would be to verify successful XO2ECAcmd_FeatureWrite() programing. Or to read back
 * and preserve during an update.
 * @param pXO2 pointer to the XO2 device to access
 * @param pFeature pointer to the Feature Row structure that will be loaded with the
 * feature row contents.
 * @return OK if successful, ERROR if failed to read
 *
 */
int XO2ECAcmd_FeatureRowRead(XO2Handle_t *pXO2, XO2FeatureRow_t *pFeature)
{
	unsigned char data[8];

	int i, status;

#ifdef DEBUG_ECA
	printf("XO2ECAcmd_FeatureRead()\n");
#endif

	if (pXO2->cfgEn == false)
	{
		return(ERR_XO2_NOT_IN_CFG_MODE);
	}

	status = XO2_read(pXO2, 0xE7, 0, 8, data);

	if (status != OK)
		return(ERROR);

	for (i = 0; i < 8; i++)
		pFeature->feature[i] = data[i];

	status = XO2_read(pXO2, 0xFB, 0, 2, data);

	if (status != OK)
		return(ERROR);

	pFeature->feabits[0] = data[0];
	pFeature->feabits[1] = data[1];

	return(OK);
}





/**
 * Erase the SRAM, clearing the user design.
 * This is a convience function to erase just the SRAM.
 *
 * @param pXO2 pointer to the XO2 device to access
 * @return OK if successful, ERROR if failed.
 */
int XO2ECAcmd_SRAMErase(XO2Handle_t *pXO2)
{
#ifdef DEBUG_ECA
	printf("XO2ECAcmd_SRAMErase()\n");
#endif

	return(XO2ECAcmd_EraseFlash(pXO2, XO2ECA_CMD_ERASE_SRAM));
}

