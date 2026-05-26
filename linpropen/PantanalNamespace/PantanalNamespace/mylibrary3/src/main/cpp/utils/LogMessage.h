//
// Created by 80244960 on 2023/1/17.
//

#ifndef NAMESPACE_LOGMESSAGE_H
#define NAMESPACE_LOGMESSAGE_H

#include "Severity.h"
#include "LogData.h"
#include "Logger.h"

namespace pantanal {
namespace ns {
class LogMessage {
public:
    LogMessage(Severity severity, const char* file, unsigned int line)
        : data_(std::make_unique<LogData>(severity, file, line)) {}

    ~LogMessage() {
        Log::GetInstance().Flush(data_->GetSeverity(),
                                 data_->GetFile(),
                                 data_->GetLine(),
                                 data_->GetData());
    }

    std::ostream& Stream() {
        return data_->GetBuffer();
    }
private:
    const std::unique_ptr<LogData> data_;
};
}
}

#endif //NAMESPACE_LOGMESSAGE_H
