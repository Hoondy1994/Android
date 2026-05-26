//
// Created by 80244960 on 2023/1/3.
//

#ifndef NAMESPACE_LOGGER_H
#define NAMESPACE_LOGGER_H

#include <vector>
#include <sstream>

#include "Singleton.h"
#include "ILogger.h"
#include "AndroidLogger.h"
#include "ConsoleLogger.h"

namespace pantanal {
namespace ns {
class Log : public Singleton<Log> {
public:
    Log() : max_severity_(INFO) {
        // set default logger
#if defined(__ANDROID__)
        // set default android tag to 'namespace'
        default_logger_ = std::make_unique<AndroidLogger>("namespace");
#else
        default_logger_ = std::make_unique<ConsoleLogger>();
#endif
    }

    void SetMaxSeverity(Severity severity) {
        max_severity_ = severity;
    }

    bool CheckSeverity(Severity severity) {
        return (severity >= max_severity_);
    }

    void AddLogger(ILogger* logger) {
        loggers_.push_back(logger);
    }

    void Flush(Severity severity, const char* file, unsigned int line, std::string msg) {
        if (loggers_.empty()) {
            default_logger_->Write(severity, file, line, msg);
        } else {
            for (const auto logger: loggers_) {
                logger->Write(severity, file, line, msg);
            }
        }
    }
private:
    std::vector<ILogger*> loggers_;
    std::unique_ptr<ILogger> default_logger_;
    Severity max_severity_;
};
}
}

#endif //NAMESPACE_LOGGER_H
