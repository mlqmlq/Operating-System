/* any write in the proctected area should be killed  */
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
    int ptr_aligned =  ((ptr + PGSIZE - 1 ) & ~ (PGSIZE - 1));

    for (int i = 0; i < 4; i ++) {
        mprotect((void *)ptr_aligned, 1);
    }

    if (fork() == 0) {
        int rnt_code = mprotect((void *)ptr_aligned, 1);
        if (rnt_code == 0) {
            printf(1, "write to protected page\n");
            for (int i = 0; i < PGSIZE; i++){
                ((char *)ptr_aligned)[i] = '\0';
            }
            // this process should be killed
            printf(1, "Error: write to a protected page but not trigger page fault\n");
        } else{
            printf(1, "Error: mprotect return non-zero value: %d\n", rnt_code);
        }
        printf(1, "TEST FAILED\n");
        kill(ppid);
        exit();
    } else {
        wait();
    }

    if (fork() == 0) {
        mprotect((void *)ptr_aligned, 1);
        int rnt_code = munprotect((void *)ptr_aligned, 1);
        if (rnt_code == 0) {
            printf(1, "write to an unprotected page\n");
            for (int i = 0; i < PGSIZE; i++){
                ((char *)ptr_aligned)[i] = ((char *)ptr_aligned)[i];
            }
        } else{
            printf(1, "Error: mprotect return non-zero value: %d\n", rnt_code);
            printf(1, "TEST FAILED\n");
            kill(ppid);
            exit();
        }

        // this process should not be killed
        printf(1, "TEST PASSED\n");
        kill(ppid);
        exit();
    } else {
        wait();
    }
    printf(1, "Error: munprotect not work properly\n");
    printf(1, "TEST FAILED\n");
    exit();
}