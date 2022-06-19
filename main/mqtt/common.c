#include "commons.h"

static unsigned int djb_hash(const char *str)
{
    unsigned int hash = 5381;
    while(*str)
        hash = 33 * hash ^ (unsigned char) *str++;

    return hash;
}