#pragma once

#include "item.h"
#include "state.h"

// Build a new path by concatenating base and name (caller must free)
char *build_path(const char *base, const char *name);

// Get parent directory path (caller must free). Returns NULL if already root.
char *get_parent_path(const char *path);

// List contents of the directory given by state (handle already opened)
Item_arena list_contents(State s);
