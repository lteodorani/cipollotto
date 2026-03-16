//TODO: Use uint16_t for intermediate results, not the register array itself.


#include "cipollotto.h"
#include "opcodes.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define MIN(a, b) (((a) <= (b)) ? (a) : (b))
#define MAX(a, b) (((a) => (b)) ? (a) : (b))

static const chip8_options chipOptionsVanilla = {.IPS = 900, .FPS = 60};
static const chip8_options chipOptionsSchip   = {.IPS = 960, .FPS = 64};

static const uint8_t small_font_data[FONT_SMALL_SIZE] = {
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
    0xF0, 0x80, 0xF0, 0x80, 0x80, // F
};

static const uint8_t large_font_data[FONT_LARGE_SIZE] = {
    0x3C, 0x7E, 0xE7, 0xC3, 0xC3, 0xC3, 0xC3, 0xE7, 0x7E, 0x3C, // big 0 
    0x18, 0x38, 0x58, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, // big 1
    0x3E, 0x7F, 0xC3, 0x06, 0x0C, 0x18, 0x30, 0x60, 0xFF, 0xFF, // big 2
    0x3C, 0x7E, 0xC3, 0x03, 0x0E, 0x0E, 0x03, 0xC3, 0x7E, 0x3C, // big 3
    0x06, 0x0E, 0x1E, 0x36, 0x66, 0xC6, 0xFF, 0xFF, 0x06, 0x06, // big 4
    0xFF, 0xFF, 0xC0, 0xC0, 0xFC, 0xFE, 0x03, 0xC3, 0x7E, 0x3C, // big 5
    0x3E, 0x7C, 0xE0, 0xC0, 0xFC, 0xFE, 0xC3, 0xC3, 0x7E, 0x3C, // big 6
    0xFF, 0xFF, 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x60, 0x60, // big 7
    0x3C, 0x7E, 0xC3, 0xC3, 0x7E, 0x7E, 0xC3, 0xC3, 0x7E, 0x3C, // big 8
    0x3C, 0x7E, 0xC3, 0xC3, 0x7F, 0x3F, 0x03, 0x03, 0x3E, 0x7C  // big 9
};

#define MEM      (chip8_state->MEM)
#define STACK    (chip8_state->STACK)
#define V        (chip8_state->V)
#define FB       (chip8_state->FB)
#define DT       (chip8_state->DT)
#define ST       (chip8_state->ST)
#define I        (chip8_state->I)
#define PC       (chip8_state->PC)
#define SP       (chip8_state->SP)
#define running  (chip8_state->running)
#define rnd      (chip8_state->rnd)
#define KP       (chip8_state->KP)



void chip8Step(chip8* chip8_state){
    //fetch opcode taking care of endianess. chip-8 is big endian,
    //but x86 is little endian, requiring to read the two bytes of
    //the 16-bit opcodes separatedly
    
    uint16_t opcode = (MEM[PC] << 8) | (MEM[PC+1]);
    PC+=2;
    if(PC < MEMORY_START || PC > MEMORY_END){
        chip8_state->errState = ERR_MEM_OUT_OF_BOUNDS;
        running = STATUS_STOPPED;
    }
    //dispatch
    uint16_t h   = (opcode & 0xF000) >> 12;

    switch(h)
    {
        case 0x0: {handle_0nnn(chip8_state, opcode); break;}
        case 0x1: {op_jp (chip8_state, opcode);      break;}
        case 0x2: {op_call(chip8_state, opcode);     break;}
        case 0x3: {op_se_byte (chip8_state, opcode); break;}
        case 0x4: {op_sne_byte(chip8_state, opcode); break;}
        case 0x5: {op_se_reg (chip8_state, opcode);  break;}
        case 0x6: {op_ld_byte(chip8_state, opcode);  break;}
        case 0x7: {op_add_byte(chip8_state, opcode); break;}
        case 0x8: {handle_8xyn(chip8_state, opcode); break;}
        case 0x9: {op_sne_reg(chip8_state, opcode);  break;}
        case 0xA: {op_ld_I(chip8_state, opcode);     break;}
        case 0xB: {op_jp_v0(chip8_state, opcode);    break;}
        case 0xC: {op_rnd(chip8_state, opcode);      break;}
        case 0xD: {op_drw(chip8_state, opcode);      break;}
        case 0xE: {handle_Exnn(chip8_state, opcode); break;}        
        case 0xF: {handle_Fxnn(chip8_state, opcode); break;}

        default:
            chip8_state->errState = ERR_UNKNOWN_OPCODE;
            running = STATUS_STOPPED;
            break;
    }


}

