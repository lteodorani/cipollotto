//private header. This should ever only be
//included by cipollotto.c

#pragma once
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "cipollotto.h"


#define MIN(a, b) (((a) <= (b)) ? (a) : (b))
#define MAX(a, b) (((a) => (b)) ? (a) : (b))

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

inline static uint32_t LCG_rand(void){
    //linear congruent generator (LCG)
    //a and b taken from 'Numerical Recipes'
    const  uint32_t a = 1664525;   
    const  uint32_t b = 1013904223;
    static uint32_t X = 1;
    X = a * X + b; // implicit mod 2^32 via overflow 
    return X;
}


// Helper for draw operations in lores mode.
// Duplicates bits in place. Adapted from:
// https://graphics.stanford.edu/~seander/bithacks.html#InterleaveBMN
//
// Example:
// the uint8_t 0b10110110 becomes uint16_t 0b1100111100111100
inline static uint16_t dupeBitsInPlace(uint8_t n){
    uint16_t x = n;
    x = (x | (x << 4)) & 0x0F0F0F0F;
    x = (x | (x << 2)) & 0x33333333;
    x = (x | (x << 1)) & 0x55555555;
    return x | (x << 1);
}


inline static uint16_t drw_hires(chip8* chip8_state, uint16_t originX, uint16_t originY, uint16_t n){
    uint16_t offsetX = originX % 8; // pixel offset inside a byte
    uint16_t idx_byte_of_x = originX / 8;
    uint16_t visible_bits = MIN(originX + 8, SCREEN_WIDTH) - originX;
    uint8_t  clip_mask_x = (0xFF << (8 - visible_bits));
    uint16_t collision = 0;
    uint16_t rowN = (n == 0) ? 16 : n;
    for (uint16_t row = 0; row < rowN; ++row) {

        // --- Y clipping (classic CHIP-8) ---
        uint16_t offsetY = originY + row;
        if (offsetY >= SCREEN_HEIGHT) break;
        
        // --- Compute framebuffer byte indices ---
        uint16_t fb_idx_l = BYTES_PER_ROW * offsetY + idx_byte_of_x;
        uint16_t fb_idx_r = BYTES_PER_ROW * offsetY + ((idx_byte_of_x + 1) % BYTES_PER_ROW);

        // --- Fetch current framebuffer bytes ---
        uint8_t  fb_byte_l = FB[fb_idx_l];
        uint8_t  fb_byte_r = FB[fb_idx_r];
        uint32_t sprite_bits32;
        uint8_t  sprite_l;
        uint8_t  sprite_r;
        // --- Fetch sprite row bits and apply right-edge screen clipping ---
        // Handle byte alignment and bit spill
        if(n!=0){
            uint8_t sprite_bits = MEM[(I + row) & 0x0FFF] & clip_mask_x;
            sprite_bits32 = (uint32_t)(sprite_bits) << (8 - offsetX);
            sprite_l      = (uint8_t)(sprite_bits32 >> 8);
            sprite_r      = (uint8_t)(sprite_bits32);
            printf("aa\n");
        }
        else {
            //dxy0 in hires mode draws 16x16 sprites
            sprite_l = MEM[(I + 2*row    ) & 0x0FFF];
            sprite_r = MEM[(I + 2*row + 1) & 0x0FFF] & clip_mask_x;
            sprite_bits32 = (((uint16_t)sprite_l) << 8) | ((uint16_t)sprite_r);

            sprite_bits32 = (uint32_t)(sprite_bits32) << (8 - offsetX);
            sprite_l      = (uint8_t)(sprite_bits32 >> 16);
            sprite_r      = (uint8_t)(sprite_bits32 >>  8);
            //sprite_r = (uint8_t)(sprite_bits32);
        }


        // Detect collision
        collision |= (
            (fb_byte_l & sprite_l) | 
            (fb_byte_r & sprite_r)
        ) != 0;

        // XOR sprite bits into framebuffer
        FB[fb_idx_l] ^= sprite_l;
        FB[fb_idx_r] ^= sprite_r;
    }

    return collision;

}


