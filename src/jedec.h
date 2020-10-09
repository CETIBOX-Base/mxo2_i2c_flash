/*
 *  Copyright (c) 2018-2020 CETITEC GmbH
 *
 * All rights reserved. All use of this software and documentation is
 * subject to the License Agreement located in the file LICENSE.
 */

#ifndef JEDEC_H
#define JEDEC_H

#include <stdio.h>
#include "XO2_ECA/XO2_dev.h"

XO2_JEDEC_t *jedec_parse(FILE *jedfile);
void jedec_free(XO2_JEDEC_t *jedec);

#endif
