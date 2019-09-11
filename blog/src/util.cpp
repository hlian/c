#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>

#include "util.hpp"

/* `chroot()`s, `stat()`s, `setregid()`s, and `setruid()`s if the user
   is root, otherwise does nothing. */
void drop_privileges(char *dir) {
    struct stat stats;
    int ret;

    if (getuid() != 0) {
        fprintf(stderr, "[log] running as: user %d\n", getuid());
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

    fprintf(stderr, "[log] privileges: dropped\n");
    fprintf(stderr, "[log] running as: user %d\n", getuid());
}
