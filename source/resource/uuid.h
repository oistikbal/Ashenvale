#pragma once
#include <cstdint>
#include <functional>
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

inline bool operator==(const uuid &lhs, const uuid &rhs)
{
    return lhs.data.u64[0] == rhs.data.u64[0] && lhs.data.u64[1] == rhs.data.u64[1];
}

uuid uuid_generate_random();
std::string uuid_to_string(const uuid &uuid);
} // namespace ash

template <> struct std::hash<ash::uuid>
{
    size_t operator()(const ash::uuid &value) const noexcept
    {
        return std::hash<uint64_t>{}(value.data.u64[0]) ^ (std::hash<uint64_t>{}(value.data.u64[1]) << 1);
    }
};
