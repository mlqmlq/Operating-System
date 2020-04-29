/* any write in the proctected area should be killed  */
#include "types.h"
#include "stat.h"
#include "user.h"

#define PGSIZE  4096
int
main(int argc, char *argv[])
{
    int ppid = getpid();
    uint ptr = (uint) sbrk(4 * PGSIZE);
    // round up ptr_aligned
    uint ptr_aligned =  ((ptr + PGSIZE - 1 ) & ~ (PGSIZE - 1));
    int rnt_code = mprotect((void *)ptr_aligned, 3);

    if (fork() == 0) {
        if (rnt_code == 0) {
            printf(1, "write to a protected page\n");
            for (int i = 3 * PGSIZE - 1; i >= 0; i--){
                ((char *)ptr_aligned)[i] = ((char *)ptr_aligned)[i];
            }
            // this process should be killed
            printf(1, "ERROR: write to a protected page but not trigger page fault\n");
        } else{
            printf(1, "ERROR: mprotect return non-zero value: %d\n", rnt_code);
        }
        printf(1, "TEST FAILED\n");
        kill(ppid);
        exit();
    } else {
        wait();
    }

    if (fork() == 0) {
        if (rnt_code == 0) {
            printf(1, "write to a protected page\n");
            
            for (int i = PGSIZE; i < 2 * PGSIZE; i++){
                ((char *)ptr_aligned)[i] = ((char *)ptr_aligned)[i];
            }
            // this process should be killed
            printf(1, "ERROR: write to a protected page but not trigger page fault\n");
        } else{
            printf(1, "ERROR: mprotect return non-zero value: %d\n", rnt_code);
        }

        printf(1, "TEST FAILED\n");
        kill(ppid);
        exit();
    } else {
        wait();
    }

    munprotect((void *)(ptr_aligned + PGSIZE), 2);
    if (fork() == 0) {
        printf(1, "write to a protected page\n");
        for (int i = 0; i < PGSIZE; i ++){
            ((char *)ptr_aligned)[i] = ((char *)ptr_aligned)[i];
        }
        // this process should be killed
        printf(1, "ERROR: write to a protected page but not trigger page fault\n");
        printf(1, "TEST FAILED\n");
        kill(ppid);
        exit();
    } else {
        wait();
    }

    if (fork() == 0) {
        printf(1, "write to an unprotected page\n");
        for (int i = PGSIZE; i < 3 * PGSIZE; i ++){
            ((char *)ptr_aligned)[i] = ((char *)ptr_aligned)[i];
        }
        // this process should not be killed
        printf(1, "TEST PASSED\n");
        kill(ppid);
        exit();
    } else {
        wait();
    }

    printf(1, "ERROR: write to an unprotected page but trigger page fault\n");
    printf(1, "TEST FAILED\n");
    exit();
}