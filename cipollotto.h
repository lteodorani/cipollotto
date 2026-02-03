#ifndef CHIP8_H
#define CHIP8_H
#include <stdint.h>
#include <stdbool.h>

#define PROG_START_ADDR   0x200
#define PROG_PREAMBLE     0x1FC //before prog we execute 00E0(clr screen) and 004B(turn disp. on)
#define MEMORY_END        0xFFF
#define MEMORY_START      0x000
#define FONT_START_ADDR   0x000
#define FONT_SET_SIZE     0x050 //font set should be 80 bytes placed between 0x000 and 0x50
#define SCREEN_WIDTH      64
#define SCREEN_HEIGHT     32
#define PIXEL_NUMBER      (SCREEN_HEIGHT * SCREEN_WIDTH)
#define BYTES_PER_ROW     (SCREEN_WIDTH / 8)
#define FB_SIZE           (BYTES_PER_ROW * SCREEN_HEIGHT)
#define STACK_SIZE_UINT16 16

typedef enum{
    ERR_OK = 0,
    ERR_STACK_UNDERFLOW,
    ERR_STACK_OVERFLOW,
    ERR_INVALID_OPCODE,
    ERR_MEM_OUT_OF_BOUNDS,
    ERR_IGNORED_OPCODE,
    ERR_UNKNOWN_OPCODE,
    ERR_UNIMPL_OPCODE
} error;

typedef enum{
   STATUS_RUNNING = 0,
   STATUS_PAUSED,
   STATUS_STOPPED 
} status;

typedef struct{
    uint8_t  MEM[4096];  //memory
    uint8_t  FB[256];    //framebuffer of 64x32 on/off pixels (256 bytes)
    uint16_t STACK[16];  //stack
    uint16_t V[16], I;   //V0...VF, I regs. V regs really should 8 but i store them in 16 bit container
    uint8_t  KP[16];     //keypad
    uint8_t  DT, ST, SP; //timers, stack pointer registers
    uint16_t PC;         //program counter register

    error    errState;
    bool     drawFlag;
    status   running;
    uint16_t rnd;
    

} chip8;

void chip8Init(chip8* chip8_state);
void chip8Step(chip8* chip8_state);

#endif