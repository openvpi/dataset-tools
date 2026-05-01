#pragma once

#include <string>

namespace dstools {

struct DsItemRecord {
    enum class Status { Pending = 0, Completed = 1, Failed = 2 };

    Status status = Status::Pending;
    std::string inputFile;
    std::string outputFile;
    std::string errorMsg;
    std::string timestamp;
};

} // namespace dstools
