#ifndef _INCLUDE_H_
#define _INCLUDE_H_

typedef enum {FALSE = 0, TRUE = !FALSE} bool;

#define CHJ_DEBUG

#ifdef CHJ_DEBUG
#define PRINT_DEBUG(format, args...)   printf(format, ##args)
#else
#define PRINT_DEBUG(format, args...)
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "MQTTPacket.h"
#include "malloc.h"
#include "cJSON.h"

#endif
