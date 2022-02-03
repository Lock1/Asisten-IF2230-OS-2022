#include "header/std_lib.h"

void memcpy(byte *dest, byte *src, unsigned int n) {
    unsigned int i;
    for (i = 0; i < n; i++)
        dest[i] = src[i];
}

unsigned int strlen(char *string) {
    unsigned int i = 0;
    while (string[i] != '\0')
        i++;
    return i;
}

bool strcmp(char *s1, char *s2) {
    unsigned int i = 0;
    if (strlen(s1) == strlen(s2)) {
        while (s1[i] != '\0') {
            if (s1[i] != s2[i])
                return 1;
            i++;
        }

        return 0;
    }

    return 1;
}

void strcpy(byte *dst, byte *src) {
    unsigned int i = 0;
    while (src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
}

void clear(byte *ptr, unsigned int n) {
    unsigned int i;
    for (i = 0; i < n; i++)
        ptr[i] = 0x00;
}
