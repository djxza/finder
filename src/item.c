#include <string.h>

#include "item.h"

void mode_to_str(mode_t mode, char out[11]) {
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

int cmp_item(const void *a, const void *b) {
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

void free_item_arena(Item_arena *arena) {
  for (size_t i = 0; i < arena->size; ++i)
    free(arena->handle[i].name);
  free(arena->handle);
  arena->handle = NULL;
  arena->size = 0;
}
