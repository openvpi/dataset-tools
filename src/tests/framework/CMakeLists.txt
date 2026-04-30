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
    TestIDocument
    TestIFileIOProviderMock
)

foreach(test_name ${DSFW_TESTS})
    add_executable(${test_name} ${test_name}.cpp)
    target_link_libraries(${test_name} PRIVATE
        dsfw-core
        Qt${QT_VERSION_MAJOR}::Test
    )
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

# Integration tests (require QApplication / UI components)
set(DSFW_INTEGRATION_TESTS
    TestAppShellIntegration
)

foreach(test_name ${DSFW_INTEGRATION_TESTS})
    add_executable(${test_name} ${test_name}.cpp)
    target_link_libraries(${test_name} PRIVATE
        dsfw-ui-core
        Qt${QT_VERSION_MAJOR}::Widgets
        Qt${QT_VERSION_MAJOR}::Test
    )
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
