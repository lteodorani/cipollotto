//TODO: Use u16 for intermediate results, not the register array itself.


#include "cipollotto.h"
#include "cipollotto_opcodes.h"
#include "common.h"
#include <stdio.h>
#include <string.h>


static const u8 small_font_data[FONT_SMALL_SIZE] = {
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

static const u8 large_font_data[FONT_LARGE_SIZE] = {
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

#define MEM      (c8->state.MEM)
#define STACK    (c8->state.STACK)
#define V        (c8->state.V)
#define FB       (c8->state.FB)
#define DT       (c8->state.DT)
#define ST       (c8->state.ST)
#define I        (c8->state.I)
#define PC       (c8->state.PC)
#define SP       (c8->state.SP)
#define KP       (c8->state.KP)




void chip8Step(chip8* c8){
    //fetch opcode taking care of endianess. chip-8 is big endian,
    //but x86 is little endian
    
    u16 opcode = (MEM[PC] << 8) | (MEM[PC+1]);
    PC+=2;
    if(PC < MEMORY_START || PC > MEMORY_END){
        c8->errState = CIPOLLOTTO_ERR_MEMORY_OOB;
        c8->runState = CIPOLLOTTO_STATUS_HALTED;
    }
    //dispatch
    u16 h   = (opcode & 0xF000) >> 12;

    switch(h)
    {
        case 0x0: {handle_0nnn(c8, opcode); break;}
        case 0x1: {op_jp (c8, opcode);      break;}
        case 0x2: {op_call(c8, opcode);     break;}
        case 0x3: {op_se_byte (c8, opcode); break;}
        case 0x4: {op_sne_byte(c8, opcode); break;}
        case 0x5: {op_se_reg (c8, opcode);  break;}
        case 0x6: {op_ld_byte(c8, opcode);  break;}
        case 0x7: {op_add_byte(c8, opcode); break;}
        case 0x8: {handle_8xyn(c8, opcode); break;}
        case 0x9: {op_sne_reg(c8, opcode);  break;}
        case 0xA: {op_ld_I(c8, opcode);     break;}
        case 0xB: {op_jp_v0(c8, opcode);    break;}
        case 0xC: {op_rnd(c8, opcode);      break;}
        case 0xD: {op_drw(c8, opcode);      break;}
        case 0xE: {handle_Exnn(c8, opcode); break;}        
        case 0xF: {handle_Fxnn(c8, opcode); break;}

        default:
            c8->errState = CIPOLLOTTO_ERR_INVALID_OPCODE;
            c8->runState = CIPOLLOTTO_STATUS_HALTED;
            break;
    }


}

void schipStep(chip8* c8){
    //fetch opcode taking care of endianess. chip-8 is big endian,
    //but x86 is little endian, requiring to read the two bytes of
    //the 16-bit opcodes separatedly
    u16 opcode = (MEM[PC] << 8) | (MEM[PC+1]);
    PC+=2;
    if(PC < MEMORY_START || PC > MEMORY_END){
        c8->errState = CIPOLLOTTO_ERR_MEMORY_OOB;
        c8->runState = CIPOLLOTTO_STATUS_HALTED;
    }
    //dispatch
    u16 h   = (opcode & 0xF000) >> 12;

    switch(h)
    {
        case 0x0: {handle_00_schip(c8, opcode);     break;}
        case 0x1: {op_jp (c8, opcode);              break;}
        case 0x2: {op_call(c8, opcode);             break;}
        case 0x3: {op_se_byte (c8, opcode);         break;}
        case 0x4: {op_sne_byte(c8, opcode);         break;}
        case 0x5: {op_se_reg (c8, opcode);          break;}
        case 0x6: {op_ld_byte(c8, opcode);          break;}
        case 0x7: {op_add_byte(c8, opcode);         break;}
        case 0x8: {handle_8xyn_schip(c8, opcode);   break;}
        case 0x9: {op_sne_reg(c8, opcode);          break;}
        case 0xA: {op_ld_I(c8, opcode);             break;}
        case 0xB: {op_jp_vx(c8, opcode);            break;}
        case 0xC: {op_rnd(c8, opcode);              break;}
        case 0xD: {op_drw_schip(c8, opcode);        break;}
        case 0xE: {handle_Exnn(c8, opcode);         break;}        
        case 0xF: {handle_Fxnn_schip(c8, opcode);   break;}

        default:
            c8->errState = CIPOLLOTTO_ERR_INVALID_OPCODE;
            c8->runState = CIPOLLOTTO_STATUS_HALTED;
            break;
    }
}

//initialization
void chip8Init(chip8* c8, chip8_variant variant, const char* romFilename){
    

    memset(c8, 0, sizeof(chip8));

    PC = PROG_START_ADDR;
    SP = 0;
    
    if(variant == CIPOLLOTTO_VARIANT_SCHIP){
        memcpy(&MEM[FONT_SMALL_ADDR], small_font_data, FONT_SMALL_SIZE);
        memcpy(&MEM[FONT_LARGE_ADDR], large_font_data, FONT_LARGE_SIZE);
        c8->clock.IPS = 960;
        c8->clock.FPS = 64;
        c8->opExecute = schipStep;
    }
        
    else{
        memcpy(&MEM[FONT_SMALL_ADDR], small_font_data, FONT_SMALL_SIZE);
        c8->clock.IPS = 900;
        c8->clock.FPS = 60;
        c8->opExecute = chip8Step;
    }
    c8->variant = variant;
    c8->errState = CIPOLLOTTO_OK;
    c8->runState = CIPOLLOTTO_STATUS_RUNNING;


    FILE* fROM = fopen(romFilename, "rb");
    if(fROM == NULL){
        printf("Error occured opening %s\n", romFilename);
        return;
    }

    fseek(fROM, 0, SEEK_END);
    long fsize = ftell(fROM);
    fseek(fROM, 0, SEEK_SET);
    if(fsize > (MEMORY_END - PROG_START_ADDR)){
        printf("Provided ROM exceeds chip-8 memory limit of 4kB.\n");
        return;
    }
    fread(MEM+PROG_START_ADDR, 1, fsize, fROM);
    fclose(fROM);

}


void memdump(chip8* c8, const char* filename){
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
#undef KP