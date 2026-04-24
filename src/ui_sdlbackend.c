#include "cipollotto.h"
#include "ui.h"
#include "common.h"
#include "ui_luts.h"

#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_keycode.h>
#include <SDL3/SDL_log.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_stdinc.h>
#include <SDL3/SDL_video.h>
#include <stdarg.h>
#include <SDL3/SDL.h>
#include <stdio.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string.h>


#define WINDOW_W 1200
#define WINDOW_H 600

#define TEXTURE_WIDTH 128
#define TEXTURE_HEIGTH 64


static SDL_Window*   win_canvas = NULL;
static SDL_Renderer* ren_canvas = NULL;
static SDL_Texture*  tex_canvas = NULL;

static SDL_Window*   win_info = NULL;
static SDL_Renderer* ren_info = NULL;

int ui_init(void){
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    win_canvas = SDL_CreateWindow(
        "Cipollotto",
        1200, 600, 
        SDL_WINDOW_RESIZABLE
    );
    
    win_info   = SDL_CreateWindow("Debug", 800, 600, SDL_WINDOW_RESIZABLE);
    ren_canvas = SDL_CreateRenderer(win_canvas, NULL);
    ren_info   = SDL_CreateRenderer(win_info,   NULL);


    //Texture format:
    // 4 x 8 = 32 bits
    tex_canvas = SDL_CreateTexture(
        ren_canvas, 
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING, 
        TEXTURE_WIDTH, TEXTURE_HEIGTH);

    if(!win_canvas || !ren_canvas || !tex_canvas || !win_info){
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Could not initialize display. %s", SDL_GetError());
        SDL_Quit();
        return -1;
    }
    SDL_SetTextureScaleMode(tex_canvas, SDL_SCALEMODE_NEAREST);


    SDL_SetRenderDrawColor(ren_info, 0x1c, 0x1c, 0x1c, 0xff);
    SDL_RenderClear(ren_info);
    return 0;
}


void ui_deinit(void){
    SDL_DestroyRenderer(ren_canvas);
    SDL_DestroyRenderer(ren_info);
    SDL_DestroyWindow(win_canvas);
    SDL_DestroyWindow(win_info);
    SDL_Quit();
}



void ui_input(chip8* c8){
    SDL_Event evt;
    while(SDL_PollEvent(&evt)){
        switch (evt.type) {
            case SDL_EVENT_QUIT:
                c8->runState = CIPOLLOTTO_STATUS_HALTED;
                break;


            case SDL_EVENT_KEY_DOWN:
                switch (evt.key.key) {
                    case SDLK_ESCAPE: {c8->runState = CIPOLLOTTO_STATUS_HALTED; break;}
                    case SDLK_P: {
                        c8->runState = 
                        (c8->runState == CIPOLLOTTO_STATUS_RUNNING) ? CIPOLLOTTO_STATUS_PAUSED : CIPOLLOTTO_STATUS_RUNNING;
                        break;
                    }
                    case SDLK_1: {c8->state.KP[0x1] = 1; break;}
                    case SDLK_2: {c8->state.KP[0x2] = 1; break;}
                    case SDLK_3: {c8->state.KP[0x3] = 1; break;}
                    case SDLK_4: {c8->state.KP[0xC] = 1; break;}
                    case SDLK_Q: {c8->state.KP[0x4] = 1; break;}
                    case SDLK_W: {c8->state.KP[0x5] = 1; break;}
                    case SDLK_E: {c8->state.KP[0x6] = 1; break;}
                    case SDLK_R: {c8->state.KP[0xD] = 1; break;} 
                    case SDLK_A: {c8->state.KP[0x7] = 1; break;}
                    case SDLK_S: {c8->state.KP[0x8] = 1; break;}
                    case SDLK_D: {c8->state.KP[0x9] = 1; break;}
                    case SDLK_F: {c8->state.KP[0xE] = 1; break;}
                    case SDLK_Z: {c8->state.KP[0xA] = 1; break;}
                    case SDLK_X: {c8->state.KP[0x0] = 1; break;}
                    case SDLK_C: {c8->state.KP[0xB] = 1; break;}
                    case SDLK_V: {c8->state.KP[0xF] = 1; break;}
                    default: break;

                }
                break;

                case SDL_EVENT_KEY_UP:
                    switch (evt.key.key) {
                        
                        case SDLK_1: {c8->state.KP[0x1] = 0; break;}
                        case SDLK_2: {c8->state.KP[0x2] = 0; break;}
                        case SDLK_3: {c8->state.KP[0x3] = 0; break;}
                        case SDLK_4: {c8->state.KP[0xC] = 0; break;}
                        case SDLK_Q: {c8->state.KP[0x4] = 0; break;}
                        case SDLK_W: {c8->state.KP[0x5] = 0; break;}
                        case SDLK_E: {c8->state.KP[0x6] = 0; break;}
                        case SDLK_R: {c8->state.KP[0xD] = 0; break;} 
                        case SDLK_A: {c8->state.KP[0x7] = 0; break;}
                        case SDLK_S: {c8->state.KP[0x8] = 0; break;}
                        case SDLK_D: {c8->state.KP[0x9] = 0; break;}
                        case SDLK_F: {c8->state.KP[0xE] = 0; break;}
                        case SDLK_Z: {c8->state.KP[0xA] = 0; break;}
                        case SDLK_X: {c8->state.KP[0x0] = 0; break;}
                        case SDLK_C: {c8->state.KP[0xB] = 0; break;}
                        case SDLK_V: {c8->state.KP[0xF] = 0; break;}
                        default: break;

                }
                break;




        }
        
    }
}



