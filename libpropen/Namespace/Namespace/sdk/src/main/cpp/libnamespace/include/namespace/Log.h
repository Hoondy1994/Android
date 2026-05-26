//
// Created by 80244960 on 2022/12/21.
//

#ifndef NAMESPACE_LOG_H
#define NAMESPACE_LOG_H

#include <memory>

namespace pantanal {
namespace ns {

enum LogLevel {
    VERBOSE,
    DEBUG,
    INFO,
    WARN,
    ERROR
};

class Logger;

class Log {
public:
    Log();

    std::ostream &GetOutput();

private:
    const std::unique_ptr<LogData> data_;
    static Logger logger_;
};

#define LOG(level) \
::pantanal::ns::Log(level).GetOutput()

}
}
#endif //NAMESPACE_LOG_H