inline static uint16_t drw_lores(chip8* chip8_state, uint16_t originX, uint16_t originY, uint16_t n){
    uint16_t offsetX = originX % 8; // pixel offset a byte
    uint16_t idx_byte_of_x = originX / 8;
    uint16_t visible_bits = MIN(originX + 8, SCREEN_WIDTH) - originX;
    uint16_t  clip_mask_x = (0xFFFFu << (16 - visible_bits));
    uint16_t collision = 0;
    uint16_t rowN = (n == 0) ? 16 : n;
    for (uint16_t row = 0; row < rowN; ++row) {
        
        // --- Y clipping (classic CHIP-8) ---
        uint16_t offsetY = originY + 2*row;
        if (offsetY >= SCREEN_HEIGHT) break;
        
        // --- Compute framebuffer byte indices ---
        uint16_t fb_idx_tl = BYTES_PER_ROW * offsetY + idx_byte_of_x;
        uint16_t fb_idx_tc = BYTES_PER_ROW * offsetY + ((idx_byte_of_x + 1) % BYTES_PER_ROW);
        uint16_t fb_idx_tr = BYTES_PER_ROW * offsetY + ((idx_byte_of_x + 2) % BYTES_PER_ROW);
        
        uint16_t fb_idx_bl = BYTES_PER_ROW * (offsetY + 1) + idx_byte_of_x;
        uint16_t fb_idx_bc = BYTES_PER_ROW * (offsetY + 1) + ((idx_byte_of_x + 1) % BYTES_PER_ROW);
        uint16_t fb_idx_br = BYTES_PER_ROW * (offsetY + 1) + ((idx_byte_of_x + 2) % BYTES_PER_ROW);

        // --- Fetch current framebuffer bytes ---
        uint8_t fb_byte_l = FB[fb_idx_tl];
        uint8_t fb_byte_c = FB[fb_idx_tc];
        uint8_t fb_byte_r = FB[fb_idx_tr];

        // --- Fetch sprite row bits and apply right-edge screen clipping ---
        uint16_t sprite_bits = dupeBitsInPlace(MEM[(I + row) & 0x0FFF]) & clip_mask_x;

        uint32_t sprite_bits32 = (uint32_t)(sprite_bits) << (8 - offsetX);
        uint8_t sprite_l = (uint8_t)(sprite_bits32 >> 16);
        uint8_t sprite_c = (uint8_t)(sprite_bits32 >>  8);
        uint8_t sprite_r = (uint8_t)(sprite_bits32);
        
        // Detect collision
        collision |= (
            (fb_byte_l & sprite_l) |
            (fb_byte_c & sprite_c) | 
            (fb_byte_r & sprite_r)
        ) != 0;

        // XOR sprite bits into framebuffer,
        // then copy bottom subpixel rows
        FB[fb_idx_tl] ^= sprite_l; 
        FB[fb_idx_tc] ^= sprite_c;
        FB[fb_idx_tr] ^= sprite_r;
        FB[fb_idx_bl]  = FB[fb_idx_tl];
        FB[fb_idx_bc]  = FB[fb_idx_tc];
        FB[fb_idx_br]  = FB[fb_idx_tr];
    }

    return collision;
}

static inline void op_drw_schip11(chip8* chip8_state, uint16_t opcode){
    uint16_t n   =  opcode & 0x000F;
    uint16_t y   = (opcode & 0x00F0) >> 4;
    uint16_t x   = (opcode & 0x0F00) >> 8;

    uint16_t collision = 0;

    //draw hires or DXY0
    if(chip8_state->extendedMode){
        uint16_t originX = V[x] % SCREEN_WIDTH;
        uint16_t originY = V[y] % SCREEN_HEIGHT;
        collision = drw_hires(chip8_state, originX, originY, n);
    }

    // draw lores
    else {
        uint16_t originX = 2*V[x] % SCREEN_WIDTH;      // scaled and wrapped start X
        uint16_t originY = 2*V[y] % SCREEN_HEIGHT;     // scaled wrapped start Y
        collision = drw_lores(chip8_state, originX, originY, n);
        printf("colls: %d\n", collision);
    }

    V[0xF] = collision;    
    chip8_state->drawFlag = true;
}

