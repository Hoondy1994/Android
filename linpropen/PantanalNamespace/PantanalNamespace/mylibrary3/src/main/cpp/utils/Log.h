//
// Created by 80244960 on 2023/1/3.
//

#ifndef NAMESPACE_LOG_H
#define NAMESPACE_LOG_H

#include "Severity.h"
#include "LogMessage.h"

#define SEVERITY_MACRO(severity) \
        ::pantanal::ns::severity

#define LOG(severity) \
        ::pantanal::ns::Log::GetInstance().CheckSeverity(SEVERITY_MACRO(severity)) && \
        ::pantanal::ns::LogMessage(SEVERITY_MACRO(severity), __FILE__, __LINE__).Stream()

#define LOGV LOG(VERBOSE)
#define LOGD LOG(DEBUG)
#define LOGI LOG(INFO)
#define LOGW LOG(WARN)
#define LOGE LOG(ERROR)

#endif //NAMESPACE_LOG_H
