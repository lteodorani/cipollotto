//private header. This should ever only be
//included by cipollotto.c

#pragma once
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "cipollotto.h"


#define MIN(a, b) (((a) <= (b)) ? (a) : (b))
#define MAX(a, b) (((a) => (b)) ? (a) : (b))

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
inline static uint16_t dupeBitsInPlace16(uint8_t n){
    uint16_t x = n;
    x = (x | (x << 4)) & 0x0F0F0F0F;
    x = (x | (x << 2)) & 0x33333333;
    x = (x | (x << 1)) & 0x55555555;
    return x | (x << 1);
}

inline static uint32_t dupeBitsInPlace32(uint8_t n){
    uint32_t x = n;
    x = (x | (x << 4)) & 0x0F0F0F0F;
    x = (x | (x << 2)) & 0x33333333;
    x = (x | (x << 1)) & 0x55555555;
    return x | (x << 1);
}

inline static uint8_t drawBytes(chip8* c8, uint16_t row_idx, uint16_t col_idx, uint32_t pixel_data){
    
    // if n is a power of two, and x unsigned
    // x % n is the same of x & (n-1), but cheaper.
    // this is done automatically by gcc even in -O0
    // However could be exploited for a branchless
    // screen wrap behaviour quirk selector like this:
    //uint16_t MOD_mask = (BYTES_PER_ROW - 1) * (!wrap) + wrap;
    
    uint16_t indices[3] = {
        (uint16_t)(row_idx + col_idx),
        (uint16_t)(row_idx + ((col_idx + 1) % BYTES_PER_ROW)),
        (uint16_t)(row_idx + ((col_idx + 2) % BYTES_PER_ROW))
    };
                                    //left                center              right
    uint8_t oldBytes[3] = {FB[indices[0]], FB[indices[1]], FB[indices[2]]};

    uint8_t newBytes[3] = {
        (uint8_t)(pixel_data >> 16),
        (uint8_t)(pixel_data >> 8),
        (uint8_t)(pixel_data)
    };

    // XOR sprite bits into framebuffer
    FB[indices[0]] ^= newBytes[0];
    FB[indices[1]] ^= newBytes[1];
    FB[indices[2]] ^= newBytes[2];

    // Detect collision
    uint8_t collisionHappened = (
        (oldBytes[0] & newBytes[0]) | 
        (oldBytes[1] & newBytes[1]) |
        (oldBytes[2] & newBytes[2])
    ) != 0;

    return collisionHappened;
}

inline static uint8_t drawDoubleBytes(chip8* c8, uint16_t row_idx, uint16_t col_idx, uint32_t pixel_data){
    
    // if n is a power of two, and x unsigned
    // x % n is the same of x & (n-1), but cheaper.
    // this is done automatically by gcc even in -O0
    // However could be exploited for a branchless
    // screen wrap behaviour quirk selector like this:
    //uint16_t MOD_mask = (BYTES_PER_ROW - 1) * (!wrap) + wrap;
    
    uint16_t indices[6] = {
        //top row of pixels
        (uint16_t)(row_idx + col_idx),
        (uint16_t)(row_idx + ((col_idx + 1) % BYTES_PER_ROW)),
        (uint16_t)(row_idx + ((col_idx + 2) % BYTES_PER_ROW)),

        //bottom row of pixels
        (uint16_t)(row_idx + BYTES_PER_ROW + col_idx),
        (uint16_t)(row_idx + BYTES_PER_ROW + ((col_idx + 1) % BYTES_PER_ROW)),
        (uint16_t)(row_idx + BYTES_PER_ROW + ((col_idx + 2) % BYTES_PER_ROW)),
    };
                                    //left                center              right
    uint8_t oldBytes[3] = {FB[indices[0]], FB[indices[1]], FB[indices[2]]};

    uint8_t newBytes[3] = {
        (uint8_t)(pixel_data >> 16),
        (uint8_t)(pixel_data >> 8),
        (uint8_t)(pixel_data)
    };

    // XOR top sprite bits into framebuffer,
    // copy the top row to the bottom one
    FB[indices[0]] ^= newBytes[0];
    FB[indices[1]] ^= newBytes[1];
    FB[indices[2]] ^= newBytes[2];
    FB[indices[3]]  = FB[indices[0]];
    FB[indices[4]]  = FB[indices[1]];
    FB[indices[5]]  = FB[indices[2]];

    // Detect collision only in the top
    // row pixels. Bott are just a copy
    uint8_t collisionHappened = (
        (oldBytes[0] & newBytes[0]) | 
        (oldBytes[1] & newBytes[1]) |
        (oldBytes[2] & newBytes[2])
    ) != 0;

    return collisionHappened;
}

