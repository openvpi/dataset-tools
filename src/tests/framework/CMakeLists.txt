find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Test REQUIRED)

set(CMAKE_AUTOMOC ON)

set(DSFW_TESTS
    TestResult
    TestJsonHelper
    TestAppSettings
    TestServiceLocator
    TestAsyncTask
    TestLocalFileIOProvider
    TestModelManager
    TestModelDownloader
)

foreach(test_name ${DSFW_TESTS})
    add_executable(${test_name} ${test_name}.cpp)
    target_link_libraries(${test_name} PRIVATE
        dsfw-core
        Qt${QT_VERSION_MAJOR}::Test
    )
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
