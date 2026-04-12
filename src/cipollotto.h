#pragma once
#include <stdint.h>
#include <stdbool.h>

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

typedef struct{
    uint64_t IPS;
    uint64_t FPS;
} chip8_options;



typedef struct chip8{
    //virtual machine metadata
    chip8_variant chipVariant;
    chip8_options chipOptions;

    //state description
    uint8_t  MEM[4096];   //Working RAM
    uint8_t  FB[FB_SIZE]; //Framebuffer
    uint16_t STACK[16];   //Stack

    // TODO: convert to u8 opcodes.h
        uint16_t V[16];
        uint16_t I;           //V0...VF, I regs. V regs really should 8 but i store them in 16 bit container
    
        uint8_t  KP[16];      //Keypad
    uint8_t  DT, ST, SP;  //Timers, stack pointer
    uint16_t PC;          //Program counter

    
    // runtime configuration
    chip8_error    errState;
    bool           drawFlag;
    bool           extendedMode;
    chip8_status   running;


    void (*opExecute)(struct chip8* chip8_state);
    

} chip8;

void memdump(chip8* chip8_state, const char *filename);
void chip8Init(chip8* chip8_state, chip8_variant variant);
