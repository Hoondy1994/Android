//
// Created by 80244960 on 2023/1/18.
//

#ifndef NAMESPACE_ILOGGER_H
#define NAMESPACE_ILOGGER_H

#include "Severity.h"
#include "LogData.h"

namespace pantanal {
namespace ns {
class ILogger {
public:
    virtual ~ILogger() {}

    virtual void Write(Severity severity,
                       const char* file,
                       unsigned int line,
                       std::string msg) = 0;
};
}
}
#endif //NAMESPACE_ILOGGER_H
