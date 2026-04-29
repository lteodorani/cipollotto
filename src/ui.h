#pragma once

#include "cipollotto.h"
#include "common.h"

int  ui_init(void);
void ui_destroy(void);
void ui_input(chip8* chip8_state);
void ui_audio(chip8* chip8_state);

void ui_renderFB(chip8* chip8_state);
void ui_presentFB(chip8* chip8_state);