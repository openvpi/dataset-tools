#pragma once

#include <dstools/IFileIOProvider.h>
#include <memory>

namespace dstools {

class Theme;

class ServiceLocator {
public:
    static Theme &theme();
    static void setTheme(std::unique_ptr<Theme> theme);
    static void resetTheme();

    static IFileIOProvider *fileIO();
    static void setFileIO(IFileIOProvider *provider);
    static void resetFileIO();

    static void resetAll();
};

}
