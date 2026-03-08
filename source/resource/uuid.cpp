#include "uuid.h"
#include <algorithm>
#include <random>
#include <format>

namespace
{
thread_local std::random_device g_random_device;
thread_local std::mt19937_64 g_random_engine(g_random_device());
thread_local std::uniform_int_distribution<uint64_t> g_uniform_dist;
} // namespace

namespace ash
{
uuid uuid_generate_random()
{
    uuid out = {};

    std::ranges::generate(out.data.u8, std::ref(g_random_device));
    out.data.u8[6] = static_cast<uint8_t>((out.data.u8[6] & 0x0F) | 0x40);
    out.data.u8[8] = static_cast<uint8_t>((out.data.u8[8] & 0x3F) | 0x80);
    return out;
}

std::string uuid_to_string(const uuid& uuid)
{
    return std::format("{:02x}{:02x}{:02x}{:02x}-"
                       "{:02x}{:02x}-"
                       "{:02x}{:02x}-"
                       "{:02x}{:02x}-"
                       "{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}",
                       uuid.data.u8[0], uuid.data.u8[1], uuid.data.u8[2], uuid.data.u8[3], uuid.data.u8[4],
                       uuid.data.u8[5], uuid.data.u8[6], uuid.data.u8[7], uuid.data.u8[8], uuid.data.u8[9],
                       uuid.data.u8[10], uuid.data.u8[11], uuid.data.u8[12], uuid.data.u8[13], uuid.data.u8[14],
                       uuid.data.u8[15]);
}
} // namespace ash
