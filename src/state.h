#pragma once

#include <dirent.h>
#include <stdbool.h>
#include <stdio.h>

typedef enum { MODE_NORMAL, MODE_SEARCH } Mode;

typedef struct {
  char *path; // dynamically allocated full path
  DIR *handle;
  int selected; // index into filtered list
  int offset;   // scroll offset (first visible item in filtered list)
  bool running;

  // New fields for statusline & search
  Mode mode;
  char *search_pattern; // current search string (allocated)
  char *error_message;  // last error (cleared on next key)
  size_t *filtered;     // indices into the full item list
  size_t filtered_count;
} State;

// Initialize state from command line arguments
State state_init(int argc, const char **argv);

// Clean up state (close dir, free all allocated members)
void state_kill(State *ptr);

// Load a new directory (fullpath must be a valid string)
void load_dir(State *ptr, const char *fullpath);
