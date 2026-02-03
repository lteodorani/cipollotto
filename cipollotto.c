#include "cipollotto.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdint.h>
#include <string.h>

#define MIN(a, b) (((a) <= (b)) ? (a) : (b))
#define MAX(a, b) (((a) => (b)) ? (a) : (b))

static const uint8_t font_data[FONT_SET_SIZE] = {
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

uint32_t LCG_rand(void)
{
    //linear congruent generator (LCG)
    //a and b taken from 'Numerical Recipes'
    const  uint32_t a = 1664525;   
    const  uint32_t b = 1013904223;
    static uint32_t X = 1;
    X = a * X + b; // implicit mod 2^32 via overflow 
    return X;
}

// Draws an n-byte sprite from memory[I] at screen coordinates (Vx, Vy).
static void spriteToFB(chip8* chip8_state, uint8_t n, uint16_t x, uint16_t y)
{
    uint16_t screen_x = x % SCREEN_WIDTH;      // wrapped start X
    uint16_t screen_y = y % SCREEN_HEIGHT;     // wrapped start Y
    uint8_t  any_collision = 0;

    uint16_t bit_shift = screen_x % 8;         // pixel offset inside a framebuffer byte
    uint16_t visible_bits = MIN(screen_x + 8, SCREEN_WIDTH) - screen_x;
    uint8_t  clip_mask_x = (0xFF << (8 - visible_bits));

    for (uint16_t row = 0; row < n; ++row) {

        // --- Y clipping (classic CHIP-8) ---
        if ((screen_y + row) >= SCREEN_HEIGHT) break;
        uint16_t target_y = screen_y + row;

        // --- Compute framebuffer byte indices ---
        uint16_t fb_byte_x = screen_x / 8;
        uint16_t fb_index_primary   = BYTES_PER_ROW * target_y + fb_byte_x;
        uint16_t fb_index_secondary = BYTES_PER_ROW * target_y + ((fb_byte_x + 1) % BYTES_PER_ROW);

        // --- Fetch current framebuffer bytes ---
        uint8_t fb_byte_primary   = FB[fb_index_primary];
        uint8_t fb_byte_secondary = FB[fb_index_secondary];

        // --- Fetch sprite row bits and apply right-edge clipping ---
        uint8_t sprite_row_bits = MEM[(I + row) & 0x0FFF] & clip_mask_x;

        if (bit_shift != 0) {
            // Handle byte alignment and bit spill
            uint8_t spill_mask   = sprite_row_bits & ((1 << bit_shift) - 1);
            uint8_t spill_shifted = spill_mask << (8 - bit_shift);

            // Detect collision
            any_collision |= (
                (fb_byte_primary   & (sprite_row_bits >> bit_shift)) |
                (fb_byte_secondary & spill_shifted)
            ) != 0;

            // XOR sprite bits into framebuffer
            FB[fb_index_primary]   ^= sprite_row_bits >> bit_shift;
            FB[fb_index_secondary] ^= spill_shifted;

        } else {
            // Aligned case — all 8 bits fit in one byte
            any_collision |= (fb_byte_primary & sprite_row_bits) != 0;
            FB[fb_index_primary] ^= sprite_row_bits;
        }
    }

    V[0xF] = any_collision;
}


void chip8Init(chip8* chip8_state){
    //initialization
    memset(chip8_state, 0, sizeof(chip8));
    memcpy(MEM+FONT_START_ADDR, font_data, FONT_SET_SIZE);
    PC = PROG_START_ADDR;
    SP = 0;
    chip8_state->errState = ERR_OK;
    running = STATUS_RUNNING;
}

void chip8Step(chip8 *chip8_state){
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
    uint16_t n   =  opcode & 0x000F;
    uint16_t kk  =  opcode & 0x00FF;
    uint16_t nnn =  opcode & 0x0FFF;
    uint16_t y   = (opcode & 0x00F0) >> 4;
    uint16_t x   = (opcode & 0x0F00) >> 8;
    uint16_t h   = (opcode & 0xF000) >> 12;

    switch(h)
    {
        case 0x0:
            switch(nnn) {
                case 0x0E0:
                    memset(FB, 0x00, sizeof(FB));
                    chip8_state->drawFlag = true;
                    break;
                case 0x0EE: //ret from subroutine
                    if(SP>0){
                        SP--;
                        PC = STACK[SP];
                    }
                    else {
                        chip8_state->errState = ERR_STACK_UNDERFLOW;
                        running = STATUS_PAUSED;
                    }
                    break;
                    
                default:
                    chip8_state->errState = ERR_IGNORED_OPCODE;
                    break;
            }
            break;
            
        case 0x1:
            //jump to nnn
            PC = nnn;
            break;
        
        case 0x2:
            //call subroutine at nnn
            //increment SP, then put current PC at the top. Then PC is set to nnn.
            if(SP < STACK_SIZE_UINT16 - 1){
                STACK[SP] = PC;
                SP++;
                PC = nnn;
            }else {
                chip8_state->errState = ERR_STACK_OVERFLOW;
                running = STATUS_PAUSED;
            }
            break;
        case 0x3:
            //Skips the next instruction if VX equals NN 
            PC += (V[x] == kk) * 2;
            break;
        case 0x4:
            //Skips the next instruction if VX does not equal NN 
            PC += (V[x] != kk) * 2;
            break;

        case 0x5:
            //Skips the next instruction if VX equals VY
            PC += (V[x] == V[y]) * 2;
            break;

        case 0x6:
            //Sets VX to kk.
            V[x] = kk & 0x00FF;
            break;

        case 0x7:
            //Adds NN to VX (carry flag is not changed).
            V[x] = (V[x] + kk) & 0x00FF;
            break;

        case 0x8:
            //Sets VX to the value of VY.
            switch (n) {
                case 0x0:
                    V[x] = V[y];
                    break;

                case 0x1:
                    V[ x ] |=  V[y];
                    V[0xF] = 0;
                    break;

                case 0x2:
                    V[x] &=  V[y];
                    V[0xF] = 0;
                    break;

                case 0x3:
                    V[x] ^= V[y];
                    V[0xF] = 0;
                    break;

                case 0x4:{
                    uint16_t sum16 = V[x] + V[y];
                    V[ x ] = sum16 & 0x00FF;
                    V[0xF] = sum16 >> 8;
                    break;}

                case 0x5:{
                    uint16_t sub16 = V[x] - V[y];
                    V[ x ] = sub16 & 0x00FF;
                    V[0xF] = (~(sub16 >> 8) & 1);
                    break;}

                case 0x6:{
                    uint16_t spill = V[y] &  1;
                    V[ x ] = V[y] >> 1;
                    V[0xF] = spill;
                    break;}

                case 0x7:{
                    uint16_t sub16 = V[y] - V[x];
                    V[ x ] = sub16 & 0x00FF;
                    V[0xF] = (~(sub16 >> 8) & 1);
                    break;}

                case 0xE:{
                    uint16_t spill = (V[y] & 0x0080) >> 7;
                    V[ x ] = (V[y] << 1) & 0x00FF;
                    V[0xF] = spill;
                    break;}

            }
            break;
        case 0x9:
            if(V[x]!=V[y])
                PC+=2;
            break;
        case 0xA:
            I = nnn;
            break;
        case 0xB:
            PC = nnn + V[0];
            break;
        case 0xC:
            rnd = (uint16_t)(LCG_rand()>>16);
            V[x] = rnd & kk;
            break;
        case 0xD:
            //display n bytes sprite starting at addr stored in I at coords VX,VY
            spriteToFB(chip8_state, n, V[x], V[y]);
            chip8_state->drawFlag = true;
            break;

        case 0xE:
            if(kk == 0x9E){
                if(KP[V[x & 0xF]])
                    PC+=2;
            }
            
            if(kk == 0xA1){
                if(!(KP[V[x] & 0xF]))
                    PC+=2;
                
                    
            }
            break;

        
        case 0xF:
            switch (kk) {
                case 0x07:
                    V[x] = (uint16_t)DT;
                    break;


                case 0x0A:{
                    bool key_pressed = false;
                    for(int i=0; i<16; i++){
                        if(KP[i]){
                            key_pressed = true;
                            V[x] = (uint16_t)i;
                            break;
                        }
                    }
                    if(!key_pressed) {PC-=2;}
                    break;
                }


                case 0x15:
                    DT = (uint8_t)V[x];
                    break;
                case 0x18:
                    ST = (uint8_t)V[x];
                    break;
                case 0x1E:
                    I += V[x] & 0xFF;
                    break;
                case 0x29:
                    //I is set to location of sprite corresponding to value of Vx.
                    I = V[x] * 5;
                    break;
                case 0x33:
                    if(I <= MEMORY_END - 3){
                        MEM[I  ] =  V[x] / 100;
                        MEM[I+1] = (V[x] / 10) % 10;
                        MEM[I+2] =  V[x] % 10;
                    }
                    else {
                        chip8_state->errState = ERR_MEM_OUT_OF_BOUNDS;
                        running = STATUS_PAUSED;
                    }
                    break;
                case 0x55:
                    if(I <= MEMORY_END - x){
                        for(uint16_t j=0; j<=x; ++j)
                            MEM[I++] = (uint8_t)(V[j]);
                    }
                    else {
                        chip8_state->errState = ERR_MEM_OUT_OF_BOUNDS;
                        running = STATUS_PAUSED;
                    }
                    break;
                case 0x65:
                    if(I <= MEMORY_END - x){
                        for(uint16_t j=0; j<=x; ++j)
                            V[j] = MEM[I++];
                    }
                    else {
                        chip8_state->errState = ERR_MEM_OUT_OF_BOUNDS;
                        running = STATUS_PAUSED;
                    }
                    break;







                
            }
            break;

        default:
            chip8_state->errState = ERR_UNKNOWN_OPCODE;
            break;
    }


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