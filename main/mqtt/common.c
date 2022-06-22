#include "commons.h"

hash_t djb_hash(const char* str)
{
    hash_t hash = 5381;
    while(*str)
        hash = 33 * hash ^ (unsigned char) *str++;

    return hash;
}

//Returns location of null char or slash
//*hash will be final value by return
const char* djb_hash_toslash(const char* str, hash_t *hash)
{
    *hash = 5381;
    while(*str != '\0' && *str != '/')
        *hash = 33 * (*hash) ^ (unsigned char) *str++;
    
    return str;
}