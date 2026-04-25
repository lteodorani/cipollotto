#pragma once

#include "common.h"

#define PROG_START_ADDR   0x200
#define MEMORY_END        0xFFF
#define MEMORY_START      0x000

#define FONT_SMALL_ADDR   0x000
#define FONT_SMALL_SIZE   80
#define FONT_LARGE_ADDR   0x050
#define FONT_LARGE_SIZE   100

#define SCREEN_WIDTH      128
#define SCREEN_HEIGHT     64
#define PIXEL_NUMBER      (SCREEN_HEIGHT * SCREEN_WIDTH)
#define BYTES_PER_ROW     (SCREEN_WIDTH /  8)
#define WORDS_PER_ROW     (SCREEN_WIDTH / 16)
#define FB_SIZE           (BYTES_PER_ROW * SCREEN_HEIGHT)
#define STACK_SIZE_UINT16 16


typedef enum{
    CIPOLLOTTO_OK = 0,
    CIPOLLOTTO_ERR_STACK_OOB,
    CIPOLLOTTO_ERR_MEMORY_OOB,
    CIPOLLOTTO_ERR_INVALID_OPCODE,
} chip8_error;


typedef enum{
   CIPOLLOTTO_STATUS_RUNNING = 0,
   CIPOLLOTTO_STATUS_HALTED,
   CIPOLLOTTO_STATUS_PAUSED,
    
} chip8_status;

typedef enum{
    CIPOLLOTTO_VARIANT_CHIP8 = 0,
    CIPOLLOTTO_VARIANT_SCHIP
} chip8_variant;



typedef struct chip8{

    void (*opExecute)(struct chip8* chip8_state);

    struct {
        u32 IPS;
        u32 FPS;
    } clock;

    chip8_variant variant;
    chip8_status  runState;
    chip8_error   errState;
    bool          drawFlag;
    bool          extMode;   


    struct {
        //state description
        u8  MEM[4096];   //Working RAM
        u8  FB[FB_SIZE]; //Framebuffer
        u16 STACK[16];   //Stack
        u8  V[16];
        u16 I;           //V0...VF, I regs.
        u8  KP[16];      //Keypad
        u16 PC;          //Program counter
        u8  DT, ST, SP;  //Timers, stack pointer
    } state;
    

} chip8;

void memdump(chip8* chip8_state, const char *filename);
void chip8Init(chip8* chip8_state, chip8_variant variant, const char* romFilename);