void ui_refresh(void){
    int win_h, win_w;
    SDL_GetWindowSize(win_canvas, &win_w, &win_h);
    SDL_FRect dst = {0, 0, (float)win_w, (float)win_h};
    SDL_RenderClear(ren_canvas);
    SDL_RenderTexture(ren_canvas, tex_canvas,
        NULL, &dst);

    SDL_RenderPresent(ren_canvas);

}
/*
void renderFBtoUI(chip8* c8) {
    uint32_t tex_pixels[TEXTURE_WIDTH * TEXTURE_HEIGTH];
    for (int row = 0; row < TEXTURE_HEIGTH; ++row) {
        for (int col_per_byte = 0; col_per_byte < TEXTURE_WIDTH / 8; col_per_byte++) {

            int idx = BYTES_PER_ROW * row + col_per_byte;
            uint8_t byte = c8->state.FB[idx];
            
            memcpy((void *)(tex_pixels+(row*TEXTURE_WIDTH+col_per_byte*8)),
              draw_table_large[byte],
                sizeof(uint32_t)*8
            );
            
            //uint8_t nib1 = byte >> 4;
            //uint8_t nib2 = byte & 0x0F;

            //tex_pixels[row*TEXTURE_WIDTH+col_per_byte*8+0] = draw_table[nib1][0];
            //tex_pixels[row*TEXTURE_WIDTH+col_per_byte*8+1] = draw_table[nib1][1];
            //tex_pixels[row*TEXTURE_WIDTH+col_per_byte*8+2] = draw_table[nib1][2];
            //tex_pixels[row*TEXTURE_WIDTH+col_per_byte*8+3] = draw_table[nib1][3];
            //tex_pixels[row*TEXTURE_WIDTH+col_per_byte*8+4] = draw_table[nib2][0];
            //tex_pixels[row*TEXTURE_WIDTH+col_per_byte*8+5] = draw_table[nib2][1];
            //tex_pixels[row*TEXTURE_WIDTH+col_per_byte*8+6] = draw_table[nib2][2];
            //tex_pixels[row*TEXTURE_WIDTH+col_per_byte*8+7] = draw_table[nib2][3];
        }
    }

    SDL_UpdateTexture(tex_canvas, NULL, 
        tex_pixels, sizeof(uint32_t)*TEXTURE_WIDTH);
}
*/

