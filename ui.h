#ifndef UI_H
#define UI_H

#include "cipollotto.h"

int  ui_init(void);
void ui_deinit(void);
void ui_refresh(void);
void ui_input(chip8* chip8_state);

void renderFBtoUI(chip8* chip8_state);
void stat_print(chip8* chip8_state); 
void info_print(const char *formatted_msg, ...);


#endif