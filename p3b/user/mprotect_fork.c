/* proctected area should be inherited in the fork */
#include "types.h"
#include "stat.h"
#include "user.h"

#define PGSIZE  4096
int
main(int argc, char *argv[])
{
    int ppid = getpid();
    uint ptr = (uint) sbrk(2 * PGSIZE);
    // round up ptr_aligned
    uint ptr_aligned =  ((ptr + PGSIZE - 1 ) & ~ (PGSIZE - 1));

    for (int i = 0; i < 4; i ++) {
        mprotect((void *)ptr_aligned, 1);
    }
    int rnt_code = mprotect((void *)ptr_aligned, 1);

    if (fork() == 0) {
        if (rnt_code == 0) {
            printf(1, "write to protected page in the child\n");
            for (int i = 0; i < PGSIZE; i++){
                ((char *)ptr_aligned)[i] = ((char *)ptr_aligned)[i];
            }
            // this process should be killed
            printf(1, "Error : write to a protected page in the child process but fail to trigger page fault\n");
        } else{
            printf(1, "Error: mprotect return non-zero value: %d\n", rnt_code);
        }
        printf(1, "TEST FAILED\n");
        kill(ppid);
        exit();
    } else {
        wait();
    }

    printf(1, "TEST PASSED\n");
    exit();
}