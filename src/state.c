#include "state.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h> // for realpath

#include "utils.h"

State state_init(int argc, const char **argv) {
  State st = {0};
  const char *start_path = (argc == 1) ? "." : argv[1];
  char *abs = realpath(start_path, NULL);
  st.path = abs ? abs : strdup(start_path);
  ASSERT(st.path != NULL, "strdup failed");

  st.handle = opendir(st.path);
  ASSERT(st.handle != NULL, "No such directory %s", st.path);

  st.running = true;
  st.selected = 0;
  st.offset = 0;

  // Initialise new fields
  st.mode = MODE_NORMAL;
  st.search_pattern = NULL;
  st.error_message = NULL;
  st.filtered = NULL;
  st.filtered_count = 0;

  return st;
}

void state_kill(State *ptr) {
  if (ptr->handle)
    closedir(ptr->handle);
  free(ptr->path);
  free(ptr->search_pattern);
  free(ptr->error_message);
  free(ptr->filtered);
}

void load_dir(State *ptr, const char *fullpath) {
  if (ptr->handle) {
    closedir(ptr->handle);
    ptr->handle = NULL;
  }
  free(ptr->path);
  ptr->path = strdup(fullpath);
  ASSERT(ptr->path != NULL, "strdup failed");

  ptr->handle = opendir(ptr->path);
  ASSERT(ptr->handle != NULL, "No such directory %s", ptr->path);
}
