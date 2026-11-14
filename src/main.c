/*
 * PCSX2-360 bootstrap.
 * This is a minimal Xenon entry point, not yet a PS2 game runtime.
 * PS2Palace360-derived modules retain their original MIT attribution.
 */
#include <stdio.h>

int main(void)
{
    /* Runtime/device initialization and PS2 boot are not implemented yet. */
    puts("PCSX2-360: bootstrap");
    for (;;) {
        /* Placeholder loop until Xbox 360 input/video/audio are integrated. */
    }
    return 0;
}