inline static uint16_t drw_hires(chip8* c8, uint16_t originX, uint16_t originY, uint16_t n){
    uint16_t collidedRowsN = 0;
    uint16_t drawnRows     = 0;
    uint16_t rowN = (n == 0) ? 16 : n; // toggle between dxy0 and dxyn

    uint16_t x_idx   = originX / 8; // x position aligned to 8bit boundaries
    uint16_t offsetX = originX % 8; // pixel offset inside an 8 bit boundary

    // right edge clipping mask
    uint16_t visBitsN = MIN(originX + 16, SCREEN_WIDTH) - originX;
    uint32_t edgeMask  = (0x00FFFFFFu << (16 - visBitsN));

    // sprite bits initialized to high-low pattern to help diagnose
    // possible errors in the drawing code logic.
    uint32_t sprite_bits = 0x55555555u;

    for (uint16_t row = 0; row < rowN; ++row) {

        // --- Y clipping ---
        uint16_t offsetY = originY + row;
        if (offsetY >= SCREEN_HEIGHT) break;
        
        // --- Fetch sprite row bits, then apply right-edge screen clipping ---
        // Sprite bits must be read as 32bits to allow for left bitshift and
        // aligned to second least significant byte from the right (bits 16-8)
        if(n!=0){
            // dxyn branch
            uint32_t sprite_row =
                ((uint32_t)MEM[(I + row) & 0x0FFF] << 8) & edgeMask;

            sprite_bits = sprite_row << (8 - offsetX);
        }
        else {
            // dxy0 branch
            uint8_t a = MEM[(I + 2 * row    ) & 0x0FFF];
            uint8_t b = MEM[(I + 2 * row + 1) & 0x0FFF];

            uint32_t sprite_row =
                (((uint32_t)a << 8) | (uint32_t)b) & edgeMask;

            sprite_bits = sprite_row << (8 - offsetX);
        }

        // blit sprite on screen 
        collidedRowsN += drawBytes(
            c8, BYTES_PER_ROW * offsetY, x_idx, sprite_bits
        );

        drawnRows++;

    }

    // hires mode on the calculator counts as collisions 
    // the numbers of rows clipped at the bottom border too
    uint16_t collisions = collidedRowsN + (rowN - drawnRows);

    //printf("%d\n", collisions);
    return collisions;

}

inline static uint16_t drw_lores(chip8* c8, uint16_t originX, uint16_t originY, uint16_t n){
    uint16_t collidedRowsN = 0;
    uint16_t drawnRows     = 0;

    uint16_t x_idx   = originX / 8; // x position aligned to 8bit boundaries
    uint16_t offsetX = originX % 8; // pixel offset inside an 8 bit boundary

    // right edge clipping mask
    uint16_t visBitsN = MIN(originX + 16, SCREEN_WIDTH) - originX;
    uint32_t edgeMask  = (0x00FFFFFFu << (16 - visBitsN));

    for (uint16_t row = 0; row < n; ++row) {

        // --- Y clipping ---
        uint16_t offsetY = originY + 2*row;
        if (offsetY >= SCREEN_HEIGHT) break;
        
        uint32_t sprite_bits = (dupeBitsInPlace32(MEM[(I + row) & 0x0FFF]) & edgeMask)
                                 << (8 - offsetX);

        // blit top and bottom row pixels on screen
        collidedRowsN += drawDoubleBytes(
            c8, BYTES_PER_ROW * offsetY, x_idx, sprite_bits
        );

        drawnRows++;

    }

    //printf("%d\n", collidedRowsN);
    return collidedRowsN;

}


