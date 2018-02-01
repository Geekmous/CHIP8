#include "chip.h"
#include <ctime>
#include <cstring>
#include <unistd.h>
#include <functional>
#include <GLUT/GLUT.h>


chip8::chip8() {
    sp = 0;
    delay_timer = 60;
    sound_timer = 60;
    isloaded = false;
    PC = 0x200;
    I = 0x0000;
    isloaded = false;
    drawFlag = false;

}
void chip8::initialize() {
    unsigned char temp[] =
{ 
  0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
  0x20, 0x60, 0x20, 0x20, 0x70, // 1
  0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
  0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
  0x90, 0x90, 0xF0, 0x10, 0x10, // 4
  0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
  0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
  0xF0, 0x10, 0x20, 0x40, 0x40, // 7
  0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
  0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
  0xF0, 0x90, 0xF0, 0x90, 0x90, // A
  0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
  0xF0, 0x80, 0x80, 0x80, 0xF0, // C
  0xE0, 0x90, 0x90, 0x90, 0xE0, // D
  0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
  0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

    memcpy(chip8_fontset, temp, 80);
    memcpy(memory, temp, 80);
    memset(V, 0, sizeof(V));
    CRASH_FLAG = false;
    logging = fopen("./log.txt", "w");


    //initalize opengl
    glutInit(0, NULL);
    glutInitDisplayMode(GLUT_RGB |GLUT_DOUBLE);
    glutInitWindowPosition(100, 100);
    glutInitWindowSize(400, 400);
    glutCreateWindow("CHIP8");

    std::function<void()> disp = std::bind(&chip8::onPerpareDraw, this);
    glutDisplayFunc(&disp);
    //glutIdleFunc()
    glutMainLoop();
}

void chip8::readROM(const char* path) {
    FILE* fp = fopen(path, "rb");
    if (fp == NULL) {
        fprintf(stderr, "FILE:%s can't open\n", path);
        return;
    }
    size_t read_num = 0;
    read_num = fread(memory + 0x200, 1, 4096, fp);
    if (read_num <= 0) {
        fprintf(stderr, "FILE:%s read error\n", path);
        fclose(fp);
        return;
    }

    isloaded = true;
    fclose(fp);
}

void chip8::run() {
    if (isloaded == false) {
        fprintf(stderr, "Not load ROM\n");
        return;
    }
    size_t i = 0;
    for (;;) {
        printf("run cycle %lu\n", i++);
        machineCicle();
        if (CRASH_FLAG) {
            fprintf(stderr, "SORRY, EMULATOR CRASH\n Will close...\n");
            break;
        }
        if (drawFlag) {
            refresh_screen();
            drawFlag = false;
        }
        if (delay_timer > 0)
            delay_timer--;
        
        if (sound_timer > 0) {
            if (sound_timer == 1) {
                printf("BEEP!\n");
            }
            sound_timer--;
        }
        usleep( 1000000.0 / 60);
    }
}

chip8::~chip8() {
    if (logging != NULL)
        fclose(logging);
}

void chip8::push(unsigned short data) {
    stack[sp++] = data;
}

unsigned short chip8::pop() {

    return stack[--sp];
}

unsigned short chip8::getOPCode() {
    unsigned short t;
    t = memory[PC] << 8 | memory[PC + 1];
    PC += 2;
    return t;
}

int chip8::runOPCode(unsigned short op) {
    char type1 = (op >> 12) & 0xf;
    switch (type1) {
        case 0x0:{
            unsigned char suffix = op & 0xff;
            if (suffix == 0xE0) {
                disp_clear();
            } else if (suffix == 0xEE) {
                //return
                PC = pop();
            } else {
                //call 
                push(PC);
                PC = op & 0xfff;
            }
            
            return 1;
        }
        case 0x1: PC = op & 0xfff;return 1;;
        case 0x2: {
            //call subroutine at NNN;
            unsigned short p = PC;
            push(PC);
            PC = op & 0xfff;
            return 1;
        }
        case 0x3: {
            unsigned short index = (op >> 8) & 0xf;
            unsigned char com = op & 0xff;
            if (V[index] == com)
                getOPCode();
            return 1;
        }
        case 0x4: {
            unsigned short index = (op >> 8) & 0xf;
            unsigned char com = op & 0xff;
            if (V[index] != com)
                getOPCode();
            return 1;
        }
        case 0x5: {
            unsigned short index1 = (op >> 8) & 0xf;
            unsigned short index2 = (op >> 4) & 0xf;
            if (V[index1] == V[index2])
                getOPCode();
            return 1;
        }
        case 0x6: {
            unsigned short index = (op >> 8) & 0xf;
            unsigned char value = op & 0xff;
            V[index] = value;
            return 1;
        }
        case 0x7: {
            unsigned short index = (op >> 8) & 0xf;
            unsigned char value = op & 0xff;
            V[index] += value;
            return 1;
        }
        case 0x8: {
            unsigned short index1 = (op >> 8) & 0xf;
            unsigned short index2 = (op >> 4) & 0xf;
            unsigned char value = op & 0xf;
            if (value == 0x0) {
                V[index1] = V[index2];
            } else if (value == 0x1) {
                V[index1] = V[index1] | V[index2];
            } else if (value == 0x2) {
                V[index1] = V[index1] & V[index2];
            } else if (value == 0x3) {
                V[index1] = V[index1] ^ V[index2];
            } else if (value == 0x4) {
                if (V[index2] > 0xff - V[index1])
                    V[0xf] = 1;
                V[index1] += V[index2];
                // if carry VF is set to 1,otherwise set to 0
            } else if (value == 0x5) {
                if (V[index1] < V[index2]) {
                    V[0xF] = 0;
                } else {
                    V[0xF] = 1;
                }
                V[index1] -= V[index2];
                
            } else if (value == 0x6) {
                V[0xF] = V[index2] & 0x1;
                V[index1] = V[index2] = V[index2] >> 1;
            } else if (value == 0x7) {
                if (V[index2] < V[index1]) {
                    V[0xF] = 0;
                } else {
                    V[0xF] = 1;
                }
                V[index1] = V[index2] - V[index1];
                //VF is set to 0 when there's a borrow, and 1 when there isn't.
            } else if (value == 0xE) {
                V[0xF] = V[index2] & 0x80;
                V[index1] = V[index2] = V[index2] << 1;
            } else {
                return -1;
            }
            
            return 1;
        }
        case 0x9: {
            unsigned short index1 = (op >> 8) & 0xf;
            unsigned short index2 = (op >> 4) & 0xf;
            if (V[index1] != V[index2]) {
                getOPCode();
            }
            return 1;
        }
        case 0xA: {
            I = op & 0xfff;
            return 1;
        }
        case 0xB: {
            PC = V[0x0] + (op & 0xfff);
            return 1;
        }
        case 0xC: {
            unsigned short index1 = (op >> 8) & 0xf;
            V[index1] = /* random */ 0x11 & (op & 0xff);
            return 1;
        }
        case 0xD: {
            //disp
            unsigned char indexx = (op >> 8) & 0xf;
            unsigned char indexy = (op >> 4) & 0xf;
            unsigned char h = op & 0xf;
            V[0xF] = 0;
            unsigned char X = V[indexx];
            unsigned char Y = V[indexy];
            for (int row = 0; row < h; row++) {
                // gfx to bit
                unsigned short pixel = memory[I + row];
                for (int column = 7; column >= 0; column--) {
                    unsigned char g = gfx[(Y + row) * 64 + (X + column)];
                    if (g == 1 && (pixel & 0x1) == 0)
                        V[0xf] = 1;
                    gfx[(Y + row) * 64 + (X + column)] = pixel & 0x1;
                    pixel >>= 1;
                }
            }
            drawFlag = true;
            return 1;
        }
        case 0xE: {
            unsigned char Key = op >> 8 & 0xf;
            unsigned char suffix = op & 0xff;
            if (suffix == 0x9E) {
                if (key[Key]) {
                    getOPCode();
                }
            } else if (suffix == 0xA1) {
                if (key[Key] == 0) {
                    getOPCode();
                }
            }
            return 1;
        }
        case 0xF: {
            unsigned short index1 = (op >> 8) & 0xf;
            unsigned char suffix = op & 0xff;
            if (suffix == 0x07) {
                V[index1] = get_delay_timer();
            } else if (suffix == 0x0A) {
                V[index1] = get_key(); // awaited
            } else if (suffix == 0x15) {
                set_delay_timer(V[index1]);
            } else if (suffix == 0x18) {
                set_sound_timer(V[index1]);
            } else if (suffix == 0x1e) {
                I += V[index1];
            } else if (suffix == 0x29) {
                I = memory[V[index1] * 5];
            } else if (suffix == 0x33) {
                memory[I] = V[index1] / 100;
                memory[I + 1] = V[index1] % 100 / 10;
                memory[I + 2] = V[index1] % 10;
            } else if (suffix == 0x55) {
                for(unsigned short i = 0; i <= index1; i++) {
                    memory[I] = V[i];
                    I++;
                }
            } else if (suffix == 0x65) {
                for (unsigned short i = 0; i <= index1; i++) {
                    V[i] = memory[I];
                    I++;
                }
            } else {
                return -1;
            }
            return 1;
        }
        default: return -1;

    }
}

void chip8::machineCicle() {
    if (logging != NULL) {
        for(int i = 0; i < 0x10; i++)
            fprintf(logging, "V[%x] = 0x%x\n", i, V[i]);
        for (int i = 0; i < 0x10; i++) {
            fprintf(logging, "STACK[%d] = 0x%x\n", i, stack[i]);
        }
        fprintf(logging, "PC: 0x%x\n", PC);
        fprintf(logging, "I: 0x%x\n", I);

    }
    unsigned short op = getOPCode();
    if (logging != NULL) {
        fprintf(logging, "op: 0x%x\n", op);
    }
    printf("OPERATIONS: 0x%x\n", op);
    fflush(logging);
    int ret = runOPCode(op);
    if (ret == -1) {
        printf("INVALIDATION OPERATION WITH 0x%x\n", op);
        CRASH_FLAG = true;
    }
}

unsigned char chip8::get_delay_timer() {
    return delay_timer;
}

unsigned char chip8::get_sound_timer() {
    return sound_timer;
}

void chip8::set_delay_timer(unsigned char value) {
    delay_timer = value;
}

void chip8::set_sound_timer(unsigned char value) {
    sound_timer = value;
}

void chip8::disp_clear() {
    //s
}

unsigned char chip8::get_key() {
    return 0xA;
}

void chip8::refresh_screen() {

}