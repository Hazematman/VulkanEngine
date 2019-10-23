#ifndef UTIL_H
#define UTIL_H
#include <stdint.h>
#include "interface.h"

void util_load_whole_file(Interface *func, const char *filename, void **data, size_t *size);

#endif