static inline void op_drw_schip(chip8* c8, uint16_t opcode){
    uint16_t n   =  opcode & 0x000F;
    uint16_t y   = (opcode & 0x00F0) >> 4;
    uint16_t x   = (opcode & 0x0F00) >> 8;

    uint16_t collision = 0;

    // hires dxyn / dxy0
    if(c8->extMode){
        uint16_t originX = V[x] % SCREEN_WIDTH;
        uint16_t originY = V[y] % SCREEN_HEIGHT;
        collision = drw_hires(c8, originX, originY, n);
    }

    // lores dxyn
    else {
        uint16_t originX = 2*V[x] % SCREEN_WIDTH;      // scaled and wrapped start X
        uint16_t originY = 2*V[y] % SCREEN_HEIGHT;     // scaled wrapped start Y
        collision = drw_lores(c8, originX, originY, n);
    }

    V[0xF] = collision;    
    c8->drawFlag = true;
}

inline static void op_drw(chip8* c8, uint16_t opcode){
    uint16_t n   =  opcode & 0x000F;
    uint16_t y   = (opcode & 0x00F0) >> 4;
    uint16_t x   = (opcode & 0x0F00) >> 8;

    uint16_t collision = 0;

    uint16_t originX = 2*V[x] % SCREEN_WIDTH;      // scaled and wrapped start X
    uint16_t originY = 2*V[y] % SCREEN_HEIGHT;     // scaled wrapped start Y
    collision = drw_lores(c8, originX, originY, n);


    V[0xF] = collision;    
    c8->drawFlag = true;
}

inline static void handle_0nnn(chip8* c8, uint16_t opcode){
    uint16_t nnn =  opcode & 0x0FFF;
    switch(nnn) {
        case 0x0E0:
            memset(FB, 0x00, sizeof(FB));
            c8->drawFlag = true;
            break;
        case 0x0EE: //ret from subroutine
            if(SP>0){
                SP--;
                PC = STACK[SP];
            }
            else {
                c8->errState = CIPOLLOTTO_ERR_STACK_OOB;
                c8->runState = CIPOLLOTTO_STATUS_HALTED;
            }
            break;

        default:
            c8->errState = CIPOLLOTTO_ERR_INVALID_OPCODE;
            break;
    }
}

//jump to nnn
//jump to nnn + V0
inline static void op_jp(chip8* c8, uint16_t opcode){
    uint16_t nnn =  opcode & 0x0FFF;
    PC = nnn;
}


inline static void op_jp_v0(chip8* c8, uint16_t opcode){
    uint16_t nnn =  opcode & 0x0FFF;
    PC = nnn + V[0];
}


/* call subroutine at nnn
1)increment SP, 
2) put current PC at the top of the stack
3) set PC to nnn.
*/
inline static void op_call(chip8* c8, uint16_t opcode){
    uint16_t nnn =  opcode & 0x0FFF;
    if(SP < STACK_SIZE_UINT16 - 1){
        STACK[SP] = PC;
        SP++;
        PC = nnn;
    }else {
        c8->errState = CIPOLLOTTO_ERR_STACK_OOB;
        c8->runState = CIPOLLOTTO_STATUS_HALTED;
    }
}

//Skips the next instruction if VX equals nn 
inline static void op_se_byte(chip8* c8, uint16_t opcode){
    uint16_t nn  =  opcode & 0x00FF;
    uint16_t x   = (opcode & 0x0F00) >> 8;
    PC += (V[x] == nn) * 2;
}

//Skips the next instruction if VX not equal nn
inline static void op_sne_byte(chip8* c8, uint16_t opcode){
    uint16_t nn  =  opcode & 0x00FF;
    uint16_t x   = (opcode & 0x0F00) >> 8;
    PC += (V[x] != nn) * 2;
}

