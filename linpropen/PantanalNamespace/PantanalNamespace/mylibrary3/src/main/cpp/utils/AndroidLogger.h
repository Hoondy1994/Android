//
// Created by 80244960 on 2023/1/6.
//

#ifndef NAMESPACE_ANDROIDLOGGER_H
#define NAMESPACE_ANDROIDLOGGER_H

#include <android/log.h>
#include "Severity.h"
#include "ILogger.h"

namespace pantanal {
namespace ns {

class AndroidLogger : public ILogger {
public:
    AndroidLogger(const std::string& tag) : tag_(tag) {}

    virtual void Write(Severity severity,
                       const char* file,
                       unsigned int line,
                       std::string msg) {
        // __builtin_available(android 30, *) seems not working
        // need to invest it
#if 0
        if (__builtin_available(android 30, *)) {
            __android_log_message message = {
                    sizeof(__android_log_message),
                    LOG_ID_DEFAULT,
                    SeverityToPriority(severity),
                    tag_.c_str(),
                    file,
                    line,
                    msg.c_str()};
            __android_log_write_log_message(&message);
        } else {
            __android_log_print(SeverityToPriority(severity),
                                tag_.c_str(),
                                "%s",
                                msg.c_str());
        }
#else
        __android_log_print(SeverityToPriority(severity),
                            tag_.c_str(),
                            "%s",
                            msg.c_str());
#endif
    }
private:
    android_LogPriority SeverityToPriority(Severity severity) {
        switch (severity) {
            case FATAL:
                return ANDROID_LOG_FATAL;
            case ERROR:
                return ANDROID_LOG_ERROR;
            case WARN:
                return ANDROID_LOG_WARN;
            case INFO:
                return ANDROID_LOG_INFO;
            case DEBUG:
                return ANDROID_LOG_DEBUG;
            case VERBOSE:
                return ANDROID_LOG_VERBOSE;
            default:
                return ANDROID_LOG_UNKNOWN;
        }
    }
    const std::string tag_;
};
}
}

#endif //NAMESPACE_ANDROIDLOGGER_H
