#include <dirent.h>
#include <libgen.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define PROJECT "Finder"

#define ASSERT(cond, msg, ...)                                                 \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, msg "\n", ##__VA_ARGS__);                                \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

// #define _Debug

#define _Arena_init(type)                                                      \
  typedef struct type##_arena {                                                \
    type *handle;                                                              \
    size_t size;                                                               \
  } type##_arena

#define append(arena, x)                                                       \
  do {                                                                         \
    ++(arena).size;                                                            \
    (arena).handle =                                                           \
        realloc((arena).handle, (arena).size * sizeof(*(arena).handle));       \
    ASSERT((arena).handle != NULL, "Failed to alloc byte");                    \
    (arena).handle[(arena).size - 1] = (x);                                    \
  } while (0)

typedef struct {
  int x;
  int y;
} Cursor;

typedef struct {
  char *path; // dynamically allocated full path
  DIR *handle;
  int selected; // currently selected item index
  int offset;   // scroll offset (first visible item)
  bool running;
} State;

// Convert mode_t to an ls -l style permission string (10 characters)
static void mode_to_str(mode_t mode, char out[11]) {
  // file type
  if (S_ISREG(mode))
    out[0] = '-';
  else if (S_ISDIR(mode))
    out[0] = 'd';
  else if (S_ISLNK(mode))
    out[0] = 'l';
  else if (S_ISCHR(mode))
    out[0] = 'c';
  else if (S_ISBLK(mode))
    out[0] = 'b';
  else if (S_ISFIFO(mode))
    out[0] = 'p';
  else if (S_ISSOCK(mode))
    out[0] = 's';
  else
    out[0] = '?';

  // owner permissions
  out[1] = (mode & S_IRUSR) ? 'r' : '-';
  out[2] = (mode & S_IWUSR) ? 'w' : '-';
  out[3] = (mode & S_IXUSR) ? 'x' : '-';

  // group permissions
  out[4] = (mode & S_IRGRP) ? 'r' : '-';
  out[5] = (mode & S_IWGRP) ? 'w' : '-';
  out[6] = (mode & S_IXGRP) ? 'x' : '-';

  // other permissions
  out[7] = (mode & S_IROTH) ? 'r' : '-';
  out[8] = (mode & S_IWOTH) ? 'w' : '-';
  out[9] = (mode & S_IXOTH) ? 'x' : '-';

  out[10] = '\0';
}

typedef struct {
  bool is_dir; // true for directories
  char *name;
  char perms[11]; // permission string (e.g., "-rw-r--r--")
} Item;

_Arena_init(Item); // defines Item_arena type

// Comparator: special entries . and .. first, then directories, then files
static int cmp_item(const void *a, const void *b) {
  const Item *ia = (const Item *)a;
  const Item *ib = (const Item *)b;

  // "." always first
  if (strcmp(ia->name, ".") == 0)
    return -1;
  if (strcmp(ib->name, ".") == 0)
    return 1;

  // ".." always second
  if (strcmp(ia->name, "..") == 0)
    return -1;
  if (strcmp(ib->name, "..") == 0)
    return 1;

  // Directories before files
  if (ia->is_dir && !ib->is_dir)
    return -1;
  if (!ia->is_dir && ib->is_dir)
    return 1;

  // Then sort alphabetically
  return strcmp(ia->name, ib->name);
}

// Build a new path by concatenating base and name (caller must free)
static char *build_path(const char *base, const char *name) {
  size_t base_len = strlen(base);
  int need_slash = (base_len > 0 && base[base_len - 1] != '/') ? 1 : 0;
  size_t len = base_len + need_slash + strlen(name) + 1;
  char *newpath = malloc(len);
  ASSERT(newpath != NULL, "malloc failed");
  snprintf(newpath, len, "%s%s%s", base, need_slash ? "/" : "", name);
  return newpath;
}

// Get parent directory path (caller must free). Returns NULL if already root.
static char *get_parent_path(const char *path) {
  char *copy = strdup(path);
  if (!copy)
    return NULL;
  char *parent = dirname(copy);
  char *result = strdup(parent);
  free(copy);
  return result;
}

// List contents of the directory given by state (handle already opened)
static Item_arena list_contents(State s) {
  Item_arena __retVal = {0};
  struct dirent *ent;

  if (s.handle != NULL) {
    while ((ent = readdir(s.handle)) != NULL) {
      char fullpath[4096];
      snprintf(fullpath, sizeof(fullpath), "%s/%s", s.path, ent->d_name);

      struct stat st;
      if (lstat(fullpath, &st) == -1)
        continue; // skip un‑stat‑able entries

      char *name_copy = strdup(ent->d_name);
      if (!name_copy) {
        perror("strdup failed");
        exit(EXIT_FAILURE);
      }

      Item x;
      x.name = name_copy;
      x.is_dir = S_ISDIR(st.st_mode);
      mode_to_str(st.st_mode, x.perms);

      append(__retVal, x);
    }
    qsort(__retVal.handle, __retVal.size, sizeof(Item), cmp_item);
  } else {
    perror("Could not open dir");
    exit(EXIT_FAILURE);
  }

  return __retVal;
}