//Skips the next instruction if VX equals VY
inline static void op_se_reg(chip8* c8, uint16_t opcode){
    uint16_t x   = (opcode & 0x0F00) >> 8;
    uint16_t y   = (opcode & 0x00F0) >> 4;
    PC += (V[x] == V[y]) * 2;
}

//Skips the next instruction if VX not equals VY
inline static void op_sne_reg(chip8* c8, uint16_t opcode){
    uint16_t x   = (opcode & 0x0F00) >> 8;
    uint16_t y   = (opcode & 0x00F0) >> 4;
    PC += (V[x] != V[y]) * 2;
}

//Set Vx equal to nn
inline static void op_ld_byte(chip8* c8, uint16_t opcode){
    uint16_t nn  =  opcode & 0x00FF;
    uint16_t x   = (opcode & 0x0F00) >> 8;
    V[x] = nn & 0x00FF;
}

//Set I equal to address of value nnn
inline static void op_ld_I(chip8* c8, uint16_t opcode){
    uint16_t nnn = opcode & 0x0FFF;
    I = nnn;
}

//Set Vx = bitwise and between a random number and nn
inline static void op_rnd(chip8* c8, uint16_t opcode){
    uint16_t nn  =  opcode & 0x00FF;
    uint16_t x   = (opcode & 0x0F00) >> 8;
    V[x] = (uint16_t)(LCG_rand()>>16) & nn;
}

//Add nn to Vn (carry flag is not changed).
inline static void op_add_byte(chip8* c8, uint16_t opcode){
    uint16_t nn  =  opcode & 0x00FF;
    uint16_t x   = (opcode & 0x0F00) >> 8;
    V[x] = (V[x] + nn) & 0x00FF;
}

