#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h> // for strncasecmp
#include <unistd.h>

#define PROJECT "Finder"

#define ASSERT(cond, msg, ...)                                                 \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, msg "\n", ##__VA_ARGS__);                                \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

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

// Case‑insensitive substring check (needle in haystack)
static inline bool str_contains_ignore_case(const char *haystack,
                                            const char *needle) {
  if (!needle || !*needle)
    return true;
  size_t needle_len = strlen(needle);
  for (; *haystack; ++haystack) {
    if (strncasecmp(haystack, needle, needle_len) == 0)
      return true;
  }
  return false;
}

// Delete a file or an empty directory.
// Returns true on success, false on error with a message written into
// error_buf.
bool rm(const char *fullpath, bool is_dir, char *error_buf, size_t buf_size);
