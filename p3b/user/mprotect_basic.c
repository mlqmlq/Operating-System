/* proctected area shouldn't be written */
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

    for (int i = 0; i < 2; i ++) {
        mprotect((void *)ptr_aligned, 1);
    }

    if (fork() == 0) {
        int rnt_code = mprotect((void *)ptr_aligned, 1);
        if (rnt_code == 0) {
            printf(1, "write to a protected page\n");
           
            for (int i = 0; i < PGSIZE; i++){
                ((char *)ptr_aligned)[i] = ((char *)ptr_aligned)[i];
            }
            // this process should be killed
            printf(1, "Error: write to a protected page but fail to trigger page fault\n");
        } else{
            printf(1, "Error: mprotect return non-zero value: %d\n", rnt_code);
        }
        printf(1, "TEST FAILED\n");    
        kill(ppid);
        exit();
    } else {
        wait();
    }

    if (fork() == 0 ) {
        int rnt_code = mprotect((void *)ptr_aligned, 1);
        if (rnt_code == 0) {
            printf(1, "write to an unprotected page\n");
            char * null_ptr = (char *) 0;
            for (int i = 0; i < ptr_aligned; i++){
                ((char *)null_ptr)[i] = ((char *)null_ptr)[i];
            }
            printf(1, "TEST PASSED\n");
        } else{
            printf(1, "Error: mprotect return non-zero value: %d\n", rnt_code);
            printf(1, "TEST FAILED\n");   
        }
        kill(ppid);
        exit();
    } else {
        wait();
    }
    printf(1, "Error: write to an unprotected page but triger a page fault\n");
    printf(1, "TEST FAILED\n");   
    exit();
}
