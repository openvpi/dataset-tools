#include <dstools/ServiceLocator.h>
#include <dstools/Theme.h>
#include <dstools/LocalFileIOProvider.h>

namespace dstools {

Theme &ServiceLocator::theme() {
    return Theme::instance();
}

void ServiceLocator::setTheme(std::unique_ptr<Theme> theme) {
    Theme::setInstance(std::move(theme));
}

void ServiceLocator::resetTheme() {
    Theme::resetInstance();
}

IFileIOProvider *ServiceLocator::fileIO() {
    return fileIOProvider();
}

void ServiceLocator::setFileIO(IFileIOProvider *provider) {
    setFileIOProvider(provider);
}

void ServiceLocator::resetFileIO() {
    resetFileIOProvider();
}

void ServiceLocator::resetAll() {
    resetTheme();
    resetFileIO();
}

}
