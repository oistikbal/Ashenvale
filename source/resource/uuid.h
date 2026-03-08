#pragma once
#include <cstdint>
#include <string>

namespace ash
{
struct uuid
{
    union {
        uint64_t u64[2];
        uint8_t u8[16];
    } data = {};
};

uuid uuid_generate_random();
std::string uuid_to_string(const uuid &uuid);
} // namespace ash