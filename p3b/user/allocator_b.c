/* any write in the proctected area should be killed  */
#include "types.h"
#include "stat.h"
#include "user.h"


#define PGSIZE 4096

#define NUMFRAMES 48

int main() {
    sbrk(NUMFRAMES * PGSIZE);

    //uint result[NUMFRAMES] = {0x904000, 0xBA6000, 0x888000, 0x594000, 
    //                            0x9D4000, 0x8E2000, 0xE54000, 0xD3F000, 0x468000, 0x700000, 0x266000, 0xD89000, 0xFAB000, 0x258000, 0xEE3000, 0xC58000, 0xB54000, 0x893000, 0x1F5000, 0xCE4000, 0x725000, 0xB58000, 0x979000, 0x929000, 0x609000, 0xFBB000, 0xD8C000, 0x314000, 0x1AE000, 0xBA9000, 0x809000, 0x45E000, 0xEBA000, 0x485000, 0x825000, 0x745000, 0x43F000, 0xC8A000, 0x823000, 0xE65000, 0xC88000, 0x699000, 0x512000, 0x98F000, 0xE8F000, 0xA2C000, 0x925000, 0x4D1000, 0x275000, 0xD87000};

    uint result[4] = {0xFF8000, 0xFFA000, 0xFFC000, 0xFFE000};
    int frames[300];

    int index = 1;
    for (; index < 300; index ++) {
        if (dump_allocated(frames, index) != 0) {
            break;
        }
    }
    index = index - 1;
    //for (int i = 0; i < index; i++)
    //    printf(1, "%x\n", frames[i]);

    for (int i = 0; i < 4; i++) {
        if (frames[index - 4 + i] != result[i]) {
            printf(1, "Error: random allocator return incorrect result under fixed sequence\n");
            printf(1, "TEST FAILED\n");
            exit();
        }
    }
    printf(1, "TEST PASSED\n");
    exit();
}
