#include "cppfuncs.h"

#include <string>
#include <cstring>

uint8_t calc_hash_cpp(uint8_t *value, uint64_t len)
{
    char c_str[1024];
    memcpy(c_str, value, len);
    c_str[len] = '\0';
    std::string str(c_str);
    size_t component_hash = std::hash<std::string>()(str);
    return (uint8_t)(component_hash % 0xfe);
}