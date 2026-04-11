#pragma once
#include <stddef.h>

/* Trim trailing whitespace (\r \n space tab) in-place. Returns new length. */
static inline size_t rtrim(char *s, size_t len) {
    if (!s || !len) return 0;
    while (len > 0 && (s[len - 1] == '\r' || s[len - 1] == '\n' || s[len - 1] == ' ' || s[len - 1] == '\t')) {
        len--;
    }
    s[len] = '\0';
    return len;
}
