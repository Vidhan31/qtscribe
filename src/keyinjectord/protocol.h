#pragma once

#include <cstdint>

namespace keyinjectord {

enum class Opcode : uint8_t {
    Paste = 0x01,
    Ping = 0x02,
};

enum class ResponseStatus : uint8_t {
    Ok = 0x00,
    UnknownCmd = 0x01,
    DeviceError = 0x02,
};

} // namespace keyinjectord