/*
inline static void op_drw_schip11(chip8* chip8_state, uint16_t opcode){
    uint16_t n   =  opcode & 0x000F;
    uint16_t y   = (opcode & 0x00F0) >> 4;
    uint16_t x   = (opcode & 0x0F00) >> 8;

    uint16_t scale    = (chip8_state->extendedMode == true) ? 1 : 2;
    uint16_t originX = scale*V[x] % SCREEN_WIDTH;      // scaled and wrapped start X
    uint16_t originY = scale*V[y] % SCREEN_HEIGHT;     // scaled wrapped start Y
    uint8_t  collision = 0;

    uint16_t offsetX = originX % 8; // pixel offset a byte
    uint16_t idx_byte_of_x = originX / 8;
    uint16_t visible_bits = MIN(originX + 8, SCREEN_WIDTH) - originX;
    uint16_t  clip_mask_x = (0xFFFFu << (16 - visible_bits));

    for (uint16_t row = 0; row < n; ++row) {

        // --- Y clipping (classic CHIP-8) ---
        uint16_t offsetY = originY + scale*row;
        if (offsetY >= SCREEN_HEIGHT) break;
        
        // --- Compute framebuffer byte indices ---
        uint16_t fb_idx_tl = BYTES_PER_ROW * offsetY + idx_byte_of_x;
        uint16_t fb_idx_tc = BYTES_PER_ROW * offsetY + ((idx_byte_of_x + 1) % BYTES_PER_ROW);
        uint16_t fb_idx_tr = BYTES_PER_ROW * offsetY + ((idx_byte_of_x + 2) % BYTES_PER_ROW);
        
        uint16_t fb_idx_bl = BYTES_PER_ROW * (offsetY + 1) + idx_byte_of_x;
        uint16_t fb_idx_bc = BYTES_PER_ROW * (offsetY + 1) + ((idx_byte_of_x + 1) % BYTES_PER_ROW);
        uint16_t fb_idx_br = BYTES_PER_ROW * (offsetY + 1) + ((idx_byte_of_x + 2) % BYTES_PER_ROW);

        // --- Fetch current framebuffer bytes ---
        uint8_t fb_byte_l = FB[fb_idx_tl];
        uint8_t fb_byte_c = FB[fb_idx_tc];
        uint8_t fb_byte_r = FB[fb_idx_tr];

        // --- Fetch sprite row bits and apply right-edge screen clipping ---
        uint16_t sprite_bits = dupeBitsInPlace(MEM[(I + row) & 0x0FFF]) & clip_mask_x;

        uint32_t sprite_bits32 = (uint32_t)(sprite_bits) << (8 - offsetX);
        uint8_t sprite_l = (uint8_t)(sprite_bits32 >> 16);
        uint8_t sprite_c = (uint8_t)(sprite_bits32 >>  8);
        uint8_t sprite_r = (uint8_t)(sprite_bits32);
        
        // Detect collision
        collision |= (
            (fb_byte_l & (uint8_t)(sprite_bits32 >> 16)) |
            (fb_byte_c & (uint8_t)(sprite_bits32 >>  8)) | 
            (fb_byte_r & (uint8_t)(sprite_bits32))
        ) != 0;

        // XOR sprite bits into framebuffer,
        // then copy bottom subpixel rows
        FB[fb_idx_tl] ^= sprite_l; 
        FB[fb_idx_tc] ^= sprite_c;
        FB[fb_idx_tr] ^= sprite_r;
        FB[fb_idx_bl]  = FB[fb_idx_tl];
        FB[fb_idx_bc]  = FB[fb_idx_tc];
        FB[fb_idx_br]  = FB[fb_idx_tr];
    }

    V[0xF] = collision;
    //printf("%d\n", collision);
    chip8_state->drawFlag = true;
}
*/

