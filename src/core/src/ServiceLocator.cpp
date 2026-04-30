#include <dstools/ServiceLocator.h>
#include <dstools/LocalFileIOProvider.h>

namespace dstools {

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
    resetFileIO();
}

}
