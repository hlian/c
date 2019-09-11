#pragma once

#include <assert.h>

/* ICMP headers and flags taken from Linux kernel. */
typedef struct {
    uint8_t type;
    uint8_t code;
    uint16_t sum;
    uint16_t id;
} __attribute__((packed)) icmp_t;

/* ICMP types. */
#define ICMP_ECHOREPLY          0       /* Echo Reply                   */
#define ICMP_DEST_UNREACH       3       /* Destination Unreachable      */
#define ICMP_ECHO               8       /* Echo Request                 */
#define ICMP_TIME_EXCEEDED      11      /* Time Exceeded                */
/* ICMP codes. */
#define ICMP_HOST_UNREACH       1       /* Host Unreachable             */
#define ICMP_PORT_UNREACH       3       /* Port Unreachable             */
#define ICMP_EXC_TTL            0       /* TTL count exceeded           */

#define assert0(ret, str) \
    do { assert(ret == 0 && str); } while(0);

void drop_privileges(char *);
uint16_t icmp_sum(icmp_t *icmp, uint32_t len);
