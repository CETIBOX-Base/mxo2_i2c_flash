/*
 *  Copyright (c) 2018 CETiTEC GmbH
 *
 * All rights reserved. All use of this software and documentation is
 * subject to the License Agreement located in the file LICENSE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "XO2_ECA/XO2_dev.h"

typedef enum {
	S_START,
	S_FUSES,
	S_FEATROW,
} jedec_state_t;

typedef struct parser_state {
	jedec_state_t state;
	uint8_t *data, *data_pos;
	unsigned data_len;
	XO2_JEDEC_t *jedec;
	unsigned cur_fuse_addr, cur_fuse_len;
	unsigned highest_cfg_addr, highest_ufm_addr;
} parser_state_t;

/* Parse a string of len*8 '0' and '1' (MSB first) into data
   Return 0 on success, -1 on error.
*/

static int parsebin(const char *line, unsigned len, uint8_t *data)
{
	if (strlen(line) < len*8)
		return -1;

	for (unsigned i = 0;i < len;++i) {
		uint8_t val = 0;
		for (unsigned j = 0;j < 8;++j) {
			int b;
			if (line[i*8+j] == '0') {
				b = 0;
			} else if (line[i*8+j] == '1') {
				b = 1;
			} else {
				fprintf(stderr, "Invalid char in bit string %c\n", line[i*8+j]);
				return -1;
			}
			val |= b<<(7-j);
		}

		*(data++) = val;
	}

	return 0;
}

/* Toplevel JEDEC parse state */
static int parse_field(parser_state_t *state, const char *line)
{
	switch(line[0]) {
	case 'N': // Comment
		if (memcmp(line, "NOTE DEVICE NAME", 16) == 0) {
			if (strstr(line, "LCMXO2-1200") != NULL) {
				state->jedec->devID = MachXO2_1200;
			} else if (strstr(line, "LCMXO2-640") != NULL) {
				state->jedec->devID = MachXO2_640;
			} else {
				fprintf(stderr, "Unsupported device\n");
				return -1;
			}
		}
		break;
	case '*': // Spurious field terminator, ignore
		break;
	case 'Q':
		if (line[1] == 'F') { // Fuse Count
			unsigned fuses;
			if (sscanf(line+2, "%u*\n", &fuses) != 1) {
				fprintf(stderr, "Could not parse:\n\t%s\n", line);
				return -1;
			}
			if (state->data) {
				fprintf(stderr, "Multiple QF records\n");
				return -1;
			}
			state->jedec->pageCnt = fuses/128;
			state->data = malloc(fuses/8);
			if (!state->data) {
				return -1;
			}
			memset(state->data, 0, fuses/8);
			state->data_pos = state->data;
			state->data_len = fuses/8;
		} else if (line[1] == 'P') { // Pin count
			// Ignore
		} else {
			fprintf(stderr, "Unknown record %c%c\n", line[0], line[1]);
		}
		break;
	case 'G': // Security setting, NYI
	case 'F': // Default Fuse state, not used in Lattice JEDEC
		break;
	case 'C': // Checksum
		{
			uint16_t csum, calc_csum = 0;
			if (sscanf(line+1, "%hx*\n", &csum) != 1) {
				fprintf(stderr, "Invalid fuse checksum: %s\n", line+1);
				return -1;
			}
			for (size_t pos = 0;pos < state->data_len;++pos) {
				uint8_t b = state->data[pos];
				// Invert bit order, from
				// http://graphics.stanford.edu/~seander/bithacks.html#ReverseByteWith64Bits
				b = ((b * 0x80200802ULL) & 0x0884422110ULL) * 0x0101010101ULL >> 32;
				calc_csum += b;
			}
			if (calc_csum != csum) {
				fprintf(stderr, "Fuse checksum failed: got %.4hx, expected %.4hx\n",
						calc_csum, csum);
				return -1;
			}
		}
		break;
	case 'L': // Fuse data
		if (sscanf(line+1, "%u\n", &state->cur_fuse_addr) != 1) {
			fprintf(stderr, "Could not parse:\n\t%s\n", line);
			return -1;
		}
		if (!state->data) {
			fprintf(stderr, "Fuse data before QF record\n");
			return -1;
		}
		// Fuse data start given as bit address
		if (state->cur_fuse_addr%8 != 0) {
			fprintf(stderr, "Fuse data not byte aligned, NYI\n");
			return -1;
		}
		// Calculate in bytes from here on
		state->cur_fuse_addr /= 8;

		if (state->cur_fuse_addr >= state->jedec->pageCnt*16) {
			fprintf(stderr, "Fuse data start exceeds flash pages\n");
			return -1;
		}

		if (state->cur_fuse_addr < XO2DevList[state->jedec->devID].Cfgpages*16) {
			state->jedec->pCfgData = state->data;
		} else {
			state->jedec->pUFMData = state->data + XO2DevList[state->jedec->devID].Cfgpages*16;
		}
		state->cur_fuse_len = 0;
		state->data_pos = state->data + (state->cur_fuse_addr);
		state->state = S_FUSES;
		break;
	case 'E': // "Architecture fuses", feature row & bits for Lattice
		if (parsebin(line+1, 8, state->jedec->pFeatureRow.feature) != 0) {
			return -1;
		}

		state->state = S_FEATROW;
		break;
	case 'U': // USERCODE
		if (line[1] == 'H') {
			if (sscanf(line+2, "%x*\n", &state->jedec->UserCode) != 1) {
				fprintf(stderr, "Invalid UserCode\n");
				return -1;
			}
		} else if (line[1] == 'A') {
			if (strlen(line) < 6) {
				fprintf(stderr, "Invalid UserCode\n");
				return -1;
			}
			state->jedec->UserCode = line[2] << 24 | line[3] << 16 | line[5] << 8 | line[5];
		} else if (line[1] == '0' || line[1] == '1') {
			if (parsebin(line+1, 4, (uint8_t*)&state->jedec->UserCode) != 0) {
				fprintf(stderr, "Invalid UserCode\n");
				return -1;
			}
		} else {
			fprintf(stderr, "Invalid UserCode\n");
			return -1;
		}
		break;
	default:
		fprintf(stderr, "Invalid record %s\n", line);
		return -1;
	}

	return 0;
}

