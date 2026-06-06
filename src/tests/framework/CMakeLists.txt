find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Test REQUIRED)

set(CMAKE_AUTOMOC ON)

# ── Unit tests (pure logic, no QApplication needed) ──────────────────────────
set(DSFW_TESTS
        TestResult
        TestJsonHelper
        TestAppSettings
        TestAsyncTask
        TestIDocument
        TestTaskProcessorRegistry
        TestPipelineContext
        TestPipelineRunner
        TestAtomicFileWriterSafety
        TestPathUtils
)

foreach (test_name ${DSFW_TESTS})
    add_executable(${test_name} ${test_name}.cpp)
    target_link_libraries(${test_name} PRIVATE
            dsfw-core
            Qt${QT_VERSION_MAJOR}::Test
    )
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach ()

# Tests that depend on dstools-domain (moved implementations)
set(DOMAIN_TESTS
        TestModelManager
)

foreach (test_name ${DOMAIN_TESTS})
    add_executable(${test_name} ${test_name}.cpp)
    target_link_libraries(${test_name} PRIVATE
            dstools-domain
            Qt${QT_VERSION_MAJOR}::Test
    )
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach ()

# Mock-based inference pipeline tests (depends on dsfw-core only)
add_executable(TestMockInference TestMockInference.cpp)
target_link_libraries(TestMockInference PRIVATE
        dsfw-core
        Qt${QT_VERSION_MAJOR}::Test
)
add_test(NAME TestMockInference COMMAND TestMockInference)

# OnnxModelBase validation tests (depends on dsfw-infer for include path and static methods)
add_executable(TestOnnxModelBase TestOnnxModelBase.cpp)
target_link_libraries(TestOnnxModelBase PRIVATE
        dsfw-infer
        Qt${QT_VERSION_MAJOR}::Test
)
add_test(NAME TestOnnxModelBase COMMAND TestOnnxModelBase)

# Audio processing tests (depends on dsfw-audio + sndfile for fixture generation)
find_package(SndFile CONFIG REQUIRED)
add_executable(TestAudioProcessing TestAudioProcessing.cpp)
target_link_libraries(TestAudioProcessing PRIVATE
        dsfw-audio
        dstools-domain
        SndFile::sndfile
        Qt${QT_VERSION_MAJOR}::Test
)
add_test(NAME TestAudioProcessing COMMAND TestAudioProcessing)

# Audio pipeline edge-case tests (empty/corrupt files, segment decode)
add_executable(TestAudioPipelineEdgeCases TestAudioPipelineEdgeCases.cpp)
target_link_libraries(TestAudioPipelineEdgeCases PRIVATE
        dsfw-audio
        dstools-domain
        SndFile::sndfile
        Qt${QT_VERSION_MAJOR}::Test
)
add_test(NAME TestAudioPipelineEdgeCases COMMAND TestAudioPipelineEdgeCases)

# ── Integration tests (require QApplication / UI components) ─────────────────
if (BUILD_INTEGRATION_TESTS)
    set(DSFW_INTEGRATION_TESTS
            TestAppShellIntegration
    )

    foreach (test_name ${DSFW_INTEGRATION_TESTS})
        add_executable(${test_name} ${test_name}.cpp)
        target_link_libraries(${test_name} PRIVATE
                dsfw-ui-core
                dsfw-widgets
                Qt${QT_VERSION_MAJOR}::Widgets
                Qt${QT_VERSION_MAJOR}::Test
        )
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach ()
endif ()