/*
inline static void op_drw_schip11(chip8* chip8_state, uint16_t opcode){
    uint16_t n   =  opcode & 0x000F;
    uint16_t y   = (opcode & 0x00F0) >> 4;
    uint16_t x   = (opcode & 0x0F00) >> 8;

    uint16_t scale    = (chip8_state->extendedMode == true) ? 1 : 2;
    uint16_t originX = scale*V[x] % SCREEN_WIDTH;      // scaled and wrapped start X
    uint16_t originY = scale*V[y] % SCREEN_HEIGHT;     // scaled wrapped start Y
    uint8_t  collision = 0;

    uint16_t offsetX = originX % 8; // pixel offset inside a framebuffer byte
    uint16_t visible_bits = MIN(originX + 8, SCREEN_WIDTH) - originX;
    uint16_t  clip_mask_x = (0xFFFFu << (16 - visible_bits));

    for (uint16_t row = 0; row < n; ++row) {

        // --- Y clipping (classic CHIP-8) ---
        uint16_t offsetY = originY + scale*row;
        if (offsetY >= SCREEN_HEIGHT) break;
        
        // --- Compute framebuffer byte indices ---
        uint16_t idx_byte_of_x = originX / 8;
        uint16_t fb_idx_tl = BYTES_PER_ROW * offsetY + idx_byte_of_x;
        uint16_t fb_idx_tc = BYTES_PER_ROW * offsetY + ((idx_byte_of_x + 1) % BYTES_PER_ROW);
        uint16_t fb_idx_tr = BYTES_PER_ROW * offsetY + ((idx_byte_of_x + 2) % BYTES_PER_ROW);
        
        uint16_t fb_idx_bl = BYTES_PER_ROW * (offsetY + 1) + idx_byte_of_x;
        uint16_t fb_idx_bc = BYTES_PER_ROW * (offsetY + 1) + ((idx_byte_of_x + 1) % BYTES_PER_ROW);
        uint16_t fb_idx_br = BYTES_PER_ROW * (offsetY + 1) + ((idx_byte_of_x + 2) % BYTES_PER_ROW);

        // --- Fetch current framebuffer bytes ---
        uint8_t fb_byte_l = FB[fb_idx_tl];
        uint8_t fb_byte_c = FB[fb_idx_tc];
        uint8_t fb_byte_r = FB[fb_idx_tr];

        // --- Fetch sprite row bits and apply right-edge screen clipping ---
        uint16_t sprite_bits = dupeBitsInPlace(MEM[(I + row) & 0x0FFF]) & clip_mask_x;

        uint32_t sprite_bits32 = (uint32_t)(sprite_bits) << (8 - offsetX);
        uint8_t sprite_l = (uint8_t)(sprite_bits32 >> 16);
        uint8_t sprite_c = (uint8_t)(sprite_bits32 >>  8);
        uint8_t sprite_r = (uint8_t)(sprite_bits32);
        
        // Detect collision
        collision |= (
            (fb_byte_l & (uint8_t)(sprite_bits32 >> 16)) |
            (fb_byte_c & (uint8_t)(sprite_bits32 >>  8)) | 
            (fb_byte_r & (uint8_t)(sprite_bits32))
        ) != 0;

        // XOR sprite bits into framebuffer,
        // then copy bottom subpixel rows
        FB[fb_idx_tl] ^= sprite_l; 
        FB[fb_idx_tc] ^= sprite_c;
        FB[fb_idx_tr] ^= sprite_r;
        FB[fb_idx_bl]  = FB[fb_idx_tl];
        FB[fb_idx_bc]  = FB[fb_idx_tc];
        FB[fb_idx_br]  = FB[fb_idx_tr];
        


    }

    V[0xF] = collision;
    //printf("%d\n", collision);
    chip8_state->drawFlag = true;
}
*/


// Draws an n-byte sprite from memory[I] at screen coordinates (Vx, Vy).
inline static void op_drw(chip8* chip8_state, uint16_t opcode){
    uint16_t n   =  opcode & 0x000F;
    uint16_t y   = (opcode & 0x00F0) >> 4;
    uint16_t x   = (opcode & 0x0F00) >> 8;

    uint16_t originX = V[x] % SCREEN_WIDTH;      // wrapped start X
    uint16_t originY = V[y] % SCREEN_HEIGHT;     // wrapped start Y
    uint8_t  collision = 0;

    uint16_t bit_shift = originX % 8;         // pixel offset inside a framebuffer byte
    uint16_t visible_bits = MIN(originX + 8, SCREEN_WIDTH) - originX;
    uint8_t  clip_mask_x = (0xFF << (8 - visible_bits));

    for (uint16_t row = 0; row < n; ++row) {

        // --- Y clipping (classic CHIP-8) ---
        uint16_t offsetY = originY + row;
        if (offsetY >= SCREEN_HEIGHT) break;
        

        // --- Compute framebuffer byte indices ---
        uint16_t fb_byte_x = originX / 8;
        uint16_t fb_index_primary   = BYTES_PER_ROW * offsetY + fb_byte_x;
        uint16_t fb_index_secondary = BYTES_PER_ROW * offsetY + ((fb_byte_x + 1) % BYTES_PER_ROW);

        // --- Fetch current framebuffer bytes ---
        uint8_t fb_byte_primary   = FB[fb_index_primary  ];
        uint8_t fb_byte_secondary = FB[fb_index_secondary];

        // --- Fetch sprite row bits and apply right-edge screen clipping ---
        uint8_t sprite_row_bits = MEM[(I + row) & 0x0FFF] & clip_mask_x;

        if (bit_shift != 0) {
            // Handle byte alignment and bit spill
            uint8_t spill_mask   = sprite_row_bits & ((1 << bit_shift) - 1);
            uint8_t spill_shifted = spill_mask << (8 - bit_shift);

            // Detect collision
            collision |= (
                (fb_byte_primary   & (sprite_row_bits >> bit_shift)) |
                (fb_byte_secondary & spill_shifted)
            ) != 0;

            // XOR sprite bits into framebuffer
            FB[fb_index_primary  ] ^= sprite_row_bits >> bit_shift;
            FB[fb_index_secondary] ^= spill_shifted;

        } else {
            // Aligned case — all 8 bits fit in one byte
            collision |= (fb_byte_primary & sprite_row_bits) != 0;
            FB[fb_index_primary] ^= sprite_row_bits;
        }
    }

    V[0xF] = collision;
    chip8_state->drawFlag = true;
}