// Load a new directory (fullpath must be a valid string)
static void load_dir(State *ptr, const char *fullpath) {
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

static State state_init(int argc, const char **argv) {
  State __retVal = {0};
  const char *start_path = (argc == 1) ? "." : argv[1];
  char *abs = realpath(start_path, NULL);
  if (abs) {
    __retVal.path = abs;
  } else {
    __retVal.path = strdup(start_path);
  }
  ASSERT(__retVal.path != NULL, "strdup failed");
  __retVal.handle = opendir(__retVal.path);
  ASSERT(__retVal.handle != NULL, "No such directory %s", __retVal.path);
  __retVal.running = true;
  __retVal.selected = 0;
  __retVal.offset = 0;
  return __retVal;
}

static void state_kill(State *ptr) {
  if (ptr->handle)
    closedir(ptr->handle);
  free(ptr->path);
  endwin();
}

// Initialize ncurses with colors
static void ncurses_init(void) {
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

// Draw the entire interface
static void redraw(State *state, Item_arena items, int height, int width) {
  (void)width; // unused parameter

  clear();

  attron(A_BOLD);
  mvprintw(0, 2, "%s - path: %s", PROJECT, state->path);
  attroff(A_BOLD);

  if (items.size == 0) {
    mvprintw(2, 2, "(empty directory)");
  } else {
    if (state->selected < 0)
      state->selected = 0;
    if (state->selected >= (int)items.size)
      state->selected = items.size - 1;

    int visible_lines = height - 3;
    if (state->selected < state->offset)
      state->offset = state->selected;
    if (state->selected >= state->offset + visible_lines)
      state->offset = state->selected - visible_lines + 1;

    for (int i = 0; i < visible_lines; i++) {
      int idx = state->offset + i;
      if (idx >= (int)items.size)
        break;

      int row = 2 + i;
      int col = 2;

      if (idx == state->selected)
        attron(A_REVERSE);
      if (items.handle[idx].is_dir)
        attron(COLOR_PAIR(1));
      else
        attron(COLOR_PAIR(2));

      mvprintw(row, col, "%s %s", items.handle[idx].perms,
               items.handle[idx].name);

      if (idx == state->selected)
        attroff(A_REVERSE);
      if (items.handle[idx].is_dir)
        attroff(COLOR_PAIR(1));
      else
        attroff(COLOR_PAIR(2));
    }
  }

  mvprintw(height - 1, 2, "Selected: %d/%zu | Press 'q' to quit",
           (items.size == 0) ? 0 : state->selected + 1, items.size);
  refresh();
}

// Helper: open the selected file with nvim
static void open_with_nvim(const char *filename) {
  // Build command "nvim <filename>" safely
  char *cmd;
  int len = asprintf(&cmd, "nvim %s", filename);
  if (len == -1) {
    // asprintf failed – fallback to manual allocation
    cmd = malloc(strlen(filename) + 6); // "nvim " + filename + '\0'
    if (!cmd) {
      mvprintw(0, 0, "Failed to allocate command string");
      return;
    }
    sprintf(cmd, "nvim %s", filename);
  }

  // Execute the command (temporarily leaves curses mode)
  def_prog_mode();       // save current tty modes
  endwin();              // end curses mode
  int ret = system(cmd); // run nvim
  if (ret != 0) {
    printf("Failed to run nvim (press any key to continue)");
    getchar();
  }
  reset_prog_mode(); // restore tty modes
  refresh();         // redraw

  free(cmd);
}

int main(int argc, const char **argv) {
  State state = state_init(argc, argv);
  Item_arena contents = list_contents(state);

#ifdef _Debug
  printf("Loaded %zu items\n", contents.size);
#endif

  ncurses_init();

  int max_y, max_x;
  getmaxyx(stdscr, max_y, max_x);

  int win_height = max_y;
  int win_width = max_x;

  redraw(&state, contents, win_height, win_width);

  while (state.running) {
    int ch = getch();
    switch (ch) {
    case 'q':
      state.running = false;
      break;
    case KEY_UP:
      if (state.selected > 0)
        state.selected--;
      redraw(&state, contents, win_height, win_width);
      break;
    case KEY_DOWN:
      if (state.selected < (int)contents.size - 1)
        state.selected++;
      redraw(&state, contents, win_height, win_width);
      break;
    case KEY_LEFT:
      // id of ..
      state.selected = 1;
    case KEY_RIGHT:
    case KEY_ENTER:
    case '\n':
    case '\r':
      if (contents.size == 0 || state.selected < 0 ||
          state.selected >= (int)contents.size)
        break;

      Item *sel = &contents.handle[state.selected];
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

        for (size_t i = 0; i < contents.size; ++i)
          free(contents.handle[i].name);
        free(contents.handle);

        contents = list_contents(state);
        state.selected = 0;
        state.offset = 0;
        redraw(&state, contents, win_height, win_width);
      } else {
        // It's a file – open with nvim
        open_with_nvim(sel->name);
        // After returning, redraw
        redraw(&state, contents, win_height, win_width);
      }
      break;
    default:
      break;
    }
  }

  // Cleanup
  for (size_t i = 0; i < contents.size; ++i)
    free(contents.handle[i].name);
  free(contents.handle);

  state_kill(&state);

  return 0;
}
