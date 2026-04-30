#pragma once

#include <dstools/IFileIOProvider.h>

namespace dstools {

class ServiceLocator {
public:
    static IFileIOProvider *fileIO();
    static void setFileIO(IFileIOProvider *provider);
    static void resetFileIO();

    static void resetAll();
};

}
