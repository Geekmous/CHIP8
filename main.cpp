
#include "chip.h"

int main() {

    chip8 chip;
    chip.initialize();
    chip.readROM("./invaders.c8");
    chip.run();

    return 0;
}