void renderFBtoUI(chip8* c8) {
    void* pixels;
    int pitch;
    SDL_LockTexture(tex_canvas, NULL, &pixels, &pitch);

    for (int row = 0; row < TEXTURE_HEIGTH; ++row) {
        // Get pointer to start of this row using pitch
        uint32_t* row_start = (uint32_t*)((uint8_t*)pixels + row * pitch);
        
        for (int col_per_byte = 0; col_per_byte < TEXTURE_WIDTH / 8; col_per_byte++) {
            int idx = BYTES_PER_ROW * row + col_per_byte;
            uint8_t byte = c8->state.FB[idx];
            
            memcpy(row_start + (col_per_byte * 8),
                   draw_table_large[byte],
                   sizeof(uint32_t) * 8
            );


        }
    }
    
    SDL_UnlockTexture(tex_canvas);
}


void info_print(const char* fmt_string, ...){
    char string[128];
    va_list args;
    va_start(args, fmt_string);
    vsnprintf(string, sizeof(string), fmt_string, args);
    va_end(args);

    int subslen  = 0;
    int cursor   = 0;
    static int linenum  = 0;
    char buff[128];
    

    SDL_SetRenderDrawColor(ren_info, 0xff, 0xff, 0xff, 0xff);
    SDL_SetRenderScale(ren_info, 2.0f, 2.0f);
    for(int c=0; c<strlen(string); c++){
        subslen++;
        if(string[c]==0x0A || string[c+1]==0){
            memset(buff, 0, sizeof(buff));
            strncpy(buff, string+cursor, subslen);
            //printf("%s", buff);
            SDL_RenderDebugText(ren_info, 0.0f, (float)linenum*10.0f, buff);
            subslen  = 0;
            cursor   = c+1;
            linenum++;
        }
    }

    SDL_RenderPresent(ren_info);
}



void crash_screen(chip8* c8)
{
    info_print(
        "state.PC=0x%04x state.SP=0x%02x I=0x%03x\n"
        "V0=0x%02x V1=0x%02x V2=0x%02x V3=0x%02x \n"
        "V4=0x%02x V5=0x%02x V6=0x%02x V7=0x%02x \n"
        "V8=0x%02x V9=0x%02x VA=0x%02x VB=0x%02x \n"
        "VC=0x%02x VD=0x%02x VE=0x%02x VF=0x%02x \n"
        "state.DT=0x%02x state.ST=0x%02x err=0x%01x run=0x%01x \n"

        "\nSTACK\n"
        
        "%04x\n%04x\n%04x\n%04x\n%04x\n%04x\n%04x\n%04x\n"
        "%04x\n%04x\n%04x\n%04x\n%04x\n%04x\n%04x\n%04x\n"

        , 
        c8->state.PC  , c8->state.SP, c8->state.I, 
        c8->state.V[0], c8->state.V[1], c8->state.V[2], c8->state.V[3],
        c8->state.V[4], c8->state.V[5], c8->state.V[6], c8->state.V[7],
        c8->state.V[8], c8->state.V[9], c8->state.V[10],c8->state.V[11],
        c8->state.V[12],c8->state.V[13],c8->state.V[14],c8->state.V[15],
        c8->state.DT, c8->state.ST, c8->errState, c8->runState,

        c8->state.STACK[0 ],
        c8->state.STACK[1 ],
        c8->state.STACK[2 ],
        c8->state.STACK[3 ],
        c8->state.STACK[4 ],
        c8->state.STACK[5 ],
        c8->state.STACK[6 ],
        c8->state.STACK[7 ],
        c8->state.STACK[8 ],
        c8->state.STACK[9 ],
        c8->state.STACK[10],
        c8->state.STACK[11],
        c8->state.STACK[12],
        c8->state.STACK[13],
        c8->state.STACK[14],
        c8->state.STACK[15]
        

    );
}