#include <dsfw/ServiceLocator.h>
#include <dsfw/LocalFileIOProvider.h>

namespace dstools {

std::unordered_map<std::type_index, std::any> &ServiceLocator::services() {
    static std::unordered_map<std::type_index, std::any> s;
    return s;
}

void ServiceLocator::resetAll() {
    services().clear();
    resetFileIO();
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

}
