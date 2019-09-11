#pragma once

#include <stddef.h>
#include "util.hpp"

typedef enum {
    PONG_START,
    PONG_GO,
    PONG_END
} pong_states_t;

typedef struct {
    pong_states_t state;
    icmp_t icmp;
    uint32_t uid;
} pong_t;

void pong_init();
void pong_close();
void pong_new(pong_t *pong);
void pong_free(pong_t *pong);
char *pong_feed(pong_t *pong, char *input, size_t length, size_t *nout);
char *pong_reset(pong_t *pong, size_t *nout);
char *pong_hello(pong_t *pong, size_t *nout);
