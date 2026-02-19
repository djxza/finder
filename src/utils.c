#include "utils.h"
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

bool rm(const char *fullpath, bool is_dir, char *error_buf, size_t buf_size) {
  int ret;
  if (is_dir) {
    ret = rmdir(fullpath);
  } else {
    ret = unlink(fullpath);
  }

  if (ret == 0)
    return true;

  // Operation failed – format error message
  const char *err_str = strerror(errno);
  snprintf(error_buf, buf_size, "Cannot delete %s: %s", fullpath, err_str);
  return false;
}