inline static void handle_0nnn(chip8* chip8_state, uint16_t opcode){
    uint16_t nnn =  opcode & 0x0FFF;
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
        
        // schip extension
        case 0x00FD:
            running = STATUS_STOPPED;
            break;

        case 0x00FF:
            chip8_state->extendedMode = true;
            memset(FB, 0, FB_SIZE);
            break;

        case 0x00FE:
            chip8_state->extendedMode = false;
            memset(FB, 0, FB_SIZE);
            break;
            
        default:
            chip8_state->errState = ERR_IGNORED_OPCODE;
            break;
    }
}

//jump to nnn
//jump to nnn + V0
inline static void op_jp(chip8* chip8_state, uint16_t opcode){
    uint16_t nnn =  opcode & 0x0FFF;
    PC = nnn;
}


inline static void op_jp_v0(chip8* chip8_state, uint16_t opcode){
    uint16_t nnn =  opcode & 0x0FFF;
    PC = nnn + V[0];
}


/* call subroutine at nnn
1)increment SP, 
2) put current PC at the top of the stack
3) set PC to nnn.
*/
inline static void op_call(chip8* chip8_state, uint16_t opcode){
    uint16_t nnn =  opcode & 0x0FFF;
    if(SP < STACK_SIZE_UINT16 - 1){
        STACK[SP] = PC;
        SP++;
        PC = nnn;
    }else {
        chip8_state->errState = ERR_STACK_OVERFLOW;
        running = STATUS_PAUSED;
    }
}

//Skips the next instruction if VX equals nn 
inline static void op_se_byte (chip8* chip8_state, uint16_t opcode){
    uint16_t nn  =  opcode & 0x00FF;
    uint16_t x   = (opcode & 0x0F00) >> 8;
    PC += (V[x] == nn) * 2;
}

//Skips the next instruction if VX not equal nn
inline static void op_sne_byte(chip8* chip8_state, uint16_t opcode){
    uint16_t nn  =  opcode & 0x00FF;
    uint16_t x   = (opcode & 0x0F00) >> 8;
    PC += (V[x] != nn) * 2;
}

//Skips the next instruction if VX equals VY
inline static void op_se_reg (chip8* chip8_state, uint16_t opcode){
    uint16_t x   = (opcode & 0x0F00) >> 8;
    uint16_t y   = (opcode & 0x00F0) >> 4;
    PC += (V[x] == V[y]) * 2;
}

//Skips the next instruction if VX not equals VY
inline static void op_sne_reg(chip8* chip8_state, uint16_t opcode){
    uint16_t x   = (opcode & 0x0F00) >> 8;
    uint16_t y   = (opcode & 0x00F0) >> 4;
    PC += (V[x] != V[y]) * 2;
}

//Set Vx equal to nn
inline static void op_ld_byte(chip8* chip8_state, uint16_t opcode){
    uint16_t nn  =  opcode & 0x00FF;
    uint16_t x   = (opcode & 0x0F00) >> 8;
    V[x] = nn & 0x00FF;
}

