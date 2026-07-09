#pragma once

#include <stdbool.h>
#include <stddef.h>

bool serial_read_line(char *buffer, size_t max_len);
void serial_write_text(const char *text);
