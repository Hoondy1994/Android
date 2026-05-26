//
// Created by 80244960 on 2023/1/12.
//

#ifndef NAMESPACE_CONSOLELOGGER_H
#define NAMESPACE_CONSOLELOGGER_H

#include <iostream>
#include "LogData.h"

namespace pantanal {
namespace ns {

class ConsoleLogger : ILogger {
public:
    ConsoleLogger() : output_stream_(std::cout) {}

    virtual void Write(Severity severity,
                       const char* file,
                       unsigned int line,
                       std::string msg) {
        // should define a formatter
        output_stream_ << msg << std::flush;
    }
private:
    std::ostream& output_stream_;
};

}
}

#endif //NAMESPACE_CONSOLELOGGER_H