//Set I equal to address of value nnn
inline static void op_ld_I(chip8* chip8_state, uint16_t opcode){
    uint16_t nnn = opcode & 0x0FFF;
    I = nnn;
}

//Set Vx = bitwise and between a random number and nn
inline static void op_rnd(chip8* chip8_state, uint16_t opcode){
    uint16_t nn  =  opcode & 0x00FF;
    uint16_t x   = (opcode & 0x0F00) >> 8;
    rnd = (uint16_t)(LCG_rand()>>16);
    V[x] = rnd & nn;
}

//Add nn to Vn (carry flag is not changed).
inline static void op_add_byte(chip8* chip8_state, uint16_t opcode){
    uint16_t nn  =  opcode & 0x00FF;
    uint16_t x   = (opcode & 0x0F00) >> 8;
    V[x] = (V[x] + nn) & 0x00FF;
}

//ALU instructions.
inline static void handle_8xyn(chip8* chip8_state, uint16_t opcode){
    uint16_t n   =  opcode & 0x000F;
    uint16_t y   = (opcode & 0x00F0) >> 4;
    uint16_t x   = (opcode & 0x0F00) >> 8;
    switch (n) {
        case 0x0: {V[x] = V[y]; break;} //Sets Vx to the value of Vy.
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

        case 0x4: {
            uint16_t sum16 = V[x] + V[y];
            V[ x ] = sum16 & 0x00FF;
            V[0xF] = sum16 >> 8;
            break;
        }

        case 0x5:{
            uint16_t sub16 = V[x] - V[y];
            V[ x ] = sub16 & 0x00FF;
            V[0xF] = (~(sub16 >> 8) & 1);
            break;
        }

        case 0x6:{
            uint16_t spill = V[y] &  1;
            V[ x ] = V[y] >> 1;
            V[0xF] = spill;
            break;
        }

        case 0x7:{
            uint16_t sub16 = V[y] - V[x];
            V[ x ] = sub16 & 0x00FF;
            V[0xF] = (~(sub16 >> 8) & 1);
            break;
        }

        case 0xE:{
            uint16_t spill = (V[y] & 0x0080) >> 7;
            V[ x ] = (V[y] << 1) & 0x00FF;
            V[0xF] = spill;
            break;
        }
    }
}


inline static void handle_Exnn(chip8* chip8_state, uint16_t opcode){
    uint16_t nn  =  opcode & 0x00FF;
    uint16_t x   = (opcode & 0x0F00) >> 8;
    switch(nn){
        case 0x9E:
            if(KP[V[x & 0xF]])
                PC+=2;
            break;

        case 0xA1:
            if(!(KP[V[x] & 0xF]))
                PC+=2;
            break;
    }
}


inline static void handle_Fxnn(chip8* chip8_state, uint16_t opcode){
    uint16_t nn  =  opcode & 0x00FF;
    uint16_t x   = (opcode & 0x0F00) >> 8;
    switch (nn) {
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
                    MEM[I+j] = (uint8_t)(V[j]);

                I += x + 1;
            }
            else {
                chip8_state->errState = ERR_MEM_OUT_OF_BOUNDS;
                running = STATUS_PAUSED;
            }
            break;
        case 0x65:
            if(I <= MEMORY_END - x){
                for(uint16_t j=0; j<=x; ++j)
                    V[j] = MEM[I+j];

                I += x + 1;
            }
            else {
                chip8_state->errState = ERR_MEM_OUT_OF_BOUNDS;
                running = STATUS_PAUSED;
            }
            break;




    }
}







/*
============================================================
                     S C H I P   S E C T I O N
============================================================

+----+---------------------------------------------+----------------------+
| #  | Quirk (summary)                             | Affected SCHIP ver.  |
+----+---------------------------------------------+----------------------+
| 1  | CHIP-48 / SCHIP-1.0 increment I only by X   | SCHIP-1.0            |
|    | after Fx55/Fx65; SCHIP-1.1 and modern       | SCHIP-1.1 / MODERN   |
|    | versions don’t increment I at all           |                      |
+----+---------------------------------------------+----------------------+
| 2  | CHIP-48 / SCHIP-1.x don’t set Vx := Vy on   | SCHIP-1.0 / 1.1      |
|    | shifts (only shift Vx)                      |                      |
+----+---------------------------------------------+----------------------+
| 3  | Original SCHIP-1.x did not clear the screen | SCHIP-1.0 / 1.1      |
|    | before certain operations                   |                      |
+----+---------------------------------------------+----------------------+
| 4  | Original SCHIP-1.1 (hires) sets VF to the   | SCHIP-1.1            |
|    | number of rows with collisions + clipped    |                      |
|    | rows at bottom                              |                      |
+----+---------------------------------------------+----------------------+
*/

