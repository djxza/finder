#pragma once

#include "item.h"
#include "state.h"

// Initialize ncurses with colors
void ncurses_init(void);

// Draw the entire interface (uses state->filtered for display)
void redraw(State *state, Item_arena items, int height, int width);

// Helper: open the selected file with nvim
void open_with_nvim(const char *filename);

// Set an error message (will be displayed and cleared on next key)
void set_error(State *state, const char *fmt, ...);

// Recompute filtered indices based on current search pattern and mode
void update_filtered(State *state, Item_arena items);
