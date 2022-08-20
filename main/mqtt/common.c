#include "commons.h"

//TODO: static inline the actual hashing to stop bifurcating identical code

hash_t djb_hash(const char* str)
{
    hash_t hash = 5381;
    while(*str)
        hash = 33 * hash ^ (unsigned char) *str++;

    return hash;
}

//Returns location of null char or slash
//*hash will be final value by return
const char* djb_hash_toslash(const char *str, hash_t* hash)
{
    *hash = 5381;
    while(*str != '\0' && *str != '/')
        *hash = 33 * (*hash) ^ (unsigned char) *str++;
    
    return str;
}

//Returns location of null char or slash, to a max length
//*hash will be final value by return
const char* djb_hash_toslash_len(const char *str, hash_t* hash, int length)
{
    *hash = 5381;
    char *idx = str;
    while(*idx != '\0' && *idx != '/' && idx < str + length)
        *hash = 33 * (*hash) ^ (unsigned char) *idx++;
    
    return idx;
}

//Returns location of null char or colon
//*hash will be final value by return
const char* djb_hash_tocolon(const char *str, hash_t* hash)
{
    *hash = 5381;
    while(*str != '\0' && *str != ':')
        *hash = 33 * (*hash) ^ (unsigned char) *str++;
    
    return str;
}

//Returns location of null char or colon
//*hash will be final value by return
const char* djb_hash_tocolon_len(const char *str, hash_t* hash, int length)
{
    *hash = 5381;
    char *idx = str;
    while(*idx != '\0' && *idx != ':' && idx < str + length)
        *hash = 33 * (*hash) ^ (unsigned char) *idx++;
    
    return idx;
}