inline static void handle_8xyn_schip(chip8* chip8_state, uint16_t opcode){
    uint16_t n   =  opcode & 0x000F;
    uint16_t y   = (opcode & 0x00F0) >> 4;
    uint16_t x   = (opcode & 0x0F00) >> 8;
    switch (n) {
        case 0x0: {V[x] = V[y]; break;} //Sets Vx to the value of Vy.
        case 0x1:
            V[ x ] |= V[y];
            break;

        case 0x2:
            V[ x ] &=  V[y];
            break;

        case 0x3:
            V[ x ] ^= V[y];
            break;

        case 0x4: {
            uint16_t sum16 = V[x] + V[y];
            V[ x ] = sum16 & 0x00FF;
            V[0xF] = sum16 >> 8;
            break;
        }

        case 0x5:{
            uint16_t sub16 = V[x] - V[y];
            V[ x ] = sub16 & 0x00FF;
            V[0xF] = (~(sub16 >> 8) & 1);
            break;
        }

        case 0x6:{
            uint16_t spill = V[x] &  1;
            V[ x ] >>= 1;
            V[0xF] = spill;
            break;
        }

        case 0x7:{
            uint16_t sub16 = V[y] - V[x];
            V[ x ] = sub16 & 0x00FF;
            V[0xF] = (~(sub16 >> 8) & 1);
            break;
        }

        case 0xE:{
            uint16_t spill = (V[x] & 0x0080) >> 7;
            V[ x ] = (V[x] << 1) & 0x00FF;
            V[0xF] = spill;
            break;
        }
    }


}


inline static void op_jp_vx(chip8* chip8_state, uint16_t opcode){
    uint16_t x   =  (opcode & 0x0F00) >> 8; 
    uint16_t nnn =  (opcode & 0x0FFF);
    PC = nnn + V[x];
}


inline static void handle_Fxnn_schip11(chip8* chip8_state, uint16_t opcode){
    uint16_t nn  =  opcode & 0x00FF;
    uint16_t x   = (opcode & 0x0F00) >> 8;
    static uint16_t userFlags[8]; //persistent memory for FX75/85
    switch (nn) {
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
            if(I > 0xFFF)
                running = STATUS_STOPPED;

            break;
        case 0x29:
            //I is set to location of sprite corresponding to value of Vx.
            I = FONT_SMALL_ADDR + V[x] * 5;
            break;

        case 0x30:
            //I is set to location of large sprite corresponding to value of Vx.
            I = FONT_LARGE_ADDR + V[x] * 10;
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

        // in SCHIP variants, FX55/65 increment I differently than classic chip-8
        // in chip-8               I is incremented by x+1
        // in chip-48 and schip1.0 I incremented by x
        // in schip1.1             I is not incremented at all
        // schip1.1 is known as 'schip legacy' to distinguish it from more recent
        // versions that have their own quirks ('modern schip' in the quirks test)
        case 0x55:
            if(I <= MEMORY_END - x){
                for(uint16_t j=0; j<=x; ++j)
                    MEM[I+j] = (uint8_t)(V[j]);
                
                //I += x;
            }
            else {
                chip8_state->errState = ERR_MEM_OUT_OF_BOUNDS;
                running = STATUS_PAUSED;
            }
            break;
        case 0x65:
            if(I <= MEMORY_END - x){
                for(uint16_t j=0; j<=x; ++j)
                    V[j] = MEM[I+j];

                //I += x;
            }
            else {
                chip8_state->errState = ERR_MEM_OUT_OF_BOUNDS;
                running = STATUS_PAUSED;
            }
            break;
            
        case 0x75:{
            uint16_t ncpy = MIN(x, 7);
            for(int i=0; i<=ncpy; i++)
                userFlags[i] = V[i];

            break;
        }
        case 0x85:{
            uint16_t ncpy = MIN(x, 7);
            for(int i=0; i<=ncpy; i++)
                V[i] = userFlags[i];

            break;
        }



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