void schipStep(chip8* chip8_state){
    //fetch opcode taking care of endianess. chip-8 is big endian,
    //but x86 is little endian, requiring to read the two bytes of
    //the 16-bit opcodes separatedly
    uint16_t opcode = (MEM[PC] << 8) | (MEM[PC+1]);
    PC+=2;
    if(PC < MEMORY_START || PC > MEMORY_END){
        chip8_state->errState = ERR_MEM_OUT_OF_BOUNDS;
        running = STATUS_PAUSED;
    }
    //dispatch
    uint16_t h   = (opcode & 0xF000) >> 12;

    switch(h)
    {
        case 0x0: {handle_00_schip(chip8_state, opcode);     break;}
        case 0x1: {op_jp (chip8_state, opcode);              break;}
        case 0x2: {op_call(chip8_state, opcode);             break;}
        case 0x3: {op_se_byte (chip8_state, opcode);         break;}
        case 0x4: {op_sne_byte(chip8_state, opcode);         break;}
        case 0x5: {op_se_reg (chip8_state, opcode);          break;}
        case 0x6: {op_ld_byte(chip8_state, opcode);          break;}
        case 0x7: {op_add_byte(chip8_state, opcode);         break;}
        case 0x8: {handle_8xyn_schip(chip8_state, opcode);   break;}
        case 0x9: {op_sne_reg(chip8_state, opcode);          break;}
        case 0xA: {op_ld_I(chip8_state, opcode);             break;}
        case 0xB: {op_jp_vx(chip8_state, opcode);            break;}
        case 0xC: {op_rnd(chip8_state, opcode);              break;}
        case 0xD: {op_drw_schip(chip8_state, opcode);        break;}
        case 0xE: {handle_Exnn(chip8_state, opcode);         break;}        
        case 0xF: {handle_Fxnn_schip(chip8_state, opcode);   break;}

        default:
            chip8_state->errState = ERR_UNKNOWN_OPCODE;
            running = STATUS_STOPPED;
            break;
    }
}

//initialization
void chip8Init(chip8* chip8_state, chip8_variant variant){
    
    memset(chip8_state, 0, sizeof(chip8));
    PC = PROG_START_ADDR;
    SP = 0;

    if(variant == VARIANT_SCHIP){
        memcpy(&MEM[FONT_SMALL_ADDR], small_font_data, FONT_SMALL_SIZE);
        memcpy(&MEM[FONT_LARGE_ADDR], large_font_data, FONT_LARGE_SIZE);
        chip8_state->chipOptions = chipOptionsSchip;
        chip8_state->opExecute   = schipStep;
    }
        
    else{
        memcpy(&MEM[FONT_SMALL_ADDR], small_font_data, FONT_SMALL_SIZE);
        chip8_state->chipOptions = chipOptionsVanilla;
        chip8_state->opExecute   = chip8Step;
    }

    chip8_state->errState = ERR_OK;
    running = STATUS_RUNNING;
}


void memdump(chip8* chip8_state, const char *filename){
    FILE* of = fopen(filename, "w");
    fwrite(MEM, sizeof(MEM), 1, of);
    fclose(of);
}


#undef MEM
#undef STACK
#undef V
#undef FB
#undef DT
#undef ST
#undef I
#undef PC
#undef SP
#undef running
#undef rnd
#undef KP