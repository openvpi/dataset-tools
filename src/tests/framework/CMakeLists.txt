find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Test REQUIRED)

set(CMAKE_AUTOMOC ON)

# ── Unit tests (pure logic, no QApplication needed) ──────────────────────────
set(DSFW_TESTS
    TestResult
    TestJsonHelper
    TestAppSettings
    TestServiceLocator
    TestAsyncTask
    TestLocalFileIOProvider
    TestIDocument
    TestIFileIOProviderMock
    TestTaskProcessorRegistry
    TestPipelineContext
    TestPipelineRunner
)

foreach(test_name ${DSFW_TESTS})
    add_executable(${test_name} ${test_name}.cpp)
    target_link_libraries(${test_name} PRIVATE
        dsfw-core
        Qt${QT_VERSION_MAJOR}::Test
    )
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

# Tests that depend on dstools-domain (moved implementations)
set(DOMAIN_TESTS
    TestModelManager
    TestModelDownloader
)

foreach(test_name ${DOMAIN_TESTS})
    add_executable(${test_name} ${test_name}.cpp)
    target_link_libraries(${test_name} PRIVATE
        dstools-domain
        Qt${QT_VERSION_MAJOR}::Test
    )
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()

# Mock-based inference pipeline tests (depends on dsfw-core only)
add_executable(TestMockInference TestMockInference.cpp)
target_link_libraries(TestMockInference PRIVATE
    dsfw-core
    Qt${QT_VERSION_MAJOR}::Test
)
add_test(NAME TestMockInference COMMAND TestMockInference)

# OnnxModelBase validation tests (depends on dstools-infer-common)
add_executable(TestOnnxModelBase TestOnnxModelBase.cpp)
target_link_libraries(TestOnnxModelBase PRIVATE
    dstools-infer-common
    Qt${QT_VERSION_MAJOR}::Test
)
add_test(NAME TestOnnxModelBase COMMAND TestOnnxModelBase)

# Audio processing tests (depends on dstools-audio + sndfile for fixture generation)
add_executable(TestAudioProcessing TestAudioProcessing.cpp)
target_link_libraries(TestAudioProcessing PRIVATE
    dstools-audio
    dstools-domain
    Qt${QT_VERSION_MAJOR}::Test
)
add_test(NAME TestAudioProcessing COMMAND TestAudioProcessing)

# ── Integration tests (require QApplication / UI components) ─────────────────
if(BUILD_INTEGRATION_TESTS)
    set(DSFW_INTEGRATION_TESTS
        TestAppShellIntegration
    )

    foreach(test_name ${DSFW_INTEGRATION_TESTS})
        add_executable(${test_name} ${test_name}.cpp)
        target_link_libraries(${test_name} PRIVATE
            dsfw-ui-core
            dsfw-widgets
            Qt${QT_VERSION_MAJOR}::Widgets
            Qt${QT_VERSION_MAJOR}::Test
        )
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()
endif()
