#ifndef CHIP_H_
#define CHIP_H_
#include <cstdio>
void refresh_screen();
void onPerpareDraw();
void initalize_OpenGL(int argc, char* argv[]);
static unsigned char g[32 * 64] = {0};


class chip8 {
public:
    /*
     * 0x000 - 0x1FF chip8 interpreter
     * 0x050 - 0x0A0 charset from '0' to 'F'
     * 0x200 - 0xFFF ROM and RAM
     */
    unsigned char memory[4096];

    unsigned short PC;

    unsigned char V[16];
    unsigned char* gfx;
    unsigned short stack[16];
    unsigned short sp;
    unsigned char key[16];
    //address register
    unsigned short I;
    //count down at 60hz
    unsigned char delay_timer;
    unsigned char sound_timer;
    bool isloaded;
    bool drawFlag;
    bool CRASH_FLAG;
    unsigned char chip8_fontset[80];
    FILE* logging;
    void push(unsigned short data);
    unsigned short pop();
    unsigned short getOPCode();
    int runOPCode(unsigned short op);
    void machineCicle();
    unsigned char get_delay_timer();
    unsigned char get_sound_timer();
    void set_delay_timer(unsigned char value);
    void set_sound_timer(unsigned char value);
    void disp_clear();
    unsigned char get_key();


public:
    chip8();
    void initialize();
    void setGFX(unsigned char** addr) {
        gfx = *addr;
    }
    void readROM(const char* path);
    void run();
    ~chip8();
};
#endif