//
// Created by 80244960 on 2023/1/5.
//

#ifndef NAMESPACE_LOGDATA_H
#define NAMESPACE_LOGDATA_H

#include <sstream>

#include "Severity.h"

namespace pantanal {
namespace ns {
class LogData {
public:
    LogData(Severity severity, const char* file, unsigned int line)
            : severity_(severity), file_(file), line_(line) {}

    virtual const std::string GetData() const {
        return data_.str();
    }

    virtual const char* GetFile() const {
        return file_;
    }

    virtual Severity GetSeverity() const {
        return severity_;
    }

    virtual unsigned int GetLine() const {
        return line_;
    }

    virtual std::ostream& GetBuffer() {
        return data_;
    }

private:
    std::ostringstream data_;
    const char* const file_;
    const unsigned int line_;
    const Severity severity_;
};
}
}
#endif //NAMESPACE_LOGDATA_H
