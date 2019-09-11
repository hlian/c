#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <netinet/in.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "util.hpp"

/* `chroot()`s, `stat()`s, `setregid()`s, and `setruid()`s if the user
   is root, otherwise does nothing. */
void drop_privileges(char *dir) {
    struct stat stats;
    int ret;

    if (getuid() != 0) {
        fprintf(stderr, "Running as user %d.\n", getuid());
        return;
    }

    ret = chroot(dir);
    assert0(ret, "drop_privileges: chroot");

    ret = chdir("/");
    assert0(ret, "drop_privileges: chdir");

    ret = stat("/", &stats);
    assert0(ret, "drop_privileges: stat");

    ret = setregid(stats.st_gid, stats.st_gid);
    assert0(ret, "drop_privileges: setregid");

    ret = setreuid(stats.st_uid, stats.st_uid);
    assert0(ret, "drop_privileges: setreuid");

    fprintf(stderr, "Privileges dropped to user %d.\n", getuid());
}

uint16_t icmp_sum(icmp_t *icmp, uint32_t len) {
    /* Save checksum and size before zeroing it out. */
    uint16_t temp = icmp->sum;
    uint32_t sum  = icmp->sum = 0;

    /* Sum all 16-bit words. */
    uint16_t *blocks = (uint16_t *) icmp;
    for (uint32_t i = 0; i < len / 2; i++) sum += ntohs(blocks[i]);

    /* End-around carry plus checksum plus cast to 16-bit. */
    sum = ~((sum & 0xffff) + ((sum & 0xf0000) >> 16)) & 0xffff;

    /* Restore checksum, convert to network byte order. */
    icmp->sum = temp;
    return sum;
}
