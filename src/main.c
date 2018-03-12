/*
 *  Copyright (c) 2018 CETiTEC GmbH
 *
 * All rights reserved. All use of this software and documentation is
 * subject to the License Agreement located in the file LICENSE.
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>

#include "XO2_ECA/XO2_api.h"
#include "jedec.h"

void usage(const char *arg0)
{
	fprintf(stderr, "Usage: %s [-l] <i2c-bus> <i2c-addr> <bitstream.jed>\n", arg0);
	fprintf(stderr, "\t-l\tLoad new bitstream after flashing\n");
}

int main(int argc, char *argv[])
{
	XO2Handle_t xo2;
	XO2RegInfo_t xo2Info;
	int err;
	bool load_after_flash = false;
	int argpos = 0;

	if (argc < 4) {
		usage(argv[0]);
		return 1;
	}

	if (strcmp(argv[1], "-l") == 0) {
		load_after_flash = true;
		argpos = 1;
	}

	XO2_JEDEC_t *jedec = jedec_parse(fopen(argv[argpos+3], "r"));
	if (!jedec) {
		fprintf(stderr, "jedec_parse failed\n");
		return 1;
	}

	XO2ECA_apiJEDECinfo(NULL, jedec);

	char *tmp, tmparr[32];
	long i2cbus = strtol(argv[argpos+1], &tmp, 0);
	if (*tmp != '\0' || i2cbus < 0) {
		fprintf(stderr, "Invalid i2c bus\n");
		usage(argv[0]);
		return 1;
	}

	snprintf(tmparr, sizeof(tmparr), "/dev/i2c-%ld", i2cbus);
	xo2.i2cfd = open("/dev/i2c-1", O_RDWR);
	if (xo2.i2cfd < 0) {
		fprintf(stderr, "open %s failed: %s\n", tmparr, strerror(errno));
		return 1;
	}

	xo2.addr = strtol(argv[argpos+2], &tmp, 0);
	if (*tmp != '\0' || xo2.addr < 0) {
		fprintf(stderr, "Invalid i2c addr\n");
		usage(argv[0]);
		return 1;
	}

	xo2.cfgEn = false;
	xo2.devType = MachXO2_640;

	err = XO2ECA_apiGetHdwInfo(&xo2, &xo2Info);
	if (err != 0) {
		fprintf(stderr, "XO2ECAcmd_readDevID failed: %s\n", strerror(err));
		return 1;
	}

	printf("Device ID: %.8x UserCode: %.8x TraceID: %.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x\n",
		   xo2Info.devID, xo2Info.UserCode, xo2Info.TraceID[0], xo2Info.TraceID[1],
		   xo2Info.TraceID[2], xo2Info.TraceID[3], xo2Info.TraceID[4], xo2Info.TraceID[5],
		   xo2Info.TraceID[6], xo2Info.TraceID[7]);

	XO2ECA_apiProgram(&xo2, jedec, XO2ECA_ERASE_PROG_CFG |
					  (load_after_flash?XO2ECA_PROGRAM_TRANSPARENT:XO2ECA_PROGRAM_NOLOAD));
	return 0;
}
