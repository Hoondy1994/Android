#pragma once

#include <cstdint>
#include <string>

namespace hal {

class IHalModule {
public:
    virtual ~IHalModule() = default;
    virtual const char* module_id() const = 0;
    virtual int32_t open() = 0;
    virtual int32_t close() = 0;
};

}  // namespace hal
