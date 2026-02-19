#include "ui.h"

#include <ncurses.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void ncurses_init(void) {
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);

  if (has_colors()) {
    start_color();
    init_pair(1, COLOR_BLUE, COLOR_BLACK);  // directories
    init_pair(2, COLOR_WHITE, COLOR_BLACK); // files
  }
}

void set_error(State *state, const char *fmt, ...) {
  free(state->error_message);
  va_list args;
  va_start(args, fmt);
  int len = vasprintf(&state->error_message, fmt, args);
  va_end(args);
  if (len == -1)
    state->error_message = NULL;
}

void update_filtered(State *state, Item_arena items) {
  // Free old filtered list
  free(state->filtered);
  state->filtered = NULL;
  state->filtered_count = 0;

  bool filter_active = (state->mode == MODE_SEARCH && state->search_pattern &&
                        strlen(state->search_pattern) > 0);

  if (!filter_active) {
    // No filter → all items
    if (items.size == 0)
      return;
    state->filtered = malloc(items.size * sizeof(size_t));
    ASSERT(state->filtered, "malloc failed");
    for (size_t i = 0; i < items.size; i++)
      state->filtered[i] = i;
    state->filtered_count = items.size;
  } else {
    // Count matches
    size_t count = 0;
    for (size_t i = 0; i < items.size; i++) {
      if (str_contains_ignore_case(items.handle[i].name, state->search_pattern))
        count++;
    }
    if (count == 0)
      return;

    state->filtered = malloc(count * sizeof(size_t));
    ASSERT(state->filtered, "malloc failed");
    size_t idx = 0;
    for (size_t i = 0; i < items.size; i++) {
      if (str_contains_ignore_case(items.handle[i].name, state->search_pattern))
        state->filtered[idx++] = i;
    }
    state->filtered_count = count;
  }

  // Keep selected within bounds
  if (state->selected >= (int)state->filtered_count)
    state->selected = state->filtered_count > 0 ? state->filtered_count - 1 : 0;
  if (state->selected < 0)
    state->selected = 0;
}

void redraw(State *state, Item_arena items, int height, int width) {
  (void)width; // unused

  clear();

  // Header
  attron(A_BOLD);
  mvprintw(0, 2, "%s - path: %s", PROJECT, state->path);
  attroff(A_BOLD);

  // Directory listing (using filtered indices)
  if (state->filtered_count == 0) {
    if (state->mode == MODE_SEARCH && state->search_pattern &&
        *state->search_pattern)
      mvprintw(2, 2, "(no matches)");
    else
      mvprintw(2, 2, "(empty directory)");
  } else {
    // Adjust scroll offset based on selected
    int visible_lines =
        height - 3; // top line + listing start at line 2, status at bottom
    if (state->selected < state->offset)
      state->offset = state->selected;
    if (state->selected >= state->offset + visible_lines)
      state->offset = state->selected - visible_lines + 1;

    for (int i = 0; i < visible_lines; i++) {
      int idx = state->offset + i;
      if (idx >= (int)state->filtered_count)
        break;

      size_t item_idx = state->filtered[idx];
      int row = 2 + i;
      int col = 2;
      Item *item = &items.handle[item_idx];

      if (idx == state->selected)
        attron(A_REVERSE);
      if (item->is_dir)
        attron(COLOR_PAIR(1));
      else
        attron(COLOR_PAIR(2));

      mvprintw(row, col, "%s %s", item->perms, item->name);

      if (idx == state->selected)
        attroff(A_REVERSE);
      if (item->is_dir)
        attroff(COLOR_PAIR(1));
      else
        attroff(COLOR_PAIR(2));
    }
  }

  // Status line (bottom)
  char status[256];
  if (state->error_message) {
    snprintf(status, sizeof(status), "ERROR: %s", state->error_message);
  } else if (state->mode == MODE_SEARCH) {
    snprintf(status, sizeof(status), "/%s",
             state->search_pattern ? state->search_pattern : "");
  } else {
    snprintf(status, sizeof(status), "NORMAL | Selected: %d/%zu",
             state->filtered_count == 0 ? 0 : state->selected + 1,
             state->filtered_count);
  }
  mvprintw(height - 1, 2, "%-*s", width - 4,
           status); // left‑aligned, padded to clear line

  refresh();
}

void open_with_nvim(const char *filename) {
  char *cmd;
  int len = asprintf(&cmd, "nvim %s", filename);
  if (len == -1) {
    cmd = malloc(strlen(filename) + 6);
    if (!cmd)
      return;
    sprintf(cmd, "nvim %s", filename);
  }

  def_prog_mode();
  endwin();
  int ret = system(cmd);
  if (ret != 0) {
    printf("Failed to run nvim (press any key to continue)");
    getchar();
  }
  reset_prog_mode();
  refresh();

  free(cmd);
}