/* JEDEC 'L' (fuse data) parse state */
static int parse_fuses(parser_state_t *state, const char *line)
{
	switch(line[0]) {
	case '0':
	case '1':
		// parse binary data
		if (state->data_pos - state->data > state->data_len-16) {
			fprintf(stderr, "Data overflow\n");
			return -1;
		}
		if (line[128] != '\n' && line[128] != '\r') {
			fprintf(stderr, "Fuse data line too long\n");
			return -1;
		}
		if (parsebin(line, 16, state->data_pos) != 0)
			return -1;

		state->data_pos += 16;
		state->cur_fuse_len += 16;

		break;
	case '*':
		if (state->cur_fuse_addr < XO2DevList[state->jedec->devID].Cfgpages*16u) {
			if (state->cur_fuse_addr + state->cur_fuse_len > XO2DevList[state->jedec->devID].Cfgpages*16u) {
				// fuse section spans Cfg / UFM boundary
				state->jedec->CfgDataSize = XO2DevList[state->jedec->devID].Cfgpages*16u;
				state->cur_fuse_len -= XO2DevList[state->jedec->devID].Cfgpages*16u - state->cur_fuse_addr;
				state->cur_fuse_addr = XO2DevList[state->jedec->devID].Cfgpages*16u;
				state->jedec->pUFMData = state->data + XO2DevList[state->jedec->devID].Cfgpages*16;
			} else if (state->cur_fuse_addr + state->cur_fuse_len > state->jedec->CfgDataSize) {
				state->jedec->CfgDataSize = (state->cur_fuse_addr + state->cur_fuse_len);
			}
		}
		if (state->cur_fuse_addr >= XO2DevList[state->jedec->devID].Cfgpages*16u) {
			if (state->cur_fuse_addr + state->cur_fuse_len - XO2DevList[state->jedec->devID].Cfgpages*16 > state->jedec->UFMDataSize) {
				state->jedec->UFMDataSize = (state->cur_fuse_addr + state->cur_fuse_len) - XO2DevList[state->jedec->devID].Cfgpages*16;
			}
		}
		state->state = S_START;
		break;
	default:
		fprintf(stderr, "Invalid line in fuse data: %s\n", line);
		return -1;
	}

	return 0;
}

/* JEDEC 'E' (feature fuse data) parse state */
static int parse_featrow(parser_state_t *state, const char *line)
{
	if (parsebin(line, 2, state->jedec->pFeatureRow.feabits) != 0) {
		fprintf(stderr, "Invalid feature bits record\n");
		return -1;
	}
	if (line[16] != '*') {
		fprintf(stderr, "Invalid feature bits record\n");
		return -1;
	}
	state->state = S_START;
	return 0;
}

XO2_JEDEC_t *jedec_parse(FILE *jedfile)
{
	parser_state_t state;
	int c;
	if (!jedfile)
		return NULL;

	while (true) {
		c = fgetc(jedfile);

		if (c == EOF) {
			fprintf(stderr, "Unexpected end of file\n");
			return NULL;
		}

		// ^B - Start of JEDEC data
		if ((unsigned char)c == 0x02)
			break;
	}

	memset(&state, 0, sizeof(state));
	state.state = S_START;
	state.jedec = malloc(sizeof(*state.jedec));
	if (!state.jedec)
		return NULL;

	int ret;
	bool do_csum = true;
	uint16_t calc_csum = 0x02; // File checksum includes the initial ^B
	while(true) {
		char *line = NULL;
		size_t linesize = 0;
		ssize_t len = getline(&line, &linesize, jedfile);
		if (len == -1) {
			free(line);
			if (feof(jedfile)) {
				fprintf(stderr, "Unexpected end of file\n");
			} else {
				fprintf(stderr, "getline() failed: %s\n", strerror(errno));
			}
			goto fail;
		}

		// File checksum includes every character including newline
		// until and including the terminating ^C
		for (int p = 0;p < len;++p) {
			if (do_csum) {
				calc_csum += line[p];
			}
			if (line[p] == 0x03) {
				do_csum = false;
			}
		}

		// File checksum is stored as four hex digits after the terminating ^C
		if (line[0] == 0x03) {
			uint16_t csum;
			if (sscanf(line+1, "%hx", &csum) != 1) {
				fprintf(stderr, "Invalid file checksum: %s\n", line+1);
				free(line);
				goto fail;
			}
			if (calc_csum != csum) {
				fprintf(stderr, "File checksum failed: got %.4hx, expected %.4hx\n",
						calc_csum, csum);
				free(line);
				goto fail;
			}
			break; // ^C - End of JEDEC data
		}
		if (len <= 1) {
			free(line);
			continue;
		}

		switch (state.state) {
		case S_START:
			ret = parse_field(&state, line);
			break;
		case S_FUSES:
			ret = parse_fuses(&state, line);
			break;
		case S_FEATROW:
			ret = parse_featrow(&state, line);
			break;
		}

		free(line);
		if (ret != 0)
			goto fail;
	}

	return state.jedec;

  fail:
	if (state.data)
		free(state.data);
	free(state.jedec);
	return NULL;
}

void jedec_free(XO2_JEDEC_t *jedec)
{
	free(jedec);
}
