#include "ui.h"
#include <ncurses.h>
#include <stdalign.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <wchar.h>
#include "cipollotto.h"

#define CANVAS_W  SCREEN_WIDTH
#define CANVAS_H  SCREEN_HEIGHT
#define INFO_W    35
#define INFO_H    32
#define KEEP_ALIVE 3

#define CANVAS_OUT_W (CANVAS_W + 2)
#define CANVAS_OUT_H (CANVAS_H + 2)
#define INFO_OUT_W   (INFO_W + 2)
#define INFO_OUT_H   (INFO_H + 2)

static WINDOW *win_canvas_border = NULL;
static WINDOW *win_canvas        = NULL;
static WINDOW *win_info_border   = NULL;
static WINDOW *win_info          = NULL;

/* attribute macros (compile-time constants in ncurses) */
#define chON  ((' ') | A_REVERSE)
#define chOFF ((' '))

/* compile-time initialized table (16 rows × 4 columns) */
static const chtype draw_table_chtype[16][4] = {
    { chOFF, chOFF, chOFF, chOFF },
    { chOFF, chOFF, chOFF, chON  },
    { chOFF, chOFF, chON,  chOFF },
    { chOFF, chOFF, chON,  chON  },
    { chOFF, chON,  chOFF, chOFF },
    { chOFF, chON,  chOFF, chON  },
    { chOFF, chON,  chON,  chOFF },
    { chOFF, chON,  chON,  chON  },
    { chON,  chOFF, chOFF, chOFF },
    { chON,  chOFF, chOFF, chON  },
    { chON,  chOFF, chON,  chOFF },
    { chON,  chOFF, chON,  chON  },
    { chON,  chON,  chOFF, chOFF },
    { chON,  chON,  chOFF, chON  },
    { chON,  chON,  chON,  chOFF },
    { chON,  chON,  chON,  chON  },
};


/* init ncurses and windows */
int ui_init(void) {
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    set_escdelay(1);
    int min_cols  = CANVAS_OUT_W + INFO_OUT_W;
    int min_lines = CANVAS_OUT_H;
    if (COLS < min_cols || LINES < min_lines) {
        endwin();
        fprintf(stderr,
            "Terminal too small: need at least %dx%d\n",
            min_cols, min_lines);
        return -1;
    }

    int maxx, maxy;
    int ui_total_width  = CANVAS_OUT_W + INFO_OUT_W;
    int ui_total_heigth = CANVAS_OUT_H;
    getmaxyx(stdscr, maxy, maxx);

    int canvas_y = (maxy - ui_total_heigth) / 2;
    int canvas_x = (maxx - ui_total_width ) / 2;

    int info_y = canvas_y;
    int info_x = canvas_x + CANVAS_OUT_W;

    // left (canvas)
    win_canvas_border = newwin(CANVAS_OUT_H, CANVAS_OUT_W, canvas_y, canvas_x);
    win_canvas        = derwin(win_canvas_border, CANVAS_H, CANVAS_W, 1, 1);

    // right (info)
    win_info_border = newwin(INFO_OUT_H, INFO_OUT_W, info_y, info_x);
    win_info        = derwin(win_info_border, INFO_H, INFO_W, 1, 1);
    keypad(win_canvas, TRUE);
    scrollok(win_info, TRUE);
    //nodelay(win_canvas, TRUE);
    wtimeout(win_canvas, 1);
    if (has_colors()) {
        start_color();
        use_default_colors();
    }

    box(win_canvas_border, 0, 0);
    box(win_info_border,   0, 0);
    wrefresh(win_canvas_border );
    wrefresh(win_info_border   );

    
    return 0;
}

void ui_deinit(void) {
    delwin(win_canvas);
    delwin(win_canvas_border);
    delwin(win_info);
    delwin(win_info_border);
    endwin();
}




void renderFBtoUI(chip8* chip8_state) {
 chtype row_string[CANVAS_W];
    for (int row = 0; row < CANVAS_H; ++row) {
        wmove(win_canvas, row, 0);
        for (int col_per_byte = 0; col_per_byte < CANVAS_W / 8; col_per_byte++) {

            int idx = BYTES_PER_ROW * row + col_per_byte;
            uint8_t byte = chip8_state->FB[idx];

            uint8_t nib1 = byte >> 4;
            uint8_t nib2 = byte & 0x0F;

            row_string[col_per_byte*8+0] = draw_table_chtype[nib1][0];
            row_string[col_per_byte*8+1] = draw_table_chtype[nib1][1];
            row_string[col_per_byte*8+2] = draw_table_chtype[nib1][2];
            row_string[col_per_byte*8+3] = draw_table_chtype[nib1][3];
            row_string[col_per_byte*8+4] = draw_table_chtype[nib2][0];
            row_string[col_per_byte*8+5] = draw_table_chtype[nib2][1];
            row_string[col_per_byte*8+6] = draw_table_chtype[nib2][2];
            row_string[col_per_byte*8+7] = draw_table_chtype[nib2][3];
        }
        waddchnstr(win_canvas, row_string, CANVAS_W);
    }
}



/* print scrolling message */
void info_print(const char *formatted_msg, ...) {
    char string[INFO_W * INFO_H];
    va_list args;
    va_start(args, formatted_msg);
    vsnprintf(string, sizeof(string), formatted_msg, args);
    va_end (args);
    waddnstr(win_info, string, sizeof(string));
}

/* update both windows */
void ui_refresh(void) {
    wrefresh(win_canvas);
    wrefresh(win_info  );
}

