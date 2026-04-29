#include "cipollotto.h"
#include "ui.h"
#include "common.h"
#include "ui_luts.h"

#include <stdarg.h>
#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define WINDOW_W 1200
#define WINDOW_H 600

#define TEXTURE_WIDTH 128
#define TEXTURE_HEIGTH 64


static SDL_Window*   ui_win;
static SDL_Renderer* ui_ren;
static SDL_Texture*  ui_tex;

static SDL_AudioDeviceID audio_dev;
static SDL_AudioStream*  audio_str;
static const int   audio_srate = 48000;
static const float audio_pitch = 440.0f;

int ui_init(void){
    if(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)){
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not initialize display. %s", SDL_GetError());
        SDL_Quit();
        return -1;
    };
    ui_win = SDL_CreateWindow(
        "Cipollotto",
        1200, 600,
        SDL_WINDOW_RESIZABLE
    );
    if(!ui_win){
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Could not initialize display. %s", SDL_GetError());
        SDL_Quit();
        return -1;
    }
    
    ui_ren = SDL_CreateRenderer(ui_win, NULL);
    if(!ui_ren){
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Could not initialize display. %s", SDL_GetError());
        SDL_Quit();
        return -1;
    }

    ui_tex = SDL_CreateTexture(
        ui_ren, 
        SDL_PIXELFORMAT_RGBA8888, // 32 bits per pixel
        SDL_TEXTUREACCESS_STREAMING, 
        TEXTURE_WIDTH, TEXTURE_HEIGTH
    );

    if(!ui_tex){
        SDL_LogError(SDL_LOG_CATEGORY_RENDER, "Could not initialize display. %s", SDL_GetError());
        SDL_Quit();
        return -1;
    }
    SDL_SetTextureScaleMode(ui_tex, SDL_SCALEMODE_NEAREST);



    // Initialize Audio
    SDL_AudioSpec spec = {
        .channels = 1,
        .format   = SDL_AUDIO_F32,
        .freq     = audio_srate
    };

    audio_dev = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
    if(!audio_dev){
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Could not initialize audio. %s", SDL_GetError());
        SDL_Quit();
        return -1;
    }
    
    audio_str = SDL_CreateAudioStream(&spec, &spec);
    if(!audio_str){
        SDL_LogError(SDL_LOG_CATEGORY_AUDIO, "Could not initialize audio. %s", SDL_GetError());
        SDL_Quit();
        return -1;
    }
    SDL_BindAudioStream  (audio_dev, audio_str);
    SDL_ResumeAudioDevice(audio_dev);


    return 0;
}


void ui_destroy(void){
    SDL_DestroyRenderer(ui_ren);
    SDL_DestroyWindow(ui_win);
    SDL_DestroyAudioStream(audio_str);
    SDL_CloseAudioDevice(audio_dev);
    SDL_Quit();
}



void ui_input(chip8* c8){
    SDL_Event evt;
    while(SDL_PollEvent(&evt)){
        switch (evt.type) {
            case SDL_EVENT_QUIT:
                c8->runState = CIPOLLOTTO_STATUS_HALTED;
                break;

            #ifdef DRAW_FLAG
            case SDL_EVENT_WINDOW_RESIZED:
                c8->drawFlag = true;
                break;
            #endif

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



void debug_print(float x, float y, const char* fmt_string, ...){
    const int maxlen = 256;
    char str[maxlen];
    va_list args;
    va_start(args, fmt_string);
    vsnprintf(str, maxlen, fmt_string, args);
    va_end(args);
    
    SDL_SetRenderDrawColor(ui_ren, 255, 0, 0, 255);
    SDL_SetRenderScale    (ui_ren, 2.0f, 2.0f);
    SDL_RenderDebugText   (ui_ren, x, y, str);
    SDL_SetRenderScale    (ui_ren, 1.0f, 1.0f);
}



void crash_screen(chip8* c8)
{
    debug_print(0.0f, 0.0f,
        "Vx = %02x %02x %02x %02x %02x %02x %02x %02x "
             "%02x %02x %02x %02x %02x %02x %02x %02x ",
              c8->state.V[0], c8->state.V[1], c8->state.V[2], c8->state.V[3], c8->state.V[4],
              c8->state.V[0], c8->state.V[1], c8->state.V[2], c8->state.V[3], c8->state.V[4],
              c8->state.V[0], c8->state.V[1], c8->state.V[2], c8->state.V[3], c8->state.V[4],
              c8->state.V[0], c8->state.V[1], c8->state.V[2], c8->state.V[3], c8->state.V[4]
    );

    debug_print(0.0f, 10.0f, "I  = %04x", c8->state.I );
    debug_print(0.0f, 20.0f, "PC = %04x", c8->state.PC);
    debug_print(0.0f, 30.0f, "ST = %02x", c8->state.ST);
    debug_print(0.0f, 40.0f, "DT = %02x", c8->state.DT);
    debug_print(0.0f, 50.0f, "SP = %02x", c8->state.SP);

    debug_print(0.0f, 70.0f, "Ins:C%d:%04x", c8->clock.cycle, c8->prevOpcode );
    debug_print(0.0f, 80.0f, "Prev Inst = %04x",c8->currOpcode );
    debug_print(0.0f, 90.0f, "Err:%d",   c8->errState   );
    
}



void ui_presentFB(chip8* c8){

    int win_h, win_w;
    SDL_GetWindowSize(ui_win, &win_w, &win_h);
    SDL_FRect dst = {0, 0, (float)win_w, (float)win_h};
    //SDL_SetRenderDrawColor(ui_ren, 0, 0, 0, 255);
    //SDL_RenderClear(ui_ren);
    SDL_RenderTexture(ui_ren, ui_tex, NULL, &dst);

    //#define _UI_DEBUG
    #ifdef UI_DEBUG
    if(c8->errState != CIPOLLOTTO_OK)
        crash_screen(c8);
    #endif    // Optionally draw debug text


    SDL_RenderPresent(ui_ren);

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

    SDL_UpdateTexture(ui_tex, NULL, 
        tex_pixels, sizeof(uint32_t)*TEXTURE_WIDTH);
}
*/

void ui_renderFB(chip8* c8) {
    void* pixels;
    int   pitch;
    SDL_LockTexture(ui_tex, NULL, &pixels, &pitch);

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
    
    SDL_UnlockTexture(ui_tex);
}


// Sine approximation — no math.h required
// Valid input: x in [0, 2π), mapped internally to [-π, π]
static float sine_approx(float x) {
    if (x > 3.14159265f) x -= 6.28318530f;   // fold to [-π, π]
    float x2 = x * x;
    return x * (1.0f - x2 * (1.0f/6.0f - x2 * (1.0f/120.0f - x2 / 5040.0f)));
}

void ui_audio(chip8* c8) {

    //Feed a small chunk of samples to SDL audio device
    const int sampleN = 512;
    float samples[sampleN];

    if (c8->state.ST > 0) {
        float period = (float)audio_srate / audio_pitch;
        for (int i = 0; i < sampleN; i++) {
            float x = (float)i;
            float phase = x - (int)(x / period) * period;  // manual fmodf

            #ifdef BEEP_SINE
                float angle = (phase / period) * 6.28318530f;  // [0, 2π)
                samples[i] = 0.25f * sine_approx(angle);
            #else
                samples[i] = (phase < period * 0.5f) ? 0.25f : -0.25f;
            #endif
            
        }

    }
    
    else {
        for (size_t i = 0; i < sampleN; i++)
            samples[i] = 0.0f;
    }

    SDL_PutAudioStreamData(audio_str, samples, sizeof(samples));
}
