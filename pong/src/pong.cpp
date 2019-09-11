#include <cstring>
#include <string>

#include <assert.h>
#include <stdio.h>
#include <sys/types.h>

#include "pong.hpp"
#include "util.hpp"

static void icmp_to_string(icmp_t *icmp, std::string *acc) {
    static char buf[100];
    sprintf(buf, "%02x%02x%04x%04x",
            icmp->type, icmp->code, icmp->sum, icmp->id);
    acc->append("shut up here you go ");
    acc->append(buf);
}

static bool string_to_icmp(icmp_t *icmp, char *input, size_t length) {
    unsigned int type, code, sum, id, sequence;
    int ret;

    ret = sscanf(input, "%02x%02x%04x%04x",
                 &type, &code, &sum, &id);
    if (ret != 4) {
        return false;
    }

    icmp->type = type;
    icmp->code = code;
    icmp->sum = sum;
    icmp->id = id;
    return true;
}

static bool verify(icmp_t *a, icmp_t *b) {
    if (b->type != ICMP_ECHOREPLY) {
        return false;
    }

    if (b->code != 0) {
        return false;
    }

    if (b->sum != icmp_sum(b, sizeof(icmp_t))) {
        return false;
    }

    if (b->id != a->id + 1) {
        return false;
    }

    return true;
}

static char *back_to_1970(std::string *in, size_t *nout) {
    *nout = in->size();
    char *out = (char *) malloc(*nout);
    strncpy(out, in->c_str(), *nout);
    return out;
}

static char *pong_error(pong_t *pong, size_t *nout) {
    static std::string msg = "ERROR, starting over. What's your name?";
    pong->icmp.id = 0;
    pong->icmp.sum = icmp_sum(&pong->icmp, sizeof(icmp_t));
    pong->state = PONG_START;
    return back_to_1970(&msg, nout);
}

FILE *LOG = NULL;
uint32_t counter = 0;

void pong_init() {
    LOG = fopen("pong.log", "a+");
    assert(LOG && "log unopened");
}

void pong_close() {
    fclose(LOG);
}

void pong_new(pong_t *pong) {
    pong->state = PONG_START;
    pong->uid = counter++;
    pong->icmp.type = ICMP_ECHO;
    pong->icmp.code = 0;
    pong->icmp.sum = icmp_sum(&pong->icmp, sizeof(icmp_t));
    pong->icmp.id = 0;
}

void pong_free(pong_t *pong) {
    (void) pong;
}

char *pong_feed(pong_t *pong, char *input, size_t length, size_t *nout) {
    std::string acc;

    fprintf(LOG, "[%04d] ", pong->uid);
    fwrite(input, 1, length, LOG);
    fputc('\n', LOG);
    fflush(LOG);

    switch(pong->state) {
    case PONG_START:
        pong->state = PONG_GO;
        icmp_to_string(&pong->icmp, &acc);
        break;

    case PONG_GO:
        icmp_t icmp;
        if (!string_to_icmp(&icmp, input, length)) {
            return pong_error(pong, nout);
        }
        if (!verify(&pong->icmp, &icmp)) {
            return pong_error(pong, nout);
        }

        if (pong->icmp.id >= 0xfe) {
            pong->state = PONG_END;
            acc.append("all right you win, big boy http://dl.dropbox.com/u/430960/can/racism.mp4");
        }
        else {
            pong->icmp.id = icmp.id + 1;
            pong->icmp.sum = icmp_sum(&pong->icmp, sizeof(icmp_t));
            icmp_to_string(&pong->icmp, &acc);
        }
        break;

    case PONG_END:
        return NULL;

    default:
        assert(false);
    }

    return back_to_1970(&acc, nout);
}

char *pong_reset(pong_t *pong, size_t *nout) {
    static std::string msg = "\nTIMEOUT, starting over. What's your name?";
    pong->icmp.id = 0;
    pong->icmp.sum = icmp_sum(&pong->icmp, sizeof(icmp_t));
    pong->state = PONG_START;
    return back_to_1970(&msg, nout);
}

char *pong_hello(pong_t *pong, size_t *nout) {
    static std::string msg =
        "Welcome to International Cooperative Multiplayer Pong. LET US PLAY A GAME. What's your name?";
    return back_to_1970(&msg, nout);
}
