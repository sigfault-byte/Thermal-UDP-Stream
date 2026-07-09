#include "serial.h"

#include <stdio.h>

bool serial_read_line(char *buffer, size_t max_len)
{
    if (buffer == NULL || max_len == 0) {
        return false;
    }
    if (fgets(buffer, max_len, stdin) == NULL) {
        return false;
    }
    for (size_t i = 0; buffer[i] != '\0'; i++) {
        if (buffer[i] == '\n' || buffer[i] == '\r') {
            buffer[i] = '\0';
            break;
        }
    }
    return buffer[0] != '\0';
}

void serial_write_text(const char *text)
{
    if (text == NULL) {
        return;
    }
    printf("%s", text);
    fflush(stdout);
}
