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
#include <unistd.h>
#include <linux/i2c-dev.h>

#include "XO2_ECA/XO2_api.h"
#include "jedec.h"

void usage(const char *arg0)
{
	fprintf(stderr, "Usage: %s [-l] [-u] <i2c-bus> <i2c-addr> <bitstream.jed>\n", arg0);
	fprintf(stderr, "\t-l\tLoad new bitstream after flashing\n");
	fprintf(stderr, "\t-u\tFlash UFM sector\n");
}

int main(int argc, char *argv[])
{
	XO2Handle_t xo2;
	XO2RegInfo_t xo2Info;
	int err;
	bool load_after_flash = false, flash_ufm = false;
	int opt;

	while ((opt = getopt(argc, argv, "lu")) != -1) {
		switch (opt) {
		case 'l':
			load_after_flash = true;
			break;
		case 'u':
			flash_ufm = true;
			break;
		default:
			usage(argv[0]);
			return 1;
		}
	}

	if (argc - optind < 3) {
		usage(argv[0]);
		return 1;
	}

	XO2_JEDEC_t *jedec = jedec_parse(fopen(argv[optind+2], "r"));
	if (!jedec) {
		fprintf(stderr, "jedec_parse failed\n");
		return 1;
	}

	XO2ECA_apiJEDECinfo(NULL, jedec);

	char *tmp, tmparr[32];
	long i2cbus = strtol(argv[optind], &tmp, 0);
	if (*tmp != '\0' || i2cbus < 0) {
		fprintf(stderr, "Invalid i2c bus\n");
		usage(argv[0]);
		return 1;
	}

	snprintf(tmparr, sizeof(tmparr), "/dev/i2c-%ld", i2cbus);
	xo2.i2cfd = open(tmparr, O_RDWR);
	if (xo2.i2cfd < 0) {
		fprintf(stderr, "open %s failed: %s\n", tmparr, strerror(errno));
		return 1;
	}

	xo2.addr = strtol(argv[optind+1], &tmp, 0);
	if (*tmp != '\0' || xo2.addr < 0) {
		fprintf(stderr, "Invalid i2c addr\n");
		usage(argv[0]);
		return 1;
	}

	xo2.cfgEn = false;
	xo2.devType = MachXO2_640;

	err = XO2ECA_apiGetHdwInfo(&xo2, &xo2Info);
	if (err != OK) {
		fprintf(stderr, "XO2ECAcmd_readDevID failed: %s\n", strerror(err));
		return 1;
	}

	printf("Device ID: %.8x UserCode: %.8x TraceID: %.2x%.2x%.2x%.2x%.2x%.2x%.2x%.2x\n",
		   xo2Info.devID, xo2Info.UserCode, xo2Info.TraceID[0], xo2Info.TraceID[1],
		   xo2Info.TraceID[2], xo2Info.TraceID[3], xo2Info.TraceID[4], xo2Info.TraceID[5],
		   xo2Info.TraceID[6], xo2Info.TraceID[7]);

	err = XO2ECA_apiProgram(&xo2, jedec, XO2ECA_ERASE_PROG_CFG |
							(flash_ufm?XO2ECA_ERASE_PROG_UFM:0) |
							(load_after_flash?XO2ECA_PROGRAM_TRANSPARENT:XO2ECA_PROGRAM_NOLOAD));
	if (err != OK) {
		fprintf(stderr, "XO2ECAcmd_apiProgram failed: %d\n", err);
		return 1;
	}

	return 0;
}
