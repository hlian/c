#pragma once

#include <assert.h>
#include <stdio.h>

#define assert0(ret, str) \
    do { if (ret != 0) { perror(str); assert(0); } } while(0);

void drop_privileges(char *);