void ui_input(chip8* chip8_state) {
    uint16_t keyHasBeenPressedFlags = 0; 
    int ch = wgetch(win_canvas);
    if(ch != ERR){
        switch (ch) {
            case 27: //se premi ESC, esci
                chip8_state->running = STATUS_STOPPED;
                break;

            case 'p':
                if(chip8_state->running == STATUS_RUNNING){
                    chip8_state->running = STATUS_PAUSED;
                    info_print("[Paused]\n");
                }else {
                    chip8_state->running = STATUS_RUNNING;
                    info_print("[Resumed]\n");
                }
                break;

            case '1':
                chip8_state->KP[0x1] = KEEP_ALIVE;
                keyHasBeenPressedFlags |= 1 << 0;
                break;
                
            case '2':
                chip8_state->KP[0x2] = KEEP_ALIVE;
                keyHasBeenPressedFlags |= 1 << 1;
                break;

            case '3':
                chip8_state->KP[0x3] = KEEP_ALIVE;
                keyHasBeenPressedFlags |= 1 << 2;
                break;
                
            case '4':
                chip8_state->KP[0xC] = KEEP_ALIVE;
                keyHasBeenPressedFlags |= 1 << 3;
                break;

            case 'q':
                chip8_state->KP[0x4] = KEEP_ALIVE;
                keyHasBeenPressedFlags |= 1 << 4;
                break;

            case 'w':
                chip8_state->KP[0x5] = KEEP_ALIVE;
                keyHasBeenPressedFlags |= 1 << 5;
                break;

            case 'e':
                chip8_state->KP[0x6] = KEEP_ALIVE;
                keyHasBeenPressedFlags |= 1 << 6;
                break;

            case 'r':
                chip8_state->KP[0xD] = KEEP_ALIVE;
                keyHasBeenPressedFlags |= 1 << 7;
                break;

            case 'a':
                chip8_state->KP[0x7] = KEEP_ALIVE;
                keyHasBeenPressedFlags |= 1 << 8;
                break;

            case 's':
                chip8_state->KP[0x8] = KEEP_ALIVE;
                keyHasBeenPressedFlags |= 1 << 9;
                break;
            
            case 'd':
                chip8_state->KP[0x9] = KEEP_ALIVE;
                keyHasBeenPressedFlags |= 1 << 10;
                break;

            case 'f':
                chip8_state->KP[0xE] = KEEP_ALIVE;
                keyHasBeenPressedFlags |= 1 << 11;
                break;

            case 'z':
                chip8_state->KP[0xA] = KEEP_ALIVE;
                keyHasBeenPressedFlags |= 1 << 12;
                break;

            case 'x':
                chip8_state->KP[0x0] = KEEP_ALIVE;
                keyHasBeenPressedFlags |= 1 << 13;
                break;

            case 'c':
                chip8_state->KP[0xB] = KEEP_ALIVE;
                keyHasBeenPressedFlags |= 1 << 14;
                break;

            case 'v':
                chip8_state->KP[0xF] = KEEP_ALIVE;
                keyHasBeenPressedFlags |= 1 << 15;
                break;
                
            default:
                //info_print("unhandled key: %d\n", ch);
                break;
        }






    }

    //decay unpressed keys
    for(int i=0; i<16; i++){
        if (chip8_state->KP[i] > 0 && 0 == (keyHasBeenPressedFlags & (1 << i)))
          chip8_state->KP[i]--;
        
            
    }
}


void stat_print(chip8* chip8_state)
{
    wmove(win_info, 0, 0);
    info_print(
        "PC=0x%04x SP=0x%02x I=0x%03x\n"
        "V0=0x%02x V1=0x%02x V2=0x%02x V3=0x%02x \n"
        "V4=0x%02x V5=0x%02x V6=0x%02x V7=0x%02x \n"
        "V8=0x%02x V9=0x%02x VA=0x%02x VB=0x%02x \n"
        "VC=0x%02x VD=0x%02x VE=0x%02x VF=0x%02x \n"
        "DT=0x%02x ST=0x%02x err=0x%01x run=0x%01x \n"

        "\nSTACK\n"
        
        "%04x\n%04x\n%04x\n%04x\n%04x\n%04x\n%04x\n%04x\n"
        "%04x\n%04x\n%04x\n%04x\n%04x\n%04x\n%04x\n%04x\n"

        , 
        chip8_state->PC  , chip8_state->SP, chip8_state->I, 
        chip8_state->V[0], chip8_state->V[1], chip8_state->V[2], chip8_state->V[3],
        chip8_state->V[4], chip8_state->V[5], chip8_state->V[6], chip8_state->V[7],
        chip8_state->V[8], chip8_state->V[9], chip8_state->V[10],chip8_state->V[11],
        chip8_state->V[12],chip8_state->V[13],chip8_state->V[14],chip8_state->V[15],
        chip8_state->DT, chip8_state->ST, chip8_state->errState, chip8_state->running,

        chip8_state->STACK[0 ],
        chip8_state->STACK[1 ],
        chip8_state->STACK[2 ],
        chip8_state->STACK[3 ],
        chip8_state->STACK[4 ],
        chip8_state->STACK[5 ],
        chip8_state->STACK[6 ],
        chip8_state->STACK[7 ],
        chip8_state->STACK[8 ],
        chip8_state->STACK[9 ],
        chip8_state->STACK[10],
        chip8_state->STACK[11],
        chip8_state->STACK[12],
        chip8_state->STACK[13],
        chip8_state->STACK[14],
        chip8_state->STACK[15]
        

    );
}