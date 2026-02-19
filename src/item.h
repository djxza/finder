#pragma once

#include <stdbool.h>
#include <sys/stat.h>

typedef struct {
  bool is_dir; // true for directories
  char *name;
  char perms[11]; // permission string (e.g., "-rw-r--r--")
} Item;

#include "utils.h"

_Arena_init(Item); // defines Item_arena

// Convert mode_t to an ls -l style permission string (10 characters)
void mode_to_str(mode_t mode, char out[11]);

// Comparator for sorting items
int cmp_item(const void *a, const void *b);

// Free all memory used by an Item_arena
void free_item_arena(Item_arena *arena);
