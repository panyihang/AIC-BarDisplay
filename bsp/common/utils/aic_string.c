/*
 * Copyright (c) 2025, ArtInChip Technology Co., Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Authors: matteo <duanmt@artinchip.com>
 */

#include <stdlib.h>
#include <string.h>

long long int str2int(char *str)
{
    if (str == NULL)
        return 0;

    if (strncmp(str, "0x", 2))
        return atoi(str);
    else
        return strtoll(str, NULL, 16);
}