//ALU instructions.
inline static void handle_8xyn(chip8* c8, uint16_t opcode){
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


inline static void handle_Exnn(chip8* c8, uint16_t opcode){
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


inline static void handle_Fxnn(chip8* c8, uint16_t opcode){
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
                c8->errState = CIPOLLOTTO_ERR_MEMORY_OOB;
                c8->runState = CIPOLLOTTO_STATUS_HALTED;
            }
            break;
        case 0x55:
            if(I <= MEMORY_END - x){
                for(uint16_t j=0; j<=x; ++j)
                    MEM[I+j] = (uint8_t)(V[j]);

                I += x + 1;
            }
            else {
                c8->errState = CIPOLLOTTO_ERR_MEMORY_OOB;
                c8->runState = CIPOLLOTTO_STATUS_HALTED;
            }
            break;
        case 0x65:
            if(I <= MEMORY_END - x){
                for(uint16_t j=0; j<=x; ++j)
                    V[j] = MEM[I+j];

                I += x + 1;
            }
            else {
                c8->errState = CIPOLLOTTO_ERR_MEMORY_OOB;
                c8->runState = CIPOLLOTTO_STATUS_HALTED;
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

inline static void handle_8xyn_schip(chip8* c8, uint16_t opcode){
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


inline static void scrolln(chip8* c8, uint16_t n){
    //clear first n-1 rows, then copy last n rows
    uint16_t deltaBytes = n * BYTES_PER_ROW;
    memmove(FB + deltaBytes, FB, FB_SIZE-deltaBytes);
    memset(FB, 0x0, deltaBytes);
    c8->drawFlag = true;
}

//scrolls horizontally right 4 pixels
inline static void scrollr(chip8* c8){
    uint8_t rowBytes[BYTES_PER_ROW];
    for(int r=0; r<SCREEN_HEIGHT; r++){
        memcpy(rowBytes, FB+r*BYTES_PER_ROW, BYTES_PER_ROW);
        uint8_t lsbPrev = 0x00, 
                lsbCurr = 0x00;
        for(int b=0; b<BYTES_PER_ROW; b++){
            lsbCurr = rowBytes[b] & 0x0Fu;
            rowBytes[b] >>= 4;
            rowBytes[b] |= (lsbPrev << 4);
            lsbPrev = lsbCurr;
        }
        memcpy(FB+r*BYTES_PER_ROW, rowBytes, BYTES_PER_ROW);
    }
    c8->drawFlag = true;
}

inline static void scrolll(chip8* c8){
    uint8_t rowBytes[BYTES_PER_ROW] = {0};
    for(int r=0; r<SCREEN_HEIGHT; r++){
        memcpy(rowBytes, FB+r*BYTES_PER_ROW, BYTES_PER_ROW);
        uint8_t msbPrev = 0x00, 
                msbCurr = 0x00;
        for(int b=BYTES_PER_ROW-1; b>=0; b--){
            msbCurr = rowBytes[b] & 0xF0u  ;
            rowBytes[b] <<= 4;
            rowBytes[b] |= (msbPrev >> 4);
            msbPrev = msbCurr;
        }
        memcpy(FB+r*BYTES_PER_ROW, rowBytes, BYTES_PER_ROW);
    }
    c8->drawFlag = true;
}

inline static void handle_0nnn_schip(chip8* c8, uint16_t opcode){
    uint16_t nnn = opcode & 0x0FFF;
    switch(nnn) {
        case 0x0E0:
            memset(FB, 0x00, sizeof(FB));
            c8->drawFlag = true;
            break;
        case 0x0EE: //ret from subroutine
            if(SP>0){
                SP--;
                PC = STACK[SP];
            }
            else {
                c8->errState = CIPOLLOTTO_ERR_STACK_OOB;
                c8->runState = CIPOLLOTTO_STATUS_HALTED;
            }
            break;
        
        // schip extension 0x00__ opcodes
        case 0x0FD:
            c8->runState = CIPOLLOTTO_STATUS_HALTED;
            break;

        case 0x0FF:
            c8->extMode = true;
            memset(FB, 0, FB_SIZE);
            break;

        case 0x0FE:
            c8->extMode = false;
            memset(FB, 0, FB_SIZE);
            break;

        case 0x0FB:
            scrollr(c8);
            break;

        case 0x0FC:
            scrolll(c8);
            break;

        default:
            c8->errState = CIPOLLOTTO_ERR_INVALID_OPCODE;
            c8->runState = CIPOLLOTTO_STATUS_HALTED;
            break;
    }
}

inline static void handle_00_schip(chip8* c8, uint16_t opcode){    
    uint16_t __n =  opcode & 0x000F;
    uint16_t _n_ =  opcode & 0x00F0;

    switch(_n_){
        case 0x0C0:
            switch(__n){
                case 0x000:
                    c8->runState = CIPOLLOTTO_STATUS_HALTED;
                    break;
                default:                    
                    scrolln(c8, __n);
                    break;
                
            }
        break;


        default:
            handle_0nnn_schip(c8, opcode);
            break;
    }
}


inline static void op_jp_vx(chip8* c8, uint16_t opcode){
    uint16_t x   =  (opcode & 0x0F00) >> 8; 
    uint16_t nnn =  (opcode & 0x0FFF);
    PC = nnn + V[x];
}


inline static void handle_Fxnn_schip(chip8* c8, uint16_t opcode){
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
                c8->runState = CIPOLLOTTO_STATUS_HALTED;

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
                c8->errState = CIPOLLOTTO_ERR_MEMORY_OOB;
                c8->runState = CIPOLLOTTO_STATUS_HALTED;
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
                c8->errState = CIPOLLOTTO_ERR_MEMORY_OOB;
                c8->runState = CIPOLLOTTO_STATUS_HALTED;
            }
            break;
        case 0x65:
            if(I <= MEMORY_END - x){
                for(uint16_t j=0; j<=x; ++j)
                    V[j] = MEM[I+j];

                //I += x;
            }
            else {
                c8->errState = CIPOLLOTTO_ERR_MEMORY_OOB;
                c8->runState = CIPOLLOTTO_STATUS_HALTED;
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
#undef KP