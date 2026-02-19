#include "dir.h"

#include <libgen.h> // for dirname
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

char *build_path(const char *base, const char *name) {
  size_t base_len = strlen(base);
  int need_slash = (base_len > 0 && base[base_len - 1] != '/') ? 1 : 0;
  size_t len = base_len + need_slash + strlen(name) + 1;
  char *newpath = malloc(len);
  ASSERT(newpath != NULL, "malloc failed");
  snprintf(newpath, len, "%s%s%s", base, need_slash ? "/" : "", name);
  return newpath;
}

char *get_parent_path(const char *path) {
  char *copy = strdup(path);
  if (!copy)
    return NULL;
  char *parent = dirname(copy);
  char *result = strdup(parent);
  free(copy);
  return result;
}

Item_arena list_contents(State s) {
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
