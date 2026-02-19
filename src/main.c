#include "dir.h"
#include "state.h"
#include "ui.h"
#include "utils.h"
#include <ctype.h>
#include <ncurses.h>

int main(int argc, const char **argv) {
  State state = state_init(argc, argv);
  Item_arena contents = list_contents(state);
  update_filtered(&state, contents); // initial filtered = all

#ifdef _Debug
  printf("Loaded %zu items\n", contents.size);
#endif

  ncurses_init();

  int max_y, max_x;
  getmaxyx(stdscr, max_y, max_x);

  redraw(&state, contents, max_y, max_x);

  while (state.running) {
    int ch = getch();

    // Clear error message on any key press
    if (state.error_message) {
      free(state.error_message);
      state.error_message = NULL;
    }

    if (state.mode == MODE_SEARCH) {
      // ----- Search mode -----
      if (ch == 27) { // ESC
        free(state.search_pattern);
        state.search_pattern = NULL;
        state.mode = MODE_NORMAL;
        update_filtered(&state, contents);
        state.selected = 0;
        state.offset = 0;
      } else if (ch == '\n' || ch == '\r') {
        // Apply filter and return to normal mode
        state.mode = MODE_NORMAL;
        if (state.selected >= (int)state.filtered_count)
          state.selected =
              state.filtered_count > 0 ? state.filtered_count - 1 : 0;
      } else if (ch == KEY_BACKSPACE || ch == 127) {
        if (state.search_pattern && strlen(state.search_pattern) > 0) {
          state.search_pattern[strlen(state.search_pattern) - 1] = '\0';
        }
        update_filtered(&state, contents);
        state.selected = 0;
        state.offset = 0;
      } else if (isprint(ch)) {
        size_t len = state.search_pattern ? strlen(state.search_pattern) : 0;
        char *newp = realloc(state.search_pattern, len + 2);
        ASSERT(newp, "realloc failed");
        state.search_pattern = newp;
        state.search_pattern[len] = ch;
        state.search_pattern[len + 1] = '\0';
        update_filtered(&state, contents);
        state.selected = 0;
        state.offset = 0;
      }
      // else ignore other keys
      redraw(&state, contents, max_y, max_x);
    } else {
      // ----- Normal mode -----
      switch (ch) {
      case 'q':
        state.running = false;
        break;
      case '/':
        state.mode = MODE_SEARCH;
        free(state.search_pattern);
        state.search_pattern = strdup("");
        ASSERT(state.search_pattern, "strdup failed");
        update_filtered(&state, contents);
        state.selected = 0;
        state.offset = 0;
        redraw(&state, contents, max_y, max_x);
        break;
      case KEY_UP:
        if (state.selected > 0)
          state.selected--;
        redraw(&state, contents, max_y, max_x);
        break;
      case KEY_DOWN:
        if (state.selected < (int)state.filtered_count - 1)
          state.selected++;
        redraw(&state, contents, max_y, max_x);
        break;
      case KEY_LEFT: {
        char *parent = get_parent_path(state.path);
        if (parent) {
          load_dir(&state, parent);
          free(parent);
          free_item_arena(&contents);
          contents = list_contents(state);
          // Reset filter state
          free(state.search_pattern);
          state.search_pattern = NULL;
          state.mode = MODE_NORMAL;
          update_filtered(&state, contents);
          state.selected = 0;
          state.offset = 0;
          redraw(&state, contents, max_y, max_x);
        }
      } break;
      case KEY_RIGHT:
      case KEY_ENTER:
      case '\n':
      case '\r':
        if (state.filtered_count == 0)
          break;
        {
          Item *sel = &contents.handle[state.filtered[state.selected]];
          if (sel->is_dir) {
            char *newpath = NULL;
            if (strcmp(sel->name, ".") == 0) {
              newpath = strdup(state.path);
            } else if (strcmp(sel->name, "..") == 0) {
              newpath = get_parent_path(state.path);
              if (!newpath)
                break; // already at root
            } else {
              newpath = build_path(state.path, sel->name);
            }

            load_dir(&state, newpath);
            free(newpath);

            free_item_arena(&contents);
            contents = list_contents(state);

            // Reset filter state
            free(state.search_pattern);
            state.search_pattern = NULL;
            state.mode = MODE_NORMAL;
            update_filtered(&state, contents);
            state.selected = 0;
            state.offset = 0;
            redraw(&state, contents, max_y, max_x);
          } else {
            open_with_nvim(sel->name);
            redraw(&state, contents, max_y, max_x);
          }
        }
        break;

      // ----- DELETE FEATURE -----
      case 'd':
      case 'D':
        if (state.filtered_count == 0)
          break;
        {
          Item *sel = &contents.handle[state.filtered[state.selected]];
          // Do not allow deletion of "." or ".."
          if (strcmp(sel->name, ".") == 0 || strcmp(sel->name, "..") == 0) {
            set_error(&state, "Cannot delete '.' or '..'");
            redraw(&state, contents, max_y, max_x);
            break;
          }

          // Build full path
          char *fullpath = build_path(state.path, sel->name);
          if (!fullpath) {
            set_error(&state, "Out of memory");
            break;
          }

          char err_buf[256];
          bool success = rm(fullpath, sel->is_dir, err_buf, sizeof(err_buf));
          free(fullpath);

          if (!success) {
            set_error(&state, "%s", err_buf);
          } else {
            // Refresh directory contents
            free_item_arena(&contents);
            contents = list_contents(state);
            // Reset filter and selection
            free(state.search_pattern);
            state.search_pattern = NULL;
            state.mode = MODE_NORMAL;
            update_filtered(&state, contents);
            if (state.selected >= (int)state.filtered_count)
              state.selected =
                  state.filtered_count > 0 ? state.filtered_count - 1 : 0;
            state.offset = 0;
          }
          redraw(&state, contents, max_y, max_x);
        }
        break;

      default:
        break;
      }
    }
  }

  // Cleanup
  free_item_arena(&contents);
  state_kill(&state);
  endwin();

  return 0;
}
