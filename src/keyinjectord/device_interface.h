#pragma once

namespace keyinjectord {

class IRawDevice {
public:
    virtual ~IRawDevice() = default;
    virtual bool emitEvent(int type, int code, int value) = 0;
};

class IDevice {
public:
    virtual ~IDevice() = default;
    virtual bool sendCtrlShiftV() = 0;
};

} // namespace keyinjectord
